#include <iostream>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "pycode.hpp"
#include "oplist.hpp"
#include "pyallocator.hpp"
#include "pyvalue.hpp"

#undef DEBUG_ON

#include "../lib/base64.hpp"
#include "../lib/debug.hpp"
#include "../lib/json.hpp"

#define PROCXX_HAS_PIPE2 0 // not sure what this is, but it is not available on our build platform
#include "../lib/process.hpp"

namespace py {

Code::Code(const json& tree) {
    DEBUG("loading in source code from json");

#ifdef DEBUG_ON
    std::cout << __FILENAME__ << " ";
    std::cout << std::setw(4) << tree << std::endl;
#endif
    
    this->co_stacksize = tree.at("co_stacksize").get<uint64_t>();
    this->co_nlocals = tree.at("co_nlocals").get<uint64_t>();
    this->co_argcount = tree.at("co_argcount").get<uint64_t>();

    /*
        decode the bytecode and push it into the bytecode property
    */
    std::string base64bytecode = tree.at("co_code").get<std::string>();
    std::string bytecode = base64_decode(base64bytecode);
    this->bytecode.reserve(bytecode.length());
    
    for (const unsigned char c : bytecode) {
        this->bytecode.push_back(c);
    }

    /*
        read the line number table into a temporary
    */
    std::vector<LineNoMapping> lnotab_orig;

    for (const json& linenumber : tree.at("lnotab")) {
        // DEBUG("loaded line number %d", linenumber);
        lnotab_orig.push_back(
            LineNoMapping {linenumber.at(0).get<uint64_t>(), linenumber.at(1).get<uint64_t>()}
        );
    }

    /*
        decode the bytecode array into fully expanded instructions w/their arguments
    */
    {
        DEBUG_ADV("decoding instructions from this->bytecode");
        size_t lnotab_idx = 0;
        uint64_t pc = 0;

        // we use pc_map to serve a dual purpose
        // 1) it lets us translate from the bytecode's jump locations to the new
        //    decoded instruction jump target locations for instructions
        //    that take a bytecode address as their argument
        // 2) it lets us validate that jump targets are indeed valid before runtime.
        pc_map.resize(this->bytecode.size());

        while (pc < this->bytecode.size()) {
            ByteCode bytecode = this->bytecode[pc];
            if (bytecode == 0) continue ;
            Instruction instruction;
            instruction.bytecode = bytecode;
            instruction.bytecode_index = pc;
            pc_map[pc] = instructions.size();
            
            // do some book keeping, translate the line number table to the new virtual bytecode format
            if (pc >= lnotab_orig[lnotab_idx].pc && lnotab_idx < lnotab_orig.size()) {
                this->lnotab.push_back(
                    LineNoMapping {this->instructions.size(), lnotab_orig[lnotab_idx].line}
                );
                lnotab_idx++;
            }

            // decode the arg if there is an argument, otherwise set it to 0
            if (bytecode >= op::HAVE_ARGUMENT) {
                // Read the argument
                instruction.arg = this->bytecode[pc + 1] | (this->bytecode[pc + 2] << 8);
                DEBUG_ADV("decoded " << pc << ":" << op::name[bytecode] << ":" << instruction.arg);
                pc += 3;
            } else {
                instruction.arg = 0;
                DEBUG_ADV("decoded " << pc << ":" << op::name[bytecode]);
                pc += 1;
            }
            this->instructions.push_back(instruction);
        }

        // stitch arguments that reference program counter
        for (auto& instr : this->instructions) {
            switch (instr.bytecode) {
                case op::POP_JUMP_IF_FALSE:
                case op::JUMP_ABSOLUTE:
                {
                    if (instr.arg >= pc_map.size() || pc_map[instr.arg] == 0) {
                        throw pyerror("invalid jump target, is not the beginning of an instruction.");
                    }
                    instr.arg = pc_map[instr.arg];
                    break;
                }
                default: continue;
            }
        }
    }
    

    // load constants
    const json& co_consts = tree.at("co_consts");
    DEBUG("loading in the co_consts constant pool");
    for (const json& element : co_consts) {
        const auto& type = element.at("type").get<std::string>();
        if (type == "literal") {
            const auto& real_type = element.at("real_type").get<std::string>();
            if (real_type == "<class 'str'>") {
                DEBUG("constant at index %lu is string", this->co_consts.size());
                this->co_consts.push_back(
                    alloc.heap_string.make(element.at("value").get<std::string>())
                );
            } else if (real_type == "<class 'int'>") {
                DEBUG("constant at index %lu is int", this->co_consts.size());
                this->co_consts.push_back(
                    element.at("value").get<int64_t>()
                );
            } else if (real_type == "<class 'float'>") {
                DEBUG("constant at index %lu is float", this->co_consts.size());
                this->co_consts.push_back(
                    element.at("value").get<double>()
                );
            } else if (real_type == "<class 'NoneType'>") {
                DEBUG("constant at index %lu is nonetype", this->co_consts.size());
                this->co_consts.push_back(
                    value::NoneType()
                );
            } else if (real_type == "<class 'bool'>") {
                DEBUG("constant at index %lu is bool", this->co_consts.size());
                this->co_consts.push_back(
                    element.at("value").get<bool>()
                );
            } else {
                throw pyerror(std::string("unrecognized type of constant: ") + real_type);
            }
        } else if (type == "code") {
            gc_ptr<Code> code = alloc.heap_code.make(Code(element));
            this->co_consts.push_back(code);
        } else {
            throw pyerror(std::string("unrecognized type of constant: ") + type);
        }
    }

    // load names
    for (const json& name : tree.at("co_names")) {
        DEBUG("loaded name %lu) %s", this->co_names.size(), name.get<std::string>().c_str())
        this->co_names.push_back(
            name.get<std::string>()
        );
    }

    // load var names
    for (const json& vname : tree.at("co_varnames")) {
        DEBUG("loaded var name %lu) %s", this->co_varnames.size(), vname.get<std::string>().c_str())
        this->co_varnames.push_back(
            vname.get<std::string>()
        );
    }

    // load cell vars
    for (const json& cvname : tree.at("co_cellvars")) {
        DEBUG("loaded cell var name %lu) %s", this->co_cellvars.size(), cvname.get<std::string>().c_str())
        this->co_cellvars.push_back(
            cvname.get<std::string>()
        );
    }

    // load free vars
    for (const json& fvname : tree.at("co_freevars")) {
        DEBUG("loaded free var name %lu) %s", this->co_freevars.size(), fvname.get<std::string>().c_str())
        this->co_freevars.push_back(
            fvname.get<std::string>()
        );
    }

    // build the co_cellmap
    {
        size_t index = 0;
        for (std::string& cellvarname : this->co_cellvars) {
            this->co_cellmap[cellvarname] = index++;
        }
    }

    // load lnotab 
    for (const json& linenumber : tree.at("lnotab")) {
        DEBUG("loaded line number %d", linenumber.at(0).get<uint64_t>());
        this->lnotab.push_back(
            LineNoMapping {linenumber.at(0).get<uint64_t>(), linenumber.at(1).get<uint64_t>()}
        );
    }

    // load co_name
    this->co_name = tree.at("co_name").get<std::string>();

    // set flags by scanning the instructions
    for (const Instruction& instr : this->instructions) {
        if (instr.bytecode == op::YIELD_FROM || instr.bytecode == op::YIELD_VALUE) {
            this->set_flag(FLAG_IS_GENERATOR_FUNCTION);
        }
    }
}   

Code::~Code() {
}

gc_ptr<Code> Code::from_program(const std::string& python, const std::string& compilePyPath) {
    procxx::process compilePyProc{"python3", compilePyPath.c_str()};
    
    DEBUG("execing compile python process");
    compilePyProc.exec();
    compilePyProc << python;
    compilePyProc.close(procxx::pipe_t::write_end());
    
    std::istreambuf_iterator<char> eos;
    std::string outputJson(std::istreambuf_iterator<char>(compilePyProc.output()), eos);
    std::string outputError(std::istreambuf_iterator<char>(compilePyProc.error()), eos);

    if (outputError.size() > 0) {
        std::cout << "ERROR PARSING SOURCE CODE: " << std::endl << outputError;
        throw pyerror("source code error");
    }
    
    DEBUG("trying to read JSON");
    json tree;
    try {
        tree = json::parse(outputJson.c_str());
    } catch (std::exception& err) {
        std::cout << outputJson << std::endl;
        throw err;
    }
    DEBUG("read successful.");
    return alloc.heap_code.make(tree);
}

}
