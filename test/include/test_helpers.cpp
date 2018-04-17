#include <istream>
#include <fstream>

#include "../../lib/debug.hpp"
#include "test_helpers.hpp"

extern std::shared_ptr<Code> build_file(const char * fname) {
    std::istreambuf_iterator<char> eos;
    
    DEBUG("loading python source from file");
    std::ifstream fstream(fname);
    std::string s(std::istreambuf_iterator<char>(fstream), eos);;
    return Code::fromProgram(s, "../pytools/compile.py");
}
