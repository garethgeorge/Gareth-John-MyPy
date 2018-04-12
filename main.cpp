#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "lib/oplist.hpp"
#include "lib/base64.hpp"

namespace po = boost::program_options;
namespace pt = boost::property_tree;
using std::string;

int main(const int argc, const char *argv[]) 
{
    for (int i = 0; i < 256; ++i) {
        std::cout << oplist[i] << std::endl;
    }

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

    std::string decoded = base64_decode(root.get<std::string>("bytecode"));

    for (int i = 0; i < decoded.length(); ++i) {
        const char c = decoded[i];
        // TODO: this naive approach does not take arguments of the bytecode
        // instructions into account, we will have to go back and do this
        printf("%2X | %s\n", (unsigned)c, oplist[c]);
    }


    return 0;
}