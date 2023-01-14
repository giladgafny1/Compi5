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
    void emit_binop(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string binop_text);

    /** 
     * Emit relop function should be called only when we must use a bool variable:
     * 1. When a bool type is assigned or read.
     * 2. When a relop is calculated by icmp (happens for example before br in LLVM). - I think this is the case for this. see Lec 6 slide 50.
     * 3. Before a bool type is returned from a function.
     * 4. before a bool type is sent as a parameter.
     **/
    void emit_relop(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string relop_text);

    void handle_and(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string label);

    void handle_or(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string label);
    
    void handle_not(const Exp_c& exp, Exp_c& new_exp);

};
#endif