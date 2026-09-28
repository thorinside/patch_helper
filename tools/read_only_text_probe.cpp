// Diagnostic-only hardware probe for read-only text properties. Never packaged as Patch Helper.
#include <distingnt/api.h>
#include <cstring>
#include <new>
struct Probe : _NT_algorithm { bool grayed = false; };
constexpr const char* colours[] = {"None", "Purple"};
constexpr _NT_parameter parameters[] = {
    {"Destination", 0, 0, 0, kNT_unitHasStrings, 0, nullptr},
    {"Cable colour", 0, 1, 1, kNT_unitEnum, 0, colours},
    {"Tag", 0, 12, 1, kNT_unitNone, 0, nullptr},
    {"Group", 0, 0, 0, kNT_unitHasStrings, 0, nullptr},
};
constexpr uint8_t indices[] = {0, 1, 2, 3};
constexpr _NT_parameterPage pages[] = {{"Input 1", 4, 1, {0, 0}, indices}};
constexpr _NT_parameterPages pageList = {1, pages};
void requirements(_NT_algorithmRequirements& r, const int32_t*) { r = {}; r.numParameters = 4; r.sram = sizeof(Probe); }
_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& m, const _NT_algorithmRequirements&, const int32_t*) {
    auto* p = new(m.sram) Probe(); p->parameters = parameters; p->parameterPages = &pageList; return p;
}
void step(_NT_algorithm* self, float*, int) {
    auto& p = *static_cast<Probe*>(self);
    if (!p.grayed) {
        NT_setParameterGrayedOut(NT_algorithmIndex(self), NT_parameterOffset(), true);
        NT_setParameterGrayedOut(NT_algorithmIndex(self), NT_parameterOffset() + 3, true);
        p.grayed = true;
    }
}
int format(_NT_algorithm*, int p, int, char* text) {
    if (p != 0 && p != 3) return 0;
    std::strcpy(text, p == 0 ? "From Beads L" : "FX"); return std::strlen(text);
}
constexpr _NT_factory makeFactory() {
    _NT_factory f{}; f.guid = NT_MULTICHAR('T','h','R','d'); f.name = "Read-only text probe"; f.description = "Temporary grey-property test";
    f.calculateRequirements = requirements; f.construct = construct; f.step = step; f.parameterString = format;
    f.tags = kNT_tagUtility; return f;
}
constexpr auto factory = makeFactory();
_NT_DRAM_SECTION
uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch(selector) {
    case kNT_selector_version: return kNT_apiVersionCurrent;
    case kNT_selector_numFactories: return 1;
    case kNT_selector_factoryInfo: return data ? 0 : reinterpret_cast<uintptr_t>(&factory);
    default: return 0;
    }
}
