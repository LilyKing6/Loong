#pragma once
#include <vector>
#include <map>
#include "Variable.h"

using namespace std;

class ActivationRecord
{
public:
    ActivationRecord();

    ActivationRecord(string name, string type, int level);

    string name() const { return m_name; }

    string type() const { return m_type; }

    void set_value(const string& key, const LVariable& value);

    LVariable& get_value(const string& key);
    
    void set_array_value(const string& key, const LVariable& value, _INT arr_index);
    
    LVariable& get_array_value(const string& key, _INT arr_index);
    
    void set_dict_value(const string& key, const LVariable& value, const LVariable& dict_index);
    
    LVariable& get_dict_value(const string& key, const LVariable& dict_index);
    
    LVariable::VARTYPE get_vartype(const string& key);
    
    void create_global(const LVariable& globalValue, const vector<LVariable>& vecArgv, const string& argvName);
    
    LVariable& get__global_value(const string& var_name);
    
    void set_global_value(const string& var_name, const LVariable& value);
    
    LVariable::VARTYPE get_global_vartype(const string& var_name);
private:
    
    map<string, LVariable> m_members;
    
    string m_name;
    
    string m_type;
    
    int m_level;
};


class LCallStack
{
public:
    
    LCallStack();
    
    ~LCallStack();
    
    void pop();
    
    ActivationRecord& peek();
    
    ActivationRecord& base();
    
    void push(const ActivationRecord& ar);
private:
    
    vector<ActivationRecord> m_stack;
};


