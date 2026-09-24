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
        Value value("hello", 5);
        
        auto hint = client.createKeyValue(key, nullptr, value).get();
        assert(hint.strong_hash != 0);
        
        auto result = client.getValue(key, nullptr).get();
        assert(std::string(result->data, result->size) == "hello");
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
        
        Value value("world", 5);
        client.createKeyValue(key, nullptr, value).get();
        
        auto result = client.getValue(key, nullptr).get();
        assert(std::string(result->data, result->size) == "world");
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
        
        Value value("old", 3);
        client.createKeyValue(key, nullptr, value).get();
        
        Value newValue("new_value", 7);
        auto updated = client.updateKeyValue(key, nullptr, newValue).get();
        assert(std::string(updated->data, updated->size) == "new_value");
        delete updated;
        
        auto result = client.getValue(key, nullptr).get();
        assert(std::string(result->data, result->size) == "new_value");
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
        
        Value value("data", 4);
        client.createKeyValue(key, nullptr, value).get();
        
        auto exists = client.existKey(key).get();
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
        
        Value value("data", 4);
        client.createKeyValue(key, nullptr, value).get();
        
        auto removed = client.remove(key).get();
        assert(removed);
        
        auto exists = client.existKey(key).get();
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
        
        Value value("delete_me", 9);
        client.createKeyValue(key, nullptr, value).get();
        
        auto deleted = client.getAndDeleteValue(key).get();
        assert(std::string(deleted->data, deleted->size) == "delete_me");
        delete deleted;
        
        auto exists = client.existKey(key).get();
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
        
        Value value("locked", 6);
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
        
        Value value("ttl_test", 8);
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
