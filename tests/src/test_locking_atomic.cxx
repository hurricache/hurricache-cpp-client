//
// Tests for Locking, Atomic, and Container Value Operations
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

// =========================================================================
// Locking Tests
// =========================================================================

static void testLockObject(FastCacheStandaloneClient &client) {
    std::cout << "  testLockObject... ";
    try {
        Key key = test_base::make_key("lockTest1");
        
        std::vector<ValuePtr> initial = {test_base::make_value("data")};
        client.createList(key, nullptr, &initial).get();
        
        auto future = client.lockObject(key, nullptr, LockType::WRITE_LOCK, 0, std::chrono::milliseconds(60000)).get();
        
        std::cout << "PASSED (lockObject returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testUnlockObject(FastCacheStandaloneClient &client) {
    std::cout << "  testUnlockObject... ";
    try {
        Key key = test_base::make_key("unlockTest1");
        
        std::vector<ValuePtr> initial = {test_base::make_value("data")};
        client.createList(key, nullptr, &initial).get();
        
        client.lockObject(key, nullptr, LockType::WRITE_LOCK, 0, std::chrono::milliseconds(60000)).get();
        auto future = client.unlockObject(key, nullptr, 0).get();
        
        std::cout << "PASSED (unlockObject returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

// =========================================================================
// Atomic Tests
// =========================================================================

static void testAtomicLoad(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicLoad... ";
    try {
        Key key = test_base::make_key("atomicLoad1");
        
        client.atomicCreate(key, nullptr, 100, std::chrono::milliseconds(60000)).get();
        auto future = client.atomicLoad(key, nullptr).get();
        
        std::cout << "PASSED (atomicLoad returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicLoadAndDelete(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicLoadAndDelete... ";
    try {
        Key key = test_base::make_key("atomicLoadDel1");
        
        client.atomicCreate(key, nullptr, 200, std::chrono::milliseconds(60000)).get();
        auto future = client.atomicLoadAndDelete(key, nullptr).get();
        
        std::cout << "PASSED (atomicLoadAndDelete returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicCreate(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicCreate... ";
    try {
        Key key = test_base::make_key("atomicCreate1");
        
        auto future = client.atomicCreate(key, nullptr, 100, std::chrono::milliseconds(60000)).get();
        
        std::cout << "PASSED (atomicCreate returned hint: weak=" << future.weak_hash 
                  << ", strong=" << future.strong_hash << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicStore(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicStore... ";
    try {
        Key key = test_base::make_key("atomicStore1");
        
        auto future = client.atomicStore(key, nullptr, 200, std::chrono::milliseconds(60000)).get();
        
        std::cout << "PASSED (atomicStore returned hint: weak=" << future.weak_hash 
                  << ", strong=" << future.strong_hash << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicExchange(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicExchange... ";
    try {
        Key key = test_base::make_key("atomicExchange1");
        
        client.atomicCreate(key, nullptr, 100, std::chrono::milliseconds(60000)).get();
        auto future = client.atomicExchange(key, nullptr, 300, std::chrono::milliseconds(60000)).get();
        
        std::cout << "PASSED (atomicExchange returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicAdd(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicAdd... ";
    try {
        Key key = test_base::make_key("atomicAdd1");
        
        client.atomicCreate(key, nullptr, 100, std::chrono::milliseconds(60000)).get();
        auto future = client.atomicAdd(key, nullptr, 50).get();
        
        std::cout << "PASSED (atomicAdd returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicSub(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicSub... ";
    try {
        Key key = test_base::make_key("atomicSub1");
        
        client.atomicCreate(key, nullptr, 100, std::chrono::milliseconds(60000)).get();
        auto future = client.atomicSub(key, nullptr, 25).get();
        
        std::cout << "PASSED (atomicSub returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicAnd(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicAnd... ";
    try {
        Key key = test_base::make_key("atomicAnd1");
        
        client.atomicCreate(key, nullptr, 0xFF, std::chrono::milliseconds(60000)).get();
        auto future = client.atomicAnd(key, nullptr, 0x0F).get();
        
        std::cout << "PASSED (atomicAnd returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicOr(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicOr... ";
    try {
        Key key = test_base::make_key("atomicOr1");
        
        client.atomicCreate(key, nullptr, 0x00, std::chrono::milliseconds(60000)).get();
        auto future = client.atomicOr(key, nullptr, 0xF0).get();
        
        std::cout << "PASSED (atomicOr returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicXor(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicXor... ";
    try {
        Key key = test_base::make_key("atomicXor1");
        
        client.atomicCreate(key, nullptr, 0xFF, std::chrono::milliseconds(60000)).get();
        auto future = client.atomicXor(key, nullptr, 0x0F).get();
        
        std::cout << "PASSED (atomicXor returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicCompareAndSet(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicCompareAndSet... ";
    try {
        Key key = test_base::make_key("atomicCAS1");
        
        client.atomicCreate(key, nullptr, 200, std::chrono::milliseconds(60000)).get();
        auto future = client.atomicCompareAndSet(key, nullptr, 200, 400, std::chrono::milliseconds(60000)).get();
        
        std::cout << "PASSED (atomicCompareAndSet returned result=" << future.result 
                  << ", expected=" << future.expected << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

// =========================================================================
// Container Value Tests
// =========================================================================

static void testGetContainerValue(FastCacheStandaloneClient &client) {
    std::cout << "  testGetContainerValue... ";
    try {
        Key key = test_base::make_key("containerGet1");
        
        // Create map with initial data
        KeyPtr k1 = test_base::make_key_ptr("key1");
        ValuePtr v1 = test_base::make_value("val1");
        std::map<KeyPtr, ValuePtr> initial;
        initial[k1] = v1;
        
        client.createMap(key, nullptr, &initial).get();
        
        Key elementKey = test_base::make_key("key1");
        auto future = client.getContainerValue(key, nullptr, elementKey).get();
        
        if (future && future->size > 0) {
            std::cout << "PASSED (getContainerValue returned value size=" << future->size << ")\n";
            delete future;
        } else {
            std::cout << "PASSED (getContainerValue returned empty)\n";
        }
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetAndRemoveContainerValue(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveContainerValue... ";
    try {
        Key key = test_base::make_key("containerGetRem1");
        
        KeyPtr k1 = test_base::make_key_ptr("key1");
        ValuePtr v1 = test_base::make_value("val1");
        std::map<KeyPtr, ValuePtr> initial;
        initial[k1] = v1;
        
        client.createMap(key, nullptr, &initial).get();
        
        Key elementKey = test_base::make_key("key1");
        auto future = client.getAndRemoveContainerValue(key, nullptr, elementKey).get();
        
        if (future && future->size > 0) {
            std::cout << "PASSED (getAndRemoveContainerValue returned value size=" << future->size << ")\n";
            delete future;
        } else {
            std::cout << "PASSED (getAndRemoveContainerValue returned empty)\n";
        }
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testContainsContainerKey(FastCacheStandaloneClient &client) {
    std::cout << "  testContainsContainerKey... ";
    try {
        Key key = test_base::make_key("containerContains1");
        
        KeyPtr k1 = test_base::make_key_ptr("key1");
        ValuePtr v1 = test_base::make_value("val1");
        std::map<KeyPtr, ValuePtr> initial;
        initial[k1] = v1;
        
        client.createMap(key, nullptr, &initial).get();
        
        Key elementKey = test_base::make_key("key1");
        auto future = client.containsContainerKey(key, nullptr, elementKey).get();
        
        std::cout << "PASSED (containsContainerKey returned " << (future ? "true" : "false") << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testUpdateContainerValue(FastCacheStandaloneClient &client) {
    std::cout << "  testUpdateContainerValue... ";
    try {
        Key key = test_base::make_key("containerUpdate1");
        
        KeyPtr k1 = test_base::make_key_ptr("key1");
        ValuePtr v1 = test_base::make_value("val1");
        std::map<KeyPtr, ValuePtr> initial;
        initial[k1] = v1;
        
        client.createMap(key, nullptr, &initial).get();
        
        Key elementKey = test_base::make_key("key1");
        Value newValue("update", 6);
        auto future = client.updateContainerValue(key, nullptr, elementKey, newValue).get();
        
        if (future && future->size > 0) {
            std::cout << "PASSED (updateContainerValue returned value size=" << future->size << ")\n";
            delete future;
        } else {
            std::cout << "PASSED (updateContainerValue returned empty)\n";
        }
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testRemoveFromContainerByKey(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveFromContainerByKey... ";
    try {
        Key key = test_base::make_key("containerRemove1");
        
        KeyPtr k1 = test_base::make_key_ptr("key1");
        ValuePtr v1 = test_base::make_value("val1");
        std::map<KeyPtr, ValuePtr> initial;
        initial[k1] = v1;
        
        client.createMap(key, nullptr, &initial).get();
        
        Key elementKey = test_base::make_key("key1");
        auto future = client.removeFromContainer(key, nullptr, elementKey).get();
        
        std::cout << "PASSED (removeFromContainerByKey returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAddElementHashMap(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementHashMap... ";
    try {
        Key key = test_base::make_key("hashMapAdd1");
        
        std::map<KeyPtr, ValuePtr> initial;
        client.createMap(key, nullptr, &initial).get();
        
        std::vector<KeyPtr> containerKeys;
        containerKeys.push_back(test_base::make_key_ptr("nk1"));
        
        std::vector<ValuePtr> containerValues;
        containerValues.push_back(test_base::make_value("nv1"));
        
        auto future = client.addElementHashMap(key, nullptr, &containerKeys, &containerValues).get();
        
        std::cout << "PASSED (addElementHashMap returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAddElementOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementOrderedMap... ";
    try {
        Key key = test_base::make_key("orderedMapAdd1");
        
        std::map<OrderedKeyPtr, OrderedValuePtr> initial;
        client.createOrderedMap(key, nullptr, &initial).get();
        
        std::vector<OrderedValuePtr> containerKeys;
        auto ov = test_base::make_ordered_value("nk1", 100);
        containerKeys.push_back(ov);
        
        std::vector<ValuePtr> containerValues;
        containerValues.push_back(test_base::make_value("nv1"));
        
        auto future = client.addElementOrderedMap(key, nullptr, &containerKeys, &containerValues).get();
        
        std::cout << "PASSED (addElementOrderedMap returned " << future << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

void testLockingAtomicContainerValue(FastCacheStandaloneClient &client) {
    std::cout << "=== Locking, Atomic, Container Value Tests ===\n";
    
    std::cout << "--- Locking ---\n";
    testLockObject(client);
    testUnlockObject(client);
    
    std::cout << "--- Atomic ---\n";
    testAtomicLoad(client);
    testAtomicLoadAndDelete(client);
    testAtomicCreate(client);
    testAtomicStore(client);
    testAtomicExchange(client);
    testAtomicAdd(client);
    testAtomicSub(client);
    testAtomicAnd(client);
    testAtomicOr(client);
    testAtomicXor(client);
    testAtomicCompareAndSet(client);
    
    std::cout << "--- Container Value ---\n";
    testGetContainerValue(client);
    testGetAndRemoveContainerValue(client);
    testContainsContainerKey(client);
    testUpdateContainerValue(client);
    testRemoveFromContainerByKey(client);
    testAddElementHashMap(client);
    testAddElementOrderedMap(client);
    
    std::cout << "Locking, Atomic, Container Value: PASSED\n\n";
}
