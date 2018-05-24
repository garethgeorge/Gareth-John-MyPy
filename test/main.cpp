#define CATCH_CONFIG_RUNNER  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch.hpp>

#include "../src/builtins/builtins.hpp"
#include "../src/pyallocator.hpp"

using namespace py;
using namespace py::builtins;

int main( int argc, char* argv[] ) {
    initialize_slice_class();
    initialize_list_class();
    initialize_cell_class();

    alloc.retain_all();

    int result = Catch::Session().run( argc, argv );
    return result;
}