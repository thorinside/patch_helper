#include <distingnt/api.h>
// Host-controlled memory and parameter storage let the ARM harness check
// the actual startup callbacks with poisoned SRAM and strict alignment.
extern "C" uint32_t probeSram() {
    auto* factory = reinterpret_cast<const _NT_factory*>(pluginEntry(kNT_selector_factoryInfo, 0));
    _NT_algorithmRequirements req{};
    factory->calculateRequirements(req, nullptr);
    return req.sram;
}

extern "C" void probe(uint8_t* memory, int16_t* values) {
    auto* factory = reinterpret_cast<const _NT_factory*>(pluginEntry(kNT_selector_factoryInfo, 0));
    _NT_algorithmRequirements req{};
    factory->calculateRequirements(req, nullptr);
    _NT_algorithmMemoryPtrs ptrs{memory, nullptr, nullptr, nullptr};
    auto* a = factory->construct(ptrs, req, nullptr);
    a->v = values;
    for (int p = 0; p < 3; ++p) factory->parameterChanged(a, p);
    factory->step(a, nullptr, 16);
    factory->draw(a);
}
