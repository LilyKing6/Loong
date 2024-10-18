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

#include "Token.h"

LToken::LToken()
{
	m_nLineNo = 0;
	m_nColumn = 0;
}

LToken::LToken(KEYWORD type, const string& value, int lineNo, int column, const string& filename)
{
	m_enuType = type;
	m_strValue = value;
	m_nLineNo = lineNo;
	m_nColumn = column;
	m_strFilename = filename;
}


LToken::~LToken()
{
}

void LToken::set_line_column(int lineNo, int column)
{
	m_nLineNo = lineNo;
	m_nColumn = column;
}

LToken LToken::GetToken(const string& key, int lineNo, int column, const string& filename)
{
	static map<string, LToken> KeyWordMap;
	if(KeyWordMap.size()==0)
	{
		KeyWordMap["main"] = LToken(PROGRAM, "main", 0, 0, "");
		KeyWordMap["if"] = LToken(IF, "if", 0, 0, "");
		KeyWordMap["else"] = LToken(ELSE, "else", 0, 0, "");
		KeyWordMap["while"] = LToken(WHILE, "while", 0, 0, "");
		KeyWordMap["break"] = LToken(BREAK, "break", 0, 0, "");
		KeyWordMap["return"] = LToken(RETURN, "return", 0, 0, "");
		KeyWordMap["continue"] = LToken(CONTINUE, "continue", 0, 0, "");
		KeyWordMap["for"] = LToken(FOR, "for", 0, 0, "");
		KeyWordMap["null"] = LToken(NONE, "null", 0, 0, "");
		KeyWordMap["true"] = LToken(BTRUE, "true", 0, 0, "");
		KeyWordMap["false"] = LToken(BFALSE, "false", 0, 0, "");
		KeyWordMap["func"] = LToken(FUNCTION, "func", 0, 0, "");
		KeyWordMap["class"] = LToken(CLASS, "class", 0, 0, "");
		KeyWordMap["static"] = LToken(STATIC, "static", 0, 0, "");
		KeyWordMap["global"] = LToken(GLOBAL, "global", 0, 0, "");
		KeyWordMap["print"] = LToken(BUILTIN, "print", 0, 0, "");
		KeyWordMap["sprintf"] = LToken(BUILTIN, "sprintf", 0, 0, "");
		KeyWordMap["printf"] = LToken(BUILTIN, "printf", 0, 0, "");

		KeyWordMap[GLOBAL_DICT_NAME] = LToken(BUILTIN, GLOBAL_DICT_NAME, 0, 0, "");
		KeyWordMap[ARGV_ARRAY_NAME] = LToken(BUILTIN, ARGV_ARRAY_NAME, 0, 0, "");
		KeyWordMap["_input"] = LToken(BUILTIN, "_input", 0, 0, "");
		KeyWordMap["_getargv"] = LToken(BUILTIN, "_getargv", 0, 0, "");
		KeyWordMap["_copy"] = LToken(BUILTIN, "_copy", 0, 0, "");
		KeyWordMap["_loadlib"] = LToken(BUILTIN, "_loadlib", 0, 0, "");
		KeyWordMap["_freelib"] = LToken(BUILTIN, "_freelib", 0, 0, "");
		KeyWordMap["_calllib"] = LToken(BUILTIN, "_calllib", 0, 0, "");
		KeyWordMap["_createthread"] = LToken(BUILTIN, "_createthread", 0, 0, "");
		KeyWordMap["_createlock"] = LToken(BUILTIN, "_createlock", 0, 0, "");
		KeyWordMap["_lock"] = LToken(BUILTIN, "_lock", 0, 0, "");
		KeyWordMap["_unlock"] = LToken(BUILTIN, "_unlock", 0, 0, "");
		KeyWordMap["_len"] = LToken(BUILTIN, "_len", 0, 0, "");
		KeyWordMap["_str"] = LToken(BUILTIN, "_str", 0, 0, "");
		KeyWordMap["_int"] = LToken(BUILTIN, "_int", 0, 0, "");
		KeyWordMap["_float"] = LToken(BUILTIN, "_float", 0, 0, "");
		KeyWordMap["_type"] = LToken(BUILTIN, "_type", 0, 0, "");
		KeyWordMap["_fun"] = LToken(BUILTIN, "_fun", 0, 0, "");
	}

	auto iter = KeyWordMap.find(key);
	if (iter != KeyWordMap.end())
	{
		iter->second.set_line_column(lineNo, column);
		iter->second.set_filename(filename);
		return iter->second;
	}

	return LToken(ID, key, lineNo, column, filename);
}

