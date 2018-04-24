#include <string>
#include <iostream>
#include <catch.hpp>
#include <variant>
#include <memory>
#include <optional>

#include <pygc.hpp>

using namespace std;
using namespace gc;

struct MyClass {
    int a;
    std::string b;
    optional<gc_ptr<MyClass>> pointer;

    MyClass(int c, string b) 
        : a(c), b(b) {
    }
};

namespace gc{
    void mark_children(gc_ptr<int>& object) {
        
    }

    void mark_children(gc_ptr<MyClass>& object) {
        if (object->pointer) {
            (*(object->pointer)).mark();
        }
    }
}

TEST_CASE("should be able to do garbage collection", "[arithmetic]") {
    SECTION("simple case -- integers") {
        gc_heap<int> myHeap;
        gc_ptr<int> ptr = myHeap.make(15);
        
        REQUIRE(myHeap.size() == 1);
        ptr.mark();
        myHeap.sweep();

        REQUIRE(myHeap.size() == 1);

        myHeap.sweep();
        REQUIRE(myHeap.size() == 0);
    }

    SECTION("complex case -- variant") {
        gc_heap<MyClass> myHeap;
        gc_ptr<MyClass> a = myHeap.make(1, "a");
        gc_ptr<MyClass> b = myHeap.make(2, "b");
        a->pointer = b;

        REQUIRE(myHeap.size() == 2);

        a.mark();
        myHeap.sweep();

        REQUIRE(myHeap.size() == 2);

        b.mark();
        myHeap.sweep();

        REQUIRE(myHeap.size() == 1);

        myHeap.sweep();

        REQUIRE(myHeap.size() == 0);
    }
}
