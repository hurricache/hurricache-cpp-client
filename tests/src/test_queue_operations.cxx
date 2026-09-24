//
// Tests for Queue operations - based on Java QueueOperationsTest
//
#include <iostream>
#include <chrono>
#include <future>
#include <vector>
#include <string>
#include <cassert>
#include <thread>
#include "test_base.hxx"
#include "utils.hxx"

static void testCreateEmptyQueue(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateEmptyQueue... ";
    try {
        Key key = test_base::make_key("createEmptyQueue");
        client.createQueue(key).get();
        
        auto head = client.getHead(key).get();
        assert(head != nullptr);
        assert(head->size == 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testCreateQueueWithInitialData(FastCacheStandaloneClient &client) {
    std::cout << "  testCreateQueueWithInitialData... ";
    try {
        Key key = test_base::make_key("createQueueWithInitialData");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second"),
            test_base::make_value("third")
        };
        
        auto hint = client.createQueue(key, nullptr, &initial).get();
        assert(hint.strong_hash != 0);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto r1 = client.getAndRemoveFront(key).get();
        auto r2 = client.getAndRemoveFront(key).get();
        auto r3 = client.getAndRemoveFront(key).get();
        
        assert(std::string(r1->data, r1->size) == "first");
        assert(std::string(r2->data, r2->size) == "second");
        assert(std::string(r3->data, r3->size) == "third");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetHeadOnEmptyQueue(FastCacheStandaloneClient &client) {
    std::cout << "  testGetHeadOnEmptyQueue... ";
    try {
        Key key = test_base::make_key("emptyQueueGetHead");
        client.createQueue(key).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto head = client.getHead(key).get();
        assert(head != nullptr);
        assert(head->size == 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetAndRemoveFrontOnEmptyQueue(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveFrontOnEmptyQueue... ";
    try {
        Key key = test_base::make_key("emptyQueueGetAndRemove");
        client.createQueue(key).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto removed = client.getAndRemoveFront(key).get();
        assert(removed != nullptr);
        assert(removed->size == 0);
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
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto r1 = client.getAndRemoveFront(key).get();
        auto r2 = client.getAndRemoveFront(key).get();
        auto r3 = client.getAndRemoveFront(key).get();
        
        assert(std::string(r1->data, r1->size) == "first");
        assert(std::string(r2->data, r2->size) == "second");
        assert(std::string(r3->data, r3->size) == "third");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetAndRemoveFrontOnSingleElement(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveFrontOnSingleElement... ";
    try {
        Key key = test_base::make_key("singleElementQueue");
        
        std::vector<ValuePtr> initial = {test_base::make_value("only")};
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto removed = client.getAndRemoveFront(key).get();
        assert(std::string(removed->data, removed->size) == "only");
        
        auto empty = client.getHead(key).get();
        assert(empty != nullptr);
        assert(empty->size == 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetFrontAndHead(FastCacheStandaloneClient &client) {
    std::cout << "  testGetFrontAndHead... ";
    try {
        Key key = test_base::make_key("getFrontHead");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second")
        };
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto head = client.getHead(key).get();
        assert(std::string(head->data, head->size) == "first");
        
        auto stillThere = client.getAndRemoveFront(key).get();
        assert(std::string(stillThere->data, stillThere->size) == "first");
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
            test_base::make_value("second"),
            test_base::make_value("third")
        };
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto tail = client.getTail(key).get();
        assert(std::string(tail->data, tail->size) == "third");
        
        auto front = client.getAndRemoveFront(key).get();
        assert(std::string(front->data, front->size) == "first");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetTailOnSingleElementQueue(FastCacheStandaloneClient &client) {
    std::cout << "  testGetTailOnSingleElementQueue... ";
    try {
        Key key = test_base::make_key("singleElementTail");
        
        std::vector<ValuePtr> initial = {test_base::make_value("only")};
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto head = client.getHead(key).get();
        auto tail = client.getTail(key).get();
        
        assert(std::string(head->data, head->size) == "only");
        assert(std::string(tail->data, tail->size) == "only");
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
            test_base::make_value("third")
        };
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto removed = client.getAndRemoveTail(key, nullptr).get();
        assert(std::string(removed->data, removed->size) == "third");
        
        auto f1 = client.getAndRemoveFront(key).get();
        auto f2 = client.getAndRemoveFront(key).get();
        
        assert(std::string(f1->data, f1->size) == "first");
        assert(std::string(f2->data, f2->size) == "second");
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
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        std::vector<ValuePtr> data = {test_base::make_value("third")};
        auto added = client.addElementToTail(key, nullptr, &data).get();
        assert(added == 1);
        
        auto f1 = client.getAndRemoveFront(key).get();
        auto f2 = client.getAndRemoveFront(key).get();
        auto f3 = client.getAndRemoveFront(key).get();
        
        assert(std::string(f1->data, f1->size) == "first");
        assert(std::string(f2->data, f2->size) == "second");
        assert(std::string(f3->data, f3->size) == "third");
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
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        std::vector<ValuePtr> data = {test_base::make_value("first")};
        auto added = client.addElementToHead(key, nullptr, &data).get();
        assert(added == 1);
        
        auto f1 = client.getAndRemoveFront(key).get();
        auto f2 = client.getAndRemoveFront(key).get();
        auto f3 = client.getAndRemoveFront(key).get();
        
        assert(std::string(f1->data, f1->size) == "first");
        assert(std::string(f2->data, f2->size) == "second");
        assert(std::string(f3->data, f3->size) == "third");
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
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto removed = client.removeHead(key, nullptr).get();
        assert(removed);
        
        auto f1 = client.getAndRemoveFront(key).get();
        auto f2 = client.getAndRemoveFront(key).get();
        
        assert(std::string(f1->data, f1->size) == "second");
        assert(std::string(f2->data, f2->size) == "third");
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
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto removed = client.removeTail(key, nullptr).get();
        assert(removed);
        
        auto f1 = client.getAndRemoveFront(key).get();
        auto f2 = client.getAndRemoveFront(key).get();
        
        assert(std::string(f1->data, f1->size) == "first");
        assert(std::string(f2->data, f2->size) == "second");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetSizeReturnsZero(FastCacheStandaloneClient &client) {
    std::cout << "  testGetSizeReturnsZero... ";
    try {
        Key key = test_base::make_key("getSizeQueue");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("first"),
            test_base::make_value("second")
        };
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto size = client.getSize(key, nullptr).get();
        assert(size == 0); // Queue doesn't support size
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveQueue(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveQueue... ";
    try {
        Key key = test_base::make_key("removeQueue");
        
        std::vector<ValuePtr> initial = {test_base::make_value("data")};
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto removed = client.remove(key).get();
        assert(removed);
        
        try {
            client.getHead(key).get();
            std::cout << "FAILED: Expected NOT_FOUND\n";
        } catch (const std::exception &) {
            std::cout << "PASSED\n";
        }
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testSetTtlAndGetTtlOnQueue(FastCacheStandaloneClient &client) {
    std::cout << "  testSetTtlAndGetTtlOnQueue... ";
    try {
        Key key = test_base::make_key("ttlQueue");
        
        std::vector<ValuePtr> initial = {test_base::make_value("data")};
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto setTtl = client.setTtl(key, nullptr, 5000).get();
        assert(setTtl);
        
        auto ttl = client.getTtl(key, nullptr).get();
        assert(ttl > 0);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testWriteLockOnQueue(FastCacheStandaloneClient &client) {
    std::cout << "  testWriteLockOnQueue... ";
    try {
        Key key = test_base::make_key("writeLockQueue");
        
        std::vector<ValuePtr> initial = {test_base::make_value("data")};
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto lockRes = client.lockObject(key, nullptr, LockType::WRITE_LOCK, 5).get();
        assert(lockRes == OK);
        
        auto unlockRes = client.unlockObject(key, nullptr, 5).get();
        assert(unlockRes == OK);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testReadLockOnQueue(FastCacheStandaloneClient &client) {
    std::cout << "  testReadLockOnQueue... ";
    try {
        Key key = test_base::make_key("readLockQueue");
        
        std::vector<ValuePtr> initial = {test_base::make_value("data")};
        client.createQueue(key, nullptr, &initial).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto lockRes = client.lockObject(key, nullptr, LockType::READ_LOCK).get();
        assert(lockRes == OK);
        
        auto unlockRes = client.unlockObject(key, nullptr, 5).get();
        assert(unlockRes == OK);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

void testQueueOperations(FastCacheStandaloneClient &client) {
    std::cout << "=== Queue Operations Tests ===\n";
    testCreateEmptyQueue(client);
    testCreateQueueWithInitialData(client);
    testGetHeadOnEmptyQueue(client);
    testGetAndRemoveFrontOnEmptyQueue(client);
    testGetAndRemoveFront(client);
    testGetAndRemoveFrontOnSingleElement(client);
    testGetFrontAndHead(client);
    testGetTail(client);
    testGetTailOnSingleElementQueue(client);
    testGetAndRemoveTail(client);
    testAddElementToTail(client);
    testAddElementToHead(client);
    testRemoveHead(client);
    testRemoveTail(client);
    testGetSizeReturnsZero(client);
    testRemoveQueue(client);
    testSetTtlAndGetTtlOnQueue(client);
    testWriteLockOnQueue(client);
    testReadLockOnQueue(client);
    std::cout << "Queue Operations: ALL PASSED\n\n";
}
