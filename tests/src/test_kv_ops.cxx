//
// Tests for Key-Value Operations
// Based on Java RawValuesTest
//
#include <iostream>
#include <chrono>
#include <future>
#include <vector>
#include <cassert>
#include <string>
#include "standalone_client.hxx"
#include "test_base.hxx"
#include "utils.hxx"

static void testCreateKeyValue(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateKeyValue... ";
    try {
        Key key = test_base::make_key("kv_test1");
        // Cleanup if exists
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        Value value(static_cast<int64_t>(42));
        
        auto hint = client.createKeyValue(key, nullptr, value).get();
        assert(hint.strong_hash != 0);
        
        auto result = client.getValue(key, nullptr).get();
        assert(result->type == INT);
        assert(result->int_value == 42);
        delete result;
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetValue(FastCacheStandaloneClient &client) {
    std::cout << "  testGetValue... ";
    try {
        Key key = test_base::make_key("kv_test_get");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        Value value(static_cast<int64_t>(100));
        client.createKeyValue(key, nullptr, value).get();
        
        auto result = client.getValue(key, nullptr).get();
        assert(result->type == INT);
        assert(result->int_value == 100);
        delete result;
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testUpdateKeyValue(FastCacheStandaloneClient &client) {
    std::cout << "  testUpdateKeyValue... ";
    try {
        Key key = test_base::make_key("kv_test_update");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        Value value(static_cast<int64_t>(200));
        client.createKeyValue(key, nullptr, value).get();
        
        Value newValue(static_cast<int64_t>(300));
        auto updated = client.updateKeyValue(key, nullptr, newValue).get();
        assert(updated->type == INT);
        assert(updated->int_value == 200);
        delete updated;
        
        auto result = client.getValue(key, nullptr).get();
        assert(result->type == INT);
        assert(result->int_value == 300);
        delete result;
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testExistKeyExisting(FastCacheStandaloneClient &client) {
    std::cout << "  testExistKeyExisting... ";
    try {
        Key key = test_base::make_key("kv_test_exist");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        Value value(static_cast<int64_t>(300));
        client.createKeyValue(key, nullptr, value).get();
        
        exists = client.existKey(key).get();
        assert(exists);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testExistKeyNonExistent(FastCacheStandaloneClient &client) {
    std::cout << "  testExistKeyNonExistent... ";
    try {
        Key key = test_base::make_key("kv_test_nonexist");
        
        auto exists = client.existKey(key).get();
        assert(!exists);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveKeyValue(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveKeyValue... ";
    try {
        Key key = test_base::make_key("kv_test_remove");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        Value value(static_cast<int64_t>(300));
        client.createKeyValue(key, nullptr, value).get();
        
        auto removed = client.remove(key).get();
        assert(removed);
        
        exists = client.existKey(key).get();
        assert(!exists);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetAndDeleteValue(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndDeleteValue... ";
    try {
        Key key = test_base::make_key("kv_test_del");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        Value value(static_cast<int64_t>(400));
        client.createKeyValue(key, nullptr, value).get();
        
        auto deleted = client.getAndDeleteValue(key).get();
        assert(deleted->type == INT);
        assert(deleted->int_value == 400);
        delete deleted;
        
        exists = client.existKey(key).get();
        assert(!exists);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testLockKeyValue(FastCacheStandaloneClient &client) {
    std::cout << "  testLockKeyValue... ";
    try {
        Key key = test_base::make_key("kv_test_lock");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        Value value(static_cast<int64_t>(500));
        client.createKeyValue(key, nullptr, value).get();
        
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

static void testSetTtlKeyValue(FastCacheStandaloneClient &client) {
    std::cout << "  testSetTtlKeyValue... ";
    try {
        Key key = test_base::make_key("kv_test_ttl");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        Value value(static_cast<int64_t>(600));
        client.createKeyValue(key, nullptr, value).get();
        
        auto setTtl = client.setTtl(key, nullptr, 5000).get();
        assert(setTtl);
        
        auto ttl = client.getTtl(key).get();
        assert(ttl > 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

void testKeyValueOperations(FastCacheStandaloneClient &client) {
    std::cout << "=== Key-Value Operations Tests ===\n";
    testCreateKeyValue(client);
    testGetValue(client);
    testUpdateKeyValue(client);
    testExistKeyExisting(client);
    testExistKeyNonExistent(client);
    testRemoveKeyValue(client);
    testGetAndDeleteValue(client);
    testLockKeyValue(client);
    testSetTtlKeyValue(client);
    std::cout << "Key-Value Operations: PASSED\n\n";
}
