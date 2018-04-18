#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include "../lib/oplist.hpp"
#include "../lib/base64.hpp"
#include "pyinterpreter.hpp"
#include "pyvalue.hpp"
#include "builtins/builtins.hpp"

// #define DEBUG_ON
// #define STATS_ON
#include "../lib/debug.hpp"
#include "../lib/json.hpp"

using std::string;
using namespace py;

int main(const int argc, const char *argv[]) 
{
#ifdef STATS_ON
    std::cout << "statistics: " << std::endl;
    std::cout << "\tsize of 'Value' union struct: " << sizeof(py::Value) << std::endl;
    std::cout << "\tsize of 'Frame': " << sizeof(py::FrameState) << std::endl;
#endif

    // const char *source_code = "{\"type\": \"code\", \"co_code\": \"ZABTAA==\", \"co_lnotab\": \"\", \"co_consts\": [{\"type\": \"literal\", \"real_type\": \"<class 'NoneType'>\", \"value\": null}], \"co_name\": \"<module>\", \"co_filename\": \"sys.stdin.py\", \"co_argcount\": 0, \"co_kwonlyargcount\": 0, \"co_nlocals\": 0, \"co_stacksize\": 1, \"co_names\": null, \"co_varnames\": null, \"co_freevars\": null, \"co_cellvars\": null}";
    // json obj = json::parse(source_code);
    // std::cout << std::setw(4) << obj << std::endl;

    // this is the new default case, take raw source code as input
    std::shared_ptr<Code> code = nullptr;

    std::istreambuf_iterator<char> eos;
    if (argc > 1 && argv[1] != nullptr) {
        DEBUG("loading python source from file");
        std::ifstream fstream(argv[1]);
        std::string s(std::istreambuf_iterator<char>(fstream), eos);;
        code = Code::from_program(s, "../pytools/compile.py");
    } else {
        DEBUG("loading python source from stdin");
        std::string s(std::istreambuf_iterator<char>(std::cin), eos);;
        code = Code::from_program(s, "../pytools/compile.py");
    }
    
    InterpreterState state(code);
    builtins::inject_builtins(state.ns_builtins);
    state.eval();

    return 0;
}