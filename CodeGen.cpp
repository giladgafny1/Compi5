
#include "CodeGen.hpp"
#include <string>
#include <algorithm>


std::string CodeGen::freshVar() {
    std::string var = "%var" + std::to_string(this->curr_reg_num);
    this->curr_reg_num++;
    return var;
}

std::string CodeGen::freshVarGlobal() 
{
    std::string var = "@.var_" + std::to_string(this->curr_reg_num);
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

    if (binop_text[0] == '/') {
        /* Division check */
        this->cb->emit("call void @division_by_zero(i32 " + exp2.var + ")");
    }
    this->cb->emit(new_exp.var + " = " + binop_instr + "i32 " + exp1.var + ", " + exp2.var);
    /* Check overflow */
    /* No need for Int_t type since already we use i32 in llvm */
    if (new_exp.type == Byte_t) {
        E_var new_new_var = this->freshVar();
        this->cb->emit(new_new_var + " = " + "and " + "i32 " + new_exp.var + ", " + "255");
        new_exp.var = new_new_var;
    }
    /* Entry is exp1 */
    new_exp.start_label = exp1.start_label;
    /* nextlist of exp1 is exp2 entry*/
   // cout<<"exp2 start label"<<exp2.start_label<<endl;
    this->cb->bpatch(exp1.nextlist, exp2.start_label);
    /* Create next list fot new exp (aftter the binop is done) */
    int next_instr = this->cb->emit("br label @");
    new_exp.nextlist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, FIRST));

}

