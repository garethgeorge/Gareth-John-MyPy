#include <istream>
#include <fstream>

#include "../../lib/debug.hpp"
#include "test_helpers.hpp"

extern gc_ptr<Code> build_file(const string& fname) {
    std::istreambuf_iterator<char> eos;
    
    DEBUG("loading python source from file");
    std::ifstream fstream(fname);
    std::string s(std::istreambuf_iterator<char>(fstream), eos);
    return Code::from_program(s, "../pytools/compile.py");
}

extern gc_ptr<Code> build_string(const string& code) {
    return Code::from_program(code, "../pytools/compile.py");
}
