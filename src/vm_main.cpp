#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include "../lib/oplist.hpp"
#include "../lib/base64.hpp"
#include "pyinterpreter.hpp"
#include "pyvalue.hpp"
#include "builtins/builtins.hpp"

// #define DEBUG_ON
// #define STATS_ON
#include "../lib/debug.hpp"

namespace po = boost::program_options;
namespace pt = boost::property_tree;
namespace fs = boost::filesystem;
using std::string;
using namespace py;

int main(const int argc, const char *argv[]) 
{
#ifdef STATS_ON
    std::cout << "statistics: " << std::endl;
    std::cout << "\tsize of 'Value' union struct: " << sizeof(py::Value) << std::endl;
    std::cout << "\tsize of 'Frame': " << sizeof(py::FrameState) << std::endl;
#endif
    fs::path full_path = fs::system_complete(argv[0]).parent_path();
    
    po::options_description desc("Options"); 
    string filename;
    bool load_json;
    desc.add_options() 
        ("help", "Print help messages")
        ("load_json", po::value<bool>(&load_json)->default_value(false)->implicit_value(false), "")
        ("file", po::value<string>(&filename)->default_value("")->implicit_value(""), "filename");
    
    po::variables_map vm; 
    try 
    {
        po::store(po::parse_command_line(argc, argv, desc),  vm); 

        if ( vm.count("help")  ) 
        { 
            std::cout << "mypy by Gareth George and John Thomason" << std::endl 
                << desc << std::endl; 
            return 0;
        } 

        po::notify(vm);
    } 
    catch(po::error& e) 
    { 
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
      std::cerr << desc << std::endl; 
      return -1;
    }
    
    std::shared_ptr<Code> code = nullptr;
    if (load_json) {
        pt::ptree root;
        try {
            std::unique_ptr<std::istream> stream;
            if (filename.size() != 0) {
                DEBUG("loading JSON from file");
                std::ifstream fstream(filename);
                pt::read_json(fstream, root);
            } else {
                DEBUG("loading JSON from stdin");
                pt::read_json(std::cin, root);
            }
        } catch (pt::json_parser::json_parser_error error) {
            std::cout << "error, this is a malformatted JSON object." << std::endl;
            exit(-1);
        }

        std::cout << 
        "successfully parsed source code from intermediate "
        "JSON representation" << std::endl;

        auto code = std::make_shared<py::Code>(root);
    } else {
        // this is the new default case, take raw source code as input
        std::istreambuf_iterator<char> eos;
        if (filename.size() != 0) {
            DEBUG("loading python source from file");
            std::ifstream fstream(filename);
            std::string s(std::istreambuf_iterator<char>(fstream), eos);;
            code = Code::fromProgram(s, (full_path / "../pytools/compile.py").string());
        } else {
            DEBUG("loading python source from stdin");
            std::string s(std::istreambuf_iterator<char>(std::cin), eos);;
            code = Code::fromProgram(s, (full_path / "../pytools/compile.py").string());
        }
    } 
    
    InterpreterState state(code);
    builtins::inject_builtins(state.ns_bulitins);
    state.eval();

    return 0;
}