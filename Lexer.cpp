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

#include "Lexer.h"

LLexer::LLexer(const string& text, const string& filename)
{
	m_strText = text;

	m_nPos = 0;

	m_curChar = 0;

	if (m_nPos >= 0 && m_nPos < m_strText.size()) 
		m_curChar = m_strText[m_nPos];

	m_nLineNo = 1;

	m_nColumn = 1;

	m_strFilename = filename; 
}

LLexer::~LLexer()
{
}

void LLexer::error()
{
	printf("Invalid character\r\n");
}

void LLexer::advance()
{
	if (m_curChar == '\n')
	{
		m_nLineNo += 1;
		m_nColumn = 0;
	}

	m_nPos++;

	if (m_nPos > m_strText.size() - 1)
	{
		m_curChar = 0; 
	}
	else
	{

		m_curChar = m_strText[m_nPos];
		m_nColumn += 1;
	}
}

char LLexer::peek()
{
	string::size_type peek_pos = m_nPos + 1;

	if (peek_pos > m_strText.size() - 1)
		return 0; 
	else
		return m_strText[peek_pos];

}

char LLexer::peek_two()
{
	string::size_type peek_pos = m_nPos + 2;

	if (peek_pos > m_strText.size() - 1)
		return 0; 
	else
		return m_strText[peek_pos];

}

void LLexer::skip_whitespace()
{
	while (m_curChar != 0 
		&& (m_curChar == ' ' || m_curChar == '	' || m_curChar == '\r' || m_curChar == '\n'))
		advance();
}

void LLexer::skip_comment()
{
	while (m_curChar != '\n' && m_curChar != 0)
		advance();
	advance();
}

void LLexer::skip_comment_block()
{

	while (!(m_curChar == '*' && peek() == '/') && m_curChar != 0)
		advance();

	advance();
	advance();
}


LToken LLexer::id()
{
	string result;
	while (m_curChar != 0)
	{
		if (m_curChar >= 'a' && m_curChar <= 'z' 
			|| m_curChar >= 'A' && m_curChar <= 'Z'
			|| m_curChar >= '0' && m_curChar <= '9'
			|| m_curChar == '_' 
			|| m_curChar == '$'
			)
			result += m_curChar;
		else
			break;
		
		advance();
	}
	return LToken::GetToken(result, m_nLineNo, m_nColumn, m_strFilename);
}

LToken LLexer::number()
{
	string result;
	while (m_curChar != 0)
	{
		if (m_curChar >= '0' && m_curChar <= '9') 
			result += m_curChar;
		else
			break;
		
		advance();
	}
	if (m_curChar == '.')
	{
		result += m_curChar;
		advance();
		while (m_curChar != 0)
		{
			if (m_curChar >= '0' && m_curChar <= '9')
				result += m_curChar;
			else
				break;
			
			advance();
		}
		return LToken(REAL, result, m_nLineNo, m_nColumn, m_strFilename);
	}
	else
	{
		return LToken(INTEGER, result, m_nLineNo, m_nColumn, m_strFilename);
	}
	return LToken::GetToken(result, m_nLineNo, m_nColumn, m_strFilename);
}

void LLexer::process_special_char(string& result)
{
	string new_result;
	for (string::size_type i = 0; i < result.size(); i++)
	{
		if (result[i] == '\\' && i < result.size() - 1)
		{
			if (result[i + 1] == '\\')
			{
				new_result += '\\';
				i++;
				continue;
			}
			if (result[i + 1] == 'r')
			{
				new_result += '\r';
				i++;
				continue;
			}
			if (result[i + 1] == 'n')
			{
				new_result += '\n';
				i++;
				continue;
			}
			if (result[i + 1] == 't')
			{
				new_result += '\t';
				i++;
				continue;
			}
			if (result[i + 1] == 'a')
			{
				new_result += '\a';
				i++;
				continue;
			}
			if (result[i + 1] == 'b')
			{
				new_result += '\b';
				i++;
				continue;
			}
			if (result[i + 1] == 'v')
			{
				new_result += '\v';
				i++;
				continue;
			}

		}
		new_result += result[i];
	}
	result = new_result;
}

