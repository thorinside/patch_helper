#include <cstddef>
#define _DISTINGNT_SERIALISATION_INTERNAL
#include <distingnt/api.h>
#include <distingnt/serialisation.h>
#define _DISTINGNT_SLOT_INTERNAL
#include <distingnt/slot.h>
#include "patch_map.h"
#include "patch_protocol.h"
#include "json_adapter.h"
#include <cassert>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

_NT_jsonStream::_NT_jsonStream(void* value) : refCon(value) {}
_NT_jsonStream::~_NT_jsonStream() = default;
void _NT_jsonStream::addMemberName(const char* value) { static_cast<JsonWriter*>(refCon)->addMemberName(value); }
void _NT_jsonStream::openObject() { static_cast<JsonWriter*>(refCon)->openObject(); }
void _NT_jsonStream::closeObject() { static_cast<JsonWriter*>(refCon)->closeObject(); }
void _NT_jsonStream::openArray() { static_cast<JsonWriter*>(refCon)->openArray(); }
void _NT_jsonStream::closeArray() { static_cast<JsonWriter*>(refCon)->closeArray(); }
void _NT_jsonStream::addNumber(int value) { static_cast<JsonWriter*>(refCon)->addNumber(value); }
void _NT_jsonStream::addString(const char* value) { static_cast<JsonWriter*>(refCon)->addString(value); }
_NT_jsonParse::_NT_jsonParse(void* value, int index) : refCon(value), i(index) {}
_NT_jsonParse::~_NT_jsonParse() = default;
bool _NT_jsonParse::numberOfObjectMembers(int& value) { return static_cast<JsonReader*>(refCon)->numberOfObjectMembers(value); }
bool _NT_jsonParse::numberOfArrayElements(int& value) { return static_cast<JsonReader*>(refCon)->numberOfArrayElements(value); }
bool _NT_jsonParse::matchName(const char* value) { return static_cast<JsonReader*>(refCon)->matchName(value); }
bool _NT_jsonParse::skipMember() { return static_cast<JsonReader*>(refCon)->skipMember(); }
bool _NT_jsonParse::number(int& value) { return static_cast<JsonReader*>(refCon)->number(value); }
bool _NT_jsonParse::string(const char*& value) { return static_cast<JsonReader*>(refCon)->string(value); }

std::vector<std::string> drawn;
void NT_drawText(int, int, const char* text, int, _NT_textAlignment, _NT_textSize) { drawn.emplace_back(text); }
int NT_intToString(char* output, int32_t value) {
    char text[12];
    const auto result = std::to_chars(text, text + sizeof(text), value);
    const auto length = result.ptr - text;
    std::memcpy(output, text, length);
    output[length] = 0;
    return static_cast<int>(length);
}

_NT_algorithm* activeAlgorithm = nullptr;
std::vector<uint8_t> midiReply;
bool NT_getSlot(_NT_slot& slot, uint32_t index) { slot.refCon = activeAlgorithm; return index == 0; }
uint32_t _NT_slot::guid() const { return NT_MULTICHAR('T', 'h', 'P', 'h'); }
_NT_algorithm* _NT_slot::plugin() const { return static_cast<_NT_algorithm*>(refCon); }
void NT_sendMidiSysEx(uint32_t destination, const uint8_t* data, uint32_t size, bool end) {
    assert(destination == kNT_destinationUSB && end);
    midiReply.assign(data, data + size);
}

