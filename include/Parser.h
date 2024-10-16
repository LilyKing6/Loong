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
#include "Lexer.h"
#include "AST.h"
#include "GlobalData.h"

class LParser
{
public:
	enum STATICTYPE { VAR, FUN };

	LParser() {}
	LParser(const LLexer& lexer, LGlobalData* pGlobalData);
	~LParser();

	AST* parse();
	
	AST* create_node(AST* node);
	
	string error_msg() { return m_error; }
	
	void set_outfile(FILE* out) { m_pFileOut = out; }
	
	LGlobalData* global_data() { return m_pGlobalData; }
	
	void set_curdir(const string& curdir) { m_curdir = curdir; }
	
	string curdir() { return m_curdir; }

private:
    
    void re_printf(const char* format, ...);
	
	void error(const string& err, const LToken* pToken = NULL);
	
	void consume(KEYWORD token_type);
	
	bool CanSkipSEMI(AST* node);
	
	void parse_content(const string& content, vector<AST*>& globals, string filename);
	
	void global_check_init();
	
	void global_check_start(const vector<AST*>& nodes);
	
	AST* function(string classname = "", bool bStatic = false);
	
	AST* class_def(bool bStatic = false);
	
	AST* function_exec();
	
	AST* program();
	
	AST* block();
	
	AST* compound_statement();
	
	vector<AST*> statement_list(string classname = "");
	
	AST* statement(string classname = "");
	
	AST* if_statement();
	
	AST* while_statement();
	
	AST* for_statement();
	
	AST* break_statement();
	
	AST* continue_statement();
	
	AST* return_statement();
	
	AST* builtin_statement();
	
	AST* include_statement();
	
	AST* global_statement();
	
	AST* array_dict(const LToken& prev_token);
	
	AST* assignment_array_dict(const LToken& prev_token, const LToken& token, AST* left);
	
	AST* factor();

	AST* term_bitwise_and();

	AST* term_bitwise_xor();

	AST* term_bitwise_or();

	AST* term_bitwise_not();

	AST* term_left_shift();

	AST* term_right_shift();
	
	AST* term_square_dot();
	
	AST* term_not();
	
	AST* term_mul_div();
	
	AST* term_plus_minus();
	
	AST* term_comparison();
	
	AST* term_equal();
	
	AST* term_and();
	
	AST* term_or();
	
	AST* term_plus_plus();
	
	AST* expr();
	
	AST* variable();

private:
	
	LLexer m_lexer;
	LToken m_current_token;
	LToken m_prev_token;
	string m_error;
	string m_filename;
	LGlobalData* m_pGlobalData;
	FILE* m_pFileOut;
	string m_curdir;              
	map<string, STATICTYPE> m_statics; 
	LCheckStack m_global_check;
};



