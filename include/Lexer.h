#pragma once
#include <string>
#include "Token.h"

class LLexer
{
public:
	
	LLexer(){}
	
	LLexer(const string& text, const string& filename);
	
	~LLexer();

	void error();
	
	void advance();
	
	void skip_whitespace();
	
	void skip_comment();
	
	void skip_comment_block();
	
	char peek();

	char peek_two();
	
	LToken get_next_token();
	
	LToken peek_next_token();
	
	LToken id();
	
	LToken number();
	
	LToken str();
	
	void process_special_char(string& result);

private:
	
	string m_strText;
	string::size_type m_nPos;
	char m_curChar;
	int		m_nLineNo;
	int		m_nColumn;
	string m_strFilename;
};

