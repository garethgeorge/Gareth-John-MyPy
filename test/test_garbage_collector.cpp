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
    gc_ptr<MyClass> pointer;

    MyClass(int c, string b) 
        : a(c), b(b) {
    }
};

struct Baz;

using MyType = std::variant<int, gc_ptr<Baz>>;

struct Baz {
    std::vector<MyType> values;
};

namespace gc{
    void mark_children(gc_ptr<int>& object) {
        
    }

    void mark_children(gc_ptr<MyClass>& object) {
        if (object->pointer != nullptr) {
            object->pointer.mark();
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

    SECTION("complex case, no variant yet") {
        gc_heap<MyClass> myHeap;
        gc_ptr<MyClass> a = myHeap.make(1, "a");
        gc_ptr<MyClass> b = myHeap.make(2, "b");
        gc_ptr<MyClass> c(std::move(b));
        b = std::move(c);

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

    SECTION("complex case -- variant of just ints") {
        
        gc_heap<Baz> bazzes;
        MyType foo = bazzes.make();
    }
}
