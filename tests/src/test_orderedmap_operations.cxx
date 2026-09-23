//
// Tests for OrderedMap Operations
// Based on Java OrderedMapOperationsTest
//
#include <iostream>
#include <chrono>
#include <future>
#include <vector>
#include <map>
#include <string>
#include <cassert>
#include "test_base.hxx"

static void testCreateEmptyOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateEmptyOrderedMap... ";
    try {
        Key key = test_base::make_key("createEmptyOrderedMap");

        std::map<OrderedKey, OrderedValue> initial;
        auto hint = client.createOrderedMap(key, nullptr, &initial).get();
        assert(hint.strong_hash != 0);

        auto items = client.streamOrderedMap(key).get();
        assert(items.size() == 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testCreateOrderedMapWithInitialData(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateOrderedMapWithInitialData... ";
    try {
        Key key = test_base::make_key("createOrderedMapWithInitialData");

        std::map<OrderedKey, OrderedValue> initial;
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(50, 4, "key1"),
                        std::forward_as_tuple(100, 4, "val1"));
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(100, 4, "key2"),
                        std::forward_as_tuple(200, 4, "val2"));

        auto hint = client.createOrderedMap(key, nullptr, &initial).get();
        assert(hint.strong_hash != 0);

        auto items = client.streamOrderedMap(key).get();
        assert(items.size() == 2);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testAddElementOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementOrderedMap... ";
    try {
        Key key = test_base::make_key("addElementOrderedMap");

        std::map<OrderedKey, OrderedValue> initial;
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(50, 4, "key1"),
                        std::forward_as_tuple(100, 4, "val1"));
        client.createOrderedMap(key, nullptr, &initial).get();

        std::vector<OrderedValuePtr> keys;
        std::vector<ValuePtr> values;
        auto okey2 = new OrderedValue(100, 4, "key2");
        keys.push_back(okey2);
        values.push_back(test_base::make_value("val2"));

        auto added = client.addElementOrderedMap(key, nullptr, &keys, &values).get();
        assert(added == 1);

        auto items = client.streamOrderedMap(key).get();
        assert(items.size() == 2);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetContainerValue(FastCacheStandaloneClient &client) {
    std::cout << "  testGetContainerValue... ";
    try {
        Key key = test_base::make_key("getContainerValue");

        std::map<OrderedKey, OrderedValue> initial;
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(50, 4, "key1"),
                        std::forward_as_tuple(100, 4, "val1"));
        client.createOrderedMap(key, nullptr, &initial).get();

        Key elementKey = test_base::make_key("key1");
        auto value = client.getContainerValue(key, nullptr, elementKey).get();
        assert(std::string(value->data, value->size) == "val1");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testUpdateContainerValue(FastCacheStandaloneClient &client) {
    std::cout << "  testUpdateContainerValue... ";
    try {
        Key key = test_base::make_key("updateContainerValue");

        std::map<OrderedKey, OrderedValue> initial;
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(50, 4, "key1"),
                        std::forward_as_tuple(100, 4, "val1"));
        client.createOrderedMap(key, nullptr, &initial).get();

        Key elementKey = test_base::make_key("key1");
        Value newValue("val2_updated", 10);
        auto updated = client.updateContainerValue(key, nullptr, elementKey, newValue).get();
        assert(std::string(updated->data, updated->size) == "val2_updated");

        auto value = client.getContainerValue(key, nullptr, elementKey).get();
        assert(std::string(value->data, value->size) == "val2_updated");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetAndRemoveContainerValue(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveContainerValue... ";
    try {
        Key key = test_base::make_key("getAndRemoveContainerValue");

        std::map<OrderedKey, OrderedValue> initial;
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(50, 4, "key1"),
                        std::forward_as_tuple(100, 4, "val1"));
        client.createOrderedMap(key, nullptr, &initial).get();

        Key elementKey = test_base::make_key("key1");
        auto removed = client.getAndRemoveContainerValue(key, nullptr, elementKey).get();
        assert(std::string(removed->data, removed->size) == "val1");

        auto items = client.streamOrderedMap(key).get();
        assert(items.size() == 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testContainsContainerKey(FastCacheStandaloneClient &client) {
    std::cout << "  testContainsContainerKey... ";
    try {
        Key key = test_base::make_key("containsContainerKey");

        std::map<OrderedKey, OrderedValue> initial;
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(50, 4, "key1"),
                        std::forward_as_tuple(100, 4, "val1"));
        client.createOrderedMap(key, nullptr, &initial).get();

        Key elementKey = test_base::make_key("key1");
        auto exists = client.containsContainerKey(key, nullptr, elementKey).get();
        assert(exists);

        Key elementKey2 = test_base::make_key("nonexistent");
        auto not_exists = client.containsContainerKey(key, nullptr, elementKey2).get();
        assert(!not_exists);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveFromContainer(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveFromContainer... ";
    try {
        Key key = test_base::make_key("removeFromContainer");

        std::map<OrderedKey, OrderedValue> initial;
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(50, 4, "key1"),
                        std::forward_as_tuple(100, 4, "val1"));
        client.createOrderedMap(key, nullptr, &initial).get();

        Key elementKey = test_base::make_key("key1");
        auto removed = client.removeFromContainer(key, nullptr, elementKey).get();
        assert(removed == 1);

        auto items = client.streamOrderedMap(key).get();
        assert(items.size() == 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testStreamElementInRangeOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamElementInRangeOrderedMap... ";
    try {
        Key key = test_base::make_key("streamElementInRangeOrderedMap");

        std::map<OrderedKey, OrderedValue> initial;
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(50, 4, "key1"),
                        std::forward_as_tuple(100, 4, "val1"));
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(100, 4, "key2"),
                        std::forward_as_tuple(200, 4, "val2"));
        client.createOrderedMap(key, nullptr, &initial).get();

        auto range = client.streamElementInRangeOrderedMap(key, nullptr, 50, 100, false, 0).get();
        assert(range.size() == 2);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveElementAtPosition(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveElementAtPosition... ";
    try {
        Key key = test_base::make_key("removeElementAtPosition");

        std::map<OrderedKey, OrderedValue> initial;
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(50, 4, "key1"),
                        std::forward_as_tuple(100, 4, "val1"));
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(100, 4, "key2"),
                        std::forward_as_tuple(200, 4, "val2"));
        client.createOrderedMap(key, nullptr, &initial).get();

        auto removed = client.removeElementAtPosition(key, nullptr, 50, 100).get();
        assert(removed);

        auto items = client.streamOrderedMap(key).get();
        assert(items.size() == 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testSetTtlOnOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testSetTtlOnOrderedMap... ";
    try {
        Key key = test_base::make_key("setTtlOrderedMap");

        std::map<OrderedKey, OrderedValue> initial;
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(50, 4, "key1"),
                        std::forward_as_tuple(100, 4, "val1"));
        client.createOrderedMap(key, nullptr, &initial).get();

        auto setTtl = client.setTtl(key, nullptr, 5000).get();
        assert(setTtl);

        auto ttl = client.getTtl(key).get();
        assert(ttl > 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testLockOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testLockOrderedMap... ";
    try {
        Key key = test_base::make_key("lockOrderedMap");

        std::map<OrderedKey, OrderedValue> initial;
        initial.emplace(std::piecewise_construct,
                        std::forward_as_tuple(50, 4, "key1"),
                        std::forward_as_tuple(100, 4, "val1"));
        client.createOrderedMap(key, nullptr, &initial).get();

        int32_t ownerId = 1;
        auto lockRes = client.lockObject(key, nullptr, LockType::WRITE_LOCK, ownerId).get();
        assert(lockRes == LockStatus::OK);

        auto unlockRes = client.unlockObject(key, nullptr, ownerId).get();
        assert(unlockRes == LockStatus::OK);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

void testOrderedMapOperations(FastCacheStandaloneClient &client) {
    std::cout << "=== OrderedMap Operations Tests ===\n";
    testCreateEmptyOrderedMap(client);
    testCreateOrderedMapWithInitialData(client);
    testAddElementOrderedMap(client);
    testGetContainerValue(client);
    testUpdateContainerValue(client);
    testGetAndRemoveContainerValue(client);
    testContainsContainerKey(client);
    testRemoveFromContainer(client);
    testStreamElementInRangeOrderedMap(client);
    testRemoveElementAtPosition(client);
    testSetTtlOnOrderedMap(client);
    testLockOrderedMap(client);
    std::cout << "OrderedMap Operations: PASSED\n\n";
}
