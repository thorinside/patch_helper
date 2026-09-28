// Diagnostic-only plug-in. Never packaged as Patch Helper.
// Unit 18 matches the live firmware's Mixer Mono channel Name definition.
#if defined(__arm__) && (!defined(__PIC__) || __PIC__ != 2)
#error "The NT hardware build requires -fPIC"
#endif
#include <cstddef>
#include <distingnt/api.h>
#include <distingnt/serialisation.h>
#include <distingnt/slot.h>
#include <new>

namespace {
struct Probe : _NT_algorithm {
    int changes = 0;
    int lastParameter = -1;
    int lastValue = -1;
    int formats = 0;
    int formatParameter = -1;
    int formatValue = -1;
    int nativeStringMessages = 0;
    int controlMessages = 0;
    char text[kNT_parameterStringSize] = "Probe";
};
constexpr _NT_parameter parameters[] = {
    {"Native text", 0, 0, 0, 18, 0, nullptr},
    {"Numeric control", 0, 127, 0, kNT_unitNone, 0, nullptr},
    {"Gain example type", 0, 0, 0, kNT_unitHasStrings, 0, nullptr},
};
constexpr uint8_t indices[] = {0, 1, 2};
constexpr _NT_parameterPage pages[] = {{"Probe", 3, 0, {0, 0}, indices}};
constexpr _NT_parameterPages pageList{1, pages};
void requirements(_NT_algorithmRequirements& req, const int32_t*) {
    req = {};
    req.numParameters = 3;
    req.sram = sizeof(Probe);
}
_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs,
                         const _NT_algorithmRequirements&, const int32_t*) {
    auto* p = new (ptrs.sram) Probe();
    p->parameters = parameters;
    p->parameterPages = &pageList;
    return p;
}
void changed(_NT_algorithm* self, int parameter) {
    auto& p = *static_cast<Probe*>(self);
    if (p.changes < 1000000) ++p.changes;
    p.lastParameter = parameter;
    p.lastValue = self->v && parameter >= 0 && parameter < 3 ? self->v[parameter] : -1;
}
int format(_NT_algorithm* self, int parameter, int value, char* buffer) {
    auto& p = *static_cast<Probe*>(self);
    if (p.formats < 1000000) ++p.formats;
    p.formatParameter = parameter;
    p.formatValue = value;
    if (parameter != 0 && parameter != 2) { buffer[0] = 0; return 0; }
    int length = 0;
    while (length < kNT_parameterStringSize - 1 && p.text[length]) {
        buffer[length] = p.text[length];
        ++length;
    }
    buffer[length] = 0;
    return length;
}
void midiSysEx(const uint8_t* data, uint32_t size) {
    if (size && data[0] == 0xf0) { ++data; --size; }
    if (size && data[size - 1] == 0xf7) --size;
    // Private diagnostic control: 7D ThTp <ASCII text>.
    const bool control = size >= 5 && data[0] == 0x7d && data[1] == 'T' &&
        data[2] == 'h' && data[3] == 'T' && data[4] == 'p';
    const bool native = size >= 10 && data[0] == 0 && data[1] == 0x21 &&
        data[2] == 0x27 && data[3] == 0x6d && data[5] == 0x53;
    if (!control && !native) return;
    for (uint32_t index = 0; index < NT_algorithmCount(); ++index) {
        _NT_slot slot;
        if (!NT_getSlot(slot, index) || slot.guid() != NT_MULTICHAR('T', 'h', 'T', 'p')) continue;
        auto* p = static_cast<Probe*>(slot.plugin());
        if (!p) continue;
        if (native && (data[6] != index || data[7] || data[8] ||
            (data[9] != NT_parameterOffset() && data[9] != NT_parameterOffset() + 2))) continue;
        int& counter = control ? p->controlMessages : p->nativeStringMessages;
        if (counter < 1000000) ++counter;
        const uint32_t start = control ? 5 : 10;
        uint32_t n = 0;
        while (n < kNT_parameterStringSize - 1 && start + n < size && data[start + n]) {
            p->text[n] = static_cast<char>(data[start + n]);
            ++n;
        }
        p->text[n] = 0;
    }
}
void step(_NT_algorithm*, float*, int) {}
bool draw(_NT_algorithm* self) {
    const auto& p = *static_cast<Probe*>(self);
    char buffer[16]{};
    NT_drawText(0, 24, "Changes:");
    NT_intToString(buffer, p.changes);
    NT_drawText(80, 24, buffer);
    NT_drawText(0, 38, "Last parameter:");
    NT_intToString(buffer, p.lastParameter);
    NT_drawText(110, 38, buffer);
    return false;
}
void serialise(_NT_algorithm* self, _NT_jsonStream& stream) {
    const auto& p = *static_cast<Probe*>(self);
    char text[kNT_parameterStringSize]{};
    _NT_slot slot;
    if (NT_getSlot(slot, NT_algorithmIndex(self)))
        slot.parameterString(NT_parameterOffset(), text);
    stream.addMemberName("native_text_probe");
    stream.openObject();
    stream.addMemberName("changes"); stream.addNumber(p.changes);
    stream.addMemberName("lastParameter"); stream.addNumber(p.lastParameter);
    stream.addMemberName("lastValue"); stream.addNumber(p.lastValue);
    stream.addMemberName("formats"); stream.addNumber(p.formats);
    stream.addMemberName("formatParameter"); stream.addNumber(p.formatParameter);
    stream.addMemberName("formatValue"); stream.addNumber(p.formatValue);
    stream.addMemberName("slotString"); stream.addString(text);
    stream.addMemberName("nativeStringMessages"); stream.addNumber(p.nativeStringMessages);
    stream.addMemberName("controlMessages"); stream.addNumber(p.controlMessages);
    stream.closeObject();
}
constexpr _NT_factory factory() {
    _NT_factory f{};
    f.guid = NT_MULTICHAR('T', 'h', 'T', 'p');
    f.name = "Native Text Probe";
    f.description = "Diagnostic, not a user plug-in";
    f.calculateRequirements = requirements;
    f.construct = construct;
    f.parameterChanged = changed;
    f.parameterString = format;
    f.step = step;
    f.draw = draw;
    f.serialise = serialise;
    f.midiSysEx = midiSysEx;
    f.tags = kNT_tagUtility;
    return f;
}
constexpr auto definition = factory();
}
_NT_DRAM_SECTION
uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
    case kNT_selector_version: return kNT_apiVersionCurrent;
    case kNT_selector_numFactories: return 1;
    case kNT_selector_factoryInfo: return data ? 0 : reinterpret_cast<uintptr_t>(&definition);
    default: return 0;
    }
}