LToken LLexer::str()
{
	string result;
	while (m_curChar != '"' && m_curChar != 0)
	{
		if (m_curChar == '\\')
		{
			//int nPrevPos = m_nPos - 1;
			if (m_nPos >= 1 && m_strText[m_nPos - 1] != '\\')
			{
				string::size_type nNextPos = m_nPos + 1;
				if (nNextPos < m_strText.size() && m_strText[nNextPos] == '"')
					advance();
			}
		}

		result += m_curChar;
		advance();
	}
	advance();

	process_special_char(result);

	return LToken(STRING, result, m_nLineNo, m_nColumn, m_strFilename);
}

LToken LLexer::peek_next_token()
{

	string::size_type peek_pos = m_nPos;
	string result = "";
	while (m_strText[peek_pos] != 0)
	{
		
		if (m_strText[peek_pos] == ' ' || m_strText[peek_pos] == '	' || m_strText[peek_pos] == '\r' || m_strText[peek_pos] == '\n')
		{
			peek_pos++;
			continue;
		}
		
		if (m_strText[peek_pos] == '/' && m_strText[peek_pos + 1] == '/')
		{
			peek_pos++;
			peek_pos++;
			while (m_strText[peek_pos] != 0 &&
				m_strText[peek_pos] != '\n' && m_strText[peek_pos] != 0)
				peek_pos++;
			peek_pos++;
			continue;
		}
		
		if (m_strText[peek_pos] == '/' && m_strText[peek_pos + 1] == '*')
		{
			peek_pos++;
			peek_pos++;
			while (m_strText[peek_pos] != 0 &&
				!(m_strText[peek_pos] == '*' && m_strText[peek_pos + 1] == '/'))
				peek_pos++;
			peek_pos++;
			peek_pos++;
			continue;
		}

		break;

	}

	while (m_strText[peek_pos] != 0 &&
		(m_strText[peek_pos] >= 'a' && m_strText[peek_pos] <= 'z' || m_strText[peek_pos] >= 'A' && m_strText[peek_pos] <= 'Z'
		|| m_strText[peek_pos] >= '0' && m_strText[peek_pos] <= '9' 
		|| m_strText[peek_pos] == '_' || m_strText[peek_pos] == '$'))
	{
		result += m_strText[peek_pos];
		peek_pos++;
	}

	return LToken::GetToken(result, m_nLineNo, m_nColumn, m_strFilename);
}

