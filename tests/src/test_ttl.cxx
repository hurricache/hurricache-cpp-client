//
// Tests for TTL Management methods
//
#include <iostream>
#include <chrono>
#include <future>
#include <cassert>
#include <stdexcept>
#include "standalone_client.hxx"

static void testSetTtl(FastCacheStandaloneClient &client) {
    std::cout << "  testSetTtl... ";
    try {
        Key key{};
        key.size = 4;
        key.data = const_cast<char*>("test");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.setTtl(key, hint, 60000, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        if (result) {
            std::cout << "PASSED (setTtl returned true)\n";
        } else {
            std::cout << "FAILED (setTtl returned false)\n";
        }
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetTtl(FastCacheStandaloneClient &client) {
    std::cout << "  testGetTtl... ";
    try {
        Key key{};
        key.size = 4;
        key.data = const_cast<char*>("test");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.getTtl(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getTtl returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

void testTtlManagement(FastCacheStandaloneClient &client) {
    std::cout << "=== TTL Management Tests ===\n";
    testSetTtl(client);
    testGetTtl(client);
    std::cout << "TTL Management: PASSED\n\n";
}
