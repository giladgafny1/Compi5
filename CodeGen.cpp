
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
        //add check divide by zero
    this->cb->emit(new_exp.var + "=" + binop_text + "i32" + exp1.var + ", " + exp2.var);
}





