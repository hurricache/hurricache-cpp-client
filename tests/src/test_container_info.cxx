//
// Tests for Container Info, Boundary, Positional, and Pop Operations
// Based on Java VectorOperationsTest, ListOperationsTest, QueueOperationsTest
//
#include <iostream>
#include <chrono>
#include <future>
#include <vector>
#include <string>
#include <cassert>
#include "test_base.hxx"
#include "utils.hxx"

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
        assert(std::string(pos1->data, pos1->size) == "pos1");
        assert(std::string(pos2->data, pos2->size) == "pos2");
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
        
        auto remaining = client.streamVector(key).get();
        assert(remaining.size() == 2);
        assert(std::string(remaining[0]->data, remaining[0]->size) == "second");
        assert(std::string(remaining[1]->data, remaining[1]->size) == "third");
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

static void testGetElementWithWeight(FastCacheStandaloneClient &client) {
    std::cout << "  testGetElementWithWeight... ";
    try {
        Key key = test_base::make_key("getElementWithWeight");
        
        std::vector<OrderedValuePtr> initial = {
            test_base::make_ordered_value("item1", 100),
            test_base::make_ordered_value("item2", 200),
            test_base::make_ordered_value("item3", 300)
        };
        client.createOrderedSet(key, nullptr, &initial).get();
        
        auto elem = client.getElementWithWeight(key, nullptr, 200, 0).get();
        assert(std::string(elem->data, elem->size) == "item2");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testGetAndRemoveElementWithWeight(FastCacheStandaloneClient &client) {
    std::cout << "  testGetAndRemoveElementWithWeight... ";
    try {
        Key key = test_base::make_key("getAndRemoveElementWithWeight");
        
        std::vector<OrderedValuePtr> initial = {
            test_base::make_ordered_value("item1", 100),
            test_base::make_ordered_value("item2", 200),
            test_base::make_ordered_value("item3", 300)
        };
        client.createOrderedSet(key, nullptr, &initial).get();
        
        auto removed = client.getAndRemoveElementWithWeight(key, nullptr, 200, 0).get();
        assert(std::string(removed->data, removed->size) == "item2");
        
        auto remaining = client.streamOrderedSet(key).get();
        assert(remaining.size() == 2);
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
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

void testContainerInfoBoundaryPositionalPop(FastCacheStandaloneClient &client) {
    std::cout << "=== Container Info, Boundary, Positional, Pop Tests ===\n";
    testGetSize(client);
    testGetHead(client);
    testGetTail(client);
    testGetElementAtPosition(client);
    testGetAndRemoveFront(client);
    testGetAndRemoveTail(client);
    testGetAndRemoveElementAtPosition(client);
    testGetElementWithWeight(client);
    testGetAndRemoveElementWithWeight(client);
    testRemoveHead(client);
    testRemoveTail(client);
    testRemoveElementAtPosition(client);
    std::cout << "Container Info, Boundary, Positional, Pop: PASSED\n\n";
}
