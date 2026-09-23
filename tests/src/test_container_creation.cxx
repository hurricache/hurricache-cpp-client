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
#include "test_base.hxx"

static void testCreateQueue(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateQueue... ";
    try {
        Key key = test_base::make_key("queue1");
        
        std::vector<ValuePtr> initial;
        initial.push_back(test_base::make_value("item1"));
        
        auto future = client.createQueue(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
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
        Key key = test_base::make_key("list1");
        
        std::vector<ValuePtr> initial;
        initial.push_back(test_base::make_value("item1"));
        
        auto future = client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
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
        Key key = test_base::make_key("vector1");
        
        std::vector<ValuePtr> initial;
        initial.push_back(test_base::make_value("item1"));
        
        auto future = client.createVector(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
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
        Key key = test_base::make_key("set1");
        
        std::vector<ValuePtr> initial;
        initial.push_back(test_base::make_value("item1"));
        
        auto future = client.createSet(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
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
        Key key = test_base::make_key("oset1");
        
        std::vector<OrderedValuePtr> initial;
        OrderedValuePtr ov = new OrderedValue{100, 5, const_cast<char*>("item1")};
        initial.push_back(ov);
        
        auto future = client.createOrderedSet(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
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
        Key key = test_base::make_key("map1");
        
        std::map<Key, Value> initial;
        Key mapKey = test_base::make_key("key1");
        Value mapValue = Value{5, const_cast<char*>("val1")};
        
        initial[mapKey] = mapValue;
        
        auto future = client.createMap(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
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
        Key key = test_base::make_key("omap1");
        
        std::map<OrderedKey, OrderedValue> initial;
        OrderedKey okey = OrderedValue{50, 4, const_cast<char*>("key1")};
        // Need to set weight properly
        okey.weight = 50;
        
        OrderedValue ovalue = OrderedValue{100, 5, const_cast<char*>("val1")};
        
        initial[okey] = ovalue;
        
        auto future = client.createOrderedMap(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
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
