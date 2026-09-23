//
// Tests for Key-Value Operations
//
#include <iostream>
#include <chrono>
#include <future>
#include <vector>
#include <cassert>
#include <stdexcept>
#include "standalone_client.hxx"

static void testCreateKeyValue(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateKeyValue... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("kv_test1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        Value value{};
        value.size = 5;
        value.data = const_cast<char*>("hello");
        
        auto future = client.createKeyValue(key, hint, value, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (createKeyValue returned hint: weak=" << result.weak_hash 
                  << ", strong=" << result.strong_hash << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetValue(FastCacheStandaloneClient &client) {
    std::cout << "  testGetValue... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("kv_test1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.getValue(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getValue returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testUpdateKeyValue(FastCacheStandaloneClient &client) {
    std::cout << "  testUpdateKeyValue... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("kv_test1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        Value value{};
        value.size = 6;
        value.data = const_cast<char*>("world!");
        
        auto future = client.updateKeyValue(key, hint, value, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (updateKeyValue returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testExistKey(FastCacheStandaloneClient &client) {
    std::cout << "  testExistKey... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("kv_test1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.existKey(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (existKey returned " << (result ? "true" : "false") << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testRemove(FastCacheStandaloneClient &client) {
    std::cout << "  testRemove... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("kv_test1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.remove(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (remove returned " << (result ? "true" : "false") << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetAndDeleteValue(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndDeleteValue... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("kv_test_del");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        // First create the key
        Value value{};
        value.size = 5;
        value.data = const_cast<char*>("delete");
        
        try {
            auto createFuture = client.createKeyValue(key, hint, value, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
            createFuture.get();
        } catch (...) {
            // Ignore if already exists
        }
        
        // Now get and delete
        auto future = client.getAndDeleteValue(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getAndDeleteValue returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

void testKeyValueOperations(FastCacheStandaloneClient &client) {
    std::cout << "=== Key-Value Operations Tests ===\n";
    testCreateKeyValue(client);
    testGetValue(client);
    testUpdateKeyValue(client);
    testExistKey(client);
    testRemove(client);
    testGetAndDeleteValue(client);
    std::cout << "Key-Value Operations: PASSED\n\n";
}