void testProtocolBounds() {
    using namespace patch_helper;
    PatchMap map;
    Session session{43, 0};
    uint8_t reply[128]{};
    std::vector<uint8_t> request(20, 0);
    std::copy(std::begin(kPrefix), std::end(kPrefix), request.begin());
    request[6] = 4; request[12] = 43;
    request.push_back(63); request.insert(request.end(), 63, 'T');
    assert(isRequest(request.data(), request.size()));
    assert(respond(map, session, request.data(), request.size(), reply) == 21);
    assert(reply[20] == 0 && std::strlen(map.title) == 63);
    request.resize(20); request[6] = 3; request[16] = 1;
    request.insert(request.end(), {19, 11, 12, 63});
    request.insert(request.end(), 63, 'D'); request.push_back(31);
    request.insert(request.end(), 31, 'G');
    const auto original = map;
    for (std::size_t size = 20; size < request.size(); ++size) {
        assert(isRequest(request.data(), size));
        respond(map, session, request.data(), size, reply);
        assert(reply[20] == 1 && session.revision == 1);
        assert(std::memcmp(&map, &original, sizeof(map)) == 0);
    }
    respond(map, session, request.data(), request.size(), reply);
    assert(reply[20] == 0 && session.revision == 2);
    request.resize(21); request[6] = 2; request[16] = 2; request[20] = 19;
    assert(respond(map, session, request.data(), request.size(), reply) == 120);
    assert(reply[20] == 0 && reply[21] == 19 && reply[22] == 11 && reply[23] == 12);
    session.revision = kMaxWireInteger;
    request.resize(20); request[6] = 4;
    writeInteger(request.data() + 16, kMaxWireInteger);
    request.push_back(0);
    respond(map, session, request.data(), request.size(), reply);
    assert(reply[20] == 2 && std::strlen(map.title) == 63);
    request.resize(20); request[6] = 1;
    respond(map, session, request.data(), request.size(), reply);
    assert(reply[20] == 1); // A wrap must never retain the old lease.
    request[12] = 44;
    respond(map, session, request.data(), request.size(), reply);
    assert(reply[20] == 0 && session.revision == 0 && session.lease == 44);
}

