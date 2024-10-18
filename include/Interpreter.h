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
#include "Parser.h"
#include "Variable.h"
#include "CallStack.h"
#include "Library.h"
#include "Version.h"
#include <vector>

class LInterpreter
{
public:
    
    LInterpreter(const LParser& parser);
    
    ~LInterpreter();
    
    LVariable interpret();
    
    string error_msg(){ return m_error; }
    
    void set_outfile(FILE* out){ m_pFileOut = out; }
    
    void visit(AST* node, LVariable& res);
    
    LParser& get_parser(){ return m_parser; }
    
    LCallStack& get_callstack(){ return m_callstack; }
    
    void set_callstack(LCallStack& stack){ m_callstack=stack; }
    
    void set_argv(const vector<LVariable>& argv){ m_vecArgv = argv; }
    
    void set_globalvalue(const LVariable globalvalue){ m_globalValue = globalvalue; }
    
    void set_argvname(const string& name){ m_argvName = name; }

private:
    
    void visit_BinOp(BinOp* node, LVariable& res);
    
    void visit_Num(Num* node, LVariable& res);
    
    void visit_Bool(Bool* node, LVariable& res);
    
    void visit_Str(Str* node, LVariable& res);
    
    void visit_Array(Array* node, LVariable& res);
    
    void visit_Dict(Dict* node, LVariable& res);
    
    void visit_Var(Var* node, LVariable& res);
    
    void visit_Assign(Assign* node, LVariable& res);
    
    void visit_Program(Program* node, LVariable& res);
    
    void visit_Block(Block* node, LVariable& res);
    
    void visit_Compound(Compound* node, LVariable& res);
    
    void visit_IfCompound(IfCompound* node, LVariable& res);
    
    void visit_WhileCompound(WhileCompound* node, LVariable& res);
    
    void visit_ForCompound(ForCompound* node, LVariable& res);
    
    void visit_Break(BreakStatement* node, LVariable& res);
    
    void visit_Continue(ContinueStatement* node, LVariable& res);
    
    void visit_Return(ReturnStatement* node, LVariable& res);
    
    void visit_Include(IncludeStatement* node, LVariable& res);
    
    void visit_Builtin(BuiltinStatement* node, LVariable& res);
    
    void visit_Function(FunctionStatement* node, LVariable& res);
    
    void visit_FunctionExec(FunctionExec* node, LVariable& res);
    
    void visit_Class(FunctionExec* node, LVariable& res);
    
    void visit_Member(AST* obj, AST* member, LVariable& res);
    
    void visit_Index(AST* obj, AST* idx, LVariable& res);
    
    void visit_Not(AST* obj, LVariable& res);
    
    void exec_function(FunctionStatement* fun, vector<AST*>& exprs, LToken& token, LVariable& res);
    
    void exec_class(ClassStatement* cls, vector<AST*>& exprs, LToken& token, LVariable& res);
    
    void copy_object(LVariable& object, LVariable& res);
    
    void print_object(LVariable& object);
    
    void get_index_value(LVariable& var, LVariable& idx, LVariable& res);
    
    void set_index_value(const string& var_name, LVariable& var, LVariable& idx_value, const LVariable& result, const LToken& token);
    
    bool check_condition(LVariable& condition);
    
    void re_printf(const char* format, ...);
    
    void warning(const string& warn, const LToken& token);
    
    void error(const string& err, const LToken& token);

private:
    LParser m_parser; 
    LCallStack m_callstack; 
    Lib_String m_libString; 
    Lib_Array m_libArray; 
    Lib_Dict m_libDict; 
    Lib_Class m_libClass; 
    Dll m_dll; 
    string m_error; 
    FILE* m_pFileOut; 
    vector<LVariable> m_vecArgv; 
    LVariable m_globalValue; 
    string m_argvName; 
};
