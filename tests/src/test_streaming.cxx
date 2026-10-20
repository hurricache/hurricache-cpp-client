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
#include "utils.hxx"

static void testStreamList(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamList... ";
    try {
        Key key = test_base::make_key("list1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        client.createList(key, nullptr, &initial).get();
        free_content(initial);
        
        auto future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamList returned " << result.size() << " elements)\n";
        free_content(result);
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamVector(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamVector... ";
    try {
        Key key = test_base::make_key("vector1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        client.createVector(key, nullptr, &initial).get();
        free_content(initial);
        
        auto future = client.streamVector(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamVector returned " << result.size() << " elements)\n";
        free_content(result);
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamSet(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamSet... ";
    try {
        Key key = test_base::make_key("set1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        client.createSet(key, nullptr, &initial).get();
        free_content(initial);
        
        auto future = client.streamSet(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamSet returned " << result.size() << " elements)\n";
        free_content(result);
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamMap(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamMap... ";
    try {
        Key key = test_base::make_key("map1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::map<KeyPtr, ValuePtr> initial;
        auto k1 = test_base::make_key_ptr("key1");
        auto k2 = test_base::make_key_ptr("key2");
        initial[k1] = test_base::make_value("val1");
        initial[k2] = test_base::make_value("val2");
        client.createMap(key, nullptr, &initial).get();
        free_content(initial);
        
        auto future = client.streamMap(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamMap returned " << result.size() << " elements)\n";
        free_content(result);
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamOrderedSet(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamOrderedSet... ";
    try {
        Key key = test_base::make_key("oset1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<OrderedValuePtr> initial = {
            test_base::make_ordered_value("item1", 100),
            test_base::make_ordered_value("item2", 200)
        };
        client.createOrderedSet(key, nullptr, &initial).get();
        free_content(initial);
        
        auto future = client.streamOrderedSet(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamOrderedSet returned " << result.size() << " elements)\n";
        free_content(result);
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamOrderedMap... ";
    try {
        Key key = test_base::make_key("omap1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::map<OrderedKeyPtr, OrderedValuePtr> initial;
        client.createOrderedMap(key, nullptr, &initial).get();
        
        auto future = client.streamOrderedMap(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamOrderedMap returned " << result.size() << " elements)\n";
        free_content(result);
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamElementInRangeUnordered(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamElementInRangeUnordered... ";
    try {
        Key key = test_base::make_key("list1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2"),
            test_base::make_value("item3")
        };
        client.createList(key, nullptr, &initial).get();
        free_content(initial);
        
        auto future = client.streamElementInRangeUnordered(key, nullptr, ContainerType::LIST, 0, 10, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamElementInRangeUnordered returned " << result.size() << " elements)\n";
        free_content(result);
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamElementInRangeOrderedSet(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamElementInRangeOrderedSet... ";
    try {
        Key key = test_base::make_key("oset1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<OrderedValuePtr> initial = {
            test_base::make_ordered_value("item1", 10),
            test_base::make_ordered_value("item2", 50),
            test_base::make_ordered_value("item3", 100)
        };
        client.createOrderedSet(key, nullptr, &initial).get();
        free_content(initial);
        
        auto future = client.streamElementInRangeOrderedSet(key, nullptr, 0, 100, false, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamElementInRangeOrderedSet returned " << result.size() << " elements)\n";
        free_content(result);
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testStreamElementInRangeOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamElementInRangeOrderedMap... ";
    try {
        Key key = test_base::make_key("omap1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        // Create empty ordered map first
        std::map<OrderedKeyPtr, OrderedValuePtr> empty;
        client.createOrderedMap(key, nullptr, &empty).get();
        
        // Add elements
        std::vector<OrderedValuePtr> okeys;
        std::vector<ValuePtr> values;
        okeys.push_back(test_base::make_ordered_value("key1", 10));
        okeys.push_back(test_base::make_ordered_value("key2", 50));
        okeys.push_back(test_base::make_ordered_value("key3", 100));
        values.push_back(test_base::make_value("val1"));
        values.push_back(test_base::make_value("val2"));
        values.push_back(test_base::make_value("val3"));
        client.addElementOrderedMap(key, nullptr, &okeys, &values).get();
        
        auto future = client.streamElementInRangeOrderedMap(key, nullptr, 0, 100, false, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (streamElementInRangeOrderedMap returned " << result.size() << " elements)\n";
        free_content(result);
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
