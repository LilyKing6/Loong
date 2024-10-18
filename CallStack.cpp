#include "CallStack.h"
#include "Token.h"

static LVariable _emptyVar = LVariable();

ActivationRecord::ActivationRecord()
{
}

ActivationRecord::ActivationRecord(string name, string type, int level)
{
    m_name = name;
    m_type = type;
    m_level = level;
}

void ActivationRecord::set_value(const string& key, const LVariable& value)
{ 
    m_members[key] = value;
}

LVariable& ActivationRecord::get_value(const string& key)
{
    if (m_members.find(key) != m_members.end())
        return m_members[key];
    return _emptyVar;
}

void ActivationRecord::set_array_value(const string& key, const LVariable& value, _INT arr_index)
{
    if (arr_index >= 0)
    {
        if (m_members.find(key) != m_members.end())
        {
            if (m_members[key].type() == LVariable::ARRAY)
            {
                vector<LVariable>* arr = m_members[key].arrValue();
                if (arr == NULL)
                    return;
                if (arr_index >= 0 && (size_t)arr_index < arr->size())
                    (*arr)[arr_index] = value;
            }
            else if (m_members[key].type() == LVariable::STRING)
            {
                string& str = m_members[key].strValue();
                if (arr_index >= 0 && (size_t)arr_index < str.size())
                {
                    if (value.type() == LVariable::STRING && value.strValue().size() == 1)
                        str[arr_index] = value.strValue()[0]; 
                }
            }
        }
    }
}

LVariable& ActivationRecord::get_array_value(const string& key, _INT arr_index)
{
    if (arr_index >= 0)
    {
        if (m_members.find(key) != m_members.end())
        {

            if (m_members[key].type() == LVariable::STRING)
            {
                const string& strValue = m_members[key].strValue();

                if (arr_index >= 0 && (size_t)arr_index < strValue.size())
                {
                    static LVariable charVar = LVariable();
                    charVar = LVariable(string(1, strValue[arr_index]));
                    return charVar;
                }
            }

            else if (m_members[key].type() == LVariable::ARRAY)
            {
                vector<LVariable>* arr = m_members[key].arrValue();

                if (arr == NULL)
                    return _emptyVar;

                if (arr_index >= 0 && (size_t)arr_index < arr->size())
                    return (*arr)[arr_index];
            }
        }
    }

    return _emptyVar;
}


LVariable::VARTYPE ActivationRecord::get_vartype(const string& key)
{
    if (m_members.find(key) != m_members.end())
        return m_members[key].type();
    return LVariable::EMPTY;
}

void ActivationRecord::set_dict_value(const string& key, const LVariable& value, const LVariable& dict_index)
{
    if (m_members.find(key) != m_members.end())
    {
        map<LVariable, LVariable>* dict = m_members[key].dictValue();
        if (dict == NULL)
            return;
        (*dict)[dict_index] = value;
    }
}

LVariable& ActivationRecord::get_dict_value(const string& key, const LVariable& dict_index)
{
    if (m_members.find(key) != m_members.end())
    {
        map<LVariable, LVariable>* dict = m_members[key].dictValue();
        if (dict == NULL)
            return _emptyVar;
        if (dict->find(dict_index) != dict->end())
            return (*dict)[dict_index];
    }

    return _emptyVar;
}

void ActivationRecord::create_global(const LVariable& globalValue, const vector<LVariable>& vecArgv, const string& argvName)
{
    LVariable global_dict;
    global_dict.setDict();
    set_value(GLOBAL_DICT_NAME, global_dict);
    if (globalValue.type() == LVariable::DICT)
        set_value(GLOBAL_DICT_NAME, globalValue);

    LVariable argv_array;
    argv_array.setArray(0);
    for (size_t i = 0; i < vecArgv.size(); i++)
        argv_array.arrValue()->push_back(vecArgv[i]);
    if (argvName.size() > 0)
        set_global_value(argvName, argv_array);
    else
        set_global_value(ARGV_ARRAY_NAME, argv_array);
}

LVariable& ActivationRecord::get__global_value(const string& var_name)
{
    LVariable& global_dict = get_value(GLOBAL_DICT_NAME);
    if (global_dict.type() == LVariable::DICT)
    {
        if (global_dict.dictValue()->find(var_name) != global_dict.dictValue()->end())
            return (*global_dict.dictValue())[var_name];
    }

    return _emptyVar;
}

void ActivationRecord::set_global_value(const string& var_name, const LVariable& value)
{
    set_dict_value(GLOBAL_DICT_NAME, value, var_name);
}

LVariable::VARTYPE ActivationRecord::get_global_vartype(const string& var_name)
{
    LVariable& global_value = get__global_value(var_name);
    return global_value.type();
}

LCallStack::LCallStack()
{

}

LCallStack::~LCallStack()
{

}

void LCallStack::pop()
{ 
    if (m_stack.size() == 0)
        return;

    m_stack.pop_back();
}

void LCallStack::push(const ActivationRecord& ar)
{ 
    m_stack.push_back(ar);
}

ActivationRecord& LCallStack::peek()
{
    static ActivationRecord emptyAR;
    if (m_stack.size() == 0)
        return emptyAR;

    return m_stack[m_stack.size() - 1];
}

ActivationRecord& LCallStack::base()
{
    static ActivationRecord emptyAR;
    if (m_stack.size() == 0)
        return emptyAR;

    return m_stack[0];
}
