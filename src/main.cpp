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


int main(const int argc, const char *argv[]) 
{
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

    auto code = std::make_shared<mypy::Code>(root);

    // std::string decoded = base64_decode(root.get<std::string>("co_code"));
    
    // for (int i = 0; i < decoded.length(); ++i) {
    //     const unsigned char c = decoded[i];
    //     // TODO: this naive approach does not take arguments of the bytecode
    //     // instructions into account, we will have to go back and add actual
    //     // understanding of the instructions to skip these
    //     // I would propose adding a bytecode class into lib/oplist which 
    //     // can hold this clearly necessary metadata.
    //     printf("%2X | %s\n", (unsigned)c, oplist[c]);
    // }

    return 0;
}