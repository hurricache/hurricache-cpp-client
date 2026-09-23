//
// Helper utilities for HurriCache client tests
//
#pragma once

#include <string>
#include "standalone_client.hxx"

namespace test_base {

    // Helper to create Key from string
    static Key make_key(const std::string &str) {
        Key key{};
        key.size = static_cast<uint32_t>(str.size());
        key.data = const_cast<char*>(str.c_str());
        return key;
    }

    // Helper to create Value from string
    static ValuePtr make_value(const std::string &str) {
        return new Value{static_cast<uint64_t>(str.size()),const_cast<char*>(str.c_str())};
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
