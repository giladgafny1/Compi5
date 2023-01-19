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
    std::string current_var_for_function = "";
    CodeGen(CodeBuffer *cb): cb(cb){};
    /* Generates the symbol table if it doesn't exist. otherwise returns the instance */
    
    std::string freshVar();
    /** 
     * Allocating mem on stack for local variables.
     * Always allocating 32 bits (4 bytes).
     * If bool/byte type, then uses zext instr.
     * Returns offset of allocated var.
     */

    void emit_binop(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string binop_text);

    /* Boolean handling functions */
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

    void handle_true(Exp_c& new_exp);

    void handle_false(Exp_c& new_exp);

    /* Both for General Expressions and for booleans*/
    void handle_parentheses(const Exp_c& exp, Exp_c& new_exp);

    void handle_convert(const Exp_c& exp, Exp_c& new_exp);

    void alloca_ver_for_function();

    void store_var(int offset, const Exp_c& exp);

    std::string initialize_var(int offset, type_enum type);

    std::string load_var(int offset);
    
    /* Returns label of expression*/
    std::string emit_num_assign(Exp_c &new_exp, std::string var, std::string value);

    void deal_with_if(Exp_c& exp, const std::string label, Statement_c &s);

    void deal_with_call(Call_c& call, std::vector<Exp_c*>& expressions);

    void define_function(FuncDecl_c& func);

    void function_end(RetType_c& type);

    void deal_with_return(Exp_c &exp);

    void deal_with_return();

    void deal_with_break(Statement_c &s);

    void deal_with_while(Exp_c &exp, Marker &marker_exp, Marker & marker_s, Statement_c &s);

    void deal_with_else(Exp_c &exp);

    void end_else_if(Exp_c &exp, Statement_c &s1, Statement_c &s2, Marker &marker_s1, Marker &marker_s2);


};
#endif