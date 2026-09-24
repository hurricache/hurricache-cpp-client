//
// Tests for Set operations - based on Java SetOperationsTest
//
#include <iostream>
#include <chrono>
#include <future>
#include <vector>
#include <string>
#include <cassert>
#include <set>
#include "test_base.hxx"
#include "utils.hxx"

static void testCreateEmptySet(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateEmptySet... ";
    try {
        Key key = test_base::make_key("createEmptySet");
        client.createSet(key).get();
        
        auto items = client.streamSet(key).get();
        assert(items.size() == 0);
        free_content(items);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testCreateSetWithInitialData(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateSetWithInitialData... ";
    try {
        Key key = test_base::make_key("createSetWithInitialData");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("v1"),
            test_base::make_value("v2"),
            test_base::make_value("v3")
        };
        
        auto hint = client.createSet(key, nullptr, &initial).get();
        assert(hint.strong_hash != 0);
        
        auto items = client.streamSet(key).get();
        assert(items.size() == 3);
        free_content(items);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testStreamSetContents(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamSetContents... ";
    try {
        Key key = test_base::make_key("streamSetContents");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("a"),
            test_base::make_value("b"),
            test_base::make_value("c"),
            test_base::make_value("d"),
            test_base::make_value("e")
        };
        
        client.createSet(key, nullptr, &initial).get();
        
        auto items = client.streamSet(key).get();
        assert(items.size() == 5);
        free_content(items);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testAddElementUnordered(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementUnordered... ";
    try {
        Key key = test_base::make_key("addElementUnordered");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("a"),
            test_base::make_value("b")
        };
        client.createSet(key, nullptr, &initial).get();
        
        std::vector<ValuePtr> data = {test_base::make_value("c"), test_base::make_value("d")};
        auto added = client.addElementUnordered(key, nullptr, &data).get();
        assert(added == 2);
        
        auto items = client.streamSet(key).get();
        assert(items.size() == 4);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetSize(FastCacheStandaloneClient &client) {
    std::cout << "  testGetSize... ";
    try {
        Key key = test_base::make_key("getSize");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("a"),
            test_base::make_value("b"),
            test_base::make_value("c")
        };
        client.createSet(key, nullptr, &initial).get();
        
        auto size = client.getSize(key, nullptr).get();
        assert(size == 3);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveSet(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveSet... ";
    try {
        Key key = test_base::make_key("removeSet");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        client.createSet(key, nullptr, &initial).get();
        
        auto removed = client.remove(key, nullptr).get();
        assert(removed);
        
        try {
            client.streamSet(key).get();
            std::cout << "FAILED: Expected NOT_FOUND\n";
        } catch (const std::exception &) {
            std::cout << "PASSED\n";
        }
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testSetTtlAndGetTtl(FastCacheStandaloneClient &client) {
    std::cout << "  testSetTtlAndGetTtl... ";
    try {
        Key key = test_base::make_key("setTtlAndGetTtl");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("a"),
            test_base::make_value("b")
        };
        client.createSet(key, nullptr, &initial).get();
        
        auto setTtl = client.setTtl(key, nullptr, 5000).get();
        assert(setTtl);
        
        auto ttl = client.getTtl(key, nullptr).get();
        assert(ttl > 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testWriteLockOnSet(FastCacheStandaloneClient &client) {
    std::cout << "  testWriteLockOnSet... ";
    try {
        Key key = test_base::make_key("writeLockSet");
        
        std::vector<ValuePtr> initial = {test_base::make_value("data")};
        client.createSet(key, nullptr, &initial).get();
        
        auto lockRes = client.lockObject(key, nullptr, LockType::WRITE_LOCK, 1).get();
        assert(lockRes == OK);
        
        auto unlockRes = client.unlockObject(key, nullptr, 1).get();
        assert(unlockRes == OK);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testReadLockOnSet(FastCacheStandaloneClient &client) {
    std::cout << "  testReadLockOnSet... ";
    try {
        Key key = test_base::make_key("readLockSet");
        
        std::vector<ValuePtr> initial = {test_base::make_value("data")};
        client.createSet(key, nullptr, &initial).get();
        
        auto lockRes = client.lockObject(key, nullptr, LockType::READ_LOCK).get();
        assert(lockRes == OK);
        
        auto unlockRes = client.unlockObject(key).get();
        assert(unlockRes == OK);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

void testSetOperations(FastCacheStandaloneClient &client) {
    std::cout << "=== Set Operations Tests ===\n";
    testCreateEmptySet(client);
    testCreateSetWithInitialData(client);
    testStreamSetContents(client);
    testAddElementUnordered(client);
    testGetSize(client);
    testRemoveSet(client);
    testSetTtlAndGetTtl(client);
    testWriteLockOnSet(client);
    testReadLockOnSet(client);
    std::cout << "Set Operations: ALL PASSED\n\n";
}