void CodeGen::emit_relop(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp, const std::string relop_text)
{
    std::string relop_instr = "";
    bool sign;
    if (relop_text.compare("==") == 0){
        relop_instr = "eq ";
    }
    else if (relop_text.compare("!=") == 0) {
        relop_instr = "ne ";
    }
    else if (relop_text.compare("<") == 0){
        relop_instr = "lt ";
    }
    else if (relop_text.compare(">") == 0) {
        relop_instr = "gt ";
    }
    else if (relop_text.compare("<=") == 0) {
        relop_instr = "le ";
    }
    else if (relop_text.compare(">=") == 0) {
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

void CodeGen::handle_and(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp)
{
    /* Filled according to slide 50 */
    this->cb->bpatch(exp1.truelist, exp2.start_label);
    new_exp.truelist = exp2.truelist;
    new_exp.falselist = this->cb->merge(exp1.falselist, exp2.falselist);
    /* Exp1 AND Exp2 starts with Exp1*/
    new_exp.start_label = exp1.start_label;
}

void CodeGen::handle_or(const Exp_c& exp1, const Exp_c& exp2, Exp_c& new_exp)
{
    this->cb->bpatch(exp1.falselist, exp2.start_label);
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
        new_exp.start_label = exp.start_label;
    }
    else {
        /* Bool type */
        new_exp.var = exp.var;
        new_exp.truelist = exp.truelist;
        new_exp.falselist = exp.falselist;
        new_exp.start_label = exp.start_label;
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

string getSize(type_enum type)
{
    if ( type == String_t)
        return "i8*";
    return "i32";
}
/*
void CodeGen::handle_trenary(Exp_c& if_true_exp, Exp_c& bool_exp, Exp_c& if_false_exp, Exp_c& new_exp)
{
    // Set truelist/falselist of bool 
    std::string label_true = cb->genLabel();
    cout<<label_true<<endl;
    this->cb->bpatch(bool_exp.truelist, label_true);
    int next_quad = cb->emit(new_exp.var +  " = " + getSize(new_exp.type) + " " + if_true_exp.var);
    cb->bpatch(cb->makelist(std::pair<int, BranchLabelIndex>(next_quad, FIRST)),if_true_exp.start_label);

    std::string label_false = cb->genLabel();
    cout<<label_false<<endl;
    this->cb->bpatch(bool_exp.falselist, label_false);
    int next_quad1 = cb->emit(new_exp.var +  " = " + getSize(new_exp.type) + " " + if_false_exp.var);
    cb->bpatch(cb->makelist(std::pair<int, BranchLabelIndex>(next_quad1, FIRST)),if_false_exp.start_label);  
    new_exp.start_label = bool_exp.start_label;
    // Both exp will have a same next list (since from both the code jump to the same, not yet set, spot)
    new_exp.nextlist = cb->merge(if_true_exp.nextlist, if_false_exp.nextlist);
    new_exp.is_trenary = true;
    new_exp.true_label = if_true_exp.start_label;
    new_exp.false_label = if_false_exp.start_label;
}
*/

void CodeGen::handle_trenary(Exp_c& if_true_exp, Exp_c& bool_exp, Exp_c& if_false_exp, Exp_c& new_exp)
{
    /* Set truelist/falselist of bool */
    this->cb->bpatch(bool_exp.truelist, if_true_exp.start_label);
    this->cb->bpatch(bool_exp.falselist, if_false_exp.start_label);
    
    new_exp.start_label = bool_exp.start_label;
    /* Both exp will have a same next list (since from both the code jump to the same, not yet set, spot)*/
    new_exp.nextlist = cb->merge(if_true_exp.nextlist, if_false_exp.nextlist);
    new_exp.is_trenary = true;
    new_exp.true_label = if_true_exp.start_label;
    new_exp.false_label = if_false_exp.start_label;
}
void CodeGen::alloca_ver_for_function(FuncDecl_c &func)
{
    this->current_var_for_function = this->freshVar();
    cb->emit(this->current_var_for_function + " = alloca i32, i32 50");
    //int instr = cb->emit("br label @");
    //std::cout << ("HERE") << endl;
    //func.nextlist = this->cb->makelist(std::pair<int, BranchLabelIndex>(instr, FIRST));
}

void CodeGen::store_var(int offset, Exp_c& exp, Statement_c &s)
{
    //std::string var = exp.var;
    InstrList next_list = exp.nextlist;
    if(exp.type == Bool_t) {
        Exp_c* new_exp = bool_exp(exp);
        //var = new_exp->var;
        exp.var = new_exp->var;
        next_list = new_exp->nextlist;
    }
    std::string label = cb->genLabel();
    //cout<<"in store val the label is"<< label<<endl;
    cb->bpatch(next_list, label);
   // cb->bpatch(exp.nextlist, label);
    s.start_label = exp.start_label;
    if (exp.is_trenary) {
        //int next_quad = cb->emit("br label @");
       // std::string fake_label = cb->genLabel();
       // cb->bpatch(cb->makelist(std::pair<int, BranchLabelIndex>(next_quad, FIRST)),fake_label);
       // cout<<"fake label"<<fake_label<<endl;
        exp.var = trenary_phi(exp);
    }
    std::string get_ptr = this->freshVar();
    //debugging var6:
    //std::cout << "value is " << exp.value << endl;
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
    exp.start_label = cb->genLabel();
    if (offset >= 0) {
      //  cout<<"the label is "<<exp.start_label<<endl; 
        std::string new_var = this->freshVar();
        std::string get_ptr = this->freshVar();
        cb->emit(get_ptr + " = getelementptr i32, i32* " + this->current_var_for_function + ", i32 " + std::to_string(offset));
        cb->emit(new_var + " = load i32, i32* " + get_ptr);
        exp.var = new_var;
    }
    else {
        exp.var = "%" + std::to_string((-1) * offset - 1);
    }
    
    if (exp.type == Bool_t) {
        E_var var = this->freshVar();
        this->cb->emit(var + " = " + "icmp eq i32 " + exp.var + ", 0");
        int next_instr = this->cb->emit("br i1 " + var + ", label @, label @");
        exp.falselist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, FIRST));
        exp.truelist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, SECOND));
    }
    else {
        int nextinstr = cb->emit("br label @");
        exp.nextlist = this->cb->makelist(std::pair<int, BranchLabelIndex>(nextinstr, FIRST));
    }
    return exp.var;
}

std::string CodeGen::emit_num_assign(Exp_c &new_exp, std::string var, std::string value)
{
    std::string marker = cb->genLabel();
    cb->emit(var + " = add i32 " + value + ", 0");
    int nextinstr = cb->emit("br label @");
    new_exp.nextlist = this->cb->makelist(std::pair<int, BranchLabelIndex>(nextinstr, FIRST));
    return marker;
}

