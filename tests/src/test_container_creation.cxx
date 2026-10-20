//
// Tests for Container Creation
// Based on Java tests for createQueue, createList, createVector, createSet, createOrderedSet, createMap, createOrderedMap
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

static void testCreateQueue(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateQueue... ";
    try {
        Key key = test_base::make_key("queue1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        
        auto hint = client.createQueue(key, nullptr, &initial).get();
        assert(hint.strong_hash != 0);
        free_content(initial);
        // Verify by reading from queue
        auto r1 = client.getAndRemoveFront(key).get();
        auto r2 = client.getAndRemoveFront(key).get();
        assert(std::string(r1->data, r1->size) == "item1");
        delete r1;
        assert(std::string(r2->data, r2->size) == "item2");
        delete r2;

        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testCreateList(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateList... ";
    try {
        Key key = test_base::make_key("list1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        
        auto hint = client.createList(key, nullptr, &initial).get();
        assert(hint.strong_hash != 0);
        
        auto items = client.streamList(key).get();
        assert(items.size() == 2);
        free_content(items);
        free_content(initial);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testCreateVector(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateVector... ";
    try {
        Key key = test_base::make_key("vector1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        
        auto hint = client.createVector(key, nullptr, &initial).get();
        assert(hint.strong_hash != 0);
        
        auto items = client.streamVector(key).get();
        assert(items.size() == 2);
        free_content(items);
        free_content(initial);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testCreateSet(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateSet... ";
    try {
        Key key = test_base::make_key("set1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        
        auto hint = client.createSet(key, nullptr, &initial).get();
        assert(hint.strong_hash != 0);
        
        auto items = client.streamSet(key).get();
        assert(items.size() == 2);
        free_content(items);
        free_content(initial);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testCreateOrderedSet(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateOrderedSet... ";
    try {
        Key key = test_base::make_key("oset1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<OrderedValuePtr> initial = {
            test_base::make_ordered_value("item1", 100),
            test_base::make_ordered_value("item2", 200)
        };
        
        auto hint = client.createOrderedSet(key, nullptr, &initial).get();
        assert(hint.strong_hash != 0);
        
        auto items = client.streamOrderedSet(key).get();
        assert(items.size() == 2);
        free_content(items);
        free_content(initial);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testCreateMap(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateMap... ";
    try {
        Key key = test_base::make_key("map1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::map<KeyPtr, ValuePtr> initial;
        auto k1 = test_base::make_key_ptr("key1");
        auto k2 = test_base::make_key_ptr("key2");
        initial[k1] = test_base::make_value("val1");
        initial[k2] = test_base::make_value("val2");

        auto hint = client.createMap(key, nullptr, &initial).get();
        assert(hint.strong_hash != 0);

        auto items = client.streamMap(key).get();
        assert(items.size() == 2);
        free_content(items);
        free_content(initial);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testCreateOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateOrderedMap... ";
    try {
        Key key = test_base::make_key("omap1");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();

        // Create empty ordered map first
        std::map<OrderedKeyPtr, OrderedValuePtr> initial;
        auto hint = client.createOrderedMap(key, nullptr, &initial).get();
        assert(hint.strong_hash != 0);

        auto items = client.streamOrderedMap(key).get();
        assert(items.size() == 0);
        free_content(items);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
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
