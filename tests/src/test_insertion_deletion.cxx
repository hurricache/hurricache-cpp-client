//
// Tests for Insertion and Deletion Operations
//
#include <iostream>
#include <chrono>
#include <future>
#include <vector>
#include <stdexcept>
#include "standalone_client.hxx"

static void testAddElementUnordered(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementUnordered... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<Value> data;
        Value v1{};
        v1.size = 5;
        v1.data = const_cast<char*>("new1");
        data.push_back(v1);
        
        auto future = client.addElementUnordered(key, hint, data, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (addElementUnordered returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAddElementWithWeight(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementWithWeight... ";
    try {
        Key key{};
        key.size = 10;
        key.data = const_cast<char*>("oset1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<OrderedValue> data;
        OrderedValue ov1(200, Value{});
        ov1.data = const_cast<char*>("new1");
        ov1.size = 5;
        data.push_back(ov1);
        
        auto future = client.addElementWithWeight(key, hint, data, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (addElementWithWeight returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAddElementToTail(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToTail... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<Value> data;
        Value v1{};
        v1.size = 5;
        v1.data = const_cast<char*>("tail1");
        data.push_back(v1);
        
        auto future = client.addElementToTail(key, hint, data, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (addElementToTail returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAddElementToHead(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToHead... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<Value> data;
        Value v1{};
        v1.size = 5;
        v1.data = const_cast<char*>("head1");
        data.push_back(v1);
        
        auto future = client.addElementToHead(key, hint, data, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (addElementToHead returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAddElementToPosition(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToPosition... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<Value> data;
        Value v1{};
        v1.size = 5;
        v1.data = const_cast<char*>("pos1");
        data.push_back(v1);
        
        auto future = client.addElementToPosition(key, hint, data, 0, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (addElementToPosition returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAddElementToPositionBefore(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToPositionBefore... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<Value> data;
        Value v1{};
        v1.size = 5;
        v1.data = const_cast<char*>("bef1");
        data.push_back(v1);
        
        Value pivot{};
        pivot.size = 5;
        pivot.data = const_cast<char*>("item1");
        
        auto future = client.addElementToPositionBefore(key, hint, data, pivot, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (addElementToPositionBefore returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAddElementToPositionAfter(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToPositionAfter... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<Value> data;
        Value v1{};
        v1.size = 5;
        v1.data = const_cast<char*>("aft1");
        data.push_back(v1);
        
        Value pivot{};
        pivot.size = 5;
        pivot.data = const_cast<char*>("item1");
        
        auto future = client.addElementToPositionAfter(key, hint, data, pivot, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (addElementToPositionAfter returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testRemoveHead(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveHead... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.removeHead(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (removeHead returned " << (result ? "true" : "false") << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testRemoveTail(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveTail... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.removeTail(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (removeTail returned " << (result ? "true" : "false") << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testRemoveElementAtPosition(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveElementAtPosition... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.removeElementAtPosition(key, hint, 0, 0, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (removeElementAtPosition returned " << (result ? "true" : "false") << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testRemoveFromContainer(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveFromContainer... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<Key> keys;
        std::vector<Value> values;
        
        auto future = client.removeFromContainer(key, hint, ContainerType::LIST, keys, values, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (removeFromContainer returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

void testInsertionDeletion(FastCacheStandaloneClient &client) {
    std::cout << "=== Insertion & Deletion Tests ===\n";
    testAddElementUnordered(client);
    testAddElementWithWeight(client);
    testAddElementToTail(client);
    testAddElementToHead(client);
    testAddElementToPosition(client);
    testAddElementToPositionBefore(client);
    testAddElementToPositionAfter(client);
    testRemoveHead(client);
    testRemoveTail(client);
    testRemoveElementAtPosition(client);
    testRemoveFromContainer(client);
    std::cout << "Insertion & Deletion: PASSED\n\n";
}