void CodeGen::handle_string(Exp_c &exp, String_c &string)
{
    //cout<<"in handle string"<<endl;
    std::string var = freshVarGlobal();
    std::string size_to_emit = "[" + to_string(string.str.length() - 1) + " x i8]";
    std::string str = string.str;
    //not sure if value is needed
    exp.value = str;
    str.pop_back();
    std::string str_to_emit = "c" + str + "\\00";

    cb->emitGlobal(var + " = constant " + size_to_emit + " " + str_to_emit + "\"");

    std::string label = cb->genLabel();
    std::string for_print = var;
    std::replace(var.begin(), var.end(), '@', '%');
    exp.var = var;
    exp.start_label = label;
    cb->emit(var + " = getelementptr " + size_to_emit + ", " + size_to_emit + "* " + for_print + ", i32 0, i32 0");
    int nextinstr = cb->emit("br label @");
    exp.nextlist = this->cb->makelist(std::pair<int, BranchLabelIndex>(nextinstr, FIRST));
}

void CodeGen::deal_with_if(Statement_c &new_s, Exp_c& exp, Statement_c &s)
{
   // cout<<"in deal with if"<<endl;
    cb->bpatch(exp.truelist, s.start_label);
    new_s.start_label = exp.start_label;
    new_s.nextlist = cb->merge(exp.falselist, s.nextlist);
    if (!(s.breaklist.empty()))
    {
        new_s.breaklist = s.breaklist;
    }
    if (!(s.conlist.empty()))
    {
        new_s.conlist = s.conlist;
    }
    //cout<<"end deal with if"<<endl;
}

void CodeGen::handle_bool_explist(Exp_c &exp)
{
    //<<"in handle bool explist"<<endl;
    std::string true_label = cb->genLabel();
    cb->emit(freshVar() + " = zext i1 true to i32");
    int address1 = cb->emit("br label @");

    std::string false_label = cb->genLabel();
    cb->emit(freshVar() + " = zext i1 false to i32");
    int address2 = cb->emit("br label @");

    std::string next_label = cb->genLabel();
    InstrList next = cb->merge(cb->makelist(pair<int, BranchLabelIndex>(address1, FIRST)),
                               cb->makelist(pair<int, BranchLabelIndex>(address2, FIRST)));

    cb->bpatch(exp.truelist, true_label);
    cb->bpatch(exp.falselist, false_label);
    cb->bpatch(next, next_label);
    exp.var = freshVar();
    cb->emit(exp.var + " = phi i32 [ 1, %" + true_label + "], [0, %" + false_label + "]");
    int instr = cb->emit("br label @");
    exp.nextlist = cb->makelist(std::pair<int, BranchLabelIndex>(instr, FIRST));
}

