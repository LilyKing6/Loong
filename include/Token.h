/*
License for Loong

Copyright 2024 Lily King

All Rights Reserved

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE. 
*/

#pragma once
#include <string>
#include <map>
using namespace std;

#define GLOBAL_DICT_NAME "__G__"

#define ARGV_ARRAY_NAME "__ARGV__"

enum KEYWORD 
{
    PROGRAM,       // 程序
    BEGIN,         // 开始
    END,           // 结束

    // 条件语句
    IF,            // 如果
    ELSE,          // 否则

    // 循环语句
    WHILE,         // 当...时
    FOR,           // 对于

    ID,            // 标识符
    INTEGER,       // 整数
    REAL,          // 实数
    STRING,        // 字符串
    GLOBAL,        // 全局

    //判断
    EQUAL,         // 等于
    NOT_EQUAL,     // 不等于
    NOT,           // 非
    ASSIGN,        // 赋值

    // 算术运算符
    PLUS,          // 加
    MINUS,         // 减
    MUL,           // 乘
    DIV,           // 除
    MOD,           // 取模

    SEMI,          // 分号
    LPAREN,        // 左括号
    RPAREN,        // 右括号
    BREAK,         // 中断
    RETURN,        // 返回
    CONTINUE,      // 继续

    GREATER,       // 大于
    LESS,          // 小于
    GREATER_EQUAL, // 大于等于
    LESS_EQUAL,    // 小于等于
    AND,           // 与
    OR,            // 或

    BITWISE_AND,   // 按位与
    BITWISE_OR,    // 按位或
    BITWISE_XOR,   // 按位异或
    BITWISE_NOT,   // 按位取反

    LEFT_SHIFT,    // 左移
    RIGHT_SHIFT,   // 右移

    COLON,         // 冒号
    BUILTIN,       // 内置
    FUNCTION,      // 函数
    COMMA,         // 逗号
    PLUS_PLUS,     // 自增
    MINUS_MINUS,   // 自减

    PLUS_EQUAL,    // 加等于
    MINUS_EQUAL,   // 减等于
    MUL_EQUAL,     // 乘等于
    DIV_EQUAL,     // 除等于
    MOD_EQUAL,     // 取模等于

    BITWISE_AND_EQUAL, // 按位与等于
    BITWISE_OR_EQUAL,  // 按位或等于
    BITWISE_XOR_EQUAL, // 按位异或等于
    BITWISE_NOT_EQUAL, // 按位取反等于
    LEFT_SHIFT_EQUAL,  // 左移等于
    RIGHT_SHIFT_EQUAL, // 右移等于
    
    LSQUARE,       // 左方括号
    RSQUARE,       // 右方括号
    DOT,           // 点
    SHARP,         // 井号
    NONE,          // 无
    BTRUE,         // 布尔真
    BFALSE,        // 布尔假

    CLASS,         // 类
    STATIC,        // 静态
    EOFI           // 文件结束标识
};

class LToken
{
public:
    LToken();
    
    LToken(KEYWORD type, const string& value, int lineNo, int column, const string& filename);
    
    ~LToken();
    
    KEYWORD type() { return m_enuType; }
    
    string value() const { return m_strValue; }
    
    int lineNo() const { return m_nLineNo; }
    
    int column() const { return m_nColumn; }
    
    string filename() const { return m_strFilename; }
    
    void set_filename(const string& filename){ m_strFilename = filename; }
    
    void set_line_column(int lineNo, int column);
    
    static LToken GetToken(const string& key, int lineNo, int column, const string& filename);

private:
    KEYWORD m_enuType; 
    string m_strValue; 
    int m_nLineNo; 
    int m_nColumn; 
    string m_strFilename; 
};


