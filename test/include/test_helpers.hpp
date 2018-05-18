#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <memory>
#include <string>
#include <iostream>

#include "../../src/pycode.hpp"
#include "../../src/pyinterpreter.hpp"


using namespace std;
using namespace py;

extern std::shared_ptr<Code> build_file(const string&);
extern std::shared_ptr<Code> build_string(const string&);

// when we have a shared pointer, we use a different method of comparing the 
// wrapped values
template<typename T>
extern ValueCFunction make_builtin_check_value(std::shared_ptr<T> value) {
    return std::make_shared<value::CFunction>([value](FrameState& frame, ArgList& args) {
        REQUIRE(*std::get<std::shared_ptr<T>>(args[0]) == *value);
        frame.value_stack.push_back(value::NoneType());
        return ;
    });
}

template<typename T>
extern ValueCFunction make_builtin_check_value(T value) {
    return std::make_shared<value::CFunction>([value](FrameState& frame, ArgList& args) {
        REQUIRE(std::get<T>(args[0]) == value);
        frame.value_stack.push_back(value::NoneType());
        return ;
    });
}

constexpr const char *test_program_dir = "../test/py_programs";

#endif