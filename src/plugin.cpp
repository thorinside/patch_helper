#if defined(__arm__) && (!defined(__PIC__) || __PIC__ != 2)
#error "The NT hardware build requires -fPIC"
#endif
#include <cstddef>
#include <distingnt/api.h>
#include <distingnt/serialisation.h>
#include <distingnt/slot.h>
#include <new>
#include "patch_map.h"
#include "patch_protocol.h"

namespace {
// Keep the three preview-era parameter indices for saved mappings, but expose
// only independent per-socket controls on the native pages.
constexpr int kLegacyParameters = 3;
constexpr int kParameterCount = kLegacyParameters + 2 * patch_helper::kNativeSockets;
static_assert(kParameterCount <= 240);
struct Algorithm : _NT_algorithm {
    patch_helper::PatchMap map;
    patch_helper::PatchMap scratch;
    patch_helper::Session session;
    std::array<_NT_parameter, kParameterCount> definitions{};
    std::array<std::array<uint8_t, 2>, patch_helper::kNativeSockets> pageIndices{};
    std::array<std::array<char, 16>, patch_helper::kNativeSockets> pageNames{};
    std::array<_NT_parameterPage, patch_helper::kNativeSockets + 1> pages{};
    _NT_parameterPages pageList{};
    bool projecting = false;
    bool needsProjection = true;
};
constexpr _NT_parameter parameters[] = {
    {"First socket", 1, 20, 1, kNT_unitNone, 0, nullptr},
    {"Cable colour", 0, patch_helper::kColourCount - 1, 0, kNT_unitEnum, 0, patch_helper::kColours},
    {"Tag", 0, 12, 0, kNT_unitNone, 0, nullptr},
};
void socketName(char* name, int socket) {
    if (socket < 20) {
        std::strcpy(name, socket < 12 ? "Input " : "Output ");
        NT_intToString(name + std::strlen(name), socket < 12 ? socket + 1 : socket - 11);
    } else {
        std::strcpy(name, "E");
        NT_intToString(name + 1, (socket - 20) / 8 + 1);
        std::strcat(name, " Out ");
        NT_intToString(name + std::strlen(name), (socket - 20) % 8 + 1);
    }
}

void requirements(_NT_algorithmRequirements& req, const int32_t*) {
    req = {};
    req.numParameters = kParameterCount;
    req.sram = sizeof(Algorithm);
}
_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs,
                         const _NT_algorithmRequirements&, const int32_t*) {
    // Placement construction uses only the block declared in requirements().
    // This does not invoke heap allocation.
    auto* algorithm = new (ptrs.sram) Algorithm();
    std::copy(std::begin(parameters), std::end(parameters), algorithm->definitions.begin());
    algorithm->parameters = algorithm->definitions.data();
    for (int socket = 0; socket < patch_helper::kNativeSockets; ++socket) {
        const int colour = kLegacyParameters + 2 * socket;
        algorithm->definitions[colour] = parameters[1];
        algorithm->definitions[colour + 1] = parameters[2];
        algorithm->pageIndices[socket] = {static_cast<uint8_t>(colour), static_cast<uint8_t>(colour + 1)};
        socketName(algorithm->pageNames[socket].data(), socket);
        algorithm->pages[socket] = {algorithm->pageNames[socket].data(), 2, 1, {0, 0}, algorithm->pageIndices[socket].data()};
    }
    static constexpr uint8_t legacyIndices[] = {0, 1, 2};
    algorithm->pages[patch_helper::kNativeSockets] = {"Legacy bank 13", 3, 0, {0, 0}, legacyIndices};
    algorithm->pageList = {patch_helper::kSocketCount, algorithm->pages.data()};
    algorithm->parameterPages = &algorithm->pageList;
    return algorithm;
}
int selectedSocket(const Algorithm& algorithm) {
    return std::clamp(algorithm.v ? int(algorithm.v[0]) - 1 : 0, 0, algorithm.map.socketCount() - 1);
}

