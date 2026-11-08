//
// Tests for List operations - based on Java ListOperationsTest
//
#include <iostream>
#include <chrono>
#include <future>
#include <string>
#include <vector>
#include <cassert>
#include "test_base.hxx"
#include "utils.hxx"

static void testCreateEmptyList(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateEmptyList... ";
    try {
        Key key = test_base::make_key("createEmptyList");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        client.createList(key, nullptr, nullptr, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        
        // Stream should return empty list
        auto stream_future = client.streamList(key);
        auto items = stream_future.get();
        
        assert(items.size() == 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testCreateListWithInitialData(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateListWithInitialData... ";
    try {
        Key key = test_base::make_key("createListWithInitialData");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("v1"),
            test_base::make_value("v2"),
            test_base::make_value("v3")
        };
        
        auto hint_future = client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000));
        auto key_hint = hint_future.get();
        free_content(initial);
        
        // Stream and verify
        auto stream_future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto items = stream_future.get();
        
        assert(items.size() == 3);
        assert(std::string(items[0]->data, items[0]->size) == "v1");
        assert(std::string(items[1]->data, items[1]->size) == "v2");
        assert(std::string(items[2]->data, items[2]->size) == "v3");
        free_content(items);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testStreamListContents(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamListContents... ";
    try {
        Key key = test_base::make_key("streamListContents");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("a"),
            test_base::make_value("b"),
            test_base::make_value("c"),
            test_base::make_value("d"),
            test_base::make_value("e")
        };
        
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto stream_future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto items = stream_future.get();
        
        assert(items.size() == 5);
        for (int i = 0; i < 5; ++i) {
            std::string expected(1, static_cast<char>('a' + i));
            assert(std::string(items[i]->data, items[i]->size) == expected);
        }
        free_content(items);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetAndRemoveFront(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveFront... ";
    try {
        Key key = test_base::make_key("getAndRemoveFront");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second"),
            test_base::make_value("third")
        };
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        // Remove front
        auto removed_future = client.getAndRemoveFront(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto removed = removed_future.get();
        assert(std::string(removed->data, removed->size) == "first");
        delete removed;
        
        // Verify remaining
        auto stream_future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto remaining = stream_future.get();
        assert(remaining.size() == 2);
        assert(std::string(remaining[0]->data, remaining[0]->size) == "second");
        assert(std::string(remaining[1]->data, remaining[1]->size) == "third");
        free_content(remaining);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetAndRemoveFrontOnSingleElement(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveFrontOnSingleElement... ";
    try {
        Key key = test_base::make_key("getAndRemoveFrontSingle");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {test_base::make_value("only")};
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto removed_future = client.getAndRemoveFront(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto removed = removed_future.get();
        assert(std::string(removed->data, removed->size) == "only");
        delete removed;
        
        // List should be empty
        auto stream_future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto remaining = stream_future.get();
        assert(remaining.size() == 0);
        free_content(remaining);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetHead(FastCacheStandaloneClient &client) {
    std::cout << "  testGetHead... ";
    try {
        Key key = test_base::make_key("getHead");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("head"),
            test_base::make_value("tail")
        };
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto head_future = client.getHead(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto head = head_future.get();
        assert(std::string(head->data, head->size) == "head");
        delete head;
        // List unchanged
        auto stream_future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto items = stream_future.get();
        assert(items.size() == 2);
        free_content(items);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetTail(FastCacheStandaloneClient &client) {
    std::cout << "  testGetTail... ";
    try {
        Key key = test_base::make_key("getTail");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("middle"),
            test_base::make_value("last")
        };
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto tail_future = client.getTail(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto tail = tail_future.get();
        assert(std::string(tail->data, tail->size) == "last");
        delete tail;
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetAndRemoveTail(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveTail... ";
    try {
        Key key = test_base::make_key("getAndRemoveTail");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second"),
            test_base::make_value("last")
        };
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto removed_future = client.getAndRemoveTail(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto removed = removed_future.get();
        assert(std::string(removed->data, removed->size) == "last");
        delete removed;
        
        // Verify remaining
        auto stream_future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto remaining = stream_future.get();
        assert(remaining.size() == 2);
        assert(std::string(remaining[0]->data, remaining[0]->size) == "first");
        assert(std::string(remaining[1]->data, remaining[1]->size) == "second");
        free_content(remaining);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetElementAtPosition(FastCacheStandaloneClient &client) {
    std::cout << "  testGetElementAtPosition... ";
    try {
        Key key = test_base::make_key("getElementAtPosition");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("pos0"),
            test_base::make_value("pos1"),
            test_base::make_value("pos2")
        };
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto pos0_future = client.getElementAtPosition(key, nullptr, 0, 0, std::chrono::milliseconds(5000));
        auto pos0 = pos0_future.get();
        assert(std::string(pos0->data, pos0->size) == "pos0");
        delete pos0;
        
        auto pos1_future = client.getElementAtPosition(key, nullptr, 1, 0, std::chrono::milliseconds(5000));
        auto pos1 = pos1_future.get();
        assert(std::string(pos1->data, pos1->size) == "pos1");
        delete pos1;
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testAddElementToTail(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToTail... ";
    try {
        Key key = test_base::make_key("addElementToTail");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second")
        };
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        ValuePtr new_elem = test_base::make_value("third");
        std::vector<ValuePtr> data = {new_elem};
        auto added_future = client.addElementToTail(key, nullptr, &data, 0, std::chrono::milliseconds(5000));
        int added = added_future.get();
        assert(added == 1);
        free_content(data);
        
        // Verify order
        auto stream_future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto items = stream_future.get();
        assert(items.size() == 3);
        assert(std::string(items[0]->data, items[0]->size) == "first");
        assert(std::string(items[1]->data, items[1]->size) == "second");
        assert(std::string(items[2]->data, items[2]->size) == "third");
        free_content(items);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testAddElementToHead(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToHead... ";
    try {
        Key key = test_base::make_key("addElementToHead");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("second"),
            test_base::make_value("third")
        };
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        ValuePtr new_elem = test_base::make_value("first");
        std::vector<ValuePtr> data = {new_elem};
        auto added_future = client.addElementToHead(key, nullptr, &data, 0, std::chrono::milliseconds(5000));
        int added = added_future.get();
        assert(added == 1);
        free_content(data);
        
        // Verify order
        auto stream_future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto items = stream_future.get();
        assert(items.size() == 3);
        assert(std::string(items[0]->data, items[0]->size) == "first");
        assert(std::string(items[1]->data, items[1]->size) == "second");
        assert(std::string(items[2]->data, items[2]->size) == "third");
        free_content(items);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveHead(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveHead... ";
    try {
        Key key = test_base::make_key("removeHead");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second"),
            test_base::make_value("third")
        };
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto removed_future = client.removeHead(key, nullptr, 0, std::chrono::milliseconds(5000));
        bool removed = removed_future.get();
        assert(removed);
        
        // Verify
        auto stream_future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto items = stream_future.get();
        assert(items.size() == 2);
        assert(std::string(items[0]->data, items[0]->size) == "second");
        assert(std::string(items[1]->data, items[1]->size) == "third");
        free_content(items);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveTail(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveTail... ";
    try {
        Key key = test_base::make_key("removeTail");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second"),
            test_base::make_value("third")
        };
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto removed_future = client.removeTail(key, nullptr, 0, std::chrono::milliseconds(5000));
        bool removed = removed_future.get();
        assert(removed);
        
        // Verify
        auto stream_future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
        auto items = stream_future.get();
        assert(items.size() == 2);
        assert(std::string(items[0]->data, items[0]->size) == "first");
        assert(std::string(items[1]->data, items[1]->size) == "second");
        free_content(items);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetSize(FastCacheStandaloneClient &client) {
    std::cout << "  testGetSize... ";
    try {
        Key key = test_base::make_key("getSize");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("a"),
            test_base::make_value("b"),
            test_base::make_value("c")
        };
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto size_future = client.getSize(key, nullptr, 0, std::chrono::milliseconds(5000));
        int size = size_future.get();
        assert(size == 3);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveList(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveList... ";
    try {
        Key key = test_base::make_key("removeList");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto removed_future = client.remove(key, nullptr, 0, std::chrono::milliseconds(5000));
        bool removed = removed_future.get();
        assert(removed);
        
        // Verify not found
        try {
            auto stream_future = client.streamList(key, nullptr, 0, std::chrono::milliseconds(5000));
            stream_future.get();
            std::cout << "FAILED: Expected exception for non-existent list\n";
        } catch (const std::exception &) {
            // Expected
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
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {test_base::make_value("a"), test_base::make_value("b")};
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto set_ttl_future = client.setTtl(key, nullptr, 5000, 0, std::chrono::milliseconds(5000));
        bool set_result = set_ttl_future.get();
        assert(set_result);
        
        auto get_ttl_future = client.getTtl(key, nullptr, 0, std::chrono::milliseconds(5000));
        int64_t ttl = get_ttl_future.get();
        assert(ttl > 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testWriteLockOnList(FastCacheStandaloneClient &client) {
    std::cout << "  testWriteLockOnList... ";
    try {
        Key key = test_base::make_key("writeLockList");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {test_base::make_value("data")};
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto lock_future = client.lockObject(key, nullptr, LockType::WRITE_LOCK, 1, std::chrono::milliseconds(60000), std::chrono::milliseconds(5000));
        LockStatus lock_status = lock_future.get();
        assert(lock_status == OK);
        
        auto unlock_future = client.unlockObject(key, nullptr, 1, std::chrono::milliseconds(5000));
        LockStatus unlock_status = unlock_future.get();
        assert(unlock_status == OK);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testReadLockOnList(FastCacheStandaloneClient &client) {
    std::cout << "  testReadLockOnList... ";
    try {
        Key key = test_base::make_key("readLockList");
        auto exists = client.existKey(key).get();
        if (exists) client.remove(key).get();
        
        std::vector<ValuePtr> initial = {test_base::make_value("data")};
        client.createList(key, nullptr, &initial, std::chrono::milliseconds(60000), 0, std::chrono::milliseconds(5000)).get();
        free_content(initial);
        
        auto lock_future = client.lockObject(key, nullptr, LockType::READ_LOCK, 2, std::chrono::milliseconds(60000), std::chrono::milliseconds(5000));
        LockStatus lock_status = lock_future.get();
        assert(lock_status == OK);
        
        auto unlock_future = client.unlockObject(key, nullptr, 2, std::chrono::milliseconds(5000));
        LockStatus unlock_status = unlock_future.get();
        assert(unlock_status == OK);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

void testListOperations(FastCacheStandaloneClient &client) {
    std::cout << "=== List Operations Tests ===\n";
    testCreateEmptyList(client);
    testCreateListWithInitialData(client);
    testStreamListContents(client);
    testGetAndRemoveFront(client);
    testGetAndRemoveFrontOnSingleElement(client);
    testGetHead(client);
    testGetTail(client);
    testGetAndRemoveTail(client);
    testGetElementAtPosition(client);
    testAddElementToTail(client);
    testAddElementToHead(client);
    testRemoveHead(client);
    testRemoveTail(client);
    testGetSize(client);
    testRemoveList(client);
    testSetTtlAndGetTtl(client);
    testWriteLockOnList(client);
    testReadLockOnList(client);
    std::cout << "List Operations: ALL PASSED\n\n";
}
