#include <catch.hpp>
#include <iostream>
#include <variant>
#include <vector>
#include <memory>
#define DEBUG_ON
#include <debug.hpp>

#include "../src/pygc.hpp"

using namespace py::gc;
using namespace std;

namespace py {
    namespace gc {
        // for each child for garbage collected integers
        void for_each_child(gc_ptr<int>& val, auto&& visitor) {
            // pass
        }
    }
}



TEST_CASE("garbage collector, integers", "[lib][gc]") {
    SECTION( "should clear all objects when sweeping without first marking" ) {
        // make a heap, and get an integer pointer
        gc_heap<int> heap;
        auto heapObj = heap.make(15);
        REQUIRE(heap.heap_size() == 1);

        // now sweep, it has never been marked so it should be removed
        heap.sweep_marked_objects();

        REQUIRE(heap.heap_size() == 0);
    }


    SECTION( "should only clear unmarked objects when sweeping after marking" ) {
        // make a heap, and get an integer pointer
        gc_heap<int> heap;
        auto heapObj = heap.make(15);
        REQUIRE(heap.heap_size() == 1);

        // mark that integer and try to GC, it should not be removed
        auto tmp = heap.get_mark_visitor();
        heapObj.gc_visit(tmp);
        heap.sweep_marked_objects();
        
        REQUIRE(heap.heap_size() == 1);
        
        // now sweep again without re marking it, it should be removed
        heap.sweep_marked_objects();

        REQUIRE(heap.heap_size() == 0);
    }
}



struct Vector;

using MyType = std::variant<
    int, 
    Vector
>;

struct Vector {
    vector<gc_ptr<MyType>> values;
};

namespace py {
    namespace gc {

        template<typename T>
        struct variant_visitor {
            T gc_visitor;

            variant_visitor(T gc_visitor) : gc_visitor(gc_visitor) { };
            
            void operator()(int val) { };
            void operator()(Vector& val) {
                for (gc_ptr<MyType>& child : val.values) {
                    child.gc_visit(gc_visitor);
                }
            }
        };
        

        // for each child for garbage collected 
        void for_each_child(gc_ptr<MyType>& val, auto visitor) {
            std::cout << "for_each_child visitor was called" << std::endl;
            std::visit(variant_visitor(visitor), *val);
        }
    }
}

TEST_CASE("garbage collector, std::variant<int, Vector>", "[lib][gc]") {

    // setup the heap with its initial values, and one cycle
    gc_heap<MyType> heap;
    auto my_obj = heap.make(Vector());
    std::get<Vector>(*my_obj).values.push_back(heap.make(15)); // add an integer value
    std::get<Vector>(*my_obj).values.push_back(my_obj); // add reference to itself

    SECTION( "should clear all objects when sweeping without first marking" ) {
        REQUIRE(heap.heap_size() == 2);
        heap.sweep_marked_objects();
        REQUIRE(heap.heap_size() == 0);
    }

    SECTION( "should only clear unmarked objects when sweeping after marking" ) {
        REQUIRE(heap.heap_size() == 2);
        auto val = heap.get_mark_visitor();
        
        my_obj.gc_visit(val);
        heap.sweep_marked_objects();
        
        REQUIRE(heap.heap_size() == 2);

        heap.sweep_marked_objects();

        REQUIRE(heap.heap_size() == 0);
    }
}