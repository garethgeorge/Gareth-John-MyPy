#include "pyvalue.hpp"
#include "pyallocator.hpp"

namespace py {

namespace value {

PyObject::PyObject(ValuePyClass cls) : static_attrs(cls) {
    this->attrs = alloc.heap_namespace.make();
};

}

}