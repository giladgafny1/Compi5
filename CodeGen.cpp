
#include "CodeGen.hpp"
#include <string>

std::string CodeGen::freshVar() {
    std::string var = "%var" + std::to_string(this->curr_reg_num);
    this->curr_reg_num++;
    return var;
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
    E_var new_var = this->freshVar();
    new_exp.var = new_var;
    std::string binop_start_label = this->cb->genLabel();
    /* nextlist of exp2 is the start of these operations */
    this->cb->bpatch(exp2.nextlist, binop_start_label);
    this->cb->emit(new_exp.var + " = " + binop_instr + "i32 " + exp1.var + ", " + exp2.var);
    /* Check overflow */
    /* No need for Int_t type since already we use i32 in llvm */
    if (new_exp.type == Byte_t) {
        this->cb->emit(new_var + " = " + "and " + "i32 " + new_exp.var + ", " + "255");
    }
    /* Entry is exp1 */
    new_exp.start_label = exp1.start_label;
    /* nextlist of exp1 is exp2 entry*/
    this->cb->bpatch(exp1.nextlist, exp2.start_label);
    /* Create next list fot new exp (aftter the binop is done) */
    int next_instr = this->cb->emit("br label @");
    new_exp.nextlist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, FIRST));

}

void CodeGen::emit_relop(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string relop_text)
{
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
    /* add by mor */
    cb->bpatch(exp2.nextlist, relop_start_label);
}

void CodeGen::handle_and(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string label)
{
    /* Filled according to slide 50 */
    this->cb->bpatch(exp1.truelist, label);
    new_exp.truelist = exp2.truelist;
    new_exp.falselist = this->cb->merge(exp1.falselist, exp2.falselist);
    /* Exp1 AND Exp2 starts with Exp1*/
    new_exp.start_label = exp1.start_label;
}

void CodeGen::handle_or(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string label)
{
    this->cb->bpatch(exp1.falselist, label);
    new_exp.truelist = this->cb->merge(exp1.truelist, exp2.truelist);
    new_exp.falselist = exp2.falselist;
    /* Exp1 OR Exp2 starts with Exp1*/
    new_exp.start_label = exp1.start_label;
}
    
void CodeGen::handle_not(const Exp_c& exp, Exp_c& new_exp)
{
    new_exp.truelist = exp.falselist;
    new_exp.falselist = exp.truelist;
    /* NOT EXP start with exp */
    new_exp.start_label = exp.start_label;
}

void CodeGen::handle_true(Exp_c& new_exp)
{
    std::string label = cb->genLabel();
    new_exp.start_label = label;
    int next_instr = this->cb->emit("br label @");
    new_exp.truelist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, FIRST));
    /* falselist is empty */  
      
}

void CodeGen::handle_false(Exp_c& new_exp)
{
    std::string label = cb->genLabel();
    new_exp.start_label = label;
    int next_instr = this->cb->emit("br label @");
    new_exp.falselist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, FIRST));
    /* truelist is empty */
}

void CodeGen::handle_parentheses(const Exp_c& exp, Exp_c& new_exp)
{
    if (exp.type != Bool_t) {
        new_exp.var = exp.var;
        new_exp.nextlist = exp.nextlist;
    }
    else {
        /* Bool type */
        new_exp.var = exp.var;
        new_exp.truelist = exp.truelist;
        new_exp.falselist = exp.falselist;
    }
}

void CodeGen::handle_convert(const Exp_c& exp, Exp_c& new_exp)
{
    /* No need to generate code here since byte and int are both i32 */
    new_exp.var = exp.var;
    new_exp.value = exp.value;
    //TODO: not sure this is right to do with memory
    new_exp.nextlist = exp.nextlist;
    new_exp.start_label = exp.start_label;
}

void CodeGen::handle_trenary(Exp_c& if_true_exp, Exp_c& bool_exp, Exp_c& if_false_exp, Exp_c& new_exp)
{
    /* Set truelist/falselist of bool */
    this->cb->bpatch(bool_exp.truelist, if_true_exp.start_label);
    this->cb->bpatch(bool_exp.falselist, if_false_exp.start_label);
    
    new_exp.start_label = bool_exp.start_label;
    /* Both exp will have a same next list (since from both the code jump to the same, not yet set, spot)*/
    new_exp.nextlist = cb->merge(if_true_exp.nextlist, if_false_exp.nextlist);
}

void CodeGen::alloca_ver_for_function()
{
    this->current_var_for_function = this->freshVar();
    cb->emit(this->current_var_for_function + " = alloca i32, i32 50");
}

void CodeGen::store_var(int offset, const Exp_c& exp, Statement_c &s)
{
    s.start_label = exp.start_label;
    std::string label = cb->genLabel();
    cb->bpatch(exp.nextlist, label);
    std::string get_ptr = this->freshVar();
    cb->emit(get_ptr + " = getelementptr i32, i32* " + this->current_var_for_function + ", i32 " + std::to_string(offset));
    cb->emit("store i32 " + exp.var + ", i32* " + get_ptr);
    int nextinstr = cb->emit("br label @");
    s.nextlist = this->cb->makelist(std::pair<int, BranchLabelIndex>(nextinstr, FIRST));
}

