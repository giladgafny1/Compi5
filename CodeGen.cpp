
#include "CodeGen.hpp"
#include <string>

std::string CodeGen::freshVar() {
    std::string var = "%var" + std::to_string(this->curr_reg_num);
    this->curr_reg_num++;
    return var;
}

void CodeGen::emit_binop(Exp_c& exp1, Exp_c& exp2, Exp_c& new_exp, std::string binop_text)
{
    //if (binop_text == "/")
        //add check divide by zero - Gilad: It needs to be in the code since it's a run-time thing.
    std::string binop_instr;
    if (binop_text[0] == '+') {
        //TODO: make sure parser gets the text as +
        binop_instr = 'add ';
    }
    else if (binop_text[0] == '-') {
        binop_instr = 'sub ';
    }
    else if (binop_text[0] == '*') {
        binop_instr = 'mul ';
    }
    else if (binop_text[0] == '/') {
        //TODO: casting? I think no need to since we'll keep everything in i32 anyway. And the casting "check" is done in semantic checks. Result will be in new_exp
        if (new_exp.type == Byte_t) {
            binop_instr = 'udiv ';
        }
        else {
            binop_instr = 'sdiv ';
        }
    }
    this->cb->emit(new_exp.var + " = " + binop_instr + "i32 " + exp1.var + ", " + exp2.var);

    /* Check overflow */
    /* No need for Int_t type since already we use i32 in llvm */
    if (new_exp.type == Byte_t) {
        std::string new_var = freshVar();
        this->cb->emit(new_var + " = " + "and " + "i32 " + new_exp.var + ", " + "255");
        new_exp.var = new_var;
    }
}

void CodeGen::emit_relop(Exp_c& exp1, Exp_c& exp2, Exp_c& new_exp, std::string relop_text) {
    
}





