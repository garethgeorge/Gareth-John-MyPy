#ifndef BUILTINS_H
#define BUILTINS_H

#include "../pyinterpreter.hpp"
#include "../pyvalue.hpp"

namespace py {
namespace builtins {

extern void inject_builtins(Namespace& ns);

extern std::unordered_map<std::string, ValueCMethod> builtin_list_attributes; // methods for lists 

// initializers for the various builtin classes, should be called at program startup
// i.e. from main or from pycode
extern void initialize_slice_class();
extern void initialize_list_class();
extern void initialize_cell_class();

extern ValuePyClass slice_class;
extern ValuePyClass cell_class;
extern ValuePyObject builtins_slice_get_slice_object(Value start,Value stop,Value step);



}
}

#endif