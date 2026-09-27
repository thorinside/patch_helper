#pragma once
#include <nlohmann/json.hpp>
#include <limits>
#include <string>
#include <vector>

using Json = nlohmann::ordered_json;

struct JsonWriter {
    Json value = Json::object();
    std::vector<Json*> parents{&value};
    std::string key;
    Json* append(Json item) {
        auto& parent = *parents.back();
        if (parent.is_array()) {
            parent.push_back(std::move(item));
            return &parent.back();
        }
        parent[key] = std::move(item);
        return &parent[key];
    }
    void addMemberName(const char* name) { key = name; }
    void openObject() { parents.push_back(append(Json::object())); }
    void closeObject() { parents.pop_back(); }
    void openArray() { parents.push_back(append(Json::array())); }
    void closeArray() { parents.pop_back(); }
    void addNumber(int value) { append(value); }
    void addString(const char* value) { append(value); }
};

// Flatten a JSON tree into the cursor contract used by the firmware parser.
// Containers consume their header, matchName consumes only a matching key.
struct JsonReader {
    struct Token { Json value; std::size_t end; bool key; };
    std::vector<Token> tokens;
    std::size_t position = 0;
    explicit JsonReader(const Json& value) { flatten(value); }
    void flatten(const Json& value) {
        const auto start = tokens.size();
        tokens.push_back({value, 0, false});
        if (value.is_object()) {
            for (const auto& item : value.items()) {
                tokens.push_back({item.key(), tokens.size() + 1, true});
                flatten(item.value());
            }
        } else if (value.is_array()) {
            for (const auto& item : value) flatten(item);
        }
        tokens[start].end = tokens.size();
    }
    bool numberOfObjectMembers(int& size) { return container(size, true); }
    bool numberOfArrayElements(int& size) { return container(size, false); }
    bool container(int& size, bool object) {
        if (position >= tokens.size()) return false;
        const auto& value = tokens[position].value;
        if (object ? !value.is_object() : !value.is_array()) return false;
        size = static_cast<int>(value.size()); ++position; return true;
    }
    bool matchName(const char* name) {
        if (position >= tokens.size() || !tokens[position].key || tokens[position].value != name) return false;
        ++position; return true;
    }
    bool skipMember() {
        if (position >= tokens.size() || !tokens[position].key) return false;
        ++position;
        if (position >= tokens.size()) return false;
        position = tokens[position].end; return true;
    }
    bool number(int& result) {
        if (position >= tokens.size()) return false;
        const auto& value = tokens[position].value;
        if (!value.is_number_integer() || value < std::numeric_limits<int>::min() ||
            value > std::numeric_limits<int>::max()) return false;
        result = value.get<int>(); ++position; return true;
    }
    bool string(const char*& result) {
        if (position >= tokens.size() || !tokens[position].value.is_string()) return false;
        const auto& value = tokens[position].value.get_ref<const std::string&>();
        if (value.find('\0') != std::string::npos) return false;
        result = value.c_str(); ++position; return true;
    }
};
