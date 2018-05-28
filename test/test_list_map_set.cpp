#include "../lib/catch.hpp"

#include "include/test_helpers.hpp"
#include "../src/builtins/builtins.hpp"

#include <functional>

// struct hash_visitor {
//     template<typename T>
//     size_t operator()(const ValueString val) {
//         return std::hash<std::string>(val);
//     }

//     size_t operator()(const ValuePyGenerator& gen) {
//         return hash_visitor()(gen.frame);
//     }

//     size_t operator()(const value::NoneType none) {
//         return 37;
//     }

//     template<typename T>
//     size_t operator()(const gc_ptr<T> value) {
//         return (size_t)(value.get());
//     }

//     template<typename T>
//     size_t operator()(const std::shared_ptr<T> value) {
//         return (size_t)(value.get());
//     }

//     size_t operator()(const int64_t value) {
//         return std::hash<double>()(value);
//     }

//     size_t operator()(const double value) {
//         return std::hash<double>()(value);
//     }

//     template<typename T>
//     size_t operator()(const T value) {
//         return std::hash<T>()(value);
//     }
// };

// struct equality_visitor {
//     bool operator()(const ValueString l, const ValueString r) const {
//         return *l == *r;
//     }

//     bool operator()(const ValuePyGenerator& l, const ValuePyGenerator& r) const {
//         return l.frame == r.frame;
//     }

//     bool operator()(const value::NoneType, const value::NoneType) const {
//         return true;
//     }

//     template<typename T>
//     bool operator()(const gc_ptr<T>& l, const gc_ptr<T>& r) const {
//         return (size_t)(l.get()) == (size_t)(r.get());
//     }

//     template<typename T>
//     bool operator()(const std::shared_ptr<T>& l, const std::shared_ptr<T>& r) const {
//         return (size_t)(l.get()) == (size_t)(r.get());
//     }

//     bool operator()(const value::NoneType, const value::NoneType) {
//         return true;
//     }

//     template<typename T1, typename T2>
//     bool operator()(const T1 v1, const T2 v2) const {
//         const constexpr bool is_numeric_1 = std::is_same<T1, double>::value || std::is_same<T1, int64_t>::value;
//         const constexpr bool is_numeric_2 = std::is_same<T2, double>::value || std::is_same<T2, int64_t>::value;

//         if constexpr(is_numeric_1 && is_numeric_2) {
//             return v1 == v2;
//         } else {
//             return false;
//         }
//     }
// };

// namespace std {
//     template <> struct hash<Value>
//     {
//         size_t operator()(const Value& val) const
//         {
//             return std::visit(hash_visitor(), val);
//         }
//     };
// }

// bool operator == (const Value a, const Value b) {
//     return std::visit(equality_visitor(), a, b);
// }

TEST_CASE("lists should work", "[lists]") {

    SECTION("can create an unordered_map<Value, Value>") {
        // std::unordered_map<Value, Value> myMap;
        // myMap[Value((int64_t)15)] = alloc.heap_string.make("hello worlddd").retain();
    }

    SECTION("can create a list") {
        auto code = build_string(R"(
[1,2,3,4,5]
        )");
        InterpreterState state(code);
        state.eval();
    }

    SECTION("can access a value in a list") {
        auto code = build_string(R"(
check_val([1,2,3,4,5][4])
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_val"] = make_builtin_check_value((int64_t)5);
        state.eval();
    }

    SECTION("can access a value in a list") {
        auto code = build_string(R"(
check_val([1,2,3,4,5][-1])
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_val"] = make_builtin_check_value((int64_t)5);
        state.eval();
    }

    SECTION("can loop through a list") {
        auto code = build_string(R"(
myList = [1, 2, 3, 4, 5]
x = 0
while x < len(myList):
  check_val(myList[x] == x + 1)
  x += 1
        )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_val"] = make_builtin_check_value((bool)true);
        state.eval();
    }

}