std::string CodeGen::initialize_var(Statement_c &new_s, int offset, type_enum type)
{
    std::string new_var = this->freshVar();
    std::string get_ptr = this->freshVar();
    new_s.start_label = cb->genLabel();
    if (type != Bool_t)
        cb->emit(new_var + " = add i32 " + "0" + ", 0");
    else{
       // this->handle_false();?
    }
    cb->emit(get_ptr + " = getelementptr i32, i32* " + this->current_var_for_function + ", i32 " + std::to_string(offset));
    cb->emit("store i32 " + new_var + ", i32* " + get_ptr);
    int nextinstr = cb->emit("br label @");
    new_s.nextlist = this->cb->makelist(std::pair<int, BranchLabelIndex>(nextinstr, FIRST));
    return new_var;
}

std::string CodeGen::load_var(Exp_c& exp, int offset)
{
    std::string new_var = this->freshVar();
    std::string get_ptr = this->freshVar();
    exp.start_label = cb->genLabel();
    cb->emit(get_ptr + " = getelementptr i32, i32* " + this->current_var_for_function + ", i32 " + std::to_string(offset));
    cb->emit(new_var + " = load i32, i32* " + get_ptr);
    int nextinstr = cb->emit("br label @");
    exp.nextlist = this->cb->makelist(std::pair<int, BranchLabelIndex>(nextinstr, FIRST));
    exp.var = new_var;
    return new_var;
}

std::string CodeGen::emit_num_assign(Exp_c &new_exp, std::string var, std::string value)
{
    std::string marker = cb->genLabel();
    cb->emit(var + " = add i32 " + value + ", 0");
    int nextinstr = cb->emit("br label @");
    new_exp.nextlist = this->cb->makelist(std::pair<int, BranchLabelIndex>(nextinstr, FIRST));
    return marker;
}

void CodeGen::deal_with_if(Statement_c &new_s, Exp_c& exp, Statement_c &s)
{
    cb->bpatch(exp.truelist, s.start_label);
    std::string next_label = cb->genLabel();

    new_s.start_label = exp.start_label;
    new_s.nextlist = cb->merge(exp.falselist, s.nextlist);
}

// Gilad to Mor: i think we need a label here, case Call can be -> to EXP, so I added ot
void CodeGen::deal_with_call(Call_c& call, std::vector<Exp_c*>& expressions)
{
    call.var = freshVar();
    /* Call starts with the "calculation" of it's first parameter */
    call.start_label = expressions[expressions.size() - 1]->start_label;
    std::string args_to_print = "";
    int i;
    /* Gilad: I think this should be from backwards to start, they said in the order it was written and it is a right recursion (like a stack) */
    for (i = expressions.size() - 1; i > 0 ; i--)
    {
        args_to_print += "i32 " + expressions[i]->var;
        args_to_print+= ", ";
        /* Untill last Exp: */
        this->cb->bpatch(expressions[i]->nextlist, expressions[i-1]->start_label);
    }

    std::string label = cb->genLabel();
    
    if (!expressions.empty()) {
        /* Last Expression */
        args_to_print += "i32 " + expressions[0]->var;
        cb->bpatch(expressions[0]->nextlist, label);
    }
    if (call.type == Void_t)
    {
        cb->emit("call void @" + call.name + "(" + args_to_print + ")");
    }
    else{
            cb->emit(call.var + " = call i32 @" + call.name + "(" + args_to_print + ")");
    }
    int address = cb->emit("br label @");
    call.nextlist = cb->makelist(std::pair<int, BranchLabelIndex>(address, FIRST));
}

void CodeGen::deal_with_call(Call_c& call)
{
    call.var = freshVar();
    /* Call starts with the "calculation" of it's first parameter */
    std::string label = cb->genLabel();
    call.start_label = label;
    std::string args_to_print = "";
    if (call.type == Void_t)
    {
        cb->emit("call void @" + call.name + "(" + args_to_print + ")");
    }
    else{
            cb->emit(call.var + "= call i32 @" + call.name + "(" + args_to_print + ")");
    }
    int address = cb->emit("br label @");
    call.nextlist = cb->makelist(std::pair<int, BranchLabelIndex>(address, FIRST));
}

void CodeGen::define_function(FuncDecl_c& func)
{
    std::string args_to_print = "";
    for (auto &decl : func.decls)
    {
        args_to_print += "i32 , ";
    }
    cout<<func.name<<endl;
    if (!func.decls.empty())
        args_to_print.erase(args_to_print.size() -2);
    cout<<func.name + "!"<<endl;
    cb->emit("define i32 @" + func.name + "(" + args_to_print + "){");
}

void CodeGen::function_end(RetType_c& type)
{
    if(type.type == Void_t)
        cb->emit("ret void");
    else
        cb->emit("ret i32 0");
    cb->emit("}");
}

