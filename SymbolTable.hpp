#ifndef COMPI_HW3_SYMBOL_TABLE_HPP
#define COMPI_HW3_SYMBOL_TABLE_HPP

#include <stack>
#include "Stypes.hpp"
#include "hw3_output.hpp"
#include <algorithm>
extern int yylineno;
using namespace output;

class TableEntry{
public:
    const std::string name;
    type_enum type;
    int offset;
    bool isfunction;
    std::vector<std::string> argtypes;
    TableEntry(const std::string& name, type_enum type, int offset, bool isfunction = false) : name(name), type(type), offset(offset), isfunction(isfunction){};
    
};

class Table{
public:
    std::vector<TableEntry*> entry_list;
    Table* parent;
    bool iswhile;
    Table(Table* parent, bool iswhile) : parent(parent) , iswhile(iswhile){};
    void insert(TableEntry *entry);
    bool contains(std::string name);
};

class SymbolTable {
public:
    std::stack<Table*> tables;
    std::stack<int> offsets;
    std::string function_name_scope = "";

    SymbolTable() = default;
    /* Generates the symbol table if it doesn't exist. otherwise returns the instance */
    static SymbolTable *getSymTable();
    /* Creates a new scope and returns a pointer to the new table added */
    Table* newScope(bool iswhile);
    /* Closes current scope */
    void closeScope();
    /* Maketable makes a table which points to it's parent */
    Table* makeGlob();
    Table *makeTable(Table *parent, bool iswhile);
    void insert(Table *table, const std::string& name, type_enum type, int offset, bool isfunc = false);
    void addFunction(Table *table, const std::string& name, type_enum type, int offset);
    bool isFirstNoneParamInCurScope(Table* table);
    void push_t(Table& table);
    void push_o(int offset);
    Table pop_t();
    int pop_o();
    Table top_t();
    int top_o();

    bool compareByteInt(std::string type1, std::string type2);
    bool isDec(const std::string& name, bool function);
    bool isAlreadyDecInScope(const std::string& name);
    void addFunctionParam(const FormalDecl_c& decl, int offset);
    void checkFunctionParams(std::vector<Exp_c*>& expressions, const std::string& name);
    void checkFunctionParams(const std::string& name);
    bool inScopeWhile(Table *table);
    bool checkSamefunctionReturnType(type_enum type, bool is_void);
    type_enum getTypeByName(const std::string& name);
    bool isFunctionAlreadyDecInScope(const std::string& name);
    void isMainExist();
    
};


#endif