#ifndef BUILTINS_H
#define BUILTINS_H

#include "../pyinterpreter.hpp"
#include "../pyvalue.hpp"

namespace py {
namespace builtins {

extern void inject_builtins(Namespace& ns);

extern std::unordered_map<std::string, ValueCMethod> builtin_list_attributes; // methods for lists 

}
}

#endif