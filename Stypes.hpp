//
// Created by Gilad on 02/12/2022.
//

#ifndef COMPI_HW3_STYPES_H
#define COMPI_HW3_STYPES_H

#include <iostream>
#include <vector>
#include "string"
#include "bp.hpp"
#define NO_OFFSET -1
#define YYSTYPE Node*


enum type_enum {
    Int_t,
    Bool_t,
    Byte_t,
    String_t,
    Void_t,
    None_t
};

typedef std::string E_var;
typedef std::vector<std::pair<int,BranchLabelIndex>> InstrList;

class Node {
public:
     virtual ~Node() {};
};

class Type_c : public Node {
public:
    type_enum type;

    Type_c(type_enum type) : type(type) {};
};

class RetType_c : public Node {
public:
    type_enum type;

    RetType_c(type_enum type) : type(type) {};
};

class Call_c : public Node {
public:
    type_enum type;
    const std::string name;
    E_var var;
    std::string start_label;
    InstrList truelist;
    InstrList falselist;
    InstrList nextlist;
    Call_c(type_enum type1, const std::string name) : type(type1), name(name) {};
};

class Exp_c : public Node {
public:
    type_enum type;
    E_var var;
    InstrList truelist;
    InstrList falselist;
    InstrList nextlist;
    std::string start_label;
    std::string value;
    
    bool is_trenary = false;
    std::string true_label;
    std::string false_label;

    Exp_c(type_enum type, E_var var) : type(type), var(var){}
    Exp_c(type_enum type) : type(type){}
    
};

class ExpList_c : public Node {
public:
    std::vector<Exp_c*> expressions;
    ExpList_c(const std::vector<Exp_c*>& expressions) : expressions(expressions) {};
};

class FormalDecl_c : public Node {
public:
    type_enum type;
    const std::string name;
    E_var var;
    FormalDecl_c(type_enum type, const std::string& name, E_var var) : type(type), name(name), var(var){};
};

class FuncDecl_c : public Node{
    public:
    type_enum type;
    std::string name;
    std::vector<FormalDecl_c*> decls;
    InstrList nextlist;
    
    FuncDecl_c(type_enum type , const std::vector<FormalDecl_c*>& decls, std::string name) :
    type(type), decls(decls), name(name) {};
};

class FormalsList_c: public Node {
public:
    std::vector<FormalDecl_c*> decls;

    FormalsList_c(const std::vector<FormalDecl_c*>& decls) : decls(decls) {};
};

class Formals_c : public Node {
public:
    std::vector<FormalDecl_c*> decls;
    Formals_c(){};
    Formals_c(const std::vector<FormalDecl_c*>& decls) : decls(decls) {};
};

class Relop_c : public Node {
public:
    const std::string relop_txt;
    Relop_c(const std::string relop_txt) : relop_txt(relop_txt){};
};

class String_c : public Node {
public:
    const std::string str;
    String_c(const std::string str) : str(str){};
};

class ID_c : public Node {
public:
    const std::string name;
    E_var var;
    type_enum type = None_t;
    ID_c(const std::string name) : name(name){};
};

class Num_c : public Node {
public:
    const std::string num_str;
    type_enum type = Int_t;
    Num_c(const std::string num_str) : num_str(num_str){};
};

class Marker : public Node {
public:
    std::string label;

    Marker(const std::string label) : label(label) {}; 

    void print_m()
    {
        cout<<"this is the label"<<label<<endl;
    }
};

class N_Marker : public Node {
public:
    std::string label;
    InstrList nextlist;

    N_Marker(InstrList nextlist){
        this->nextlist = nextlist;
    }; 
};

class Statement_c : public Node {
public:
    InstrList nextlist;
    /* Used for break statements in while */
    InstrList breaklist;
    std::string start_label;
    bool was_backpatched = false;
    bool is_break = false;
    Statement_c(){};
};

class Statements_c : public Node {
public:
    InstrList nextlist;
    std::string start_label;
    std::vector<Statement_c*> s_list;
    InstrList breaklist;
    Statements_c(const std::vector<Statement_c*>& s_list) : s_list(s_list) {};
    Statements_c(std::string start_label, const std::vector<Statement_c*>& s_list) : start_label(start_label), s_list(s_list) {};

};

bool checkBoolExp(Exp_c& exp);
bool checkBoolExp(Exp_c& exp1, Exp_c& exp2);
bool checkTypeExp(Type_c& type, Exp_c& exp);
bool checkTypeExpId(type_enum type, Exp_c& exp);
bool isDec(ID_c *id);
std::string typeToString(type_enum type);
type_enum checkNumType(Exp_c& exp1, Exp_c& exp2);
type_enum checkSameTypeExp(Exp_c& exp1 , Exp_c& exp2);
type_enum checkAssigment(type_enum type, Exp_c& exp1);

void merge_lists(Exp_c& exp, Call_c& call);



#endif //COMPI_HW3_STYPES_H
