//
// Created by Gilad on 02/12/2022.
//

#include "Stypes.hpp"

bool checkBoolExp(Exp_c& exp) {
    return exp.type==Bool_t;
}
bool checkBoolExp(Exp_c& exp1, Exp_c& exp2) {
    return exp1.type==Bool_t && exp2.type == Bool_t;
}
bool checkTypeExp(Type_c& type, Exp_c& exp) {
     if (type.type==exp.type)
        return true;
        
    if(type.type == Int_t && exp.type == Byte_t)
        return true;
    return false;
}

bool checkTypeExpId(type_enum type, Exp_c& exp) {
    if (type==exp.type)
        return true;
    if(type == Int_t && exp.type == Byte_t)
        return true;
    return false;
}

type_enum checkSameTypeExp(Exp_c& exp1 , Exp_c& exp2)
{
    if (exp1.type == exp2.type)
        return exp1.type;
    if (exp1.type == Int_t && exp2.type == Byte_t)
        return exp1.type;
    if (exp1.type == Byte_t && exp2.type == Int_t)
        return exp2.type;
    return None_t;
}
type_enum checkNumType(Exp_c& exp1 , Exp_c& exp2)
{
    if (exp1.type == Int_t && exp2.type == Int_t)
        return Int_t;
    if (exp1.type == Byte_t && exp2.type == Byte_t)
        return Byte_t;
    if (exp1.type == Int_t && exp2.type == Byte_t)
        return Int_t;
    if (exp1.type == Byte_t && exp2.type == Int_t)
        return Int_t;
    return None_t;
    
}

type_enum checkAssigment(type_enum type, Exp_c& exp1)
{
    if(type == Int_t && (exp1.type == Byte_t || exp1.type == Int_t))
        return Int_t;
    if(type == Byte_t && (exp1.type == Int_t || exp1.type == Byte_t))
        return Byte_t;
    return None_t;
}

std::string typeToString(type_enum type)
{
    switch (type)
    {
    case Int_t:
        return "INT";
        break;
    case Bool_t:
        return "BOOL";
        break;
    case Byte_t:
        return "BYTE";
        break;
    case String_t:
        return "STRING";
        break;;
    case Void_t:
        return "VOID";
        break;
    default:
        break;
    }
    return "";
}

