#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "../lib/oplist.hpp"
#include "../lib/base64.hpp"
#include "pyinterpreter.hpp"
#include "pyvalue.hpp"
#include "builtins/builtins.hpp"

namespace po = boost::program_options;
namespace pt = boost::property_tree;
using std::string;
using namespace py;

int main(const int argc, const char *argv[]) 
{
    std::cout << "statistics: " << std::endl;
    std::cout << "\tsize of 'Value' union struct: " << sizeof(py::Value) << std::endl;
    std::cout << "\tsize of 'Frame': " << sizeof(py::FrameState) << std::endl;

    pt::ptree root;
    try {
        std::ifstream fstream(argv[1]);
        pt::read_json(fstream, root);
    } catch (pt::json_parser::json_parser_error error) {
        std::cout << "error, this is a malformatted JSON object." << std::endl;
        exit(-1);
    }
    
    std::cout << 
        "successfully parsed source code from intermediate "
        "JSON representation" << std::endl;

    auto code = std::make_shared<py::Code>(root);
    
    InterpreterState state(code);
    builtins::inject_builtins(state.ns_bulitins);
    state.eval();

    return 0;
}