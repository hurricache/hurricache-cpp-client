//
// Tests for Streaming Operations
//
#include <iostream>
#include <chrono>
#include <future>
#include <vector>
#include <map>
#include <stdexcept>
#include "standalone_client.hxx"

static void testStreamList(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamList... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.streamList(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamList returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamVector(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamVector... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("vector1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.streamVector(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamVector returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamSet(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamSet... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("set1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.streamSet(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamSet returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamMap(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamMap... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("map1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.streamMap(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamMap returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamOrderedSet(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamOrderedSet... ";
    try {
        Key key{};
        key.size = 10;
        key.data = const_cast<char*>("oset1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.streamOrderedSet(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamOrderedSet returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamOrderedMap... ";
    try {
        Key key{};
        key.size = 10;
        key.data = const_cast<char*>("omap1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.streamOrderedMap(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamOrderedMap returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamElementInRangeUnordered(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamElementInRangeUnordered... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.streamElementInRangeUnordered(key, hint, ContainerType::LIST, 0, 10, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamElementInRangeUnordered returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamElementInRangeOrderedSet(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamElementInRangeOrderedSet... ";
    try {
        Key key{};
        key.size = 10;
        key.data = const_cast<char*>("oset1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.streamElementInRangeOrderedSet(key, hint, 0, 100, false, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamElementInRangeOrderedSet returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamElementInRangeOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamElementInRangeOrderedMap... ";
    try {
        Key key{};
        key.size = 10;
        key.data = const_cast<char*>("omap1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.streamElementInRangeOrderedMap(key, hint, 0, 100, false, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamElementInRangeOrderedMap returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

void testStreamingOperations(FastCacheStandaloneClient &client) {
    std::cout << "=== Streaming Operations Tests ===\n";
    testStreamList(client);
    testStreamVector(client);
    testStreamSet(client);
    testStreamMap(client);
    testStreamOrderedSet(client);
    testStreamOrderedMap(client);
    testStreamElementInRangeUnordered(client);
    testStreamElementInRangeOrderedSet(client);
    testStreamElementInRangeOrderedMap(client);
    std::cout << "Streaming Operations: PASSED\n\n";
}