void CodeGen::deal_with_return(Exp_c &exp, Statement_c &s)
{
    s.start_label = exp.start_label;
    std::string label = cb->genLabel();
    // what? next list is before the ret is even typed?
    //cb->bpatch(s.nextlist, next_label);
    cb->emit("ret i32 " + exp.var);
    /* after expressions is calculated goto return code */
    cb->bpatch(exp.nextlist, label);
    /* Creating here a nextlist just to prevent segfaults on merges, since we assume all S have nextlists
        need to make sure it is backpatched though! */
    //int addr = cb->emit("br label @");
    //s.nextlist = cb->makelist(std::pair<int, BranchLabelIndex>(addr, FIRST));
    //Junk vector for trying
    s.nextlist.push_back(std::pair<int, BranchLabelIndex>(-1, FIRST));
}

void CodeGen::deal_with_return(Statement_c &s)
{
    s.start_label = cb->emit("ret void");
    /* Creating here a nextlist just to prevent segfaults on merges, since we assume all S have nextlists
        need to make sure it is backpatched though! */
    //int addr = cb->emit("br label @");

    //Junk vector for trying
    s.nextlist.push_back(std::pair<int, BranchLabelIndex>(-1, FIRST));
}

void CodeGen::deal_with_break(Statement_c &s)
{
    int next_quad = cb->emit("br label @");
    s.breaklist = cb->makelist(std::pair<int, BranchLabelIndex>(next_quad, FIRST));
    cout<<s.nextlist.back().first<<endl;
}

void CodeGen::handle_continue(Statement_c &s)
{
    std::string label = cb->genLabel();
    s.start_label = label;
    /* Will be backpatched to the condition of the while */
    int address = cb->emit("br label @");
    s.nextlist = cb->makelist(std::pair<int, BranchLabelIndex>(address, FIRST));
    
}

void CodeGen::begin_while()
{
    //std::string start_while = cb->genLabel();
    // GILAD: % instead of @ (I understand you didn't mean backpatching, but actually the label). but I think this will be an infitite loop so I commented it out
    //cb->emit("br label %" + start_while);
    //this->labels_while.push(start_while);
}

void CodeGen::end_while(Statement_c &new_s, Exp_c &exp, Statement_c &s)
{
    new_s.start_label = exp.start_label;
    /* To start condition - TODO: maybe not needed if S always has nextlist */
    cb->emit("br label %" + exp.start_label);
    /* End of statment goes to condition of while */
    cb->bpatch(s.nextlist, exp.start_label);
    cb->bpatch(exp.truelist, s.start_label);
    //labels_while.pop();
    if (!s.breaklist.empty())
        /* Will be backpatched to start label of next statement */
        /* Todo: what if last statement in scope - maybe boolean (was backpatched)*/
        new_s.nextlist = cb->merge(exp.falselist, s.breaklist);
    else
        new_s.nextlist = exp.falselist;
    
}

void CodeGen::deal_with_else(Exp_c &exp)
{   //TODO: not sure if needed - not needed
    int next_quad = cb->emit("br label @");
    exp.nextlist = cb->merge(exp.nextlist, cb->makelist(std::pair<int, BranchLabelIndex>(next_quad, FIRST)));
}

void CodeGen::end_else_if(Statement_c &new_s ,Exp_c &exp, Statement_c &s1, Statement_c &s2)
{
    cb->bpatch(exp.truelist, s1.start_label);
    cb->bpatch(exp.falselist, s2.start_label);

    new_s.start_label = exp.start_label;
    std::cout << "start label is " << new_s.start_label << endl;
    new_s.nextlist = cb->merge(s1.nextlist, s2.nextlist);
}

void CodeGen::call_as_statement(Call_c& call, Statement_c &s)
{
    s.nextlist = call.nextlist;
    s.start_label = call.start_label;
}

 void CodeGen::merge_statement_lists(Statement_c &s, Statement_c &s_list)
 {
    cout<<"in merge now"<<endl;
    s_list.nextlist = cb->merge(s.nextlist, s_list.nextlist);
 }

 void CodeGen::handle_first_statement(Statements_c &statements, Statement_c &s)
 {
    std::cout << "in func handle_first_statement" << endl;
    statements.start_label = s.start_label;
    std::cout << "before nlist -> crushing on statements.nextlist" << endl;
    std::cout << s.start_label << endl;
    statements.nextlist = s.nextlist;
    std::cout << "after func" << endl;

 }

 void CodeGen::handle_statements(Statements_c &statements, Statement_c &s)
 {
    /* start_label already assigned because $$ = $1 was done */
    /* next list of last statement in list, will point to start label of new statement */
    cb->bpatch(statements.nextlist, s.start_label);
    statements.s_list.push_back(&s);
    /* next list of the vector is the next list of last statement */
    statements.nextlist = s.nextlist;
 }

 void CodeGen::handle_statement_close(Statement_c &new_s, Statements_c &statements)
 {
    new_s.start_label = statements.start_label;
    new_s.nextlist = statements.nextlist;
 }
