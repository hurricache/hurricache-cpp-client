//
// Helper utilities for HurriCache client tests
//
#pragma once

#include <string>
#include "standalone_client.hxx"

namespace test_base {
    // Helper to create Key from string
    static Key make_key(const std::string &str) {
        auto size = static_cast<uint32_t>(str.size());
        auto data = const_cast<char *>(str.c_str());
        return Key(size, data);
    }

    // Helper to create Value from string
    static ValuePtr make_value(const std::string &str) {
        return new Value(const_cast<char *>(str.c_str()), static_cast<uint64_t>(str.size()));
    }

    // Helper to create KeyHint
    static KeyHint make_hint(uint32_t weak = 0, uint32_t strong = 0) {
        KeyHint hint{};
        hint.weak_hash = weak;
        hint.strong_hash = strong;
        return hint;
    }

    static std::shared_ptr<KeyHint> create_hint(uint32_t weak = 0, uint32_t strong = 0) {
        return std::make_shared<KeyHint>(weak, strong);
    }
}
