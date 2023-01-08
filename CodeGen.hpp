#ifndef COMPI_HW5_CODEGEN_HPP
#define COMPI_HW5_CODEGEN_HPP

#include <stdio.h>
#include <string>
#include "bp.hpp"
#include "Stypes.hpp"

class CodeGen {
public:
    int curr_reg_num = 0;
    CodeBuffer *cb;
    CodeGen(CodeBuffer *cb): cb(cb){};
    /* Generates the symbol table if it doesn't exist. otherwise returns the instance */
    
    std::string freshVar();
    //void alloc_stack(int alloc_size, )
    void emit_binop(Exp_c& exp1, Exp_c& exp2, Exp_c& new_exp, std::string binop_text);
};

#endif