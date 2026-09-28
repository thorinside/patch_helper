#if defined(__arm__) && (!defined(__PIC__) || __PIC__ != 2)
#error "The NT hardware build requires -fPIC"
#endif
#include <cstddef>
#include <cmath>
#include <distingnt/api.h>
#include <distingnt/serialisation.h>
#include <distingnt/slot.h>
#include <new>
#include "patch_map.h"
#include "patch_protocol.h"

namespace {
// Keep the three preview-era parameter indices for saved mappings, but expose
// independent per-socket controls and formatted text on the native pages.
constexpr int kLegacyParameters = 3;
constexpr int kNumericParameters = kLegacyParameters + 2 * patch_helper::kNativeSockets;
constexpr int kNativeText = kNumericParameters;
constexpr int kExpanderText = kNativeText + 2 * patch_helper::kSocketCount;
constexpr int kBank = kExpanderText + 16;
constexpr int kBankName = kBank + 1;
constexpr int kLegacyText = kBankName + 1;
constexpr int kParameterCount = kLegacyText + 2;
constexpr int kBankPage = patch_helper::kSocketCount;
constexpr int kExpanderPages = kBankPage + 1;
constexpr int kCompatibilityPage = kExpanderPages + 8;
static_assert(kParameterCount <= 240);
struct Algorithm : _NT_algorithm {
    patch_helper::PatchMap map;
    patch_helper::PatchMap scratch;
    patch_helper::Session session;
    std::array<_NT_parameter, kParameterCount> definitions{};
    std::array<std::array<uint8_t, 4>, patch_helper::kSocketCount + 8> pageIndices{};
    std::array<std::array<char, 16>, patch_helper::kSocketCount + 8> pageNames{};
    std::array<_NT_parameterPage, kCompatibilityPage + 1> pages{};
    _NT_parameterPages pageList{};
    bool projecting = false;
    bool needsProjection = true;
    bool needsGray = true;
    int firstVisibleSocket = 0;
    int uiSocket = 0;
    int uiField = 0; // Editable fields: cable colour, tag. Text is read-only.
    int uiKnownValue = -1;
    float uiLastValuePot = 0.0f;
    bool uiValuePickup = true;
    int configuredBank = -1;
    int configuredExpanders = -1;
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
    }
    for (int p = kNativeText; p < kBank; p += 2) {
        algorithm->definitions[p] = {"Destination", 0, 0, 0, kNT_unitHasStrings, 0, nullptr};
        algorithm->definitions[p + 1] = {"Group", 0, 0, 0, kNT_unitHasStrings, 0, nullptr};
    }
    for (int socket = 0; socket < patch_helper::kSocketCount; ++socket) {
        const int colour = kLegacyParameters + 2 * socket;
        const int text = kNativeText + 2 * socket;
        algorithm->pageIndices[socket] = {static_cast<uint8_t>(text), static_cast<uint8_t>(colour),
            static_cast<uint8_t>(colour + 1), static_cast<uint8_t>(text + 1)};
        socketName(algorithm->pageNames[socket].data(), socket);
        algorithm->pages[socket] = {algorithm->pageNames[socket].data(), 4, 1, {0, 0}, algorithm->pageIndices[socket].data()};
    }
    algorithm->definitions[kBank] = {"Bank", 1, 1, 1, kNT_unitNone, 0, nullptr};
    algorithm->definitions[kBankName] = {"Name", 0, 0, 0, kNT_unitHasStrings, 0, nullptr};
    algorithm->definitions[kLegacyText] = algorithm->definitions[kNativeText];
    algorithm->definitions[kLegacyText + 1] = algorithm->definitions[kNativeText + 1];
    static constexpr uint8_t bankIndices[] = {kBank, kBankName};
    algorithm->pages[kBankPage] = {"Expander bank", 2, 0, {0, 0}, bankIndices};
    static constexpr uint8_t legacyIndices[] = {0, kLegacyText, 1, 2, kLegacyText + 1};
    algorithm->pages[kCompatibilityPage] = {"Other sockets", 5, 0, {0, 0}, legacyIndices};
    algorithm->pageList = {patch_helper::kSocketCount, algorithm->pages.data()};
    algorithm->parameterPages = &algorithm->pageList;
    return algorithm;
}
int selectedSocket(const Algorithm& algorithm) {
    return std::clamp(algorithm.v ? int(algorithm.v[0]) - 1 : 0, 0, algorithm.map.socketCount() - 1);
}