int main(int argc, char** argv) {
    testProtocolBounds();
    assert(argc == 2);
    std::ifstream input(argv[1]);
    const auto fixture = Json::parse(input);
    const auto* factory = reinterpret_cast<const _NT_factory*>(pluginEntry(kNT_selector_factoryInfo, 0));
    assert(factory && factory->guid == NT_MULTICHAR('T', 'h', 'P', 'h'));
    assert(pluginEntry(kNT_selector_factoryInfo, 1) == 0);
    assert(pluginEntry(kNT_selector_version, 0) == kNT_apiVersion13);
    assert(pluginEntry(kNT_selector_numFactories, 0) == 1);
    _NT_algorithmRequirements req{};
    factory->calculateRequirements(req, nullptr);
    assert(req.numParameters == 1 && req.dtc == 0 && req.dram == 0 && req.itc == 0);
    std::vector<std::max_align_t> memory((req.sram + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t));
    _NT_algorithmMemoryPtrs ptrs{reinterpret_cast<uint8_t*>(memory.data()), nullptr, nullptr, nullptr};
    auto* algorithm = factory->construct(ptrs, req, nullptr);
    activeAlgorithm = algorithm;
    int16_t page = 1;
    algorithm->v = &page;
    const auto load = [&](const Json& json) {
        JsonReader reader(json); _NT_jsonParse parse(&reader, 0);
        return factory->deserialise(algorithm, parse);
    };
    const auto save = [&]() {
        JsonWriter writer; _NT_jsonStream stream(&writer);
        factory->serialise(algorithm, stream); return writer.value;
    };
    std::ifstream wireInput("tests/fixtures/midi-session.json");
    const auto frames = Json::parse(wireInput);
    for (const auto& frame : frames) {
        const auto inputBytes = frame["request"].get<std::vector<uint8_t>>();
        const auto expectedBytes = frame["response"].get<std::vector<uint8_t>>();
        factory->midiSysEx(inputBytes.data(), inputBytes.size());
        assert(std::vector<uint8_t>(expectedBytes.begin() + 1, expectedBytes.end() - 1) == midiReply);
    }
    assert(load(Json::object()));
    // Exercise the real factory MIDI callback, including malformed and stale writes.
    std::vector<uint8_t> request(20, 0);
    std::copy(std::begin(patch_helper::kPrefix), std::end(patch_helper::kPrefix), request.begin());
    request[6] = 1; request[8] = 9; request[12] = 42;
    factory->midiSysEx(request.data(), request.size());
    assert(midiReply[6] == 0x41 && midiReply[20] == 0 && midiReply[21] == 12);
    request[6] = 3;
    request.insert(request.end(), {0, 4, 7, 4, 'E', 'c', 'h', 'o', 0});
    factory->midiSysEx(request.data(), request.size());
    assert(midiReply[20] == 0 && patch_helper::readInteger(midiReply.data() + 16) == 1);
    assert(save()["patch_helper"]["connections"][0]["destination"] == "Echo");
    factory->midiSysEx(request.data(), request.size());
    assert(midiReply[20] == 3); // Duplicate or delayed writes cannot apply twice.
    request[16] = 1; request[12] = 43;
    factory->midiSysEx(request.data(), request.size()); assert(midiReply[20] == 2);
    request[12] = 42; request.back() = 32;
    factory->midiSysEx(request.data(), request.size()); assert(midiReply[20] == 1);
    assert(save()["patch_helper"]["connections"][0]["destination"] == "Echo");
    assert(load(Json::object())); // Preset replacement invalidates an old editor.
    request.back() = 0; request[16] = 0;
    factory->midiSysEx(request.data(), request.size()); assert(midiReply[20] == 2);
    request[0] = 0x7e; midiReply.clear();
    factory->midiSysEx(request.data(), request.size()); assert(midiReply.empty());
    const auto defaults = save();
    assert(defaults["patch_helper"]["connections"].size() == 20);
    assert(load(fixture));
    assert(save() == fixture);
    factory->draw(algorithm);
    assert(drawn[0] == "Studio patch" && drawn[1] == "In 1");
    const auto saved = save();
    const auto reject = [&](Json json) { assert(!load(json)); assert(save() == saved); };
    auto broken = fixture; broken["patch_helper"]["version"] = 2; reject(broken);
    broken = fixture; broken["patch_helper"]["title"] = std::string(64, 'x'); reject(broken);
    broken = fixture; broken["patch_helper"]["title"] = "bad\ntext"; reject(broken);
    broken = fixture; broken["patch_helper"]["title"] = "café"; reject(broken);
    broken = fixture; broken["patch_helper"]["extra"] = true; reject(broken);
    broken = fixture; broken["patch_helper"].erase("title"); reject(broken);
    for (const auto& key : {"socket", "colour", "tag"}) {
        for (const auto& value : {Json(-1), Json(999), Json(1.5), Json("1"), Json(nullptr)}) {
            broken = fixture; broken["patch_helper"]["connections"][0][key] = value; reject(broken);
        }
    }
    broken = fixture; broken["patch_helper"]["connections"][1]["socket"] = 0; reject(broken);
    broken = fixture; broken["patch_helper"]["connections"].erase(0); reject(broken);
    broken = fixture; broken["patch_helper"]["connections"][0]["group"] = std::string(32, 'x'); reject(broken);
    broken = fixture; broken["patch_helper"]["connections"][0]["destination"] = std::string(64, 'x'); reject(broken);
    // Input order is immaterial; socket identities define the stored order.
    auto reordered = fixture;
    std::reverse(reordered["patch_helper"]["connections"].begin(), reordered["patch_helper"]["connections"].end());
    assert(load(reordered)); assert(save() == saved);
    // Other slot properties belong to the firmware and must be skipped.
    auto slot = fixture; slot["guid"] = "ThPh"; slot["parameters"] = Json::array({1});
    assert(load(slot)); assert(save() == saved);
    for (int frames : {0, 1, 4, 32}) {
        std::vector<float> buses(64 * 128, 3.125f); const auto before = buses;
        factory->step(algorithm, buses.data(), frames); assert(buses == before);
    }
    for (int16_t value : {int16_t(-32768), int16_t(1), int16_t(20), int16_t(32767)}) {
        page = value; assert(factory->draw(algorithm));
    }
    patch_helper::Connection connection;
    connection.colour = 4; connection.tag = 12;
    assert(patch_helper::copyText(connection.destination, "reverb left"));
    assert(connection.connected());
    assert(patch_helper::copyText(connection.destination, ""));
    assert(!connection.connected() && connection.colour == 4 && connection.tag == 12);
    assert(load(Json::object())); assert(save() == defaults);
    std::cout << "PASS: preset round trip, validation/atomicity, socket identity, display bounds, untouched buses\n";
}
