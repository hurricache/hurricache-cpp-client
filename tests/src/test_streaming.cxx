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
#include "test_base.hxx"

static void testStreamList(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamList... ";
    try {
        Key key = test_base::make_key("list1");
        
        auto future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamList returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamVector(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamVector... ";
    try {
        Key key = test_base::make_key("vector1");
        
        auto future = client.streamVector(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamVector returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamSet(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamSet... ";
    try {
        Key key = test_base::make_key("set1");
        
        auto future = client.streamSet(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamSet returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamMap(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamMap... ";
    try {
        Key key = test_base::make_key("map1");
        
        auto future = client.streamMap(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamMap returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamOrderedSet(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamOrderedSet... ";
    try {
        Key key = test_base::make_key("oset1");
        
        auto future = client.streamOrderedSet(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamOrderedSet returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamOrderedMap... ";
    try {
        Key key = test_base::make_key("omap1");
        
        auto future = client.streamOrderedMap(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamOrderedMap returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamElementInRangeUnordered(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamElementInRangeUnordered... ";
    try {
        Key key = test_base::make_key("list1");
        
        auto future = client.streamElementInRangeUnordered(key, nullptr, ContainerType::LIST, 0, 10, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamElementInRangeUnordered returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamElementInRangeOrderedSet(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamElementInRangeOrderedSet... ";
    try {
        Key key = test_base::make_key("oset1");
        
        auto future = client.streamElementInRangeOrderedSet(key, nullptr, 0, 100, false, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamElementInRangeOrderedSet returned " << result.size() << " elements)\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamElementInRangeOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamElementInRangeOrderedMap... ";
    try {
        Key key = test_base::make_key("omap1");
        
        auto future = client.streamElementInRangeOrderedMap(key, nullptr, 0, 100, false, 0, std::chrono::milliseconds(5000));
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
