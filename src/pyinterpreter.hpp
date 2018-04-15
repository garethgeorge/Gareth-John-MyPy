#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stack>
#include <unordered_map>
#include "pyerror.hpp"
#include "pyvalue.hpp"
#include "optflags.hpp"

#include "pycode.hpp"
#include "pyframe.hpp"
#include "pyvalue.hpp"

namespace py {

// see https://www.boost.org/doc/libs/1_55_0/doc/html/variant/tutorial.html
// we use boost::variant as a compact union type carrying type information
struct Code;

using Namespace = std::unordered_map<std::string, Value>;

struct InterpreterState {
    std::stack<FrameState> callstack;
    Namespace ns_globals;
    Namespace ns_bulitins;

    InterpreterState(std::shared_ptr<Code>& code);

    void eval();
};

}


#endif 