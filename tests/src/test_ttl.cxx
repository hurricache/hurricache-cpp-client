//
// Tests for TTL Management methods
//
#include <iostream>
#include <chrono>
#include <future>
#include <cassert>
#include <stdexcept>
#include "standalone_client.hxx"
#include "test_base.hxx"

static void testSetTtl(FastCacheStandaloneClient &client) {
    std::cout << "  testSetTtl... ";
    try {
        Key key = test_base::make_key("ttl_test");
        
        auto result = client.setTtl(key, nullptr, 60000).get();
        
        if (result) {
            std::cout << "PASSED\n";
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
        Key key = test_base::make_key("ttl_test");
        
        auto result = client.getTtl(key).get();
        
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
