#include <iostream>

#include "builtins.hpp"
#include "builtins_helpers.hpp"
#include "../lib/debug.hpp"
#include "../pyvalue_helpers.hpp"

namespace py {
namespace builtins {

ValuePyClass slice_class = std::make_shared<value::PyClass>(value::PyClass("SLICE_CLASS"));

void initialize_slice_class(){
    (*(slice_class->attrs))["start"] = value::NoneType();
    (*(slice_class->attrs))["stop"] = value::NoneType();
    (*(slice_class->attrs))["step"] = value::NoneType();
}

ValuePyObject builtins_slice_get_slice_object(Value start, Value stop, Value step){
    DEBUG_ADV("Creating Slice: " << start << ", " << stop << ", " << step);

    ValuePyObject obj = std::make_shared<value::PyObject>(slice_class);
    obj->store_attr("start",start);
    obj->store_attr("stop",stop);
    obj->store_attr("step",step);
}

}
}