LToken LLexer::get_next_token()
{
    while (m_curChar != 0)
    {
        
        if (m_curChar == ' ' || m_curChar == '	' || m_curChar == '\r' || m_curChar == '\n')
        {
            skip_whitespace();
            continue;
        }
        
        if (m_curChar == '/' && peek() == '/')
        {
            advance();
            advance();
            skip_comment();
            continue;
        }
        
        if (m_curChar == '#' && peek() == '!')
        {
            advance();
            advance();
            skip_comment();
            continue;
        }
        
        if (m_curChar == '/' && peek() == '*')
        {
            advance();
            advance();
            skip_comment_block();
            continue;
        }
        
        if (m_curChar == '{')
        {
            advance();
            return LToken(BEGIN, "{", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '}')
        {
            advance();
            return LToken(END, "}", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '[')
        {
            advance();
            return LToken(LSQUARE, "[", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == ']')
        {
            advance();
            return LToken(RSQUARE, "]", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '"')
        {
            advance();
            return str();
        }
        
		if (m_curChar >= 'a' && m_curChar <= 'z' 
			|| m_curChar >= 'A' && m_curChar <= 'Z' 
			|| m_curChar == '_' || m_curChar == '$')
		{
			
			return id();
		}
        
        if (m_curChar >= '0' && m_curChar <= '9')
        {
            return number();
        }
        
        if (m_curChar == '&' && peek() == '&')
        {
            advance();
            advance();
            return LToken(AND, "&&", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '|' && peek() == '|')
        {
            advance();
            advance();
            return LToken(OR, "||", m_nLineNo, m_nColumn, m_strFilename);
        }

        
        if (m_curChar == '<' && peek() == '<' && peek_two() != '=')
        {
            advance();
            advance();
            return LToken(LEFT_SHIFT, "<<", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '>' && peek() == '>' && peek_two() != '=')
        {
            advance();
            advance();
            return LToken(RIGHT_SHIFT, ">>", m_nLineNo, m_nColumn, m_strFilename);
        }

        
        if (m_curChar == '=' && peek() == '=')
        {
            advance();
            advance();
            return LToken(EQUAL, "==", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '!' && peek() == '=')
        {
            advance();
            advance();
            return LToken(NOT_EQUAL, "!=", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '>'&& peek() == '=')
        {
            advance();
            advance();
            return LToken(GREATER_EQUAL, ">=", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '<'&& peek() == '=')
        {
            advance();
            advance();
            return LToken(LESS_EQUAL, "<=", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '>' && peek() != '>')
        {
            advance();
            return LToken(GREATER, ">", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '<' && peek() != '<')
        {
            advance();
            return LToken(LESS, "<", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '=')
        {
            advance();
            return LToken(ASSIGN, "=", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '!')
        {
            advance();
            return LToken(NOT, "!", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '+' && peek() == '+')
        {
            advance();
            advance();
            return LToken(PLUS_PLUS, "++", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '-' && peek() == '-')
        {
            advance();
            advance();
            return LToken(MINUS_MINUS, "--", m_nLineNo, m_nColumn, m_strFilename);
        }

        
        if (m_curChar == '+' && peek() == '=')
        {
            advance();
            advance();
            return LToken(PLUS_EQUAL, "+=", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '-' && peek() == '=')
        {
            advance();
            advance();
            return LToken(MINUS_EQUAL, "-=", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '*' && peek() == '=')
        {
            advance();
            advance();
            return LToken(MUL_EQUAL, "*=", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '/' && peek() == '=')
        {
            advance();
            advance();
            return LToken(DIV_EQUAL, "/=", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '%' && peek() == '=')
        {
            advance();
            advance();
            return LToken(MOD_EQUAL, "%=", m_nLineNo, m_nColumn, m_strFilename);
        }

        
        if (m_curChar == '<' && peek() == '<' && peek_two() == '=')
        {
            advance();
            advance();
            advance();
            return LToken(LEFT_SHIFT_EQUAL, "<<=", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '>' && peek() == '>' && peek_two() == '=')
        {
            advance();
            advance();
            advance();
            return LToken(RIGHT_SHIFT_EQUAL, ">>=", m_nLineNo, m_nColumn, m_strFilename);
        }

        
        if (m_curChar == '&' && peek() == '=')
        {
            advance();
            advance();
            return LToken(BITWISE_AND_EQUAL, "&=", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '|' && peek() == '=')
        {
            advance();
            advance();
            return LToken(BITWISE_OR_EQUAL, "|=", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '^' && peek() == '=')
        {
            advance();
            advance();
            return LToken(BITWISE_XOR_EQUAL, "^=", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '~' && peek() == '=')
        {
            advance();
            advance();
            return LToken(BITWISE_NOT_EQUAL, "~=", m_nLineNo, m_nColumn, m_strFilename);
        }

        
        if (m_curChar == '+')
        {
            advance();
            return LToken(PLUS, "+", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '-')
        {
            advance();
            return LToken(MINUS, "-", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '*')
        {
            advance();
            return LToken(MUL, "*", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '/')
        {
            advance();
            return LToken(DIV, "/", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '%')
        {
            advance();
            return LToken(MOD, "%", m_nLineNo, m_nColumn, m_strFilename);
        }

        
        if (m_curChar == '&' && peek() != '&')
        {
            advance();
            return LToken(BITWISE_AND, "&", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '|' && peek() != '|')
        {
            advance();
            return LToken(BITWISE_OR, "|", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '^')
        {
            advance();
            return LToken(BITWISE_XOR, "^", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '~')
        {
            advance();
            return LToken(BITWISE_NOT, "~", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        
        if (m_curChar == '(')
        {
            advance();
            return LToken(LPAREN, "(", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == ')')
        {
            advance();
            return LToken(RPAREN, ")", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == ';')
        {
            advance();
            return LToken(SEMI, ";", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == ',')
        {
            advance();
            return LToken(COMMA, ",", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == ':')
        {
            advance();
            return LToken(COLON, ":", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '.')
        {
            advance();
            return LToken(DOT, ".", m_nLineNo, m_nColumn, m_strFilename);
        }
        
        if (m_curChar == '#')
        {
            advance();
            return LToken(SHARP, "#", m_nLineNo, m_nColumn, m_strFilename);
        }

        error();
        break;
    }

    
    return  LToken(EOFI, "", m_nLineNo, m_nColumn, m_strFilename);
}
