#pragma once
#include <string>
#include <unordered_map>

class Preferences {
public:
    bool begin(const char*, bool = false) { return true; }
    void end() {}

    bool getBool(const char* key, bool defaultValue = false) const {
        auto it = store().find(key ? key : "");
        if (it == store().end()) return defaultValue;
        return it->second;
    }

    void putBool(const char* key, bool value) {
        store()[key ? key : ""] = value;
    }

private:
    static std::unordered_map<std::string, bool>& store() {
        static std::unordered_map<std::string, bool> s;
        return s;
    }
};
