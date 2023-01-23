#ifndef COMPI_HW5_CODEGEN_HPP
#define COMPI_HW5_CODEGEN_HPP

#include <stdio.h>
#include <string>
#include "bp.hpp"
#include "Stypes.hpp"
#include "stack"

class CodeGen {
public:
    int curr_reg_num = 0;
    CodeBuffer *cb;
    std::string current_var_for_function = "";
    std::stack<std::string> labels_while; 
    int count_label=0;
    CodeGen(CodeBuffer *cb): cb(cb){};  
    /* Generates the symbol table if it doesn't exist. otherwise returns the instance */
    
    std::string freshVar();

    std::string freshVarGlobal();

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

    void handle_and(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp);

    void handle_or(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp);
    
    void handle_not(const Exp_c& exp, Exp_c& new_exp);

    void handle_true(Exp_c& new_exp);

    void handle_false(Exp_c& new_exp);

    /* Both for General Expressions and for booleans*/
    void handle_parentheses(const Exp_c& exp, Exp_c& new_exp);

    void handle_convert(const Exp_c& exp, Exp_c& new_exp);

    void handle_trenary(Exp_c& if_true_exp, Exp_c& bool_exp, Exp_c& if_false_exp, Exp_c& new_exp);

    void alloca_ver_for_function(FuncDecl_c& func_decl);

    void store_var(int offset, Exp_c& exp, Statement_c &s);

    std::string initialize_var(Statement_c &new_s, int offset, type_enum type);

    std::string load_var(Exp_c& exp, int offset);
    
    /* Returns label of expression*/
    std::string emit_num_assign(Exp_c &new_exp, std::string var, std::string value);

    void deal_with_if(Statement_c &new_s, Exp_c& exp, Statement_c &s);

    void deal_with_call(Call_c& call, std::vector<Exp_c*>& expressions);

    void deal_with_call(Call_c& call);

    void define_function(FuncDecl_c& func);

    void function_end(RetType_c& type, Statements_c &s, N_Marker &n_marker);

    void deal_with_return(Exp_c &exp, Statement_c &s);

    void deal_with_return(Statement_c &s);

    void deal_with_break(Statement_c &s);

    void handle_continue(Statement_c &s);

    void begin_while();

    void end_while(Statement_c &new_s, Exp_c &exp, Statement_c &s);

    void deal_with_else(Exp_c &exp);

    void end_else_if(Statement_c &new_s, Exp_c &exp, Statement_c &s1, Statement_c &s2);

    void call_as_statement(Call_c& call, Statement_c &s);

    void merge_statement_lists(Statement_c &s, Statement_c &s_list);

    void handle_first_statement(Statements_c &statements, Statement_c &s);

    void handle_statements(Statements_c &new_statements, Statement_c &s);

    void handle_statement_close(Statement_c &new_s, Statements_c &statements);

    Exp_c* bool_exp(Exp_c &exp);

    std::string trenary_phi(Exp_c &exp);

    void handle_string(Exp_c &exp, String_c &string);

    void handle_bool_explist(Exp_c &exp);

    void emit_start();
};
#endif