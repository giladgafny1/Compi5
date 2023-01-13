
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
    std::string binop_instr = "";
    switch (binop_text[0]) {
        case '+':
            //TODO: make sure parser gets the text as +
            binop_instr = "add ";
            break;
        case '-':
            binop_instr = "sub ";
            break;
        case '*':
            binop_instr = "mul ";
            break;
        case '/':
            //TODO: casting? I think no need to since we'll keep everything in i32 anyway. And the casting "check" is done in semantic checks. Result will be in new_exp
            if (new_exp.type == Byte_t) {
                binop_instr = "udiv ";
            }
            else {
                binop_instr = "sdiv ";
            }
            break;
        default:
            break;
    }
    this->cb->emit(new_exp.var + " = " + binop_instr + "i32 " + exp1.var + ", " + exp2.var);

    /* Check overflow */
    /* No need for Int_t type since already we use i32 in llvm */
    if (new_exp.type == Byte_t) {
        E_var new_var = this->freshVar();
        this->cb->emit(new_var + " = " + "and " + "i32 " + new_exp.var + ", " + "255");
        new_exp.var = new_var;
    }
}

void CodeGen::emit_relop(Exp_c& exp1, Exp_c& exp2, Exp_c& new_exp, std::string relop_text) {
    std::string relop_instr = "";
    bool sign;
    if (relop_text == "==") {
        relop_instr = "eq ";
    }
    else if (relop_text == "!=") {
        relop_instr = "ne ";
    }
    else if (relop_text[0] == '<') {
        relop_instr = "lt ";
    }
    else if (relop_text[0] == '>') {
        relop_instr = "gt ";
    }
    else if (relop_text == "<=") {
        relop_instr = "le ";
    }
    else if (relop_text == ">=") {
        relop_instr = "ge ";
    }

    if (exp1.type == Int_t || exp2.type == Int_t)
    /* The comparisson of ints if one of the types is and integer (and not byte)*/
        sign = true;
    else
        sign = false;
    
    if (relop_instr != "eq " && relop_instr != "ne ") {
        if (sign)
            relop_instr = "s" + relop_instr;
        else
            relop_instr = "u" + relop_instr;
    }

    /* Generating a var here (and not in parser), becuase not all relops need a var */
    E_var var = this->freshVar();
    this->cb->emit(var + " = " + "icmp " + relop_instr + exp1.var + ", " + exp2.var);
    new_exp.var = var;
    int address = this->cb->emit("br i1 " + var + ", label @, label @");
    new_exp.truelist = this->cb->makelist(std::pair<int, BranchLabelIndex>(address, FIRST));
    new_exp.falselist = this->cb->makelist(std::pair<int, BranchLabelIndex>(address, SECOND));
}

void emit_and(Exp_c& exp1, Exp_c&, Exp_c& new_exp) {
    /* Fill according to slide 50 */
}






