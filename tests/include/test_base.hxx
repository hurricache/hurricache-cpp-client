//
// Helper utilities for HurriCache client tests
//
#pragma once

#include <string>
#include <cstring>
#include <random>
#include "standalone_client.hxx"

namespace test_base {
    // Thread-local random generator for unique key suffixes
    static std::string make_unique_suffix() {
        thread_local std::mt19937 gen(std::random_device{}());
        thread_local std::uniform_int_distribution<uint64_t> dist(100000, 999999);
        return std::to_string(dist(gen));
    }

    // Helper to create Key from string (with unique suffix to avoid conflicts)
    static Key make_key(const std::string &str) {
        std::string unique_key = str + "_" + make_unique_suffix();
        return Key(unique_key.length(), const_cast<char *>(unique_key.c_str()));
    }

    // Helper to create Value from string
    static ValuePtr make_value(const std::string &str) {
        return new Value(const_cast<char *>(str.c_str()), str.length());
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

    // Helper to create OrderedValue from string and weight
    // Note: copies the string data to avoid dangling pointers
    static OrderedValuePtr make_ordered_value(const char *str, uint64_t weight) {
        uint64_t len = strlen(str);
        return new OrderedValue(weight,str,len);
    }

    static OrderedValuePtr make_ordered_value(const std::string &str, uint64_t weight) {
        return new OrderedValue(weight,str.c_str(),str.length());
    }

    // Helper to create KeyPtr
    static KeyPtr make_key_ptr(const std::string &str) {
        return new Key(str.length(), str.c_str());
    }

    static KeyPtr make_key_ptr(const char *str, uint32_t size) {
        return new Key(size, str);
    }
}
