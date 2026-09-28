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
#include <limits>

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
std::vector<std::string> headerDrawn;
std::vector<std::pair<int, int>> drawPositions;
void NT_drawText(int x, int y, const char* text, int, _NT_textAlignment align, _NT_textSize) {
    if (y == 8) { headerDrawn.emplace_back(text); return; }
    assert(y >= 21 && y <= 60); // Keep list text below the control row.
    assert(align == (x == 188 ? kNT_textRight : kNT_textLeft));
    drawPositions.emplace_back(x, y);
    drawn.emplace_back(text);
}
void NT_drawShapeI(_NT_shape shape, int x0, int y0, int x1, int y1, int) {
    assert(shape == kNT_rectangle && x0 >= 0 && y0 >= 0 && x1 <= 255 && y1 <= 63);
    assert(y0 == 0 ? y1 == 9 : y0 >= 12);
}
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
std::vector<uint8_t> midiWireReply;
bool NT_getSlot(_NT_slot& slot, uint32_t index) { slot.refCon = activeAlgorithm; return index == 0; }
uint32_t _NT_slot::guid() const { return NT_MULTICHAR('T', 'h', 'P', 'h'); }
_NT_algorithm* _NT_slot::plugin() const { return static_cast<_NT_algorithm*>(refCon); }
void NT_sendMidiSysEx(uint32_t destination, const uint8_t* data, uint32_t size, bool end) {
    assert(destination == kNT_destinationUSB && end);
    // The API adds F7 only; the caller must supply the opening F0.
    assert(size > 1 && data[0] == 0xf0);
    assert(data[size - 1] != 0xf7);
    midiWireReply.assign(data, data + size);
    midiWireReply.push_back(0xf7);
    midiReply.assign(data + 1, data + size);
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
    for (std::size_t i = 0; i < sizeof(kPrefix); ++i) {
        request[i] ^= 1;
        assert(!isRequest(request.data(), request.size()));
        request[i] ^= 1;
    }
    for (std::size_t size = 0; size < kHeaderBytes; ++size)
        assert(!isRequest(request.data(), size));
    assert(respond(map, session, request.data(), request.size(), reply) == 21);
    assert(reply[20] == 0 && std::strlen(map.title) == 63);
    request.resize(20); request[6] = 3; request[16] = 1;
    request.insert(request.end(), {19, 11, 12, 63});
    request.insert(request.end(), 63, 'D'); request.push_back(32);
    request.insert(request.end(), 32, 'G');
    // Loaded legacy text can be retained while another field changes.
    copyText(map.connections[19].destination, std::string(63, 'D').c_str());
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
    assert(respond(map, session, request.data(), request.size(), reply) == 121);
    assert(reply[20] == 0 && reply[21] == 19 && reply[22] == 11 && reply[23] == 12);
    request.resize(20); request[6] = 9; request[16] = 0;
    respond(map, session, request.data(), request.size(), reply, 19);
    assert(reply[20] == 0 && readInteger(reply + 16) == 2);
    assert(reply[86] == 20 && reply[87] == 11 && reply[88] == 12);
    request.push_back(0);
    respond(map, session, request.data(), request.size(), reply, 19);
    assert(reply[20] == 1 && session.revision == 2);
    request.resize(20); request[12] = 42;
    respond(map, session, request.data(), request.size(), reply, 19);
    assert(reply[20] == 2);
    request[12] = 43;
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

int32_t NT_algorithmIndex(const _NT_algorithm*) { return 0; }
void NT_updateParameterDefinition(uint32_t, uint32_t) {}
void NT_updateParameterPages(uint32_t) {}
std::array<bool, 231> grayed{};
void NT_setParameterGrayedOut(uint32_t index, uint32_t p, bool gray) {
    assert(index == 0 && p >= 7 && p < 7 + grayed.size());
    grayed[p - 7] = gray;
}

uint32_t NT_parameterOffset() { return 7; }
void NT_setParameterFromAudio(uint32_t index, uint32_t p, int16_t value) {
    assert(index == 0 && p >= 7 && p < 7 + 231);
    const_cast<int16_t*>(activeAlgorithm->v)[p - 7] = value;
    const auto* f = reinterpret_cast<const _NT_factory*>(pluginEntry(kNT_selector_factoryInfo, 0));
    f->parameterChanged(activeAlgorithm, p - 7);
}

void NT_setParameterFromUi(uint32_t index, uint32_t p, int16_t value) {
    NT_setParameterFromAudio(index, p, value);
}

void testEditableTextLimits() {
    using namespace patch_helper;
    PatchMap map;
    Session session{42, 0};
    uint8_t reply[128]{};
    auto writeRow = [&](const std::string& destination, const std::string& group) {
        std::vector<uint8_t> request(20, 0);
        std::copy(std::begin(kPrefix), std::end(kPrefix), request.begin());
        request[6] = 3; request[12] = 42;
        writeInteger(request.data() + 16, session.revision);
        request.insert(request.end(), {0, 4, 7, static_cast<uint8_t>(destination.size())});
        request.insert(request.end(), destination.begin(), destination.end());
        request.push_back(static_cast<uint8_t>(group.size()));
        request.insert(request.end(), group.begin(), group.end());
        assert(isRequest(request.data(), request.size()));
        respond(map, session, request.data(), request.size(), reply);
        return reply[20];
    };
    assert(writeRow(std::string(32, 'D'), std::string(32, 'G')) == 0);
    assert(session.revision == 1 && std::strlen(map.connections[0].destination) == 32);
    assert(writeRow(std::string(33, 'D'), "") == 1);
    assert(writeRow("", std::string(33, 'G')) == 1);
    assert(session.revision == 1 && std::strlen(map.connections[0].group) == 32);
    copyText(map.connections[0].destination, std::string(63, 'L').c_str());
    assert(writeRow(std::string(63, 'L'), std::string(32, 'G')) == 0);
    assert(writeRow(std::string(63, 'X'), "") == 1);
    assert(std::strlen(map.connections[0].destination) == 63);
    assert(writeRow("Short replacement", "") == 0);
    for (const int length : {32, 33}) {
        std::vector<uint8_t> request(20, 0);
        std::copy(std::begin(kPrefix), std::end(kPrefix), request.begin());
        request[6] = 5; request[12] = 42;
        writeInteger(request.data() + 16, session.revision);
        request.push_back(0); request.push_back(length);
        request.insert(request.end(), length, 'E');
        respond(map, session, request.data(), request.size(), reply);
        assert(reply[20] == (length == 32 ? 0 : 1));
    }
    assert(map.expanderCount == 1 && std::strlen(map.expanders[0].name) == 32);
}

int main(int argc, char** argv) {
    testProtocolBounds();
    testEditableTextLimits();
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
    assert(req.numParameters == 231 && req.dtc == 0 && req.dram == 0 && req.itc == 0);
    std::vector<std::max_align_t> memory((req.sram + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t));
    _NT_algorithmMemoryPtrs ptrs{reinterpret_cast<uint8_t*>(memory.data()), nullptr, nullptr, nullptr};
    auto* algorithm = factory->construct(ptrs, req, nullptr);
    activeAlgorithm = algorithm;
    for (int framing = 0; framing < 4; ++framing) {
        std::vector<uint8_t> message(21, 0);
        std::copy(std::begin(patch_helper::kPrefix), std::end(patch_helper::kPrefix), message.begin());
        message[6] = 1; message[12] = 100 + framing; message[20] = 2;
        if (framing & 1) message.insert(message.begin(), 0xf0);
        if (framing & 2) message.push_back(0xf7);
        midiReply.clear();
        factory->midiSysEx(message.data(), message.size());
        assert(midiReply.size() > 20 && midiReply[20] == 0);
        assert(midiWireReply.front() == 0xf0 && midiWireReply.back() == 0xf7);
    }
    int16_t values[231] = {1, 0, 0};
    auto& page = values[0];
    algorithm->v = values;
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
        assert(expectedBytes == midiWireReply);
    }
    assert(load(Json::object()));
    std::ifstream expandedWireInput("tests/fixtures/expanded-session.json");
    for (const auto& frame : Json::parse(expandedWireInput)) {
        const auto bytes = frame["request"].get<std::vector<uint8_t>>();
        const auto expected = frame["response"].get<std::vector<uint8_t>>();
        factory->midiSysEx(bytes.data(), bytes.size());
        assert(expected == midiWireReply);
    }
    const auto persistedExpanded = save();
    assert(load(persistedExpanded) && save() == persistedExpanded);
    assert(persistedExpanded["patch_helper"]["expanders"][1]["name"] == "Pitch");
    assert(persistedExpanded["patch_helper"]["connections"][28]["destination"] == "Plaits V/oct");
    assert(load(Json::object()));
    // The exact host watch transcript includes real on-device parameter callbacks.
    assert(load(Json::object()));
    values[0] = 1;
    factory->step(algorithm, nullptr, 0);
    std::ifstream liveInput("tests/fixtures/live-session.json");
    for (const auto& frame : Json::parse(liveInput)) {
        if (frame.contains("parameter")) {
            const int p = frame["parameter"][0];
            values[p] = frame["parameter"][1];
            factory->parameterChanged(algorithm, p);
        } else {
            const auto bytes = frame["request"].get<std::vector<uint8_t>>();
            const auto expected = frame["response"].get<std::vector<uint8_t>>();
            factory->midiSysEx(bytes.data(), bytes.size());
            assert(expected == midiWireReply);
        }
    }
    assert(save()["patch_helper"]["connections"][19]["colour"] == 4);
    assert(save()["patch_helper"]["connections"][19]["tag"] == 7);
    // Every active row has stable, independent native parameter indices.
    assert(algorithm->parameterPages->numPages == 20);
    for (int socket = 0; socket < 20; ++socket) {
        const auto& nativePage = algorithm->parameterPages->pages[socket];
        assert(nativePage.numParams == 4 && nativePage.group == 1);
        assert(nativePage.params[0] == 171 + 2 * socket);
        assert(nativePage.params[1] == 3 + 2 * socket);
        assert(nativePage.params[2] == 4 + 2 * socket);
        assert(nativePage.params[3] == 172 + 2 * socket);
        assert(grayed[nativePage.params[0]] && grayed[nativePage.params[3]]);
        assert(!grayed[nativePage.params[1]] && !grayed[nativePage.params[2]]);
    }
    assert(std::string(algorithm->parameterPages->pages[0].name) == "Input 1");
    assert(std::string(algorithm->parameterPages->pages[12].name) == "Output 1");
    values[3] = 8; factory->parameterChanged(algorithm, 3);
    assert(save()["patch_helper"]["connections"][0]["colour"] == 8);
    assert(values[1] == 8 && values[3 + 2 * 19] == 4);
    values[3] = 0; factory->parameterChanged(algorithm, 3);
    char prefix[kNT_parameterUiPrefixSize]{};
    factory->parameterUiPrefix(algorithm, 3 + 2 * 83, prefix);
    assert(std::string(prefix) == "E8 Out 8 ");
    // Changing selection projects controls without overwriting either record.
    values[0] = 1; factory->parameterChanged(algorithm, 0);
    assert(values[1] == 0 && values[2] == 0);
    values[0] = 20; factory->parameterChanged(algorithm, 0);
    assert(values[1] == 4 && values[2] == 7);
    const auto nativeEdited = save();
    assert(load(nativeEdited)); factory->step(algorithm, nullptr, 0);
    assert(save() == nativeEdited && values[2] == 7);
    // A smaller preset clamps both the value and the definition via host APIs.
    values[0] = 124;
    assert(load(Json::object())); factory->step(algorithm, nullptr, 0);
    assert(values[0] == 20 && values[1] == 0 && values[2] == 0);
    values[0] = 1;

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
    // Extended open, append each type, reload, and validate version-2 persistence.
    request.assign(21, 0);
    std::copy(std::begin(patch_helper::kPrefix), std::end(patch_helper::kPrefix), request.begin());
    request[6] = 1; request[12] = 44; request[20] = 2;
    factory->midiSysEx(request.data(), request.size());
    assert(midiReply[20] == 0 && midiReply.back() == 0);
    for (int i = 0; i < patch_helper::kNativeExpanders; ++i) {
        request.resize(22); request[6] = 5; request[16] = i; request[20] = i % 4; request[21] = 0;
        factory->midiSysEx(request.data(), request.size());
        assert(midiReply[20] == 0);
    }
    auto expanded = save();
    // Old 13-bank maps remain readable without reserving rejected parameter counts.
    for (int bank = patch_helper::kNativeExpanders; bank < patch_helper::kMaxExpanders; ++bank)
        expanded["patch_helper"]["expanders"].push_back({{"type", 0}, {"name", "Legacy"}});
    for (int socket = patch_helper::kNativeSockets; socket < patch_helper::kMaxSockets; ++socket)
        expanded["patch_helper"]["connections"].push_back({{"socket", socket}, {"destination", ""}, {"colour", 0}, {"tag", 0}, {"group", ""}});
    assert(expanded["patch_helper"]["connections"].size() == patch_helper::kMaxSockets);
    assert(expanded["patch_helper"]["expanders"].size() == patch_helper::kMaxExpanders);
    assert(load(expanded) && save() == expanded);
    request.resize(21); request[6] = 1; request[12] = 45; request[20] = 2;
    factory->midiSysEx(request.data(), request.size());
    assert(midiReply[20] == 0 && midiReply[34] == patch_helper::kMaxExpanders);
    request[6] = 5; request[16] = 0; request[20] = 0;
    factory->midiSysEx(request.data(), request.size());
    assert(midiReply[20] == 1 && save() == expanded);
    auto invalidExpanded = expanded;
    invalidExpanded["patch_helper"]["expanders"][0]["type"] = 4;
    assert(!load(invalidExpanded) && save() == expanded);
    invalidExpanded = expanded;
    invalidExpanded["patch_helper"]["connections"][1]["socket"] = 123;
    assert(!load(invalidExpanded) && save() == expanded);
    factory->step(algorithm, nullptr, 0);
    assert(algorithm->parameters[0].max == patch_helper::kMaxSockets);
    assert(algorithm->parameterPages->numPages == 30);
    assert(std::string(algorithm->parameterPages->pages[29].name) == "Other sockets");
    values[0] = 124; factory->parameterChanged(algorithm, 0);
    values[2] = 12; factory->parameterChanged(algorithm, 2);
    assert(save()["patch_helper"]["connections"][123]["tag"] == 12);
    assert(load(Json::object()));

    assert(defaults["patch_helper"]["connections"].size() == 20);
    assert(load(fixture));
    assert(save() == fixture);
    // Firmware string requests display full text without changing property
    // identity, numeric editing, selection or serialized data.
    assert(factory->parameterString);
    const auto format = [&](int parameter, int value) {
        struct { char text[kNT_parameterStringSize]; char guard[8]; } buffer{};
        std::fill(std::begin(buffer.guard), std::end(buffer.guard), 'X');
        const auto before = save();
        const int length = factory->parameterString(algorithm, parameter, value, buffer.text);
        assert(length == static_cast<int>(std::strlen(buffer.text)));
        for (char byte : buffer.guard) assert(byte == 'X');
        assert(save() == before);
        return std::string(buffer.text);
    };
    auto textMap = fixture;
    auto& textRow = textMap["patch_helper"]["connections"][0];
    textRow["destination"] = "From Beads L";
    textRow["group"] = "FX";
    textRow["colour"] = 9; textRow["tag"] = 1;
    assert(load(textMap)); factory->step(algorithm, nullptr, 0);
    assert(algorithm->parameters[3].unit == kNT_unitEnum);
    assert(algorithm->parameters[4].unit == kNT_unitNone);
    assert(algorithm->parameters[171].unit == kNT_unitHasStrings);
    assert(algorithm->parameters[171].min == 0 && algorithm->parameters[171].max == 0);
    assert(format(171, 0) == "From Beads L");
    assert(format(172, 0) == "FX");
    assert(format(171, 999) == "From Beads L"); // Numeric dummy value cannot edit text.
    values[0] = 1;
    assert(format(229, 0) == "From Beads L");
    assert(format(230, 0) == "FX");
    assert(format(0, 1).empty() && format(-1, 0).empty());
    assert(format(req.numParameters, 0).empty());
    assert(format(3, 9).empty() && format(4, 1).empty());
    textRow["destination"] = std::string(32, 'D');
    textRow["group"] = std::string(32, 'G');
    assert(load(textMap));
    assert(format(171, 0) == std::string(32, 'D'));
    assert(format(172, 0) == std::string(32, 'G'));
    textRow["destination"] = std::string(63, 'L');
    assert(load(textMap));
    assert(format(171, 0) == std::string(63, 'L'));
    assert(save()["patch_helper"]["connections"][0]["destination"].get<std::string>().size() == 63);
    textRow["destination"] = "Updated"; textRow["group"] = "New group";
    assert(load(textMap));
    assert(format(171, 0) == "Updated");
    assert(format(172, 0) == "New group");
    const auto beforeDummy = save();
    values[171] = 9; factory->parameterChanged(algorithm, 171);
    assert(save() == beforeDummy && format(171, 9) == "Updated");
    // Numeric edits still affect only their own field.
    values[3] = 4; factory->parameterChanged(algorithm, 3);
    assert(save()["patch_helper"]["connections"][0]["destination"] == "Updated");
    assert(save()["patch_helper"]["connections"][0]["colour"] == 4);
    // Eight banks retain independent numeric indices while the visible text
    // pages follow the bank selector. Selecting a bank never edits cable data.
    expanded["patch_helper"]["connections"][20]["destination"] = "Bank one";
    expanded["patch_helper"]["connections"][76]["destination"] = "Bank eight";
    expanded["patch_helper"]["connections"][76]["group"] = "Eighth group";
    expanded["patch_helper"]["expanders"][7]["name"] = "Eight NTX";
    assert(load(expanded));
    values[227] = 1; factory->step(algorithm, nullptr, 0);
    assert(format(211, 0) == "Bank one");
    assert(!grayed[227] && grayed[228]);
    const auto beforeBank = save();
    values[227] = 8; factory->parameterChanged(algorithm, 227);
    factory->step(algorithm, nullptr, 0);
    assert(save() == beforeBank);
    assert(values[0] == 77 && algorithm->parameters[227].max == 8);
    assert(std::string(algorithm->parameterPages->pages[21].name) == "E8 Out 1");
    assert(algorithm->parameterPages->pages[21].params[1] == 155);
    assert(format(211, 0) == "Bank eight" && format(212, 0) == "Eighth group");
    assert(format(228, 0) == "Eight NTX");
    values[155] = 8; factory->parameterChanged(algorithm, 155);
    assert(save()["patch_helper"]["connections"][76]["colour"] == 8);
    assert(save()["patch_helper"]["connections"][20]["colour"] == 0);
    values[0] = 124; factory->parameterChanged(algorithm, 0);
    assert(format(229, 0) == "-" && format(230, 0) == "-");
    auto oneBank = expanded;
    oneBank["patch_helper"]["expanders"] = Json::array({expanded["patch_helper"]["expanders"][0]});
    while (oneBank["patch_helper"]["connections"].size() > 28) oneBank["patch_helper"]["connections"].erase(oneBank["patch_helper"]["connections"].size() - 1);
    assert(load(oneBank)); factory->step(algorithm, nullptr, 0);
    assert(values[227] == 1 && algorithm->parameters[227].max == 1);
    assert(algorithm->parameterPages->numPages == 29 && format(211, 0) == "Bank one");
    assert(load(fixture));
    page = 1;
    factory->draw(algorithm);
    assert(drawn[0] == "In 1");
    // Selection-following list: connected context plus a temporary unused bottom row.
    assert(factory->hasCustomUi(algorithm) == (kNT_potL | kNT_potC | kNT_potR | kNT_encoderL | kNT_encoderR));
    auto scrollMap = expanded;
    for (auto& row : scrollMap["patch_helper"]["connections"]) row["destination"] = "";
    for (int socket : {0, 5, 12, 20, 76, 83})
        scrollMap["patch_helper"]["connections"][socket]["destination"] = "Connected";
    assert(load(scrollMap)); factory->step(algorithm, nullptr, 0);
    const auto labels = [&]() {
        drawn.clear(); drawPositions.clear(); assert(factory->draw(algorithm));
        std::vector<std::string> result;
        for (std::size_t i = 0; i < drawn.size(); i += 5) result.push_back(drawn[i]);
        return result;
    };
    const auto selectSocket = [&](int socket) {
        _NT_uiData ui{}; ui.controls = kNT_potL; ui.pots[0] = socket / 123.0f;
        factory->customUi(algorithm, ui);
    };
    assert((labels() == std::vector<std::string>{"In 1", "In 6", "Out 1", "E1:1"}));
    const auto beforeNavigation = save();
    const std::vector<int16_t> beforeValues(std::begin(values), std::end(values));
    request.assign(21, 0);
    std::copy(std::begin(patch_helper::kPrefix), std::end(patch_helper::kPrefix), request.begin());
    request[6] = 1; request[12] = 45; request[20] = 2;
    factory->midiSysEx(request.data(), request.size());
    const auto beforeRevision = patch_helper::readInteger(midiReply.data() + 16);
    selectSocket(76);
    assert((labels() == std::vector<std::string>{"In 6", "Out 1", "E1:1", "E8:1"}));
    selectSocket(83);
    assert((labels() == std::vector<std::string>{"Out 1", "E1:1", "E8:1", "E8:8"}));
    selectSocket(82);
    assert((labels() == std::vector<std::string>{"Out 1", "E1:1", "E8:1", "E8:7"}));
    selectSocket(81); assert(labels().back() == "E8:6");
    selectSocket(123);
    assert((labels() == std::vector<std::string>{"E1:1", "E8:1", "E8:8", "E13:8"}));
    assert(save() == beforeNavigation);
    assert(beforeValues == std::vector<int16_t>(std::begin(values), std::end(values)));
    request.resize(20); request[6] = 9;
    factory->midiSysEx(request.data(), request.size());
    assert(midiReply[20] == 0 && patch_helper::readInteger(midiReply.data() + 16) == beforeRevision);
    selectSocket(0); assert(labels().front() == "In 1");
    scrollMap["patch_helper"]["connections"][0]["destination"] = "";
    assert(load(scrollMap)); assert(labels().back() == "In 1");
    for (auto& row : scrollMap["patch_helper"]["connections"]) row["destination"] = "";
    assert(load(scrollMap)); assert((labels() == std::vector<std::string>{"In 1"}));
    assert((drawPositions == std::vector<std::pair<int, int>>{{0, 60}, {32, 60}, {144, 60}, {188, 60}, {196, 60}}));
    // An empty selected socket is still editable and every table field is drawn.
    selectSocket(83);
    _NT_uiData editUnused{}; editUnused.encoders[1] = 1;
    factory->customUi(algorithm, editUnused);
    assert(save()["patch_helper"]["connections"][83]["colour"] == 1);
    assert(save()["patch_helper"]["connections"][83]["destination"] == "");
    assert((labels() == std::vector<std::string>{"E8:8"}));
    scrollMap["patch_helper"]["connections"][83]["destination"] = std::string(32, 'D');
    scrollMap["patch_helper"]["connections"][83]["group"] = std::string(32, 'G');
    scrollMap["patch_helper"]["connections"][83]["tag"] = 12;
    assert(load(scrollMap)); labels();
    assert(drawn[1] == std::string(24, 'D') + "..." && drawn[3] == "12" && drawn[4] == std::string(12, 'G') + "...");
    scrollMap["patch_helper"]["connections"][83]["destination"] = std::string(63, 'L');
    assert(load(scrollMap)); labels();
    assert(drawn[1] == std::string(24, 'L') + "...");
    assert(save()["patch_helper"]["connections"][83]["destination"] == std::string(63, 'L'));
    // A smaller map clamps the selected socket and keeps its unused row visible.
    assert(load(fixture)); assert(labels().back() == "Out 8");
    assert(load(scrollMap)); selectSocket(0);
    // Three pots and two encoders edit the displayed channel without UI focus APIs.
    assert(load(scrollMap));
    const auto sendUi = [&](int left, int right, uint16_t controls = 0,
                            float p1 = 0, float p2 = 0, float p3 = 0) {
        _NT_uiData ui{}; ui.controls = controls; ui.encoders[0] = left; ui.encoders[1] = right;
        ui.pots[0] = p1; ui.pots[1] = p2; ui.pots[2] = p3;
        factory->customUi(algorithm, ui);
    };
    const auto header = [&]() {
        headerDrawn.clear(); factory->draw(algorithm); return headerDrawn;
    };
    _NT_float3 potTargets{}; factory->setupUi(algorithm, potTargets);
    assert(potTargets[0] == 0 && potTargets[1] == 0);
    auto unchanged = save();
    sendUi(1, 0);
    assert(header()[0] == "Input 2" && save() == unchanged);
    sendUi(-128, 0); assert(header()[0] == "Input 1");
    sendUi(127, 0); assert(header()[0] == "E13 Out 8");
    sendUi(1, 0); assert(header()[0] == "E13 Out 8");
    sendUi(0, 0, kNT_potC, 0, 1, 1); // Select tag; do not write the stale value pot.
    assert(header()[1] == "Tag" && save() == unchanged);
    sendUi(0, 1); assert(save()["patch_helper"]["connections"][123]["tag"] == 1);
    sendUi(0, 127); assert(header()[2] == "12");
    sendUi(0, -128); assert(header()[2] == "-");
    sendUi(0, 0, kNT_potL | kNT_potC | kNT_potR, 76.0f / 123, 0, 1);
    assert(header()[0] == "E8 Out 1" && header()[1] == "Cable colour");
    int oldColour = save()["patch_helper"]["connections"][76]["colour"];
    sendUi(0, 1);
    assert(save()["patch_helper"]["connections"][76]["colour"] == oldColour + 1);
    // Endpoint selection, NaN guards, pickup after a field change and remote edit.
    sendUi(0, 0, kNT_potL | kNT_potC, 0, 1, 1);
    assert(header()[0] == "Input 1" && header()[1] == "Tag");
    unchanged = save();
    sendUi(0, 0, kNT_potR, 0, 0, .9f); assert(save() == unchanged);
    sendUi(0, 0, kNT_potR, 0, 0, 0);
    sendUi(0, 0, kNT_potR, 0, 0, .5f); assert(header()[2] == "6");
    values[4] = 12; factory->parameterChanged(algorithm, 4);
    sendUi(0, 0, kNT_potR, 0, 0, .6f); assert(header()[2] == "12");
    sendUi(0, 0, kNT_potR, 0, 0, 1);
    sendUi(0, 0, kNT_potR, 0, 0, .5f); assert(header()[2] == "6");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    unchanged = save(); sendUi(0, 0, kNT_potL | kNT_potC | kNT_potR, nan, nan, nan);
    assert(save() == unchanged && header()[0] == "Input 1");
    factory->setupUi(algorithm, potTargets);
    assert(potTargets[0] == 0 && potTargets[1] == 1 && potTargets[2] == .5f);
    assert(load(fixture));
    sendUi(0, 0, kNT_potL | kNT_potC, 0, 0, 0);
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
    broken = fixture; broken["patch_helper"]["connections"][0]["group"] = std::string(33, 'x'); reject(broken);
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
