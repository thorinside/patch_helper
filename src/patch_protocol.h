#pragma once
#include "patch_map.h"
#include <cstdint>

namespace patch_helper {
// Development-only SysEx namespace. All integers are little-endian base 128.
constexpr uint8_t kPrefix[] = {0x7d, 'T', 'h', 'P', 'h', 1};
constexpr uint32_t kMaxWireInteger = 0x0fffffff;
constexpr std::size_t kHeaderBytes = 20;
enum class Status : uint8_t { ok, invalid, expired, conflict };
struct Session {
    uint32_t lease = 0;
    uint32_t revision = 0;
};
inline uint32_t readInteger(const uint8_t* data) {
    return data[0] | (uint32_t(data[1]) << 7) |
           (uint32_t(data[2]) << 14) | (uint32_t(data[3]) << 21);
}
inline void writeInteger(uint8_t* data, uint32_t value) {
    for (int i = 0; i < 4; ++i) { data[i] = value & 127; value >>= 7; }
}
inline bool isRequest(const uint8_t* data, std::size_t size) {
    if (size < kHeaderBytes || size > 120 ||
        std::memcmp(data, kPrefix, sizeof(kPrefix)) != 0) return false;
    for (std::size_t i = 0; i < size; ++i) if (data[i] > 127) return false;
    return data[6] >= 1 && data[6] <= 4;
}
template <std::size_t N>
bool readText(const uint8_t*& data, const uint8_t* end, char (&text)[N]) {
    if (data == end) return false;
    const std::size_t size = *data++;
    if (size >= N || std::size_t(end - data) < size) return false;
    for (std::size_t i = 0; i < size; ++i) {
        if (data[i] < 32 || data[i] > 126) return false;
        text[i] = static_cast<char>(data[i]);
    }
    text[size] = 0;
    data += size;
    return true;
}
inline void writeText(uint8_t*& data, const char* text) {
    const auto size = std::strlen(text);
    *data++ = static_cast<uint8_t>(size);
    std::memcpy(data, text, size); data += size;
}
// Caller validates/addresses the frame. Reply fits in 121 bytes and excludes F0/F7.
inline std::size_t respond(PatchMap& map, Session& session,
                          const uint8_t* request, std::size_t size,
                          uint8_t (&reply)[128]) {
    std::memcpy(reply, request, kHeaderBytes);
    reply[6] |= 0x40;
    auto status = Status::ok;
    const auto lease = readInteger(request + 12);
    const auto revision = readInteger(request + 16);
    const auto command = request[6];
    const uint8_t* data = request + kHeaderBytes;
    const uint8_t* end = request + size;
    uint8_t* output = reply + kHeaderBytes + 1;
    if (command == 1) {
        if (data != end || lease == 0 || lease == session.lease) status = Status::invalid;
        else {
            session.lease = lease;
            if (session.revision == kMaxWireInteger) session.revision = 0;
            writeText(output, map.title);
        }
    } else if (lease == 0 || lease != session.lease) status = Status::expired;
    else if (revision != session.revision) status = Status::conflict;
    else if (command == 2) {
        if (end - data != 1 || *data >= kSocketCount) status = Status::invalid;
        else {
            const auto& row = map.connections[*data];
            *output++ = *data;
            *output++ = static_cast<uint8_t>(row.colour);
            *output++ = static_cast<uint8_t>(row.tag);
            writeText(output, row.destination); writeText(output, row.group);
        }
    } else if (session.revision == kMaxWireInteger) status = Status::expired;
    else if (command == 3) {
        if (end - data < 5 || data[0] >= kSocketCount ||
            data[1] >= kColourCount || data[2] > 12) status = Status::invalid;
        else {
            Connection candidate;
            const auto socket = *data++;
            candidate.colour = *data++; candidate.tag = *data++;
            if (!readText(data, end, candidate.destination) ||
                !readText(data, end, candidate.group) || data != end) status = Status::invalid;
            else { map.connections[socket] = candidate; ++session.revision; }
        }
    } else if (command == 4) {
        char title[kTextBytes]{};
        if (!readText(data, end, title) || data != end) status = Status::invalid;
        else { std::memcpy(map.title, title, sizeof(title)); ++session.revision; }
    }
    reply[kHeaderBytes] = static_cast<uint8_t>(status);
    writeInteger(reply + 16, session.revision);
    return static_cast<std::size_t>(output - reply);
}
} // namespace patch_helper
