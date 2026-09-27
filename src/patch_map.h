#pragma once

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstring>

namespace patch_helper {
constexpr int kVersion = 1;
constexpr int kSocketCount = 20;
// One MIDI data byte addresses a socket. Whole eight-output banks fit through 123.
constexpr int kMaxExpanders = (128 - kSocketCount) / 8;
constexpr int kMaxSockets = kSocketCount + 8 * kMaxExpanders;
inline constexpr const char* kExpanders[] = {"NTX-8CV", "ES-5", "ESX-8GT", "ESX-8CV"};
constexpr int kTextBytes = 64;
constexpr int kGroupBytes = 32;
inline constexpr const char* kColours[] = {
    "None", "Black", "White", "Grey", "Red", "Orange", "Yellow",
    "Green", "Blue", "Purple", "Pink", "Brown"};
constexpr int kColourCount = sizeof(kColours) / sizeof(kColours[0]);

// The prototype uses printable ASCII to match the device's bitmap text display.
// Reject unsupported/overlong input rather than silently changing a saved map.
template <std::size_t N>
bool copyText(char (&target)[N], const char* source) {
    if (!source) return false;
    std::size_t length = 0;
    while (source[length]) {
        const auto byte = static_cast<unsigned char>(source[length]);
        if (length >= N - 1 || byte < 32 || byte > 126) return false;
        ++length;
    }
    std::memcpy(target, source, length + 1);
    return true;
}

struct Connection {
    char destination[kTextBytes]{};
    char group[kGroupBytes]{};
    int colour = 0;
    int tag = 0; // zero means no tag; otherwise 1..12
    bool connected() const { return destination[0] != '\0'; }
};

struct Expander {
    int type = 0;
    char name[kGroupBytes]{};
};

struct PatchMap {
    char title[kTextBytes] = "Patch Helper";
    int expanderCount = 0;
    std::array<Expander, kMaxExpanders> expanders{};
    std::array<Connection, kMaxSockets> connections{};
    int socketCount() const { return kSocketCount + 8 * expanderCount; }
};

// Stream/Parse follow the official NT preset API. Templates let native tests
// exercise the identical implementation through a desktop JSON adapter.
template <typename Stream>
void writeMap(const PatchMap& map, Stream& stream) {
    stream.addMemberName("patch_helper");
    stream.openObject();
    stream.addMemberName("version"); stream.addNumber(map.expanderCount ? 2 : kVersion);
    if (map.expanderCount) {
        stream.addMemberName("expanders"); stream.openArray();
        for (int i = 0; i < map.expanderCount; ++i) {
            stream.openObject();
            stream.addMemberName("type"); stream.addNumber(map.expanders[i].type);
            stream.addMemberName("name"); stream.addString(map.expanders[i].name);
            stream.closeObject();
        }
        stream.closeArray();
    }
    stream.addMemberName("title"); stream.addString(map.title);
    stream.addMemberName("connections"); stream.openArray();
    for (int socket = 0; socket < map.socketCount(); ++socket) {
        const auto& connection = map.connections[socket];
        stream.openObject();
        stream.addMemberName("socket"); stream.addNumber(socket);
        stream.addMemberName("destination"); stream.addString(connection.destination);
        stream.addMemberName("colour"); stream.addNumber(connection.colour);
        stream.addMemberName("tag"); stream.addNumber(connection.tag);
        stream.addMemberName("group"); stream.addString(connection.group);
        stream.closeObject();
    }
    stream.closeArray(); stream.closeObject();
}

template <typename Parse>
bool readConnection(Parse& parse, Connection& connection, int& socket) {
    int count = 0;
    if (!parse.numberOfObjectMembers(count) || count != 5) return false;
    unsigned seen = 0;
    for (int i = 0; i < count; ++i) {
        unsigned field = 0;
        const char* text = nullptr;
        if (parse.matchName("socket")) {
            field = 1;
            if (!parse.number(socket) || socket < 0 || socket >= kMaxSockets) return false;
        } else if (parse.matchName("destination")) {
            field = 2;
            if (!parse.string(text) || !copyText(connection.destination, text)) return false;
        } else if (parse.matchName("colour")) {
            field = 4;
            if (!parse.number(connection.colour) || connection.colour < 0 ||
                connection.colour >= kColourCount) return false;
        } else if (parse.matchName("tag")) {
            field = 8;
            if (!parse.number(connection.tag) || connection.tag < 0 || connection.tag > 12) return false;
        } else if (parse.matchName("group")) {
            field = 16;
            if (!parse.string(text) || !copyText(connection.group, text)) return false;
        } else return false;
        if (seen & field) return false;
        seen |= field;
    }
    return seen == 31;
}

template <typename Parse>
bool readMapBody(Parse& parse, PatchMap& candidate) {
    int count = 0;
    if (!parse.numberOfObjectMembers(count) || (count != 3 && count != 4)) return false;
    unsigned seen = 0;
    int version = 0, rowCount = 0;
    std::array<bool, kMaxSockets> sockets{};
    for (int i = 0; i < count; ++i) {
        unsigned field = 0;
        if (parse.matchName("version")) {
            field = 1;
            if (!parse.number(version) || (version != 1 && version != 2)) return false;
        } else if (parse.matchName("title")) {
            const char* text = nullptr;
            field = 2;
            if (!parse.string(text) || !copyText(candidate.title, text)) return false;
        } else if (parse.matchName("connections")) {
            field = 4;
            int size = 0;
            if (!parse.numberOfArrayElements(size) || size < kSocketCount || size > kMaxSockets) return false;
            rowCount = size;
            for (int j = 0; j < size; ++j) {
                Connection connection;
                int socket = -1;
                if (!readConnection(parse, connection, socket)) return false;
                if (sockets[socket]) return false;
                sockets[socket] = true;
                candidate.connections[socket] = connection;
            }
        } else if (parse.matchName("expanders")) {
            field = 8;
            if (!parse.numberOfArrayElements(candidate.expanderCount) ||
                candidate.expanderCount < 0 || candidate.expanderCount > kMaxExpanders) return false;
            for (int j = 0; j < candidate.expanderCount; ++j) {
                int fields = 0; unsigned found = 0;
                if (!parse.numberOfObjectMembers(fields) || fields != 2) return false;
                for (int k = 0; k < fields; ++k) {
                    if (parse.matchName("type") && !(found & 1)) {
                        found |= 1;
                        if (!parse.number(candidate.expanders[j].type) || candidate.expanders[j].type < 0 || candidate.expanders[j].type > 3) return false;
                    } else if (parse.matchName("name") && !(found & 2)) {
                        found |= 2; const char* text = nullptr;
                        if (!parse.string(text) || !copyText(candidate.expanders[j].name, text)) return false;
                    } else return false;
                }
            }
        } else return false;
        if (seen & field) return false;
        seen |= field;
    }
    if (seen != (version == 1 ? 7u : 15u) || rowCount != candidate.socketCount()) return false;
    return std::all_of(sockets.begin(), sockets.begin() + rowCount, [](bool present) { return present; });
}

template <typename Parse>
bool readMap(PatchMap& map, Parse& parse, PatchMap& candidate) {
    int count = 0;
    if (!parse.numberOfObjectMembers(count)) return false;
    candidate = PatchMap{};
    bool found = false;
    for (int i = 0; i < count; ++i) {
        if (parse.matchName("patch_helper")) {
            if (found || !readMapBody(parse, candidate)) return false;
            found = true;
        } else if (!parse.skipMember()) return false;
    }
    // A preset without custom data gets defaults. Invalid data never partially
    // overwrites a previously valid map.
    map = candidate;
    return true;
}
} // namespace patch_helper