void CodeGen::deal_with_call(Call_c& call, std::vector<Exp_c*>& expressions)
{
    //cout<<"in deal with call"<<endl;
    call.var = freshVar();
    /* Call starts with the "calculation" of it's first parameter */
    call.start_label = expressions[expressions.size() - 1]->start_label;
    std::string args_to_print = "";
    int i;
    for (i = expressions.size() - 1; i > 0 ; i--)
    {
        if (expressions[i]->type == String_t) {
            args_to_print += "i8* " + expressions[i]->var;
            args_to_print+= ", ";
        }
        else {
            if(expressions[i]->type == Bool_t)
            {
                handle_bool_explist(*(expressions[i]));
            }
            args_to_print += "i32 " + expressions[i]->var;
            args_to_print+= ", ";
        }
        /* Untill last Exp: */
        this->cb->bpatch(expressions[i]->nextlist, expressions[i-1]->start_label);
    }

    std::string label = cb->genLabel();
    int to_function = cb->emit("br label @");

    if (!expressions.empty()) {
        /* Last Expression */
         if (expressions[i]->type == String_t) {
            args_to_print += "i8* " + expressions[i]->var;
        }
        else {
            if(expressions[i]->type == Bool_t)
            {
                handle_bool_explist(*(expressions[i]));
            }
            args_to_print += "i32 " + expressions[0]->var;
        }
        cb->bpatch(expressions[0]->nextlist, label);
    }
    std::string label_function = cb->genLabel();
    cb->bpatch(cb->makelist(std::pair<int, BranchLabelIndex>(to_function, FIRST)), label_function);
    if (call.type == Void_t)
    {
        cb->emit("call void @" + call.name + "(" + args_to_print + ")");
    }
    else{
            cb->emit(call.var + " = call i32 @" + call.name + "(" + args_to_print + ")");
    }
    if(call.type == Bool_t)
    {
        std::string new_bool_var = freshVar();
        cb->emit(new_bool_var + " =icmp ne i32 0, " +call.var);
        call.var = new_bool_var;
        int next_instr = this->cb->emit("br i1 " + call.var + ", label @, label @");
        call.truelist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, FIRST));
        call.falselist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, SECOND));
    }
    else
    {
        int address = cb->emit("br label @");
        call.nextlist = cb->makelist(std::pair<int, BranchLabelIndex>(address, FIRST));
    }
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
            cb->emit(call.var + " = call i32 @" + call.name + "(" + args_to_print + ")");
    }
    if(call.type == Bool_t)
    {
        std::string new_bool_var = freshVar();
        cb->emit(new_bool_var + " =icmp ne i32 0, " +call.var);
        call.var = new_bool_var;
        int next_instr = this->cb->emit("br i1 " + call.var + ", label @, label @");
        call.truelist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, FIRST));
        call.falselist = this->cb->makelist(std::pair<int, BranchLabelIndex>(next_instr, SECOND));
    }
    else
    {
        int address = cb->emit("br label @");
        call.nextlist = cb->makelist(std::pair<int, BranchLabelIndex>(address, FIRST));
    }
}

void CodeGen::define_function(FuncDecl_c& func)
{
    std::string args_to_print = "";
    for (auto &decl : func.decls)
    {
        args_to_print += "i32 , ";
    }
    //cout<<func.name<<endl;
    if (!func.decls.empty())
        args_to_print.erase(args_to_print.size() -2);
    if (func.type == Void_t)
        cb->emit("define void @" + func.name + "(" + args_to_print + "){");
    else
        cb->emit("define i32 @" + func.name + "(" + args_to_print + "){");
}

void CodeGen::function_end(RetType_c& type, Statements_c &s, N_Marker &n_marker)
{
    cb->bpatch(n_marker.nextlist, s.start_label);
    std::string next_label = cb->genLabel();
    cb->bpatch(s.nextlist, next_label);
    if(type.type == Void_t)
        cb->emit("ret void");
    else
        cb->emit("ret i32 0");
    cb->emit("}");
}

void CodeGen::deal_with_return(Exp_c &exp, Statement_c &s)
{
    s.start_label = exp.start_label;
    InstrList next_list = exp.nextlist;
    // what? next list is before the ret is even typed?
    //cb->bpatch(s.nextlist, next_label);
    if(exp.type == Bool_t)
    {
        Exp_c* new_exp = bool_exp(exp);
        exp.var = new_exp->var;
        next_list = new_exp->nextlist;
    }
    std::string label = cb->genLabel();

    if (exp.type == String_t) {
        cb->emit("ret i8* " + exp.var);
    }
    else {
        cb->emit("ret i32 " + exp.var);
    }
    /* after expressions is calculated goto return code */
    cb->bpatch(next_list, label);
    /* Creating here a nextlist just to prevent segfaults on merges, since we assume all S have nextlists
        need to make sure it is backpatched though! */
    //int addr = cb->emit("br label @");
    //s.nextlist = cb->makelist(std::pair<int, BranchLabelIndex>(addr, FIRST));
    //Junk vector for trying
    int instr = cb->emit("br label @");
    s.nextlist = cb->makelist(std::pair<int, BranchLabelIndex>(instr, FIRST));
}

void CodeGen::deal_with_return(Statement_c &s)
{
    s.start_label =  cb->genLabel();
    cb->emit("ret void");
   // cout<<"in d.w.r the start label is "<<s.start_label<<endl;
    /* Creating here a nextlist just to prevent segfaults on merges, since we assume all S have nextlists
        need to make sure it is backpatched though! */
    //int addr = cb->emit("br label @");

    //Junk vector for trying
    int instr = cb->emit("br label @");
    s.nextlist = cb->makelist(std::pair<int, BranchLabelIndex>(instr, FIRST));
}

