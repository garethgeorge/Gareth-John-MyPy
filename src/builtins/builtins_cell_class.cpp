#include "builtins.hpp"

namespace py {
namespace builtins {

// This is needed to allow the create_cell function
ValuePyClass cell_class;

void initialize_cell_class() {
    cell_class = alloc.heap_pyclass.make("CELL_CLASS").retain();
}

}
}
