
#include "CodeGen.hpp"
#include <string>

std::string CodeGen::freshVar() {
    std::string var = "%var" + std::to_string(this->curr_reg_num);
    this->curr_reg_num++;
    return var;
}

int CodeGen::allocStack(int var_size) {
    
}

void CodeGen::emit_binop(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string binop_text)
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
    std::string binop_start_label = this->cb->genLabel();
    /* nextlist of exp2 is the start of these operations */
    this->cb->bpatch(exp2.nextlist, binop_start_label);
    this->cb->emit(new_exp.var + " = " + binop_instr + "i32 " + exp1.var + ", " + exp2.var);
    /* Check overflow */
    /* No need for Int_t type since already we use i32 in llvm */
    if (new_exp.type == Byte_t) {
        E_var new_var = this->freshVar();
        this->cb->emit(new_var + " = " + "and " + "i32 " + new_exp.var + ", " + "255");
        new_exp.var = new_var;
    }
    /* Entry is exp1 */
    new_exp.start_label = exp1.start_label;
    /* nextlist of exp1 is exp2 entry*/
    this->cb->bpatch(exp1.nextlist, exp2.start_label);
    /* Create next list fot new exp (aftter the binop is done) */
    int next_instr = this->cb->emit("br @");
    new_exp.nextlist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, FIRST));

}

void CodeGen::emit_relop(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string relop_text) {
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

    std::string relop_start_label = this->cb->genLabel();
    /* nextlist of exp2 is the start of these operations */
    E_var var = this->freshVar();
    this->cb->emit(var + " = " + "icmp " + relop_instr + " i32 " + exp1.var + ", " + exp2.var);
    new_exp.var = var;
    int next_instr = this->cb->emit("br i1 " + var + ", label @, label @");
    new_exp.truelist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, FIRST));
    new_exp.falselist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, SECOND));
    /* Entry is exp1 */
    new_exp.start_label = exp1.start_label;
    /* nextlist of exp1 is exp2 entry*/
    this->cb->bpatch(exp1.nextlist, exp2.start_label);
    /* No need for next list here, since anyway this block ends with a branch */
}

void CodeGen::handle_and(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string label) {
    /* Filled according to slide 50 */
    this->cb->bpatch(exp1.truelist, label);
    new_exp.truelist = exp2.truelist;
    new_exp.falselist = this->cb->merge(exp1.falselist, exp2.falselist);
    /* Exp1 AND Exp2 starts with Exp1*/
    new_exp.start_label = exp1.start_label;;
}

void CodeGen::handle_or(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string label) {
    this->cb->bpatch(exp1.falselist, label);
    new_exp.truelist = this->cb->merge(exp1.truelist, exp2.truelist);
    new_exp.falselist = exp2.falselist;
    /* Exp1 OR Exp2 starts with Exp1*/
    new_exp.start_label = exp1.start_label;
}
    
void CodeGen::handle_not(const Exp_c& exp, Exp_c& new_exp) {
    new_exp.truelist = exp.falselist;
    new_exp.falselist = exp.truelist;
    /* NOT EXP start with exp */
    new_exp.start_label = exp.start_label;
}

void CodeGen::handle_true(Exp_c& new_exp) {
    std::string label = cb->genLabel();
    new_exp.start_label = label;
    int next_instr = this->cb->emit("br @");
    new_exp.truelist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, FIRST));
    /* falselist is empty */  
      
}

void CodeGen::handle_false(Exp_c& new_exp) {
    std::string label = cb->genLabel();
    new_exp.start_label = label;
    int next_instr = this->cb->emit("br @");
    new_exp.falselist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, FIRST));
    /* truelist is empty */
}

void CodeGen::handle_parentheses(const Exp_c& exp, Exp_c& new_exp) {
    if (exp.type != Bool_t) {
        new_exp.var = exp.var;
    }
    else {
        /* Bool type */
        new_exp.truelist = exp.truelist;
        new_exp.falselist = exp.falselist;
    }
}

void CodeGen::handle_convert(const Exp_c& exp, Exp_c& new_exp) {
    /* No need to generate code here since byte and int are both i32 */
    new_exp.var = exp.var;
    new_exp.value = exp.value;
    //TODO: not sure this is right to do with memory
    new_exp.nextlist = exp.nextlist;
    new_exp.start_label = exp.start_label;
}

void CodeGen::alloca_ver_for_function()
{
    this->current_var_for_function = this->freshVar();
    cb->emit(this->current_var_for_function + " = alloca i32, i32 50");
}

void CodeGen::store_var(int offset, const Exp_c& exp)
{
    std::string get_ptr = this->freshVar();
    cb->emit(get_ptr + " = getelementptr i32, i32* " + this->current_var_for_function + ", i32 " + std::to_string(offset));
    cb->emit("store i32 " + exp.var + ", i32* " + get_ptr);
}

std::string CodeGen::initialize_var(int offset, type_enum type)
{
    std::string new_var = this->freshVar();
    std::string get_ptr = this->freshVar();
    if (type != Bool_t)
        cb->emit(new_var + " = add i32 " + "0" + ", 0");
    else{
       // this->handle_false();?
    }
    cb->emit(get_ptr + " = getelementptr i32, i32* " + this->current_var_for_function + ", i32 " + std::to_string(offset));
    cb->emit("store i32 " + new_var + ", i32* " + get_ptr);
    return new_var;
}

std::string CodeGen::load_var(int offset)
{
    std::string new_var = this->freshVar();
    std::string get_ptr = this->freshVar();
    cb->emit(get_ptr + " = getelementptr i32, i32* " + this->current_var_for_function + ", i32 " + std::to_string(offset));
    cb->emit(new_var + " = load i32, i32* " + get_ptr);
    return new_var;
}

std::string CodeGen::emit_num_assign(Exp_c &new_exp, std::string var, std::string value)
{
    std::string marker = cb->genLabel();
    cb->emit(var + " = add i32 " + value + ", 0");
    int nextinstr = cb->emit("br @");
    new_exp.nextlist = this->cb->makelist(std::pair<int, BranchLabelIndex>(nextinstr, FIRST));
    return marker;
}

void CodeGen::deal_with_if(Exp_c& exp, const std::string label)
{
    cb->bpatch(exp.truelist, label);
    std::string next_label = cb->genLabel();
    cb->emit("br label %" + label);
    cb->emit(label + ":");
    cb->bpatch(exp.falselist, next_label);
    cb->bpatch(exp.nextlist, next_label);
}





