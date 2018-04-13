#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "../lib/oplist.hpp"
#include "../lib/base64.hpp"
#include "./interpreter.hpp"

namespace po = boost::program_options;
namespace pt = boost::property_tree;
using std::string;
using namespace py;


int main(const int argc, const char *argv[]) 
{
    std::cout << "statistics: " << std::endl;
    std::cout << "\tsize of 'Value' union struct: " << sizeof(py::Value) << std::endl;

    pt::ptree root;
    try {
        pt::read_json(std::cin, root);
    } catch (pt::json_parser::json_parser_error error) {
        std::cout << "error, this is a malformatted JSON object." << std::endl;
        exit(-1);
    }
    
    std::cout << 
        "successfully parsed source code from intermediate "
        "JSON representation" << std::endl;

    auto code = std::make_shared<py::Code>(root);
    std::cout << "printing op codes" << std::endl;

    for (int i = 0; i < code->bytecode.size(); i++) {
        Code::ByteCode bytecode = code->bytecode[i];
        if (bytecode == 0) continue ;
        printf("%10d %s\n", i, op::name[bytecode]);
        if (bytecode == op::LOAD_CONST) {
            std::cout << "\tconstant: " << code->constants[code->bytecode[i + 1]] << std::endl;
        }
        if (bytecode >= 0x5A) {
            i += 1;
        }
    }

    return 0;
}