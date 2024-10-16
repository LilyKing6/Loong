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

#include "AST.h"

AST::AST()
{
    m_type = EMPTY;
}

AST::~AST()
{
}

BinOp::BinOp(AST* left, const LToken& op, AST* right)
{
    m_type = BINOP;

    m_left = left;

    m_token = op;

    m_right = right;
}

Num::Num(const LToken& token, NUMTYPE ntype)
{
    m_type = NUM;

    m_num_type = ntype;

    m_token = token;

    if (m_num_type == INT)
        m_nValue = _ATOI(m_token.value().c_str());
    else if (m_num_type == FLOAT)
        m_dValue = atof(m_token.value().c_str());
}

void Num::set_minus()
{
    if (m_num_type == INT)
        m_nValue = -m_nValue;
    else if (m_num_type == FLOAT)
        m_dValue = -m_dValue;
}

Bool::Bool(const LToken& token)
{ 
    m_type = BOOL; 
    m_token = token;
    if (m_token.value() == "true")
        m_value = true;
    else
        m_value = false;
}

Array::Array(const LToken& token, AST* array_size)
{
    m_type = ARRAY;
    m_token = token;
    m_array_size = array_size;
}

Dict::Dict(const LToken& token)
{
    m_type = DICT;
    m_token = token;
}

Var::Var(const LToken& token)
{
    m_type = VAR;
    m_token = token;
    m_value = m_token.value();
    m_global = false;
    m_func = false;
}

Assign::Assign(AST* left, const LToken& op, AST* right)
{
    m_type = ASSIGN;
    m_left = left;
    m_token = op;
    m_right = right;
}

ClassStatement::ClassStatement(string name, const LToken& token)
{ 
    m_type = CLASS; 
    m_name = name; 
    m_token = token;
}

FunctionStatement::FunctionStatement(string name, const LToken& token)
{ 
    m_type = FUNCTION; 
    m_name = name; 
    m_token = token; 
}

FunctionExec::FunctionExec(AST* statement, const LToken& token)
{ 
    m_var = false; 
    m_type = FUNCTION_EXEC; 
    m_statement = statement; 
    m_token = token;
}

ReturnStatement::ReturnStatement(const LToken& token, AST* expr)
{ 
    m_type = RETURN; 
    m_expr = expr;
    m_token = token;
}

BuiltinStatement::BuiltinStatement(const LToken& token)
{
    m_type = BUILTIN;
    m_token = token;
}

Member::Member(const LToken& token, MEMBERTYPE memtype)
{
    m_type = MEMBER;
    m_token = token;
    m_memtype = memtype;
}

ForCompound::ForCompound(const LToken& token, const vector<AST*>& init, AST* expr, const vector<AST*>& update)
{
    m_type = FORCOMPOUND;
    m_init = init;
    m_expr = expr;
    m_update = update;
    m_token = token;
}

WhileCompound::WhileCompound(const LToken& token, AST* expr)
{
    m_type = WHILECOMPOUND;
    m_expr = expr;
    m_token = token;
}

IfCompound::IfCompound(const LToken& token, AST* expr)
{
    m_type = IFCOMPOUND;
    m_expr = expr;
    m_token = token;
}

Compound::Compound()
{
    m_type = COMPOUND;
}

Block::Block(Compound* compound)
{ 
    m_type = BLOCK;
    m_compound = compound; 
}

Program::Program(string name, Block* block)
{ 
    m_type = PROGRAM;
    m_name = name; 
    m_block = block; 
}
