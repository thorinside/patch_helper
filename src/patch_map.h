#pragma once

#include <array>
#include <cstddef>
#include <cstring>

namespace patch_helper {
constexpr int kVersion = 1;
constexpr int kSocketCount = 20;
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

struct PatchMap {
    char title[kTextBytes] = "Patch Helper";
    std::array<Connection, kSocketCount> connections{};
};

// Stream/Parse follow the official NT preset API. Templates let native tests
// exercise the identical implementation through a desktop JSON adapter.
template <typename Stream>
void writeMap(const PatchMap& map, Stream& stream) {
    stream.addMemberName("patch_helper");
    stream.openObject();
    stream.addMemberName("version"); stream.addNumber(kVersion);
    stream.addMemberName("title"); stream.addString(map.title);
    stream.addMemberName("connections"); stream.openArray();
    for (int socket = 0; socket < kSocketCount; ++socket) {
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
            if (!parse.number(socket) || socket < 0 || socket >= kSocketCount) return false;
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
    if (!parse.numberOfObjectMembers(count) || count != 3) return false;
    unsigned seen = 0;
    for (int i = 0; i < count; ++i) {
        unsigned field = 0;
        if (parse.matchName("version")) {
            int version = 0;
            field = 1;
            if (!parse.number(version) || version != kVersion) return false;
        } else if (parse.matchName("title")) {
            const char* text = nullptr;
            field = 2;
            if (!parse.string(text) || !copyText(candidate.title, text)) return false;
        } else if (parse.matchName("connections")) {
            field = 4;
            int size = 0;
            if (!parse.numberOfArrayElements(size) || size != kSocketCount) return false;
            unsigned sockets = 0;
            for (int j = 0; j < size; ++j) {
                Connection connection;
                int socket = -1;
                if (!readConnection(parse, connection, socket)) return false;
                const unsigned bit = 1u << socket;
                if (sockets & bit) return false;
                sockets |= bit;
                candidate.connections[socket] = connection;
            }
        } else return false;
        if (seen & field) return false;
        seen |= field;
    }
    return seen == 7;
}

template <typename Parse>
bool readMap(PatchMap& map, Parse& parse) {
    int count = 0;
    if (!parse.numberOfObjectMembers(count)) return false;
    PatchMap candidate;
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