int selectedBank(const Algorithm& algorithm) {
    return std::clamp(algorithm.v ? int(algorithm.v[kBank]) - 1 : 0, 0,
        std::max(0, std::min(algorithm.map.expanderCount, patch_helper::kNativeExpanders) - 1));
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
    const int bank = selectedBank(algorithm) + 1;
    if (algorithm.v[kBank] != bank) NT_setParameterFromAudio(index, offset + kBank, bank);
    for (int p = kNativeText; p < kParameterCount; ++p) {
        if (p == kBank) continue;
        if (algorithm.v[p] != 0) NT_setParameterFromAudio(index, offset + p, 0);
        if (algorithm.needsGray) NT_setParameterGrayedOut(index, offset + p, true);
    }
    algorithm.needsGray = false;
    algorithm.projecting = false;
    algorithm.needsProjection = false;
}

void parameterChanged(_NT_algorithm* self, int p) {
    auto& algorithm = *static_cast<Algorithm*>(self);
    if (algorithm.projecting || !self->v || p < 0 || p >= kParameterCount) return;
    if (p == kBank) {
        if (algorithm.map.expanderCount) {
            algorithm.projecting = true;
            NT_setParameterFromAudio(NT_algorithmIndex(self), NT_parameterOffset(),
                patch_helper::kSocketCount + selectedBank(algorithm) * 8 + 1);
            algorithm.projecting = false;
        }
        algorithm.needsProjection = true;
        return;
    }
    if (p >= kNumericParameters) return; // Read-only text never changes map state.
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
    const int bankMax = std::max(1, std::min(algorithm.map.expanderCount, patch_helper::kNativeExpanders));
    if (algorithm.definitions[kBank].max != bankMax) {
        algorithm.definitions[kBank].max = bankMax;
        NT_updateParameterDefinition(NT_algorithmIndex(self), kBank);
        algorithm.needsProjection = true;
    }
    const int bank = selectedBank(algorithm);
    if (algorithm.configuredBank != bank || algorithm.configuredExpanders != algorithm.map.expanderCount) {
        // Only one bank's eight text pages are visible at a time. Every bank's
        // colour/tag parameters retain their own stable indices and mappings.
        for (int row = 0; row < 8; ++row) {
            const int socket = patch_helper::kSocketCount + bank * 8 + row;
            const int storage = patch_helper::kSocketCount + row;
            const int colour = kLegacyParameters + 2 * socket;
            const int text = kExpanderText + 2 * row;
            algorithm.pageIndices[storage] = {static_cast<uint8_t>(text), static_cast<uint8_t>(colour),
                static_cast<uint8_t>(colour + 1), static_cast<uint8_t>(text + 1)};
            socketName(algorithm.pageNames[storage].data(), socket);
            algorithm.pages[kExpanderPages + row] = {algorithm.pageNames[storage].data(), 4, 1,
                {0, 0}, algorithm.pageIndices[storage].data()};
        }
        algorithm.pageList.numPages = algorithm.map.expanderCount == 0 ? patch_helper::kSocketCount
            : kCompatibilityPage + (algorithm.map.expanderCount > patch_helper::kNativeExpanders ? 1 : 0);
        algorithm.configuredBank = bank;
        algorithm.configuredExpanders = algorithm.map.expanderCount;
        NT_updateParameterPages(NT_algorithmIndex(self));
        algorithm.needsProjection = true;
    }
    if (algorithm.needsProjection) projectControls(algorithm);
}

