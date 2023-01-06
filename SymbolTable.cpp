#include "SymbolTable.hpp"

SymbolTable* SymbolTable::getSymTable() {
    static SymbolTable sym_table;
    return &sym_table;
}

Table* SymbolTable::makeGlob() {
    this->offsets.push(-1);
    Table *new_table = this->makeTable(nullptr, false);
    this->tables.push(new_table);
    /* Add print and printi functions */
    this->insert(this->tables.top(), "print", Void_t, NO_OFFSET, true);//chane to void type
    this->tables.top()->entry_list.back()->argtypes.push_back("STRING");
    this->insert(this->tables.top(), "printi", Void_t, NO_OFFSET, true);
    this->tables.top()->entry_list.back()->argtypes.push_back("INT");
    return new_table;
}

Table* SymbolTable::newScope(bool iswhile) {
    this->offsets.push(this->offsets.top());
    Table *new_table = this->makeTable(this->tables.top(), iswhile);
    this->tables.push(new_table);
    return new_table;
}

void SymbolTable::closeScope() {
    output::endScope();
    Table* table_out = this->tables.top();
    for (auto & entry : table_out->entry_list)
    {
        if(!entry->isfunction){
            output::printID(entry->name, entry->offset, typeToString(entry->type));
        }
        else{
            output::printID(entry->name, 0, output::makeFunctionType(typeToString(entry->type),  entry->argtypes));
        }
        this->offsets.pop();
    }
    this->offsets.pop();
    this->tables.pop();
}


Table* SymbolTable::makeTable(Table* parent, bool iswhile) {
    Table *table = new Table(parent, iswhile);
    return table;
}

void SymbolTable::insert(Table *table, const std::string& name, type_enum type, int offset, bool isfunc) {
    if (!isfunc && offset > 0 && this->isFirstNoneParamInCurScope(table)) {
        offset--;
        }
    TableEntry *new_entry = new TableEntry(name, type, offset, isfunc);
    table->insert(new_entry);
    this->offsets.push(offset);
}

bool SymbolTable::isFirstNoneParamInCurScope(Table* table) {
    //if (not global && not mekoonan && (empty or just params above it))
    if (table->parent !=nullptr && table->parent->parent == nullptr && (table->entry_list.empty() || table->entry_list.back()->offset < 0))
        return true;
    return false;
}

void SymbolTable::addFunction(Table *table, const std::string& name, type_enum type, int offset)
{
    TableEntry *new_entry = new TableEntry(name, type, offset, true);
    table->parent->insert(new_entry);
}

void SymbolTable::addFunctionParam(const FormalDecl_c& decl, int offset) {
    // while entering function params, the top of the parent table is the symbol of the funciton itself
    TableEntry *function_sym = this->tables.top()->parent->entry_list.back();
    this->insert(this->tables.top(), decl.name, decl.type, offset);
    function_sym->argtypes.push_back(typeToString(decl.type));
}

bool SymbolTable::compareByteInt(std::string type1, std::string type2)
{
    if((!type1.compare("BYTE")) && (!type2.compare("INT")))
        return true;
    if((!type2.compare("BYTE")) && (!type1.compare("INT")))
        return false;
    return false;
}


bool SymbolTable::checkFunctionParams(std::vector<Exp_c*>& expressions, const std::string& name)
{
    Table* curr_table = this->tables.top();
    TableEntry* table_entry;
    while (curr_table != nullptr) {
        for (auto & entry : curr_table->entry_list) {
            if (entry->name == name) {
                table_entry = entry;
                break;
            }
        }
        curr_table = curr_table->parent;
    }
    if(table_entry->argtypes.size() != expressions.size())
    {
        errorPrototypeMismatch(yylineno, name, table_entry->argtypes);
        exit(1);
    }
    std::vector<Exp_c*> expressions_reverse(expressions);
    reverse((&expressions_reverse)->begin(),(&expressions_reverse)->end());
    int i = 0;
    for(auto argtype : table_entry->argtypes)
    {
        if(typeToString(expressions_reverse[i]->type).compare(argtype))
        {  
            if(!compareByteInt(typeToString(expressions_reverse[i]->type),argtype))
            {
                errorPrototypeMismatch(yylineno, name, table_entry->argtypes);
                exit(1);
            }
        }
        i++;
    }

}


