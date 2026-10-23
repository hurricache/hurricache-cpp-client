//
// Smoke test for FastCacheSmartClient against a live cluster.
// Usage: smart_smoke_test [coordinator_host]
//
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#include "smart_client.hxx"
#include "utils.hxx"

namespace {

int g_failures = 0;

void check(bool cond, const std::string &name) {
    if (cond) {
        std::cout << "[PASS] " << name << "\n";
    } else {
        std::cout << "[FAIL] " << name << "\n";
        ++g_failures;
    }
}

Key makeKey(const std::string &s) {
    return Key(static_cast<uint32_t>(s.size()), s.data());
}

bool valueEquals(const ValuePtr v, const std::string &expected) {
    return v != nullptr && v->type == BYTES && v->size == expected.size() &&
           memcmp(v->data, expected.data(), expected.size()) == 0;
}

} // namespace

int main(int argc, char *argv[]) {
    std::string host = "192.168.0.123";
    if (argc > 1) host = argv[1];

    std::vector<std::string> coordinators = {
        host + ":61000", host + ":51000", host + ":21000"
    };

    std::cout << "Smart client smoke test, coordinators: "
              << coordinators[0] << ", " << coordinators[1] << ", " << coordinators[2] << "\n";

    try {
        FastCacheSmartClient client(coordinators,
                                    std::chrono::milliseconds(0),   // ttl
                                    0,                              // client_id
                                    std::chrono::milliseconds(5000)); // timeout

        // --- KV round-trip without hint (client computes weak/strong hash) ---
        {
            auto key = makeKey("smoke_kv_1");
            Value value("hello-smart", 11);

            auto hint = client.createKeyValue(key, nullptr, value).get();
            std::cout << "  createKeyValue hint: weak=" << hint.weak_hash
                      << " strong=" << hint.strong_hash << "\n";

            auto got = client.getValue(key, nullptr).get();
            check(valueEquals(got, "hello-smart"), "kv: create + get (hint computed by client)");
            delete got;

            check(client.existKey(key, nullptr).get(), "kv: existKey");

            // Round-trip with the server-returned hint (routes to the same shard)
            auto got2 = client.getValue(key, &hint).get();
            check(valueEquals(got2, "hello-smart"), "kv: get with returned hint");
            delete got2;

            Value updated("hello-v2", 8);
            auto old = client.updateKeyValue(key, &hint, updated).get();
            check(valueEquals(old, "hello-smart"), "kv: update returns old value");
            delete old;
            auto after_update = client.getValue(key, &hint).get();
            check(valueEquals(after_update, "hello-v2"), "kv: value updated");
            delete after_update;

            auto ttl = client.getTtl(key, &hint).get();
            check(ttl == -1 || ttl >= 0, "kv: getTtl");

            check(client.remove(key, &hint).get(), "kv: remove");
            check(!client.existKey(key, &hint).get(), "kv: key gone after remove");
        }

        // --- List container ---
        {
            auto key = makeKey("smoke_list_1");
            Value v1("a", 1), v2("b", 2 - 1), v3("c", 1);
            std::vector<ValuePtr> initial{&v1, &v2, &v3};

            auto hint = client.createList(key, nullptr, &initial).get();
            check(client.getSize(key, &hint).get() == 3, "list: create with 3 elements");

            Value v4("d", 1);
            std::vector<ValuePtr> more{&v4};
            check(client.addElementToTail(key, &hint, &more).get() == 1, "list: addElementToTail");
            check(client.getSize(key, &hint).get() == 4, "list: size == 4 after push");

            auto head = client.getHead(key, &hint).get();
            check(valueEquals(head, "a"), "list: getHead");
            delete head;

            auto tail = client.getTail(key, &hint).get();
            check(valueEquals(tail, "d"), "list: getTail");
            delete tail;

            auto all = client.streamList(key, &hint).get();
            check(all.size() == 4, "list: streamList size");
            free_content(all);

            auto popped = client.getAndRemoveFront(key, &hint).get();
            check(valueEquals(popped, "a"), "list: getAndRemoveFront");
            delete popped;

            check(client.remove(key, &hint).get(), "list: remove");
        }

        // --- Map container ---
        {
            auto key = makeKey("smoke_map_1");
            auto ck = makeKey("field1");
            Value cv("map-value", 9);
            std::vector<KeyPtr> keys{&ck};
            std::vector<ValuePtr> values{&cv};

            auto hint = client.createMap(key, nullptr, nullptr).get();
            check(client.addElementHashMap(key, &hint, &keys, &values).get() == 1,
                  "map: addElementHashMap");

            auto got = client.getContainerValue(key, &hint, ck).get();
            check(valueEquals(got, "map-value"), "map: getContainerValue");
            delete got;

            check(client.containsContainerKey(key, &hint, ck).get(), "map: containsContainerKey");
            check(client.removeFromContainer(key, &hint, ck).get() == 1, "map: removeFromContainer");
            check(client.remove(key, &hint).get(), "map: remove");
        }

        // --- Atomics ---
        {
            auto key = makeKey("smoke_atomic_1");
            auto hint = client.createKeyValue(key, nullptr, Value(static_cast<int64_t>(41))).get();
            auto prev = client.atomicAdd(key, &hint, static_cast<int64_t>(1)).get();
            check(prev != nullptr && prev->type == INT && prev->int_value == 41,
                  "atomic: add returns previous value");
            delete prev;

            auto cur = client.getValue(key, &hint).get();
            check(cur != nullptr && cur->type == INT && cur->int_value == 42,
                  "atomic: value == 42 after add");
            delete cur;

            auto cas = client.atomicCompareAndSet(key, &hint, 42, 100).get();
            check(cas.result, "atomic: compareAndSet");
            check(client.remove(key, &hint).get(), "atomic: remove");
        }

        std::cout << "\nTarget: " << client.getTarget() << "\n";
        client.shutdown();
    } catch (const std::exception &e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 2;
    }

    std::cout << "\n=== " << (g_failures == 0 ? "ALL SMOKE TESTS PASSED" : "FAILURES: " + std::to_string(g_failures))
              << " ===\n";
    return g_failures == 0 ? 0 : 1;
}
