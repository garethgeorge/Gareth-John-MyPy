#ifndef BUILTINS_H
#define BUILTINS_H

#include "../pyinterpreter.hpp"

namespace py {
namespace builtins {

extern void inject_builtins(Namespace& ns);

}
}

#endif