// Anchor by socket identity so updates above the viewport do not move it.
// This bounded scan uses only stack storage and never changes preset state.
int connectedWindow(Algorithm& algorithm, std::array<int, patch_helper::kMaxSockets>& sockets,
                    int& count, int delta = 0) {
    count = 0;
    int first = 0;
    for (int socket = 0; socket < algorithm.map.socketCount(); ++socket) {
        if (!algorithm.map.connections[socket].connected()) continue;
        if (socket < algorithm.firstVisibleSocket) ++first;
        sockets[count++] = socket;
    }
    const int last = std::max(0, count - 4);
    first = std::clamp(std::clamp(first, 0, last) + delta, 0, last);
    algorithm.firstVisibleSocket = count ? sockets[first] : 0;
    return first;
}
int uiValue(const Algorithm& algorithm) {
    const auto& row = algorithm.map.connections[std::clamp(algorithm.uiSocket, 0, algorithm.map.socketCount() - 1)];
    return algorithm.uiField ? row.tag : row.colour;
}
int uiMaximum(const Algorithm& algorithm) { return algorithm.uiField ? 12 : patch_helper::kColourCount - 1; }
uint32_t hasCustomUi(_NT_algorithm*) {
    return kNT_potL | kNT_potC | kNT_potR | kNT_encoderL | kNT_encoderR | kNT_encoderButtonL;
}
void setupUi(_NT_algorithm* self, _NT_float3& pots) {
    auto& a = *static_cast<Algorithm*>(self);
    a.uiSocket = std::clamp(a.uiSocket, 0, a.map.socketCount() - 1);
    pots[0] = float(a.uiSocket) / (a.map.socketCount() - 1);
    pots[1] = float(a.uiField);
    pots[2] = float(uiValue(a)) / uiMaximum(a);
    a.uiLastValuePot = pots[2];
    a.uiKnownValue = uiValue(a);
    a.uiValuePickup = false; // Host soft takeover is initialized by setupUi().
}
int potIndex(float position, int maximum) {
    return int(std::clamp(position, 0.0f, 1.0f) * maximum + 0.5f);
}
void customUi(_NT_algorithm* self, const _NT_uiData& data) {
    auto& a = *static_cast<Algorithm*>(self);
    std::array<int, patch_helper::kMaxSockets> sockets{};
    int count;
    if (data.controls & kNT_encoderButtonL) {
        connectedWindow(a, sockets, count, data.encoders[0]);
        // Scrolling never selects a channel or edits a cable, even if a pot moves.
        if (std::isfinite(data.pots[2])) a.uiLastValuePot = data.pots[2];
        a.uiValuePickup = true;
        return;
    }
    const int previousSocket = a.uiSocket;
    const int previousField = a.uiField;
    a.uiSocket = std::clamp(a.uiSocket, 0, a.map.socketCount() - 1);
    if ((data.controls & kNT_potL) && std::isfinite(data.pots[0]))
        a.uiSocket = potIndex(data.pots[0], a.map.socketCount() - 1);
    a.uiSocket = std::clamp(a.uiSocket + data.encoders[0], 0, a.map.socketCount() - 1);
    if ((data.controls & kNT_potC) && std::isfinite(data.pots[1])) a.uiField = potIndex(data.pots[1], 1);
    const bool selectionChanged = a.uiSocket != previousSocket || a.uiField != previousField;
    if (a.uiSocket != previousSocket) {
        const int first = connectedWindow(a, sockets, count);
        for (int i = 0; i < count; ++i) {
            if (sockets[i] != a.uiSocket) continue;
            if (i < first) connectedWindow(a, sockets, count, i - first);
            else if (i >= first + 4) connectedWindow(a, sockets, count, i - first - 3);
            break;
        }
    }
    int value = uiValue(a);
    const int maximum = uiMaximum(a);
    if (selectionChanged || a.uiKnownValue != value) a.uiValuePickup = true;
    if (selectionChanged && std::isfinite(data.pots[2])) a.uiLastValuePot = data.pots[2];
    if (!selectionChanged && (data.controls & kNT_potR) && std::isfinite(data.pots[2])) {
        const float position = std::clamp(data.pots[2], 0.0f, 1.0f);
        const float target = float(value) / maximum;
        if (potIndex(position, maximum) == value ||
            (a.uiLastValuePot <= target && position >= target) ||
            (a.uiLastValuePot >= target && position <= target)) a.uiValuePickup = false;
        if (!a.uiValuePickup) value = potIndex(position, maximum);
        a.uiLastValuePot = position;
    }
    if (data.encoders[1]) {
        value = std::clamp(value + data.encoders[1], 0, maximum);
        a.uiValuePickup = true;
    }
    a.uiKnownValue = value;
    if (!self->v || value == uiValue(a)) return;
    const auto index = NT_algorithmIndex(self);
    const auto offset = NT_parameterOffset();
    if (a.uiSocket < patch_helper::kNativeSockets) {
        NT_setParameterFromUi(index, offset + kLegacyParameters + 2 * a.uiSocket + a.uiField, value);
    } else {
        // Legacy larger maps retain editable numeric fields via the old selector.
        NT_setParameterFromUi(index, offset, a.uiSocket + 1);
        NT_setParameterFromUi(index, offset + 1 + a.uiField, value);
    }
}
bool draw(_NT_algorithm* self) {
    auto& algorithm = *static_cast<Algorithm*>(self);
    algorithm.uiSocket = std::clamp(algorithm.uiSocket, 0, algorithm.map.socketCount() - 1);
    char channel[16]{};
    socketName(channel, algorithm.uiSocket);
    char value[16]{};
    if (algorithm.uiField) {
        if (uiValue(algorithm)) NT_intToString(value, uiValue(algorithm));
        else std::strcpy(value, "-");
    } else std::strcpy(value, patch_helper::kColours[uiValue(algorithm)]);
    NT_drawShapeI(kNT_rectangle, 0, 0, 255, 11, 2);
    NT_drawText(1, 9, channel, 15, kNT_textLeft, kNT_textNormal);
    NT_drawText(87, 9, algorithm.uiField ? "Tag" : "Cable colour", 15, kNT_textLeft, kNT_textNormal);
    NT_drawText(174, 9, value, 15, kNT_textLeft, kNT_textNormal);
    std::array<int, patch_helper::kMaxSockets> sockets{};
    int count;
    const int first = connectedWindow(algorithm, sockets, count);
    for (int row = 0; row < 4 && first + row < count; ++row) {
        const int socket = sockets[first + row];
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
        NT_drawText(0, y, label, socket == algorithm.uiSocket ? 15 : 8, kNT_textLeft, kNT_textTiny);
        NT_drawText(33, y, patch_helper::kColours[connection.colour], 15, kNT_textLeft, kNT_textTiny);
        // The complete destination remains in preset state; screen clipping is
        // presentation only. The tiny font fits 44 characters in this column.
        char destination[45]{};
        std::strncpy(destination, connection.destination, 44);
        NT_drawText(77, y, destination, 15, kNT_textLeft, kNT_textTiny);
    }
    return true; // The SDK cannot select native UI focus; render the matching control row.
}
int textSocket(const Algorithm& algorithm, int p) {
    if (p >= kLegacyText) return selectedSocket(algorithm);
    if (p >= kExpanderText) return patch_helper::kSocketCount + selectedBank(algorithm) * 8 + (p - kExpanderText) / 2;
    return (p - kNativeText) / 2;
}
int parameterUiPrefix(_NT_algorithm* self, int p, char* text) {
    if (p < kLegacyParameters || p >= kParameterCount || p == kBank || p == kBankName) return 0;
    const auto& algorithm = *static_cast<Algorithm*>(self);
    socketName(text, p < kNumericParameters ? (p - kLegacyParameters) / 2 : textSocket(algorithm, p));
    std::strcat(text, " ");
    return static_cast<int>(std::strlen(text));
}
// Fixed-value, greyed-out properties return the complete descriptive text.
int parameterString(_NT_algorithm* self, int p, int, char* text) {
    if (p < kNativeText || p >= kParameterCount || p == kBank) return 0;
    const auto& algorithm = *static_cast<Algorithm*>(self);
    const char* value = "";
    if (p == kBankName) {
        if (algorithm.map.expanderCount) value = algorithm.map.expanders[selectedBank(algorithm)].name;
    } else {
        const int socket = textSocket(algorithm, p);
        if (socket >= algorithm.map.socketCount()) return 0;
        const int base = p >= kLegacyText ? kLegacyText : p >= kExpanderText ? kExpanderText : kNativeText;
        const auto& row = algorithm.map.connections[socket];
        value = (p - base) % 2 == 0 ? row.destination : row.group;
    }
    // New text is 32 characters; preserved legacy destinations also fit in 64 bytes.
    std::strcpy(text, *value ? value : "-");
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
    algorithm.needsGray = true;
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
    factory.hasCustomUi = hasCustomUi;
    factory.customUi = customUi;
    factory.setupUi = setupUi;
    factory.parameterUiPrefix = parameterUiPrefix;
    factory.parameterString = parameterString;
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
