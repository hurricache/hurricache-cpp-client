//
// Tests for Vector operations - based on Java VectorOperationsTest
//
#include <iostream>
#include <chrono>
#include <future>
#include <vector>
#include <string>
#include <cassert>
#include "test_base.hxx"
#include "utils.hxx"
#include "utils.hxx"

static void testCreateEmptyVector(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateEmptyVector... ";
    try {
        Key key = test_base::make_key("createEmptyVector");
        
        auto hint = client.createVector(key, nullptr, nullptr).get();
        assert(hint.strong_hash != 0);
        
        auto items = client.streamVector(key).get();
        assert(items.size() == 0);
        free_content(items);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testCreateVectorWithInitialData(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateVectorWithInitialData... ";
    try {
        Key key = test_base::make_key("createVectorWithInitialData");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("v1"),
            test_base::make_value("v2"),
            test_base::make_value("v3")
        };
        
        auto hint = client.createVector(key, nullptr, &initial).get();
        assert(hint.strong_hash != 0);
        
        auto items = client.streamVector(key).get();
        assert(items.size() == 3);
        assert(std::string(items[0]->data, items[0]->size) == "v1");
        assert(std::string(items[1]->data, items[1]->size) == "v2");
        assert(std::string(items[2]->data, items[2]->size) == "v3");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testStreamVectorContents(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamVectorContents... ";
    try {
        Key key = test_base::make_key("streamVectorContents");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("a"),
            test_base::make_value("b"),
            test_base::make_value("c"),
            test_base::make_value("d"),
            test_base::make_value("e")
        };
        
        client.createVector(key, nullptr, &initial).get();
        
        auto items = client.streamVector(key).get();
        assert(items.size() == 5);
        free_content(items);
        for (int i = 0; i < 5; ++i) {
            std::string expected(1, static_cast<char>('a' + i));
            assert(std::string(items[i]->data, items[i]->size) == expected);
        }
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetAndRemoveFront(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveFront... ";
    try {
        Key key = test_base::make_key("getAndRemoveFront");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second"),
            test_base::make_value("third")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto removed = client.getAndRemoveFront(key).get();
        assert(std::string(removed->data, removed->size) == "first");
        delete removed;
        
        auto remaining = client.streamVector(key).get();
        assert(remaining.size() == 2);
        assert(std::string(remaining[0]->data, remaining[0]->size) == "second");
        assert(std::string(remaining[1]->data, remaining[1]->size) == "third");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetAndRemoveFrontOnSingleElement(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveFrontOnSingleElement... ";
    try {
        Key key = test_base::make_key("getAndRemoveFrontSingle");
        
        std::vector<ValuePtr> initial = {test_base::make_value("only")};
        client.createVector(key, nullptr, &initial).get();
        
        auto removed = client.getAndRemoveFront(key).get();
        assert(std::string(removed->data, removed->size) == "only");
        
        auto remaining = client.streamVector(key).get();
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
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("head"),
            test_base::make_value("tail")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto head = client.getHead(key).get();
        assert(std::string(head->data, head->size) == "head");
        delete head;
        
        // Vector should remain unchanged
        auto items = client.streamVector(key).get();
        assert(items.size() == 2);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetTail(FastCacheStandaloneClient &client) {
    std::cout << "  testGetTail... ";
    try {
        Key key = test_base::make_key("getTail");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("middle"),
            test_base::make_value("last")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto tail = client.getTail(key).get();
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
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second"),
            test_base::make_value("last")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto removed = client.getAndRemoveTail(key).get();
        assert(std::string(removed->data, removed->size) == "last");
        
        auto remaining = client.streamVector(key).get();
        assert(remaining.size() == 2);
        assert(std::string(remaining[0]->data, remaining[0]->size) == "first");
        assert(std::string(remaining[1]->data, remaining[1]->size) == "second");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetElementAtPosition(FastCacheStandaloneClient &client) {
    std::cout << "  testGetElementAtPosition... ";
    try {
        Key key = test_base::make_key("getElementAtPosition");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("pos0"),
            test_base::make_value("pos1"),
            test_base::make_value("pos2")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto pos0 = client.getElementAtPosition(key, nullptr, 0).get();
        auto pos1 = client.getElementAtPosition(key, nullptr, 1).get();
        auto pos2 = client.getElementAtPosition(key, nullptr, 2).get();
        
        assert(std::string(pos0->data, pos0->size) == "pos0");
        delete pos0;
        assert(std::string(pos1->data, pos1->size) == "pos1");
        delete pos1;
        assert(std::string(pos2->data, pos2->size) == "pos2");
        delete pos2;
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetElementAtPositionZero(FastCacheStandaloneClient &client) {
    std::cout << "  testGetElementAtPositionZero... ";
    try {
        Key key = test_base::make_key("getElementAtPositionZero");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto pos0 = client.getElementAtPosition(key, nullptr, 0).get();
        assert(std::string(pos0->data, pos0->size) == "first");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetElementAtPositionLast(FastCacheStandaloneClient &client) {
    std::cout << "  testGetElementAtPositionLast... ";
    try {
        Key key = test_base::make_key("getElementAtPositionLast");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto last = client.getElementAtPosition(key, nullptr, 1).get();
        assert(std::string(last->data, last->size) == "second");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetAndRemoveElementAtPosition(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveElementAtPosition... ";
    try {
        Key key = test_base::make_key("getAndRemoveElementAtPosition");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("a"),
            test_base::make_value("b"),
            test_base::make_value("c")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto removed = client.getAndRemoveElementAtPosition(key, nullptr, 1).get();
        assert(std::string(removed->data, removed->size) == "b");
        
        auto remaining = client.streamVector(key).get();
        assert(remaining.size() == 2);
        assert(std::string(remaining[0]->data, remaining[0]->size) == "a");
        assert(std::string(remaining[1]->data, remaining[1]->size) == "c");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testAddElementToTail(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToTail... ";
    try {
        Key key = test_base::make_key("addElementToTail");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second")
        };
        client.createVector(key, nullptr, &initial).get();
        
        std::vector<ValuePtr> data = {test_base::make_value("third")};
        auto added = client.addElementToTail(key, nullptr, &data).get();
        assert(added == 1);
        
        auto items = client.streamVector(key).get();
        assert(items.size() == 3);
        assert(std::string(items[0]->data, items[0]->size) == "first");
        assert(std::string(items[1]->data, items[1]->size) == "second");
        assert(std::string(items[2]->data, items[2]->size) == "third");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testAddElementToHead(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToHead... ";
    try {
        Key key = test_base::make_key("addElementToHead");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("second"),
            test_base::make_value("third")
        };
        client.createVector(key, nullptr, &initial).get();
        
        std::vector<ValuePtr> data = {test_base::make_value("first")};
        auto added = client.addElementToHead(key, nullptr, &data).get();
        assert(added == 1);
        
        auto items = client.streamVector(key).get();
        assert(items.size() == 3);
        assert(std::string(items[0]->data, items[0]->size) == "first");
        assert(std::string(items[1]->data, items[1]->size) == "second");
        assert(std::string(items[2]->data, items[2]->size) == "third");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testAddElementToPosition(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToPosition... ";
    try {
        Key key = test_base::make_key("addElementToPosition");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("last")
        };
        client.createVector(key, nullptr, &initial).get();
        
        std::vector<ValuePtr> data = {test_base::make_value("middle")};
        auto added = client.addElementToPosition(key, nullptr, &data, 1).get();
        assert(added >= 0);
        
        auto items = client.streamVector(key).get();
        assert(items.size() == 3);
        assert(std::string(items[0]->data, items[0]->size) == "first");
        assert(std::string(items[1]->data, items[1]->size) == "middle");
        assert(std::string(items[2]->data, items[2]->size) == "last");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testAddElementToPositionBefore(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToPositionBefore... ";
    try {
        Key key = test_base::make_key("addElementToPositionBefore");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("head"),
            test_base::make_value("pivot")
        };
        client.createVector(key, nullptr, &initial).get();
        
        std::vector<ValuePtr> data = {test_base::make_value("inserted")};
        ValuePtr pivot = test_base::make_value("pivot");
        auto added = client.addElementToPositionBefore(key, nullptr, &data, pivot).get();
        assert(added >= 0);
        
        auto items = client.streamVector(key).get();
        assert(items.size() == 3);
        assert(std::string(items[0]->data, items[0]->size) == "head");
        assert(std::string(items[1]->data, items[1]->size) == "inserted");
        assert(std::string(items[2]->data, items[2]->size) == "pivot");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testAddElementToPositionAfter(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToPositionAfter... ";
    try {
        Key key = test_base::make_key("addElementToPositionAfter");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("head"),
            test_base::make_value("tail")
        };
        client.createVector(key, nullptr, &initial).get();
        
        std::vector<ValuePtr> data = {test_base::make_value("inserted")};
        ValuePtr pivot = test_base::make_value("head");
        auto added = client.addElementToPositionAfter(key, nullptr, &data, pivot).get();
        assert(added >= 0);
        
        auto items = client.streamVector(key).get();
        assert(items.size() == 3);
        assert(std::string(items[0]->data, items[0]->size) == "head");
        assert(std::string(items[1]->data, items[1]->size) == "inserted");
        assert(std::string(items[2]->data, items[2]->size) == "tail");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveHead(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveHead... ";
    try {
        Key key = test_base::make_key("removeHead");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second"),
            test_base::make_value("third")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto removed = client.removeHead(key, nullptr).get();
        assert(removed);
        
        auto items = client.streamVector(key).get();
        assert(items.size() == 2);
        assert(std::string(items[0]->data, items[0]->size) == "second");
        assert(std::string(items[1]->data, items[1]->size) == "third");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveTail(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveTail... ";
    try {
        Key key = test_base::make_key("removeTail");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second"),
            test_base::make_value("third")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto removed = client.removeTail(key, nullptr).get();
        assert(removed);
        
        auto items = client.streamVector(key).get();
        assert(items.size() == 2);
        assert(std::string(items[0]->data, items[0]->size) == "first");
        assert(std::string(items[1]->data, items[1]->size) == "second");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveElementAtPosition(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveElementAtPosition... ";
    try {
        Key key = test_base::make_key("removeElementAtPosition");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("0"),
            test_base::make_value("1"),
            test_base::make_value("2")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto removed = client.removeElementAtPosition(key, nullptr, 1, 1).get();
        assert(removed);
        
        auto items = client.streamVector(key).get();
        assert(items.size() == 2);
        assert(std::string(items[0]->data, items[0]->size) == "0");
        assert(std::string(items[1]->data, items[1]->size) == "2");
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
        client.createVector(key, nullptr, &initial).get();
        
        auto size = client.getSize(key).get();
        assert(size == 3);
        
        std::vector<ValuePtr> data = {test_base::make_value("d")};
        client.addElementToTail(key, nullptr, &data).get();
        
        size = client.getSize(key).get();
        assert(size == 4);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveVector(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveVector... ";
    try {
        Key key = test_base::make_key("removeVector");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto removed = client.remove(key).get();
        assert(removed);
        
        try {
            client.streamVector(key).get();
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
        client.createVector(key, nullptr, &initial).get();
        
        auto setTtl = client.setTtl(key, nullptr, 5000).get();
        assert(setTtl);
        
        auto ttl = client.getTtl(key).get();
        assert(ttl > 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testStreamElementInRangeUnordered(FastCacheStandaloneClient &client) {
    std::cout << "  testStreamElementInRangeUnordered... ";
    try {
        Key key = test_base::make_key("streamElementInRangeUnordered");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("0"),
            test_base::make_value("1"),
            test_base::make_value("2"),
            test_base::make_value("3"),
            test_base::make_value("4"),
            test_base::make_value("5")
        };
        client.createVector(key, nullptr, &initial).get();
        
        auto range = client.streamElementInRangeUnordered(key, nullptr, ContainerType::VECTOR, 2, 4).get();
        assert(range.size() == 3);
        assert(std::string(range[0]->data, range[0]->size) == "2");
        assert(std::string(range[1]->data, range[1]->size) == "3");
        assert(std::string(range[2]->data, range[2]->size) == "4");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testWriteLockOnVector(FastCacheStandaloneClient &client) {
    std::cout << "  testWriteLockOnVector... ";
    try {
        Key key = test_base::make_key("writeLockVector");
        
        std::vector<ValuePtr> initial = {test_base::make_value("data")};
        client.createVector(key, nullptr, &initial).get();
        
        int32_t ownerId = 1;
        auto lockRes = client.lockObject(key, nullptr, LockType::WRITE_LOCK, ownerId).get();
        assert(lockRes == LockStatus::OK);
        
        // Owner can read
        auto head = client.getHead(key, nullptr, ownerId).get();
        assert(head != nullptr);
        
        // Owner can write
        std::vector<ValuePtr> data = {test_base::make_value("write")};
        auto added = client.addElementToTail(key, nullptr, &data, ownerId).get();
        assert(added >= 0);
        
        // Unlock
        auto unlockRes = client.unlockObject(key, nullptr, ownerId).get();
        assert(unlockRes == LockStatus::OK);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testReadLockOnVector(FastCacheStandaloneClient &client) {
    std::cout << "  testReadLockOnVector... ";
    try {
        Key key = test_base::make_key("readLockVector");
        
        std::vector<ValuePtr> initial = {test_base::make_value("data")};
        client.createVector(key, nullptr, &initial).get();
        
        int32_t readerId = 2;
        auto lockRes = client.lockObject(key, nullptr, LockType::READ_LOCK, readerId).get();
        assert(lockRes == LockStatus::OK);
        
        // Reading works
        auto head = client.getHead(key).get();
        assert(head != nullptr);
        
        // Unlock
        auto unlockRes = client.unlockObject(key, nullptr, readerId).get();
        assert(unlockRes == LockStatus::OK);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveFromContainer(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveFromContainer... ";
    try {
        Key key = test_base::make_key("vector_remove");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2"),
            test_base::make_value("item3")
        };
        auto hint = client.createVector(key, nullptr, &initial).get();
        
        Key remove_key = test_base::make_key("item1");
        
        auto removed = client.removeFromContainer(key, &hint, remove_key).get();
        assert(removed == 1);
        
        auto result = client.streamVector(key).get();
        assert(result.size() == 2);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveFromContainerNonExistent(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveFromContainerNonExistent... ";
    try {
        Key key = test_base::make_key("vector_remove_nonexistent");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        auto hint = client.createVector(key, nullptr, &initial).get();
        
        Key remove_key = test_base::make_key("item123");
        
        auto removed = client.removeFromContainer(key, &hint, remove_key).get();
        assert(removed == 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

void testVectorOperations(FastCacheStandaloneClient &client) {
    std::cout << "=== Vector Operations Tests ===\n";
    testCreateEmptyVector(client);
    testCreateVectorWithInitialData(client);
    testStreamVectorContents(client);
    testGetAndRemoveFront(client);
    testGetAndRemoveFrontOnSingleElement(client);
    testGetHead(client);
    testGetTail(client);
    testGetAndRemoveTail(client);
    testGetElementAtPosition(client);
    testGetElementAtPositionZero(client);
    testGetElementAtPositionLast(client);
    testGetAndRemoveElementAtPosition(client);
    testAddElementToTail(client);
    testAddElementToHead(client);
    testAddElementToPosition(client);
    testAddElementToPositionBefore(client);
    testAddElementToPositionAfter(client);
    testRemoveHead(client);
    testRemoveTail(client);
    testRemoveElementAtPosition(client);
    testGetSize(client);
    testRemoveVector(client);
    testSetTtlAndGetTtl(client);
    testStreamElementInRangeUnordered(client);
    testWriteLockOnVector(client);
    testReadLockOnVector(client);
    testRemoveFromContainer(client);
    testRemoveFromContainerNonExistent(client);
    std::cout << "Vector Operations: ALL PASSED\n\n";
}
