#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>

#ifdef _WIN32
    #ifdef _WIN64
        #define _INT __int64
        #define _INTFMT "%lld "
        #define _ATOI(val)    _atoi64(val)
    #else
        #define _INT int
        #define _INTFMT "%d "
        #define _ATOI(val)    atoi(val)
    #endif
#else
    #ifdef __x86_64__
        #define _INT long
        #define _INTFMT "%lld "
    #elif __i386__
        #define _INT int
        #define _INTFMT "%d "
    #endif
    #define _ATOI(val)    strtoll(val, NULL, 10)
#endif

using namespace std;


class LVecMap
{
public:
    
    void clear(){ m_vecInfo.clear(); }
    
    int size(){ return (int)m_vecInfo.size(); }
    
    int end(){ return -1; }
    
    int find(const string& key)
    {
        size_t i = 0;
        for (; i < m_vecInfo.size(); i++)
        {
            if (m_vecInfo[i].first == key)
                break;
        }
        if (i < m_vecInfo.size())
            return (int)i;

        return -1;
    }
    
    string& operator [](const string& key)
    {
        int i = find(key);
        if (i > -1)
        {
            return m_vecInfo[i].second;
        }
        else
        {
            pair<string, string> p(key, "");
            m_vecInfo.push_back(p);
            return m_vecInfo[m_vecInfo.size() - 1].second;
        }
    }
    
    vector<pair<string, string>>& map(){ return m_vecInfo; }
private:
    vector<pair<string, string>> m_vecInfo; 
};

class LVariable
{
public:
    enum VARTYPE
    { 
        EMPTY,      //空类型
        NONE,       //无类型
        STRING,     //字符串
        INT,        //整数
        FLOAT,      //浮点
        ARRAY,      //数组
        DICT,       //字典
        POINTER,    //指针
        CLASS       //类
    };
    
    enum TAGTYPE
    { 
        NORMAL,      //正常状态
        BREAK,       //中断状态
        RETURN,      //返回状态
        CONTINUE,    //继续状态
        ERR,         //错误状态
        ERR_DIVZERO  //除零错误
    };

    LVariable();
    
    LVariable(const LVariable& cv);
    
    LVariable& operator =(const LVariable& cv);
    
    LVariable(const string& value);
    
    LVariable(_INT value);
    
    LVariable& setDouble(double value);
    
    LVariable& setError();
    
    ~LVariable();
    
    void reset();
    
    void setTag(TAGTYPE tag){ m_tag = tag; }
    
    TAGTYPE tag() const { return m_tag; }
    
    void setType(VARTYPE type){ m_type = type; }
    
    VARTYPE type() const { return m_type; }
    
    _INT intValue() const { return m_nValue; }
    
    void setInt(_INT value){ m_type = INT; m_nValue = value; }
    
    string& strValue() { return m_strValue; }
    
    const string& strValue() const { return m_strValue; }
    
    double floatValue() const { return m_fValue; }
    
    vector<LVariable>* arrValue(){ return m_pVecValue; }
    
    void setArray(_INT arrSize, vector<LVariable>* pArray = NULL);
    
    map<LVariable, LVariable>* dictValue(){ return m_pMapValue; }
    
    void setDict(map<LVariable, LVariable>* pDict=NULL);
    
    LVecMap& info(){ return m_mapInfo; }
    
    void setInfo(LVecMap& info);
    
    vector<LVariable>& index(){ return m_vecIndex; }
    
    void setIndex(const vector<LVariable>& index){ m_vecIndex = index; }
    
    void* pointerValue() const { return m_pPointer; }
    
    void setPointer(void* pPointer){ m_pPointer = pPointer; }
    
    void initPointerRef(void* pPointer);
    
    LVariable operator +(const LVariable & right);
    LVariable operator -(const LVariable & right);
    LVariable operator *(const LVariable & right);
    LVariable operator /(const LVariable & right);
    LVariable operator %(const LVariable & right);
    LVariable operator ==(const LVariable & right);
    LVariable operator !=(const LVariable & right);
    LVariable operator >=(const LVariable & right);
    LVariable operator <=(const LVariable & right);
    LVariable operator >(const LVariable & right);
    LVariable operator <(const LVariable & right);
    LVariable operator &&(const LVariable & right);
    LVariable operator ||(const LVariable & right);

    LVariable operator &(const LVariable & right);
    LVariable operator |(const LVariable & right);
    LVariable operator ^(const LVariable & right);
    LVariable operator ~();
    LVariable operator <<(const LVariable & right);
    LVariable operator >>(const LVariable & right);
    
    bool operator<(const LVariable& right) const;

private:
    
    void IncreaseRefCount();
    
    void DecreaseRefCount();
    
    static map<vector<LVariable>*, int> _mapArraysRefCount;
    static map<map<LVariable, LVariable>*, int> _mapDictsRefCount;
    static map<void*, int> _mapPointersRefCount;

    TAGTYPE m_tag; 
    VARTYPE m_type; 

    string m_strValue; 
    _INT m_nValue; 
    double m_fValue; 
    void* m_pPointer; 
    vector<LVariable>*  m_pVecValue; 
    map<LVariable, LVariable>* m_pMapValue; 
    vector<LVariable>  m_vecIndex; 
    LVecMap m_mapInfo; 
};