// Project the selected record into native controls without treating these
// host setter callbacks as new edits. Persistent cable data remains authoritative.
void projectControls(Algorithm& algorithm) {
    if (!algorithm.v) return;
    algorithm.projecting = true;
    const auto index = NT_algorithmIndex(&algorithm);
    const auto offset = NT_parameterOffset();
    const int socket = selectedSocket(algorithm);
    const auto& row = algorithm.map.connections[socket];
    const int values[] = {socket + 1, row.colour, row.tag};
    for (int p = 0; p < 3; ++p) {
        if (algorithm.v[p] != values[p]) NT_setParameterFromAudio(index, offset + p, values[p]);
    }
    for (int s = 0; s < std::min(algorithm.map.socketCount(), patch_helper::kNativeSockets); ++s) {
        const auto& connection = algorithm.map.connections[s];
        const int p = kLegacyParameters + 2 * s;
        if (algorithm.v[p] != connection.colour) NT_setParameterFromAudio(index, offset + p, connection.colour);
        if (algorithm.v[p + 1] != connection.tag) NT_setParameterFromAudio(index, offset + p + 1, connection.tag);
    }
    algorithm.projecting = false;
    algorithm.needsProjection = false;
}

void parameterChanged(_NT_algorithm* self, int p) {
    auto& algorithm = *static_cast<Algorithm*>(self);
    if (algorithm.projecting || !self->v || p < 0 || p >= kParameterCount) return;
    if (p >= kLegacyParameters) {
        const int socket = (p - kLegacyParameters) / 2;
        if (socket >= algorithm.map.socketCount()) return;
        auto& row = algorithm.map.connections[socket];
        const bool colour = (p - kLegacyParameters) % 2 == 0;
        int& stored = colour ? row.colour : row.tag;
        const int value = std::clamp(int(self->v[p]), 0, colour ? patch_helper::kColourCount - 1 : 12);
        if (stored != value) {
            algorithm.projecting = true;
            NT_setParameterFromAudio(NT_algorithmIndex(self), NT_parameterOffset(), socket + 1);
            algorithm.projecting = false;
            stored = value;
            if (algorithm.session.revision == patch_helper::kMaxWireInteger) algorithm.session = {};
            else ++algorithm.session.revision;
        }
        projectControls(algorithm);
        return;
    }
    if (p == 0) { projectControls(algorithm); return; }
    auto& row = algorithm.map.connections[selectedSocket(algorithm)];
    const int value = std::clamp(int(self->v[p]), 0, p == 1 ? patch_helper::kColourCount - 1 : 12);
    int& stored = p == 1 ? row.colour : row.tag;
    if (stored != value) {
        stored = value;
        if (algorithm.session.revision == patch_helper::kMaxWireInteger) algorithm.session = {};
        else ++algorithm.session.revision;
    }
    projectControls(algorithm);
}

// A descriptive map never touches audio, CV, or routing.
void step(_NT_algorithm* self, float*, int) {
    auto& algorithm = *static_cast<Algorithm*>(self);
    if (algorithm.definitions[0].max != algorithm.map.socketCount()) {
        algorithm.definitions[0].max = algorithm.map.socketCount();
        NT_updateParameterDefinition(NT_algorithmIndex(self), 0);
        algorithm.needsProjection = true;
    }
    const auto pageCount = static_cast<uint32_t>(std::min(algorithm.map.socketCount(), patch_helper::kNativeSockets + 1));
    if (algorithm.pageList.numPages != pageCount) {
        algorithm.pageList.numPages = pageCount;
        NT_updateParameterPages(NT_algorithmIndex(self));
        algorithm.needsProjection = true;
    }
    if (algorithm.needsProjection) projectControls(algorithm);
}

