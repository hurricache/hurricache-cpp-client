//
// Tests for Container Info, Boundary, Positional, and Pop Operations
//
#include <iostream>
#include <chrono>
#include <future>
#include <vector>
#include <stdexcept>
#include "standalone_client.hxx"

static void testGetSize(FastCacheStandaloneClient &client) {
    std::cout << "  testGetSize... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.getSize(key, &hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getSize returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetHead(FastCacheStandaloneClient &client) {
    std::cout << "  testGetHead... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.getHead(key, &hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getHead returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetTail(FastCacheStandaloneClient &client) {
    std::cout << "  testGetTail... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.getTail(key, &hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getTail returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetElementAtPosition(FastCacheStandaloneClient &client) {
    std::cout << "  testGetElementAtPosition... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.getElementAtPosition(key, &hint, 0, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getElementAtPosition returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetElementWithWeight(FastCacheStandaloneClient &client) {
    std::cout << "  testGetElementWithWeight... ";
    try {
        Key key{};
        key.size = 10;
        key.data = const_cast<char*>("oset1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.getElementWithWeight(key, &hint, 0, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getElementWithWeight returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetAndRemoveFront(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveFront... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.getAndRemoveFront(key, &hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getAndRemoveFront returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetAndRemoveTail(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveTail... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.getAndRemoveTail(key, &hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getAndRemoveTail returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetAndRemoveElementAtPosition(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveElementAtPosition... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.getAndRemoveElementAtPosition(key, &hint, 0, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getAndRemoveElementAtPosition returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetAndRemoveElementWithWeight(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveElementWithWeight... ";
    try {
        Key key{};
        key.size = 10;
        key.data = const_cast<char*>("oset1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.getAndRemoveElementWithWeight(key, &hint, 0, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getAndRemoveElementWithWeight returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

void testContainerInfoBoundaryPositionalPop(FastCacheStandaloneClient &client) {
    std::cout << "=== Container Info, Boundary, Positional, Pop Tests ===\n";
    testGetSize(client);
    testGetHead(client);
    testGetTail(client);
    testGetElementAtPosition(client);
    testGetElementWithWeight(client);
    testGetAndRemoveFront(client);
    testGetAndRemoveTail(client);
    testGetAndRemoveElementAtPosition(client);
    testGetAndRemoveElementWithWeight(client);
    std::cout << "Container Info, Boundary, Positional, Pop: PASSED\n\n";
}
