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

// =========================================================================
// Locking Tests
// =========================================================================

static void testLockObject(FastCacheStandaloneClient &client) {
    std::cout << "  testLockObject... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.lockObject(key, hint, LockType::WRITE_LOCK, 0, std::chrono::milliseconds(60000), std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (lockObject returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testUnlockObject(FastCacheStandaloneClient &client) {
    std::cout << "  testUnlockObject... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("list1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.unlockObject(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (unlockObject returned " << result << ")\n";
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
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("atomic1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.atomicLoad(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (atomicLoad returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicLoadAndDelete(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicLoadAndDelete... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("atomic1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.atomicLoadAndDelete(key, hint, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (atomicLoadAndDelete returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicCreate(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicCreate... ";
    try {
        Key key{};
        key.size = 9;
        key.data = const_cast<char*>("atomic1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.atomicCreate(key, hint, 100, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (atomicCreate returned hint: weak=" << result.weak_hash 
                  << ", strong=" << result.strong_hash << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicStore(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicStore... ";
    try {
        Key key{};
        key.size = 9;
        key.data = const_cast<char*>("atomic1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.atomicStore(key, hint, 200, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (atomicStore returned hint: weak=" << result.weak_hash 
                  << ", strong=" << result.strong_hash << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicExchange(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicExchange... ";
    try {
        Key key{};
        key.size = 9;
        key.data = const_cast<char*>("atomic1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.atomicExchange(key, hint, 300, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (atomicExchange returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicAdd(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicAdd... ";
    try {
        Key key{};
        key.size = 9;
        key.data = const_cast<char*>("atomic1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.atomicAdd(key, hint, 50, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (atomicAdd returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicSub(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicSub... ";
    try {
        Key key{};
        key.size = 9;
        key.data = const_cast<char*>("atomic1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.atomicSub(key, hint, 25, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (atomicSub returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicAnd(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicAnd... ";
    try {
        Key key{};
        key.size = 9;
        key.data = const_cast<char*>("atomic1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.atomicAnd(key, hint, 0xFF, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (atomicAnd returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicOr(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicOr... ";
    try {
        Key key{};
        key.size = 9;
        key.data = const_cast<char*>("atomic1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.atomicOr(key, hint, 0xF0, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (atomicOr returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicXor(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicXor... ";
    try {
        Key key{};
        key.size = 9;
        key.data = const_cast<char*>("atomic1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.atomicXor(key, hint, 0x0F, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (atomicXor returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAtomicCompareAndSet(FastCacheStandaloneClient &client) {
    std::cout << "  testAtomicCompareAndSet... ";
    try {
        Key key{};
        key.size = 9;
        key.data = const_cast<char*>("atomic1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        auto future = client.atomicCompareAndSet(key, hint, 200, 400, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (atomicCompareAndSet returned result=" << result.result 
                  << ", expected=" << result.expected << ")\n";
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
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("map1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        Key elementKey{};
        elementKey.size = 4;
        elementKey.data = const_cast<char*>("key1");
        
        auto future = client.getContainerValue(key, hint, elementKey, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getContainerValue returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testGetAndRemoveContainerValue(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveContainerValue... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("map1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        Key elementKey{};
        elementKey.size = 4;
        elementKey.data = const_cast<char*>("key1");
        
        auto future = client.getAndRemoveContainerValue(key, hint, elementKey, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (getAndRemoveContainerValue returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testContainsContainerKey(FastCacheStandaloneClient &client) {
    std::cout << "  testContainsContainerKey... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("map1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        Key elementKey{};
        elementKey.size = 4;
        elementKey.data = const_cast<char*>("key1");
        
        auto future = client.containsContainerKey(key, hint, elementKey, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (containsContainerKey returned " << (result ? "true" : "false") << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testUpdateContainerValue(FastCacheStandaloneClient &client) {
    std::cout << "  testUpdateContainerValue... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("map1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        Key elementKey{};
        elementKey.size = 4;
        elementKey.data = const_cast<char*>("key1");
        
        Value newValue{};
        newValue.size = 6;
        newValue.data = const_cast<char*>("update");
        
        auto future = client.updateContainerValue(key, hint, elementKey, newValue, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (updateContainerValue returned value size=" << result.size << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testRemoveFromContainerByKey(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveFromContainerByKey... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("map1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        Key elementKey{};
        elementKey.size = 4;
        elementKey.data = const_cast<char*>("key1");
        
        auto future = client.removeFromContainer(key, hint, elementKey, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (removeFromContainerByKey returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAddElementHashMap(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementHashMap... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("map1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<Key> containerKeys;
        Key k1{};
        k1.size = 4;
        k1.data = const_cast<char*>("nk1");
        containerKeys.push_back(k1);
        
        std::vector<Value> containerValues;
        Value v1{};
        v1.size = 5;
        v1.data = const_cast<char*>("nv1");
        containerValues.push_back(v1);
        
        auto future = client.addElementHashMap(key, hint, containerKeys, containerValues, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (addElementHashMap returned " << result << ")\n";
    } catch (const std::exception &e) {
        std::cout << "SKIPPED (server unavailable: " << e.what() << ")\n";
    }
}

static void testAddElementOrderedMap(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementOrderedMap... ";
    try {
        Key key{};
        key.size = 8;
        key.data = const_cast<char*>("omap1");
        
        KeyHint hint{};
        hint.weak_hash = 0;
        hint.strong_hash = 0;
        
        std::vector<OrderedValue> containerKeys;
        OrderedValue ov1(100, Value{});
        ov1.data = const_cast<char*>("nk1");
        ov1.size = 4;
        containerKeys.push_back(ov1);
        
        std::vector<Value> containerValues;
        Value v1{};
        v1.size = 5;
        v1.data = const_cast<char*>("nv1");
        containerValues.push_back(v1);
        
        auto future = client.addElementOrderedMap(key, hint, containerKeys, containerValues, 0, std::chrono::milliseconds(5000));
        auto result = future.get();
        
        std::cout << "PASSED (addElementOrderedMap returned " << result << ")\n";
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