bool draw(_NT_algorithm* self) {
    const auto& algorithm = *static_cast<const Algorithm*>(self);

    int first = self->v ? self->v[0] - 1 : 0;
    if (first < 0) first = 0;
    if (first >= algorithm.map.socketCount()) first = algorithm.map.socketCount() - 1;
    for (int row = 0; row < 4 && first + row < algorithm.map.socketCount(); ++row) {
        const int socket = first + row;
        const auto& connection = algorithm.map.connections[socket];
        char label[16]{};
        if (socket >= 20) {
            std::strcpy(label, "E"); NT_intToString(label + 1, (socket - 20) / 8 + 1);
            std::strcat(label, ":"); NT_intToString(label + std::strlen(label), (socket - 20) % 8 + 1);
        } else {
            const bool input = socket < 12;
            std::strcpy(label, input ? "In " : "Out ");
            NT_intToString(label + std::strlen(label), input ? socket + 1 : socket - 11);
        }
        const int y = 21 + row * 13;
        NT_drawText(0, y, label, 15, kNT_textLeft, kNT_textTiny);
        NT_drawText(33, y, patch_helper::kColours[connection.colour], 15, kNT_textLeft, kNT_textTiny);
        // The complete destination remains in preset state; screen clipping is
        // presentation only. The tiny font fits 44 characters in this column.
        char destination[45]{};
        std::strncpy(destination, connection.connected() ? connection.destination : "(unused)", 44);
        NT_drawText(77, y, destination, 15, kNT_textLeft, kNT_textTiny);
    }
    return false; // Keep the native parameter line visible above the rows.
}
int parameterUiPrefix(_NT_algorithm*, int p, char* text) {
    if (p < kLegacyParameters || p >= kParameterCount) return 0;
    socketName(text, (p - kLegacyParameters) / 2);
    std::strcat(text, " ");
    return static_cast<int>(std::strlen(text));
}
void serialise(_NT_algorithm* self, _NT_jsonStream& stream) {
    patch_helper::writeMap(static_cast<Algorithm*>(self)->map, stream);
}
bool deserialise(_NT_algorithm* self, _NT_jsonParse& parse) {
    auto& algorithm = *static_cast<Algorithm*>(self);
    if (!patch_helper::readMap(algorithm.map, parse, algorithm.scratch)) return false;
    algorithm.session = {};
    algorithm.needsProjection = true;
    return true;
}
void midiSysEx(const uint8_t* data, uint32_t size) {
    // Accept callback payloads with either delimiter retained by the host.
    if (size && data[0] == 0xf0) { ++data; --size; }
    if (size && data[size - 1] == 0xf7) --size;
    if (!patch_helper::isRequest(data, size)) return;
    _NT_slot slot;
    if (!NT_getSlot(slot, data[7]) || slot.guid() != NT_MULTICHAR('T', 'h', 'P', 'h')) return;
    auto* algorithm = static_cast<Algorithm*>(slot.plugin());
    if (!algorithm) return;
    struct Reply {
        uint8_t start = 0xf0;
        uint8_t payload[128]{};
    } reply;
    static_assert(offsetof(Reply, payload) == 1);
    const auto length = patch_helper::respond(algorithm->map, algorithm->session, data, size, reply.payload, selectedSocket(*algorithm));
    if (reply.payload[20] == 0 && (data[6] == 3 || data[6] == 8)) algorithm->needsProjection = true;
    // The host appends F7 when end=true; it does not supply the opening F0.
    NT_sendMidiSysEx(kNT_destinationUSB, reinterpret_cast<const uint8_t*>(&reply), static_cast<uint32_t>(length + 1), true);
}
constexpr _NT_factory makeFactory() {
    _NT_factory factory{};
    factory.guid = NT_MULTICHAR('T', 'h', 'P', 'h');
    factory.name = "Patch Helper";
    factory.description = "Physical cable reference (development preview)";
    factory.calculateRequirements = requirements;
    factory.construct = construct;
    factory.step = step;
    factory.parameterChanged = parameterChanged;
    factory.draw = draw;
    factory.parameterUiPrefix = parameterUiPrefix;
    factory.tags = kNT_tagUtility;
    factory.serialise = serialise;
    factory.deserialise = deserialise;
    factory.midiSysEx = midiSysEx;
    return factory;
}
constexpr auto pluginFactory = makeFactory();
}

#ifdef __arm__
_NT_DRAM_SECTION
#endif
uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
    case kNT_selector_version: return kNT_apiVersionCurrent;
    case kNT_selector_numFactories: return 1;
    case kNT_selector_factoryInfo: return data == 0 ? reinterpret_cast<uintptr_t>(&pluginFactory) : 0;
    default: return 0;
    }
}
