#include <cstddef>
#include <distingnt/api.h>
#include <distingnt/serialisation.h>
#include <distingnt/slot.h>
#include <new>
#include "patch_map.h"
#include "patch_protocol.h"

namespace {
struct Algorithm : _NT_algorithm {
    patch_helper::PatchMap map;
    patch_helper::PatchMap scratch;
    patch_helper::Session session;
    _NT_parameter viewParameter{};
};
constexpr _NT_parameter parameters[] = {
    {"First socket", 1, 20, 1, kNT_unitNone, 0, nullptr},
};
constexpr uint8_t viewParams[] = {0};
constexpr _NT_parameterPage pages[] = {{"View", 1, 0, {0, 0}, viewParams}};
constexpr _NT_parameterPages parameterPages = {1, pages};

void requirements(_NT_algorithmRequirements& req, const int32_t*) {
    req = {};
    req.numParameters = 1;
    req.sram = sizeof(Algorithm);
}
_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs,
                         const _NT_algorithmRequirements&, const int32_t*) {
    auto* algorithm = new (ptrs.sram) Algorithm();
    algorithm->viewParameter = parameters[0];
    algorithm->parameters = &algorithm->viewParameter;
    algorithm->parameterPages = &parameterPages;
    return algorithm;
}
// A descriptive map must never touch audio, CV, or routing.
void step(_NT_algorithm* self, float*, int) {
    auto& algorithm = *static_cast<Algorithm*>(self);
    if (algorithm.viewParameter.max != algorithm.map.socketCount()) {
        algorithm.viewParameter.max = algorithm.map.socketCount();
        NT_updateParameterDefinition(NT_algorithmIndex(self), 0);
    }
}

bool draw(_NT_algorithm* self) {
    const auto& algorithm = *static_cast<const Algorithm*>(self);
    NT_drawText(0, 9, algorithm.map.title);
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
    return true;
}
void serialise(_NT_algorithm* self, _NT_jsonStream& stream) {
    patch_helper::writeMap(static_cast<Algorithm*>(self)->map, stream);
}
bool deserialise(_NT_algorithm* self, _NT_jsonParse& parse) {
    auto& algorithm = *static_cast<Algorithm*>(self);
    if (!patch_helper::readMap(algorithm.map, parse, algorithm.scratch)) return false;
    algorithm.session = {};
    return true;
}
void midiSysEx(const uint8_t* data, uint32_t size) {
    // Firmware callbacks use unframed data; accepting framed input also makes
    // the development bridge usable with hosts that retain MIDI delimiters.
    if (size >= 2 && data[0] == 0xf0 && data[size - 1] == 0xf7) { ++data; size -= 2; }
    if (!patch_helper::isRequest(data, size)) return;
    _NT_slot slot;
    if (!NT_getSlot(slot, data[7]) || slot.guid() != NT_MULTICHAR('T', 'h', 'P', 'h')) return;
    auto* algorithm = static_cast<Algorithm*>(slot.plugin());
    if (!algorithm) return;
    uint8_t reply[128]{};
    const auto length = patch_helper::respond(algorithm->map, algorithm->session, data, size, reply);
    NT_sendMidiSysEx(kNT_destinationUSB, reply, static_cast<uint32_t>(length), true);
}
constexpr _NT_factory makeFactory() {
    _NT_factory factory{};
    factory.guid = NT_MULTICHAR('T', 'h', 'P', 'h');
    factory.name = "Patch Helper";
    factory.description = "Physical cable reference (development preview)";
    factory.calculateRequirements = requirements;
    factory.construct = construct;
    factory.step = step;
    factory.draw = draw;
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
