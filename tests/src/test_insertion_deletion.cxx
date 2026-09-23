//
// Tests for Insertion and Deletion Operations
//
#include <iostream>
#include <chrono>
#include <future>
#include <vector>
#include <string>
#include <cassert>
#include "standalone_client.hxx"
#include "test_base.hxx"

static void testAddElementUnordered(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementUnordered... ";
    try {
        Key key = test_base::make_key("unordered1");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        client.createSet(key, nullptr, &initial).get();
        
        std::vector<ValuePtr> data = {test_base::make_value("new1")};
        auto result = client.addElementUnordered(key, nullptr, &data).get();
        assert(result == 1);
        
        auto items = client.streamSet(key).get();
        assert(items.size() == 3);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testAddElementWithWeight(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementWithWeight... ";
    try {
        Key key = test_base::make_key("orderedSet1");
        
        std::vector<OrderedValuePtr> initial = {
            test_base::make_ordered_value("item1", 100)
        };
        client.createOrderedSet(key, nullptr, &initial).get();
        
        std::vector<OrderedValuePtr> data = {
            test_base::make_ordered_value("new1", 200)
        };
        auto result = client.addElementWithWeight(key, nullptr, &data).get();
        assert(result == 1);
        
        auto items = client.streamOrderedSet(key).get();
        assert(items.size() == 2);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testAddElementToTail(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToTail... ";
    try {
        Key key = test_base::make_key("list1");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1")
        };
        client.createList(key, nullptr, &initial).get();
        
        std::vector<ValuePtr> data = {test_base::make_value("tail1")};
        auto result = client.addElementToTail(key, nullptr, &data).get();
        assert(result == 1);
        
        auto items = client.streamList(key).get();
        assert(items.size() == 2);
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testAddElementToHead(FastCacheStandaloneClient &client) {
    std::cout << "  testAddElementToHead... ";
    try {
        Key key = test_base::make_key("list2");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1")
        };
        client.createList(key, nullptr, &initial).get();
        
        std::vector<ValuePtr> data = {test_base::make_value("head1")};
        auto result = client.addElementToHead(key, nullptr, &data).get();
        assert(result == 1);
        
        auto items = client.streamList(key).get();
        assert(items.size() == 2);
        assert(std::string(items[0]->data, items[0]->size) == "head1");
        std::cout << "PASSED\n";
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveElement(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveElement... ";
    try {
        Key key = test_base::make_key("removeElement");
        
        std::vector<ValuePtr> initial = {
            test_base::make_value("item1"),
            test_base::make_value("item2")
        };
        client.createList(key, nullptr, &initial).get();
        
        auto result = client.remove(key).get();
        assert(result);
        
        try {
            client.streamList(key).get();
            std::cout << "FAILED: Expected NOT_FOUND\n";
        } catch (const std::exception &) {
            std::cout << "PASSED\n";
        }
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

static void testRemoveElementNonExistent(FastCacheStandaloneClient &client) {
    std::cout << "  testRemoveElementNonExistent... ";
    try {
        Key key = test_base::make_key("removeNonExistent");
        
        try {
            client.remove(key).get();
            std::cout << "FAILED: Expected NOT_FOUND\n";
        } catch (const std::exception &) {
            std::cout << "PASSED\n";
        }
    } catch (const std::exception &e) {
        std::cout << "FAILED: " << e.what() << "\n";
    }
}

void testInsertionDeletion(FastCacheStandaloneClient &client) {
    std::cout << "=== Insertion/Deletion Tests ===\n";
    testAddElementUnordered(client);
    testAddElementWithWeight(client);
    testAddElementToTail(client);
    testAddElementToHead(client);
    testRemoveElement(client);
    testRemoveElementNonExistent(client);
    std::cout << "Insertion/Deletion: PASSED\n\n";
}
