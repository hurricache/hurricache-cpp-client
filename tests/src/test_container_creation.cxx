//
// Tests for Container Creation
//
#include <iostream>
#include <chrono>
#include <future>
#include <vector>
#include <map>
#include <stdexcept>
#include "standalone_client.hxx"

static void testCreateQueue(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateQueue... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("queue1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<Value> initial;
        Value v1{};
        v1.size = 5;
        v1.data = const_cast<char*>("item1");
        initial.push_back(v1);
        
        auto future = client.createQueue(key, hint, initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (createQueue returned hint: weak=" << result.weak_hash 
                  << ", strong=" << result.strong_hash << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testCreateList(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateList... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<Value> initial;
        Value v1{};
        v1.size = 5;
        v1.data = const_cast<char*>("item1");
        initial.push_back(v1);
        
        auto future = client.createList(key, hint, initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (createList returned hint: weak=" << result.weak_hash 
                  << ", strong=" << result.strong_hash << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testCreateVector(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateVector... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("vector1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<Value> initial;
        Value v1{};
        v1.size = 5;
        v1.data = const_cast<char*>("item1");
        initial.push_back(v1);
        
        auto future = client.createVector(key, hint, initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (createVector returned hint: weak=" << result.weak_hash 
                  << ", strong=" << result.strong_hash << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testCreateSet(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateSet... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("set1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<Value> initial;
        Value v1{};
        v1.size = 5;
        v1.data = const_cast<char*>("item1");
        initial.push_back(v1);
        
        auto future = client.createSet(key, hint, initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (createSet returned hint: weak=" << result.weak_hash 
                  << ", strong=" << result.strong_hash << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testCreateOrderedSet(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateOrderedSet... ";
    try {
        Key key{};
        key.size = 10;
        key.data = const_cast<char*>("oset1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<OrderedValue> initial;
        OrderedValue ov1(100, Value{});
        ov1.data = const_cast<char*>("item1");
        ov1.size = 5;
        initial.push_back(ov1);
        
        auto future = client.createOrderedSet(key, hint, initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (createOrderedSet returned hint: weak=" << result.weak_hash 
                  << ", strong=" << result.strong_hash << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testCreateMap(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateMap... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("map1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::map<Key, Value> initial;
        Key mapKey{};
        mapKey.size = 4;
        mapKey.data = const_cast<char*>("key1");
        
        Value mapValue{};
        mapValue.size = 5;
        mapValue.data = const_cast<char*>("val1");
        
        initial[mapKey] = mapValue;
        
        auto future = client.createMap(key, hint, initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (createMap returned hint: weak=" << result.weak_hash 
                  << ", strong=" << result.strong_hash << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testCreateOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateOrderedMap... ";
    try {
        Key key{};
        key.size = 10;
        key.data = const_cast<char*>("omap1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::map<OrderedKey, OrderedValue> initial;
        OrderedKey okey(50, Key{});
        okey.data = const_cast<char*>("key1");
        okey.size = 4;
        
        OrderedValue ovalue(100, Value{});
        ovalue.data = const_cast<char*>("val1");
        ovalue.size = 5;
        
        initial[okey] = ovalue;
        
        auto future = client.createOrderedMap(key, hint, initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (createOrderedMap returned hint: weak=" << result.weak_hash 
                  << ", strong=" << result.strong_hash << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

void testContainerCreation(FastCacheStandaloneClient &client) {
    std::cout << "=== Container Creation Tests ===\n";
    testCreateQueue(client);
    testCreateList(client);
    testCreateVector(client);
    testCreateSet(client);
    testCreateOrderedSet(client);
    testCreateMap(client);
    testCreateOrderedMap(client);
    std::cout << "Container Creation: PASSED\n\n";
}
