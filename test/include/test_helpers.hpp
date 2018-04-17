#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <memory>
#include "../../src/pycode.hpp"

using namespace py;

extern std::shared_ptr<Code> build_file(const char *);

constexpr const char *test_program_dir = "../test/include";

#endif