void CodeGen::deal_with_break(Statement_c &s)
{
    s.start_label =  cb->genLabel();
  //  cout<<"in break"<<endl;
    int next_quad = cb->emit("br label @");
    s.breaklist = cb->makelist(std::pair<int, BranchLabelIndex>(next_quad, FIRST));
    s.is_break = true;
  //  cout<<"end break"<<endl;
}

void CodeGen::handle_continue(Statement_c &s)
{
    std::string label = cb->genLabel();
    s.start_label = label;
    /* Will be backpatched to the condition of the while */
    int address = cb->emit("br label @");
    s.conlist = cb->makelist(std::pair<int, BranchLabelIndex>(address, FIRST));
    
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
    if (!(s.conlist.empty()))
    {
        cb->bpatch(s.conlist, exp.start_label);
    }
    cb->bpatch(exp.truelist, s.start_label);
    //labels_while.pop();
    if (!(s.breaklist.empty())){
        /* Will be backpatched to start label of next statement */
        /* Todo: what if last statement in scope - maybe boolean (was backpatched)*/
        new_s.nextlist = cb->merge(exp.falselist, s.breaklist);
    }
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

    if (!(s1.breaklist.empty()))
    {
        if (!(s2.breaklist.empty()))
            new_s.breaklist = cb->merge(s1.breaklist, s2.breaklist);
        new_s.breaklist = s1.breaklist;
    }
    else
        if (!(s2.breaklist.empty()))
            new_s.breaklist = s2.breaklist;
    
    if (!(s1.conlist.empty()))
    {
        if (!(s2.conlist.empty()))
            new_s.conlist = cb->merge(s1.conlist, s2.conlist);
        new_s.conlist = s1.conlist;
    }
    else
        if (!(s2.conlist.empty()))
            new_s.conlist = s2.conlist;

    
    new_s.start_label = exp.start_label;
    new_s.nextlist = cb->merge(s1.nextlist, s2.nextlist);
}

void CodeGen::call_as_statement(Call_c& call, Statement_c &s)
{
    //cout<<"in call as sta"<<endl;
    s.start_label = call.start_label;
    if(call.type == Bool_t)
    {
        s.nextlist = cb->merge(call.falselist, call.truelist);
    }
    else
        s.nextlist = call.nextlist;
  //  cout <<"s.start_label from call is " << s.start_label << endl;
}

 void CodeGen::merge_statement_lists(Statement_c &s, Statement_c &s_list)
 {
    s_list.nextlist = cb->merge(s.nextlist, s_list.nextlist);
 }

 void CodeGen::handle_first_statement(Statements_c &statements, Statement_c &s)
 {
   // cout<<"in first handle sta"<<endl;
    statements.start_label = s.start_label;
    //std::cout << "before nlist -> crushing on statements.nextlist" << endl;
    //std::cout << s.nextlist.size() << endl;
    //std::cout << s.start_label << endl;
    statements.nextlist = s.nextlist;
    //std::cout << "after func" << endl;
    if (!(s.breaklist.empty()))
    {
        statements.breaklist = s.breaklist;
    }
    if (!(s.conlist.empty()))
    {
        statements.conlist = s.conlist;
    }
 }

 void CodeGen::handle_statements(Statements_c &new_statements, Statement_c &s)
 {
    //cout<<"in handle sta"<<endl;
    /* start_label already assigned because $$ = $1 was done */
    /* next list of last statement in list, will point to start label of new statement */
    //cb->printCodeBuffer();
   // cb->emit("START LABEL OF S" + s.start_label);
   // cout<<s.nextlist.front().first<<endl;
    cb->bpatch(new_statements.nextlist, s.start_label);
    new_statements.s_list.push_back(&s);
    /* next list of the vector is the next list of last statement */
    new_statements.nextlist = s.nextlist;
    if (!(s.breaklist.empty()))
    {
        if (!(new_statements.breaklist.empty()))
            new_statements.breaklist = cb->merge(new_statements.breaklist, s.breaklist);
        else
            new_statements.breaklist = s.breaklist;
    }
    if (!(s.conlist.empty()))
    {
        if (!(new_statements.conlist.empty()))
            new_statements.conlist = cb->merge(new_statements.conlist, s.conlist);
        else
        new_statements.conlist = s.conlist;
    }
 }

 void CodeGen::handle_statement_close(Statement_c &new_s, Statements_c &statements)
 {
    //cout<<"in close sta"<<endl;
    new_s.start_label = statements.start_label;
    new_s.nextlist = statements.nextlist;
    if (!(statements.breaklist.empty()))
    {
        new_s.breaklist = statements.breaklist;
    }
     if (!(statements.conlist.empty()))
    {
        new_s.conlist = statements.conlist;
    }
 }