bool SymbolTable::checkFunctionParams(const std::string& name)
{
    Table* curr_table = this->tables.top();
    TableEntry* table_entry;
    while (curr_table != nullptr) {
        for (auto & entry : curr_table->entry_list) {
            if (entry->name == name) {
                table_entry = entry;
                break;
            }
        }
        curr_table = curr_table->parent;
    }
    if(table_entry->argtypes.size() !=0)
    {
        errorPrototypeMismatch(yylineno, name, table_entry->argtypes);
        exit(1);
    }
}



bool SymbolTable::isDec(const std::string& name, bool function) {
    bool found = false;
    Table* curr_table = this->tables.top();
    while (curr_table != nullptr) {
        for (auto & entry : curr_table->entry_list) {
            if (entry->name == name) {
                if(function == entry->isfunction)
                {
                    found = true;
                }
            else{
                found = false;
            }
                break;
            }
        }
        if (found)
            break;
        curr_table = curr_table->parent;
    }
    return found;
}

bool SymbolTable::isAlreadyDecInScope(const std::string& name) {
    Table* curr_table = this->tables.top();
    bool found = false;
    while (curr_table != nullptr) {
        for (auto & entry : curr_table->entry_list) {
            if (entry->name == name) {
                found = true;
                break;
            }
        }
        curr_table = curr_table->parent;
    }   
    return found;
}


bool SymbolTable::isFunctionAlreadyDecInScope(const std::string& name) {
    Table* curr_table = this->tables.top()->parent;
    bool found = false;
    while (curr_table != nullptr) {
        for (auto & entry : curr_table->entry_list) {
            if (entry->name == name) {
                found = true;
                break;
            }
        }
        curr_table = curr_table->parent;
    }   
    return found;
}

bool SymbolTable::inScopeWhile(Table *table)
{
    Table* curr_table = table;
    bool found = false;
    while (curr_table != nullptr) {
        if(curr_table->iswhile)
        {
            return true;
        }
        curr_table = curr_table->parent;
    }
    return false;
}

bool SymbolTable::checkSamefunctionReturnType(type_enum type, bool is_void)
{
    if(this->function_name_scope == "")
        return false;
    Table* curr_table = this->tables.top()->parent;
    TableEntry* table_entry;
    while (curr_table != nullptr) {
        for (auto & entry : curr_table->entry_list) {
            if (entry->name == this->function_name_scope) {
                table_entry = entry;
                break;
            }
        }
        curr_table = curr_table->parent;
    }
    type_enum type1 = table_entry->type;
    if(type1 == Void_t && is_void)
        return true;
    if(type1 == Void_t && !is_void)
        return false;
    if(type1 == type)
        return true;
    if(type1 == Int_t && type == Byte_t)
        return true;
    return false;
}

type_enum SymbolTable::getTypeByName(const std::string& name)
{
    Table* curr_table = this->tables.top();
    while (curr_table != nullptr) {
        for (auto & entry : curr_table->entry_list) {
            if (entry->name == name) {
                return entry->type;
            }
        }
        curr_table = curr_table->parent;
    }
    return None_t;
}

void Table::insert(TableEntry *entry)
{
    this->entry_list.push_back(entry);
}

bool SymbolTable::isMainExist()
{
    Table* curr_table = this->tables.top();
    TableEntry* table_entry = nullptr;
    while (curr_table != nullptr) {
        for (auto & entry : curr_table->entry_list) {
            if (entry->name == "main") {
                table_entry = entry;
                break;
            }
        }
        curr_table = curr_table->parent;
    }
    if(table_entry == nullptr || !table_entry->isfunction || table_entry->argtypes.size() != 0 || table_entry->type != Void_t)
    {
        errorMainMissing();
        exit(1);
    }
}


