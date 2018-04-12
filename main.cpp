#include <stdio.h>
#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "pycfile.hpp"
#include "lib/base64.hpp"

namespace po = boost::program_options;
namespace pt = boost::property_tree;
using std::string;

int main(const int argc, const char *argv[]) 
{
    // std::cout << "initializing" << std::endl;
    // po::options_description desc("MyPy python bytecode runtime");
    // desc.add_options()
    //     ("help", "produce the help message");
    pt::ptree root;
    try {
        pt::read_json(std::cin, root);
    } catch (pt::json_parser::json_parser_error error) {
        std::cout << "error, this is a malformatted JSON object." << std::endl;
        exit(-1);
    }

    std::cout << "successfully parsed JSON object" << std::endl;

    string byteCode = root.get<string>("bytecode");

    return 0;
}