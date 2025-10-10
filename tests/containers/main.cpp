// Container (ud_set, ud_map) integration tests

#include <containers/ud_map.h>
#include <containers/ud_set.h>
#include <string>
#include <test.h>

using namespace v;

int main()
{
    auto [engine, tctx] = testing::init_test("containers");

    // Test ud_set basic operations
    {
        ud_set<int> set;

        tctx.assert_now(set.empty(), "New set is empty");
        tctx.assert_now(set.size() == 0, "New set has size 0");

        // Test insert
        auto [it1, inserted1] = set.insert(42);
        tctx.assert_now(inserted1, "Insert returns true for new element");
        tctx.assert_now(*it1 == 42, "Iterator points to inserted value");
        tctx.assert_now(!set.empty(), "Set not empty after insert");
        tctx.assert_now(set.size() == 1, "Set size 1 after insert");

        // Test duplicate insert
        auto [it2, inserted2] = set.insert(42);
        tctx.assert_now(!inserted2, "Duplicate insert returns false");
        tctx.assert_now(set.size() == 1, "Set size unchanged on duplicate");

        // Test find
        auto found_it = set.find(42);
        tctx.assert_now(
            found_it != set.end(), "Find returns valid iterator for existing element");
        tctx.assert_now(*found_it == 42, "Found iterator points to correct value");

        auto not_found_it = set.find(100);
        tctx.assert_now(
            not_found_it == set.end(), "Find returns end for non-existing element");

        // Test contains
        tctx.assert_now(set.contains(42), "Contains returns true for existing element");
        tctx.assert_now(
            !set.contains(100), "Contains returns false for non-existing element");
    }

    // Test ud_set with strings
    {
        ud_set<std::string> set;
        set.insert("hello");
        set.insert("world");

        tctx.assert_now(set.size() == 2, "Set with strings has correct size");
        tctx.assert_now(set.contains("hello"), "Set contains first string");
        tctx.assert_now(set.contains("world"), "Set contains second string");
        tctx.assert_now(!set.contains("test"), "Set doesn't contain non-inserted string");
    }

    // Test ud_set erase
    {
        ud_set<int> set;
        set.insert({ 1, 2, 3, 4, 5 });

        size_t erased = set.erase(3);
        tctx.assert_now(erased == 1, "Erase returns 1 for existing element");
        tctx.assert_now(set.size() == 4, "Size reduced after erase");
        tctx.assert_now(!set.contains(3), "Element removed from set");

        erased = set.erase(100);
        tctx.assert_now(erased == 0, "Erase returns 0 for non-existing element");
        tctx.assert_now(
            set.size() == 4, "Size unchanged when erasing non-existing element");
    }

    // Test ud_map basic operations
    {
        ud_map<int, std::string> map;

        tctx.assert_now(map.empty(), "New map is empty");
        tctx.assert_now(map.size() == 0, "New map has size 0");

        // Test insert/emplace
        auto [it1, inserted1] = map.emplace(42, "answer");
        tctx.assert_now(inserted1, "Emplace returns true for new key");
        tctx.assert_now(it1->first == 42, "Iterator key correct");
        tctx.assert_now(it1->second == "answer", "Iterator value correct");
        tctx.assert_now(!map.empty(), "Map not empty after emplace");
        tctx.assert_now(map.size() == 1, "Map size 1 after emplace");

        // Test duplicate key
        auto [it2, inserted2] = map.emplace(42, "duplicate");
        tctx.assert_now(!inserted2, "Duplicate key emplace returns false");
        tctx.assert_now(map.size() == 1, "Map size unchanged on duplicate key");
        tctx.assert_now(it1->second == "answer", "Original value preserved on duplicate");

        // Test operator[] access
        map[100] = "hundred";
        tctx.assert_now(map.size() == 2, "Map size increased after operator[]");
        tctx.assert_now(map[100] == "hundred", "Operator[] returns correct value");

        // Test find
        auto found_it = map.find(42);
        tctx.assert_now(
            found_it != map.end(), "Find returns valid iterator for existing key");
        tctx.assert_now(found_it->first == 42, "Found iterator key correct");
        tctx.assert_now(found_it->second == "answer", "Found iterator value correct");

        auto not_found_it = map.find(200);
        tctx.assert_now(
            not_found_it == map.end(), "Find returns end for non-existing key");

        // Test contains
        tctx.assert_now(map.contains(42), "Contains returns true for existing key");
        tctx.assert_now(
            !map.contains(200), "Contains returns false for non-existing key");
    }

    // Test ud_map with string keys
    {
        ud_map<std::string, int> map;
        map["one"] = 1;
        map["two"] = 2;

        tctx.assert_now(map.size() == 2, "Map with string keys has correct size");
        tctx.assert_now(map["one"] == 1, "String key maps to correct value");
        tctx.assert_now(map["two"] == 2, "Second string key maps to correct value");

        // Test that operator[] creates default value for non-existing key
        int new_val = map["three"];
        tctx.assert_now(new_val == 0, "Operator[] creates default value for new key");
        tctx.assert_now(
            map.size() == 3, "Map size increased when accessing new key via operator[]");
    }

    // Test ud_map erase
    {
        ud_map<int, std::string> map;
        map[1] = "one";
        map[2] = "two";
        map[3] = "three";

        size_t erased = map.erase(2);
        tctx.assert_now(erased == 1, "Erase returns 1 for existing key");
        tctx.assert_now(map.size() == 2, "Size reduced after erase");
        tctx.assert_now(!map.contains(2), "Key removed from map");

        erased = map.erase(100);
        tctx.assert_now(erased == 0, "Erase returns 0 for non-existing key");
        tctx.assert_now(map.size() == 2, "Size unchanged when erasing non-existing key");
    }

    // Test iteration over ud_set
    {
        ud_set<int> set;
        set.insert({ 10, 20, 30, 40, 50 });

        int sum   = 0;
        int count = 0;
        for (const auto& value : set)
        {
            sum += value;
            count++;
        }

        tctx.assert_now(count == 5, "Iteration visits all elements");
        tctx.assert_now(sum == 150, "Sum of iterated values correct");
    }

    // Test iteration over ud_map
    {
        ud_map<int, std::string> map;
        map[1] = "one";
        map[2] = "two";
        map[3] = "three";

        int         key_sum = 0;
        std::string concat;
        for (const auto& [key, value] : map)
        {
            key_sum += key;
            concat += value;
        }

        tctx.assert_now(key_sum == 6, "Sum of keys correct");
        tctx.assert_now(
            concat.find("one") != std::string::npos, "Values concatenated correctly");
        tctx.assert_now(
            concat.find("two") != std::string::npos, "Values concatenated correctly");
        tctx.assert_now(
            concat.find("three") != std::string::npos, "Values concatenated correctly");
    }

    // Test clear operation
    {
        ud_set<int> set;
        set.insert({ 1, 2, 3, 4, 5 });
        tctx.assert_now(set.size() == 5, "Set has elements before clear");

        set.clear();
        tctx.assert_now(set.empty(), "Set empty after clear");
        tctx.assert_now(set.size() == 0, "Set size 0 after clear");

        ud_map<int, std::string> map;
        map[1] = "one";
        map[2] = "two";
        tctx.assert_now(map.size() == 2, "Map has elements before clear");

        map.clear();
        tctx.assert_now(map.empty(), "Map empty after clear");
        tctx.assert_now(map.size() == 0, "Map size 0 after clear");
    }

    return tctx.is_failure();
}
