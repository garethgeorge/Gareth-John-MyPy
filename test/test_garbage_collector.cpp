#include <catch.hpp>
#include <iostream>
#include <variant>
#include <vector>

#include "../src/pygc.hpp"

using namespace py::gc;
using namespace std;


struct Vector;

using MyType = std::variant<
    int, 
    Vector,
>;

struct Vector {
    vector<gc_ptr<MyType>> values;
};

namespace py {
    namespace gc {

        template<typename T>
        struct variant_visitor {
            T gc_visitor;
            
            void operator()(int val) { };
            void operator()(gc_ptr<Vector>& val) {
                gc_visitor(val);
            }
        };

        // for each child for garbage collected integers
        void for_each_child(gc_ptr<int>& val, auto&& visitor) {
            // pass
        }

        // for each child for garbage collected 
        void for_each_child(gc_ptr<MyType>& val, auto&& visitor) {
            std::cout << "for_each_child visitor was called" << std::endl;
            std::visit(variant_visitor(visitor), *val);
        }
    }
}

TEST_CASE("garbage collector", "[lib][gc]") {
    SECTION( "basic garbage collection of integers" ) {
        // make a heap, and get an integer pointer
        gc_heap<int> heap;
        auto heapObj = heap.make(15);
        REQUIRE(heap.heap_size() == 1);

        // now sweep, it has never been marked so it should be removed
        heap.sweep_marked_objects();

        REQUIRE(heap.heap_size() == 0);
    }


    SECTION( "basic garbage collection of integers again" ) {
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

    SECTION( "more advanced garbage collection of std::variant<int, Vector<int>>" 
        ", no cycles") {
        
        gc_heap<MyType> heap;
        // auto my_obj = heap.make(Vector());
        // my_obj->push_back(15);

    } 
}