Exp_c* CodeGen::bool_exp(Exp_c &exp) {
    if (exp.type != Bool_t) {
        return new Exp_c(exp);
    }
    Exp_c *new_exp = new Exp_c(Bool_t);
    new_exp->var = freshVar();
    std::string true_label = "TRUE_LABEL" + std::to_string(count_label);
    std::string false_label = "FALSE_LABEL" + std::to_string(count_label);
    std::string next_label = "NEXT_LABEL" + std::to_string(count_label);
    count_label++;
    //cb->emit("br label %" + true_list);
    cb->emit(true_label + ":");
    int address1 = cb->emit("br label @");
    
    cb->emit(false_label + ":");
    int address2 = cb->emit("br label @");

    cb->emit(next_label + ":");

    InstrList next = cb->merge(cb->makelist(pair<int, BranchLabelIndex>(address1, FIRST)),
                               cb->makelist(pair<int, BranchLabelIndex>(address2, FIRST)));

    cb->bpatch(exp.truelist, true_label);
    cb->bpatch(exp.falselist, false_label);
    cb->bpatch(next, next_label);
    cb->emit(new_exp->var + " = phi i32 [ 1, %" + true_label + "], [0, %" + false_label + "]");
    int address = cb->emit("br label @");
    new_exp->nextlist = cb->makelist(pair<int, BranchLabelIndex>(address, FIRST));
    return new_exp;
}

std::string CodeGen::trenary_phi(Exp_c &exp)
{
    std::string var = freshVar();
    cb->emit(var + " = phi i32 [ 1, %" + exp.true_label + "], [0, %" + exp.false_label + "]");
    return var;
}

void CodeGen::emit_start()
{
    cb->emit("declare i32 @printf(i8*, ...)");
    cb->emit("declare void @exit(i32)");
    cb->emit("@.int_specifier = constant [4 x i8] c\"%d\\0A\\00\"");
    cb->emit("@.str_specifier = constant [4 x i8] c\"%s\\0A\\00\"");
    cb->emit("@.zero_division = constant [23 x i8] c\"Error division by zero\\00\"");
    cb->emit("");
    cb->emit("define void @printi(i32) {");
    cb->emit("  %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0");
    cb->emit("  call i32 (i8*, ...) @printf(i8* %spec_ptr, i32 %0)");
    cb->emit("  ret void");
    cb->emit("}");
    cb->emit("");
    cb->emit("define void @print(i8*) {");
    cb->emit("  %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0");
    cb->emit("  call i32 (i8*, ...) @printf(i8* %spec_ptr, i8* %0)");
    cb->emit("  ret void");
    cb->emit("}");
    cb->emit("");
    cb->emit("define void @division_by_zero(i32) {");
    cb->emit("  %is_zero = icmp eq i32 %0, 0");
    cb->emit("  br i1 %is_zero, label %error_label, label %ok_label");
    cb->emit("  ok_label:");
    cb->emit("      ret void");
    cb->emit("  error_label:");
    cb->emit("      %ptr = getelementptr [23 x i8], [23 x i8]* @.zero_division, i32 0, i32 0");
    cb->emit("      call void @print(i8* %ptr)");
    cb->emit("      call void @exit(i32 0)");
    cb->emit("      ret void");
    cb->emit("}");
    cb->emit("");
}

