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


#include "Interpreter.h"
#include "Sal.h"
#include <stdarg.h>

extern LInterpreter* _pInterpreter;

struct threadParams
{
 FunctionExec* func;
 LGlobalData* globaldata;
 LCallStack callstack;
};

#ifdef _WIN32
 #include <process.h>
 void newthread(void* pParam)
 {
  threadParams *pParams = (threadParams*)pParam;
  LGlobalData *globaldata = pParams->globaldata;
  FunctionExec* func = pParams->func;
  LCallStack callstack = pParams->callstack;
  delete pParams;

  if (globaldata == NULL) return;
  if (func == NULL) return;

  LLexer lexer("", "");
  LParser parser = LParser(lexer, globaldata);
  LInterpreter interp = LInterpreter(parser);
  interp.set_callstack(callstack);
  LVariable result;
  interp.visit(func,result);
 
  delete func;
  return;
 }
#else
 void* newthread(void* pParam)
 {
  threadParams *pParams = (threadParams*)pParam;
  LGlobalData *globaldata = pParams->globaldata;
  FunctionExec* func = pParams->func;
  LCallStack callstack = pParams->callstack;
  delete pParams;

  if (globaldata == NULL) return NULL;
  if (func == NULL) return NULL;

  LLexer lexer("", "");
  LParser parser = LParser(lexer, globaldata);
  LInterpreter interp = LInterpreter(parser);
  interp.set_callstack(callstack);
  LVariable result;
  interp.visit(func,result);

  delete func;
  return NULL;
 }
#endif

LInterpreter::LInterpreter(const LParser& parser)
{
	m_pFileOut = NULL;
	m_parser = parser;
	if (_pInterpreter == NULL)
		_pInterpreter = this;
}

LInterpreter::~LInterpreter()
{
}

void LInterpreter::re_printf(const char* format, ...)
{
	char buffer[1024];
	va_list args;
	int n;
	va_start(args, format);
	n = vsnprintf(buffer, 1024, format, args);
	va_end(args);

	if (m_pFileOut)
		fprintf(m_pFileOut, "%s", buffer);
	else
		printf("%s", buffer);
}

void LInterpreter::warning(const string& warn, const LToken& token)
{
	char fileinfo[512];
	char buff[512];
	sprintf(fileinfo, 
			"%s:%d:%d: ", 
			token.filename().c_str(), 
			token.lineNo(), 
			token.column()
			);
	re_printf("%s", fileinfo);

	PrintColor("warning: ", MAGENTA);

	sprintf(buff, 
			"%s: %s\r\n", 
			warn.c_str(), 
			token.value().c_str()
			);
	re_printf("%s", buff);
	
	m_error = buff;
}

void LInterpreter::error(const string& err, const LToken& token)
{
	char fileinfo[512];
	char buff[512];
	sprintf(fileinfo, 
			"%s:%d:%d: ", 
			token.filename().c_str(), 
			token.lineNo(), 
			token.column()
			);
	re_printf("%s", fileinfo);

	PrintColor("error: ", RED);

	sprintf(buff, 
			"%s: %s\r\n", 
			err.c_str(), 
			token.value().c_str()
			);
	re_printf("%s", buff);
	
	m_error = buff;
}

void LInterpreter::visit(AST* node, LVariable& res)
{
	res.reset();
	if (node == NULL || node->type() == AST::EMPTY || m_error.size() > 0)
		return;

	AST::ASTTYPE type = node->type();
	if (type == AST::BINOP)
	{
		visit_BinOp((BinOp*)node, res);
		if(res.tag() == LVariable::ERR_DIVZERO && res.type() == LVariable::NONE)
		{
			error("division by zero", node->token());
		}
		if (res.tag() == LVariable::ERR)
		{
			error("unsupported operand type", node->token());
		}	
	}
	else if (type == AST::NUM)
	{
		visit_Num((Num*)node, res);
	}
	else if (type == AST::BOOL)
	{
		visit_Bool((Bool*)node, res);
	}
	else if (type == AST::STR)
	{
		visit_Str((Str*)node, res);
	}
	else if (type == AST::ARRAY)
	{
		visit_Array((Array*)node, res);
	}
	else if (type == AST::DICT)
	{
		visit_Dict((Dict*)node, res);
	}
	else if (type == AST::VAR)
	{
		visit_Var((Var*)node, res);
	}
	else if (type == AST::ASSIGN)
	{
		visit_Assign((Assign*)node, res);
	}
	else if (type == AST::PROGRAM)
	{
		visit_Program((Program*)node, res);
	}
	else if (type == AST::BLOCK)
	{
		visit_Block((Block*)node, res);
	}
	else if (type == AST::COMPOUND)
	{
		visit_Compound((Compound*)node, res);
	}
	else if (type == AST::IFCOMPOUND)
	{
		visit_IfCompound((IfCompound*)node, res);
	}
	else if (type == AST::WHILECOMPOUND)
	{
		visit_WhileCompound((WhileCompound*)node, res);
	}
	else if (type == AST::FORCOMPOUND)
	{
		visit_ForCompound((ForCompound*)node, res);
	}
	else if (type == AST::BREAK)
	{
		visit_Break((BreakStatement*)node, res);
	}
	else if (type == AST::CONTINUE)
	{
		visit_Continue((ContinueStatement*)node, res);
	}
	else if (type == AST::RETURN)
	{
		visit_Return((ReturnStatement*)node, res);
	}
	else if (type == AST::BUILTIN)
	{
		visit_Builtin((BuiltinStatement*)node, res);
	}
	else if (type == AST::FUNCTION)
	{
		visit_Function((FunctionStatement*)node, res);
	}
	else if (type == AST::FUNCTION_EXEC)
	{
		visit_FunctionExec((FunctionExec*)node, res);
	}
	else if (type == AST::INCLUDE)
	{
		visit_Include((IncludeStatement*)node, res);
	}
	else if (type == AST::NONE)
	{
		res.setType(LVariable::NONE);
	}
}

void LInterpreter::visit_BinOp(BinOp* node, LVariable& res)
{
	KEYWORD type = node->token().type();
	if (type == PLUS)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res + res2;
	}
	else if (type == MINUS)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res - res2;
	}
	else if (type == MUL)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res * res2;
	}
	else if (type == DIV)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res / res2;
	}
	else if (type == MOD)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res % res2;
	}
	else if (type == EQUAL)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res == res2;
	}
	else if (type == GREATER)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res > res2;
	}
	else if (type == LESS)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res < res2;
	}
	else if (type == GREATER_EQUAL)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res >= res2;
	}
	else if (type == LESS_EQUAL)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res <= res2;
	}
	else if (type == NOT_EQUAL)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res != res2;
	}
	else if (type == AND)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res && res2;
	}
	else if (type == OR)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res || res2;
	}

	else if (type == BITWISE_AND)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res & res2;
	}
	else if (type == BITWISE_OR)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res | res2;
	}
	else if (type == BITWISE_XOR)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res ^ res2;
	}
	else if (type == BITWISE_NOT)
	{
		visit(node->right(), res);
		res = ~res;
	}
	
	else if (type == LEFT_SHIFT)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res << res2;
	}
	else if (type == RIGHT_SHIFT)
	{
		LVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res >> res2;
	}


	else if (type == DOT)
	{
		visit_Member(node->left(), node->right(), res);
	}
	else if (type == LSQUARE)
	{
		visit_Index(node->left(), node->right(), res);
	}
	else if (type == NOT)
	{
		visit_Not(node->right(), res);
	}
	else
		res.reset();
}


void LInterpreter::visit_Num(Num* node, LVariable& res)
{
	if (node->num_type() == Num::INT)
	{
		res.setInt(node->value());
	}
	else if (node->num_type() == Num::FLOAT)
	{
		res.setDouble(node->float_value());
	}
}


void LInterpreter::visit_Bool(Bool* node, LVariable& res)
{
	int n = 0;
	if (node->value())
		n = 1;
	res.setInt(n);
}


void LInterpreter::visit_Str(Str* node, LVariable& res)
{
	res.setType(LVariable::STRING);
	res.strValue() = node->value();
}


void LInterpreter::visit_Array(Array* node, LVariable& arr)
{
	arr.setArray(0);
	if (arr.arrValue() == NULL)
		return arr.reset();

	LVariable result;
	vector<AST*>& inits = node->inits();
	for (size_t i = 0; i < inits.size(); i++)
	{
		visit(inits[i], result);
		(*arr.arrValue()).push_back(result);
	}
}


void LInterpreter::visit_Dict(Dict* node, LVariable& dict)
{
	string var_name = node->token().value();
	dict.setDict();
	if (dict.dictValue() == NULL)
		return dict.reset();

	vector<AST*>& inits1 = node->inits_left();
	vector<AST*>& inits2 = node->inits_right();
	if (inits1.size() != inits2.size())
		error("\"" + var_name + "\"" + ":dict pairs error\r\n", node->token());

	LVariable left;
	LVariable right;
	for (size_t i = 0; i < inits1.size(); i++)
	{
		visit(inits1[i], left);
		visit(inits2[i], right);
		(*dict.dictValue())[left] = right;
	}
}

void LInterpreter::set_index_value(const string& var_name, LVariable& var, LVariable& idx_value, const LVariable& result, const LToken& token)
{
	LVariable index;
	LVariable* pVar_value = &idx_value;
	for (size_t i = 1; i < var.index().size() - 1; i++)
	{
		index = var.index()[i];
		if (pVar_value->type() == LVariable::DICT || pVar_value->type() == LVariable::CLASS)
		{
			if (pVar_value->dictValue()->find(index) != pVar_value->dictValue()->end())
				pVar_value = &(*pVar_value->dictValue())[index];
			else
			{
				if (i < var.index().size() - 2)
				{
					error("dict index error for " + var_name + "\r\n",token);
					break;
				}
			}
		}
		else if (pVar_value->type() == LVariable::ARRAY)
		{
			_INT idx = -1;
			if (index.type() == LVariable::INT)
				idx = index.intValue();
			if (idx >= 0 && (size_t)idx < (*pVar_value->arrValue()).size())
				pVar_value = &(*pVar_value->arrValue())[idx];
			else
			{
				if (i < var.index().size() - 2)
				{
					error("array index error for \"" + var_name + "\"" + "\r\n", token);
					break;
				}
			}
		}

	}
	if (pVar_value->type() == LVariable::DICT || pVar_value->type() == LVariable::CLASS)
	{
		index = var.index()[var.index().size() - 1];
		if (index.type()!=LVariable::EMPTY)
			(*pVar_value->dictValue())[index] = result;
	}
	else if (pVar_value->type() == LVariable::ARRAY)
	{
		_INT idx = -1;
		index = var.index()[var.index().size() - 1];
		if (index.type() == LVariable::INT)
			idx = index.intValue();
		if (idx >= 0 && (size_t)idx < (*pVar_value->arrValue()).size())
			(*pVar_value->arrValue())[index.intValue()] = result;
	}
	else if (pVar_value->type() == LVariable::STRING)
	{
		_INT idx = -1;
		index = var.index()[var.index().size() - 1];
		if (index.type() == LVariable::INT)
			idx = index.intValue();
		if (idx >= 0 && (size_t)idx < pVar_value->strValue().size())
			pVar_value->strValue()[index.intValue()] = result.strValue()[0];
	}
	else
		error("index error for \"" + var_name + "\"" + "\r\n",token);

}

void LInterpreter::get_index_value(LVariable& var, LVariable& idx, LVariable& var_value)
{
	if (var.type() == LVariable::VARTYPE::DICT || var.type() == LVariable::VARTYPE::CLASS)
	{
		LVariable dict_index = idx;
		if (dict_index.type() != LVariable::EMPTY)
		{
			map<LVariable, LVariable>* dict = var.dictValue();
			if (dict && dict->find(dict_index) != dict->end())
				var_value = (*dict)[dict_index];
		}
	}
	else
	{
		_INT arr_index = -1;
		LVariable index = idx;
		if (index.type() == LVariable::INT)
			arr_index = index.intValue();

		if (var.type() == LVariable::STRING)
		{
			const string& strValue = var.strValue();
			if (arr_index >= 0 && (size_t)arr_index < strValue.size())
			{
				var_value = LVariable(string(1, strValue[arr_index]));
			}
		}
		else if (var.type() == LVariable::ARRAY)
		{
			vector<LVariable>* arr = var.arrValue();
			if (arr && arr_index >= 0 && (size_t)arr_index < arr->size())
				var_value = (*arr)[arr_index];
		}

	}

	var_value.setInfo(var.info());
	var_value.setIndex(var.index());
	var_value.index().push_back(idx);

}

void LInterpreter::visit_Var(Var* node, LVariable& var_value)
{
	ActivationRecord* ar = NULL;
	if (node->global())
		ar = &m_callstack.base();
	else
		ar = &m_callstack.peek();

	string& var_name = node->value();
	if (node->global())
		var_value = ar->get__global_value(var_name);
	else
		var_value = ar->get_value(var_name);
	if (var_value.type()==LVariable::EMPTY)
		error("\"" + var_name + "\"" + " is undefined.\r\n", node->token());

	var_value.info()["name"] = var_name;
	if (node->global())
		var_value.info()["global"] = "1";
	else
		var_value.info()["global"] = "0";


	if (node->is_func())
	{
		if (var_value.type() == LVariable::POINTER)
		{
			FunctionStatement* fun = (FunctionStatement*)var_value.pointerValue();
			if (var_value.info().find("func_type") != var_value.info().end())
			{
				if (var_value.info()["func_type"] == "func")
				{
					return exec_function(fun, node->exprs(), node->token(), var_value);
				}
				if (var_value.info()["func_type"] == "class")
				{
					ClassStatement* cls = (ClassStatement*)fun;
					return exec_class(cls, node->exprs(), node->token(), var_value);
				}
				if (var_value.info()["func_type"] == "member")
				{
					return exec_function(fun, node->exprs(), node->token(), var_value);
				}
			}
		}
		else
			error("\"" + var_name + "\""  + " is not a function\r\n", node->token());
	}

}
void LInterpreter::visit_Assign(Assign* node, LVariable& res)
{
	if (node->left()->type()!=AST::VAR)//dict, array, class
	{
		LVariable var_value;
		visit(node->left(),var_value);
		string var_name;
		bool bGlobal = false;
		if (var_value.info().find("name") != var_value.info().end())
			var_name = var_value.info()["name"];
		if (var_value.info().find("global") != var_value.info().end())
		{
			if (var_value.info()["global"] == "1")
				bGlobal = true;
		}

		LVariable &result=res;
		visit(node->right(),result);
		result.index().clear();

		ActivationRecord* ar = NULL;
		if (bGlobal)
			ar = &m_callstack.base();
		else
			ar = &m_callstack.peek();
		if (var_value.index().size()>0)
		{
			if (bGlobal)
			{
				LVariable index = var_value.index()[0];
				LVariable::VARTYPE vartype = ar->get_global_vartype(var_name);
				if (vartype == LVariable::VARTYPE::DICT || vartype == LVariable::VARTYPE::CLASS)
				{
					LVariable& global_value = ar->get__global_value(var_name);
					if (global_value.dictValue() == NULL)
						error("global dict error for \"" + var_name + "\"" + "\r\n", node->token());
					if (var_value.index().size() == 1)
						(*global_value.dictValue())[index] = result;
					else
					{
						LVariable& idx_value=(*global_value.dictValue())[index];
						set_index_value(var_name, var_value, idx_value, result, node->token());
					}
				}
				else
				{
					_INT idx = -1;
					if (index.type() == LVariable::INT)
						idx = index.intValue();
					else
						error("array index error for \"" + var_name + "\"" + "\r\n", node->token());

					LVariable& global_value = ar->get__global_value(var_name);
					if (global_value.arrValue() == NULL)
						error("global array error for \"" + var_name + "\"" + "\r\n", node->token());

					if (var_value.index().size() == 1)
						(*global_value.arrValue())[idx] = result;
					else
					{
						LVariable& idx_value = (*global_value.arrValue())[idx];
						set_index_value(var_name, var_value, idx_value, result, node->token());
					}
				}

			}
			else
			{
				LVariable index = var_value.index()[0];
				LVariable::VARTYPE vartype = ar->get_vartype(var_name);
				if (vartype == LVariable::VARTYPE::DICT || vartype == LVariable::VARTYPE::CLASS)
				{
					if (var_value.index().size() == 1)
						ar->set_dict_value(var_name, result, index);
					else
					{
						LVariable& idx_value = ar->get_dict_value(var_name, index);
						set_index_value(var_name, var_value, idx_value, result, node->token());
					}
				}
				else
				{
					_INT idx = -1;
					if (index.type() == LVariable::INT)
						idx = index.intValue();
					else
						error("array index error for \"" + var_name + "\"" + "\r\n", node->token());

					if (var_value.index().size() == 1)
						ar->set_array_value(var_name, result, idx);
					else
					{
						LVariable& idx_value = ar->get_array_value(var_name, idx);
						set_index_value(var_name, var_value, idx_value, result, node->token());
					}
				}
			}
		}
		else
			error("error assign \r\n", node->token());

	}
	else
	{
		Var* var = (Var*)node->left();
		string& var_name = var->value();
		if (var->exprs().size() > 0)
		{
			error("function \"" + var_name + "\"" + " cannot be assigned\r\n", node->token());
			return res.reset();
		}

		LVariable &result=res;
		visit(node->right(),result);
		result.index().clear();

		ActivationRecord* ar = NULL;
		if (var->global())
			ar = &m_callstack.base();
		else
			ar = &m_callstack.peek();

		if (var->global())
			ar->set_global_value(var_name, result);
		else
			ar->set_value(var_name, result);
	}

	return res.reset();
}
void LInterpreter::visit_Program(Program* node, LVariable& res)
{
	ActivationRecord ar(node->name(),"program",1);
	ar.create_global(m_globalValue, m_vecArgv, m_argvName);

	m_callstack.push(ar);
	for (size_t i = 0; i<node->globals().size(); i++)
	{
		if(node->globals()[i]->type()==AST::ASSIGN)
			visit(node->globals()[i],res);
	}
	visit(node->block(),res);
	m_callstack.pop();
}
void LInterpreter::visit_Block(Block* node, LVariable& res)
{
	visit(node->compound(),res);
}
void LInterpreter::visit_Compound(Compound* node, LVariable& result)
{
	for (size_t i = 0; i < node->child().size(); i++)
	{
		visit(node->child()[i],result);
		if (result.tag() == LVariable::RETURN)
			return;
		if (result.tag() == LVariable::BREAK || result.tag() == LVariable::CONTINUE)
			error("break/continue is not in loop", node->child()[i]->token());
	}
}

bool LInterpreter::check_condition(LVariable& condition)
{
	if (condition.type() == LVariable::INT)
	{
		if (condition.intValue() != 0)
			return true;
		else
			return false;
	}
	if (condition.type() == LVariable::FLOAT)
	{
		if (condition.floatValue() != 0)
			return true;
		else
			return false;
	}
	if (condition.type() == LVariable::STRING)
	{
		if (condition.strValue().size() != 0)
			return true;
		else
			return false;
	}
	if (condition.type() == LVariable::POINTER)
	{
		if (condition.pointerValue() != 0)
			return true;
		else
			return false;
	}
	if (condition.type() == LVariable::ARRAY)
	{
		if (condition.arrValue()->size() != 0)
			return true;
		else
			return false;
	}
	if (condition.type() == LVariable::DICT)
	{
		if (condition.dictValue()->size() != 0)
			return true;
		else
			return false;
	}
	if (condition.type() == LVariable::CLASS)
			return true;

	return false;
}
void LInterpreter::visit_IfCompound(IfCompound* node, LVariable& result)
{
	visit(node->expr(), result);
	bool bTrue = check_condition(result);
	if (bTrue)
	{
		for (size_t i = 0; i < node->true_statements().size(); i++)
		{
			visit(node->true_statements()[i],result);
			if (result.tag() == LVariable::BREAK || result.tag() == LVariable::RETURN || result.tag() == LVariable::CONTINUE)
				return;
		}
	}
	else
	{
		for (size_t i = 0; i < node->false_statements().size(); i++)
		{
			visit(node->false_statements()[i],result);
			if (result.tag() == LVariable::BREAK || result.tag() == LVariable::RETURN || result.tag() == LVariable::CONTINUE)
				return;
		}
	}

	return result.reset();//
}
void LInterpreter::visit_ForCompound(ForCompound* node, LVariable& result)
{
	for (size_t i = 0; i < node->init_statements().size(); i++)
		visit(node->init_statements()[i], result);

	visit(node->expr(), result);
	bool bTrue = check_condition(result);
	while (bTrue)
	{
		for (size_t i = 0; i < node->statements().size(); i++)
		{
			visit(node->statements()[i],result);
			if (result.tag() == LVariable::BREAK)
				return result.reset();
			if (result.tag() == LVariable::RETURN)
				return;
			if (result.tag() == LVariable::CONTINUE)
				break;
		}

		for (size_t i = 0; i < node->update_statements().size(); i++)
			visit(node->update_statements()[i], result);
		visit(node->expr(), result);
		bTrue = check_condition(result);
	}

	return result.reset();
}
void LInterpreter::visit_WhileCompound(WhileCompound* node, LVariable& result)
{
	visit(node->expr(), result);
	bool bTrue = check_condition(result);
	while (bTrue)
	{
		for (size_t i = 0; i < node->statements().size(); i++)
		{
			visit(node->statements()[i],result);
			if (result.tag() == LVariable::BREAK)
				return result.reset();
			if (result.tag() == LVariable::RETURN)
				return;
			if (result.tag() == LVariable::CONTINUE)
				break;
		}

		visit(node->expr(), result);
		bTrue = check_condition(result);
	}

	return result.reset();
}
void LInterpreter::visit_Break(BreakStatement* node, LVariable& res)
{
	res.setTag(LVariable::BREAK);
}
void LInterpreter::visit_Continue(ContinueStatement* node, LVariable& res)
{
	res.setTag(LVariable::CONTINUE);
}
void LInterpreter::visit_Return(ReturnStatement* node, LVariable& res)
{
	visit(node->expr(), res);
	res.setTag(LVariable::RETURN);
}
void LInterpreter::visit_Include(IncludeStatement* node, LVariable& res)
{
	for (size_t i = 0; i<node->globals().size(); i++)
	{
		if (node->globals()[i]->type() == AST::ASSIGN)
			visit(node->globals()[i],res);
	}
	return res.reset();
}
void LInterpreter::visit_Builtin(BuiltinStatement* node, LVariable& res)
{
	if (node->token().value() == "print")
	{
		vector<AST*>& exprs = node->exprs();
		for (size_t i = 0; i < exprs.size(); i++)
		{
			LVariable &result=res;
			visit(exprs[i], result);
			if (result.type() == LVariable::INT)
				re_printf(_INTFMT, result.intValue());
			else if (result.type() == LVariable::FLOAT)
				re_printf("%f ", result.floatValue());
			else if (result.type() == LVariable::STRING)
			{
				if (m_pFileOut)
					fprintf(m_pFileOut, "%s ", result.strValue().c_str());
				else
					printf("%s ", result.strValue().c_str());
			}
			else if (result.type() == LVariable::POINTER)
				re_printf("%p ", result.pointerValue());
			else if (result.type() == LVariable::ARRAY)
			{
				print_object(result);
			}
			else if (result.type() == LVariable::DICT)
			{
				print_object(result);
			}
			else if (result.type() == LVariable::CLASS)
			{
				re_printf("<class %p> ", result.dictValue());
			}
			else if (result.type() == LVariable::EMPTY)
			{
				re_printf("<undefined> ");
			}
			else if (result.type() == LVariable::NONE)
			{
				re_printf("<null> ");
			}
		}
		if (exprs.size()>0)
			re_printf("\n");
	}
	else if (node->token().value() == "_loadlib")
	{
		vector<AST*>& exprs = node->exprs();

		if (exprs.size() == 1)
		{
			LVariable filename;
			visit(exprs[0], filename);
			if (filename.type() == LVariable::STRING)
			{
                std::string libPath = filename.strValue();
                void* handle = m_dll.loadlib(libPath);
                
                if (!handle) 
				{
                    handle = m_dll.loadlib(filename.strValue());
                }

				LVariable &result = res;
				result.setType(LVariable::POINTER);
				result.setPointer(handle);
				return;
			}
		}
		return res.reset();
	}

	else if (node->token().value() == "_freelib")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() == 1)
		{
			LVariable handle;
			visit(exprs[0], handle);
			if (handle.type() == LVariable::POINTER)
			{
				m_dll.freelib(handle.pointerValue());
			}
		}
		return res.reset();
	}
	else if (node->token().value() == "_calllib")
	{
		vector<LVariable> params;
		vector<AST*>& exprs = node->exprs();
		for (size_t i = 0; i < exprs.size(); i++)
		{
			LVariable result;
			visit(exprs[i], result);
			params.push_back(result);
		}
		LVariable &result=res;
		result.setType(LVariable::NONE);
		m_dll.calllib(params, result);
		return;
	}
	else if (node->token().value() == "_createthread")
	{
		LVariable &result=res;
		LVariable param0;
		vector<AST*>& exprs = node->exprs();
		if (exprs.size()>0)
			visit(exprs[0],param0);

		AST* save = NULL;
		if (param0.type() == LVariable::POINTER)
		{
			save = (AST*)param0.pointerValue();
			if (param0.info().find("func_type") != param0.info().end())
			{
				if (param0.info()["func_type"] != "func")
				{
					error("error function type for thread.", node->token());
					return;
				}
			}
		}
		if (save == NULL)
		{
			error("not found thread function name.", node->token());
			return;
		}

		FunctionExec* newnode = new FunctionExec(save, LToken());
		for (size_t i = 1; i < exprs.size(); i++)
			((FunctionExec*)newnode)->exprs().push_back(exprs[i]);

		threadParams *pThreadparams = new threadParams;
		pThreadparams->func = newnode;
		pThreadparams->globaldata = m_parser.global_data();
		pThreadparams->callstack = this->get_callstack();

#ifdef _WIN32
		result = LVariable(1);
		if (_beginthread(newthread, 0, pThreadparams) == -1)
		{
			//printf("create thread error");
			delete newnode;
			delete pThreadparams;
			result = LVariable(0);
		}
#else
		result = LVariable(1);
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_t tid;
		if (pthread_create(&tid, &attr, newthread, pThreadparams))
		{
			//printf("create thread error");
			delete newnode;
			delete pThreadparams;
			result = LVariable(0);
		}
#endif
		return;
	}
	else if (node->token().value() == "_createlock")
	{
		void* handle = m_parser.global_data()->lock().create_lock();
		LVariable &result=res;
		result.setType(LVariable::POINTER);
		result.setPointer(handle);
		return;
	}
	else if (node->token().value() == "_lock")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() != 1)
			error("lock() takes only 1 argument.\r\n", node->token());
		else
		{
			LVariable result;
			visit(exprs[0], result);
			if (result.type() == LVariable::POINTER)
				m_parser.global_data()->lock().lock(result.pointerValue());
		}
	}
	else if (node->token().value() == "_unlock")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() != 1)
			error("unlock() takes only 1 argument.\r\n", node->token());
		else
		{
			LVariable result;
			visit(exprs[0], result);
			if (result.type() == LVariable::POINTER)
				m_parser.global_data()->lock().unlock(result.pointerValue());
		}
	}
	else if (node->token().value() == "_len")
	{
		vector<LVariable> results;
		vector<AST*>& exprs = node->exprs();
		if (exprs.size()!=1)
			error("len() takes only 1 argument.\r\n", node->token());
		else
		{
			LVariable result;
			visit(exprs[0], result);
			if (result.type() == LVariable::ARRAY)
			{
				if (result.arrValue() == NULL)
				{
					res = LVariable(0);
					return;
				}
				_INT size = result.arrValue()->size();
				res = LVariable(size);
				return;
			}
			else if (result.type() == LVariable::DICT)
			{
				if (result.dictValue() == NULL)
				{
					res = LVariable(0);
					return;
				}
				_INT size = result.dictValue()->size();
				res = LVariable(size);
				return;
			}
			else if (result.type() == LVariable::STRING)
			{
				_INT size = result.strValue().size();
				res = LVariable(size);
				return;
			}
			else
				error("object has no _len()\r\n", node->token());
		}

	}
	else if (node->token().value() == "_str")
	{
		vector<LVariable> results;
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() != 1)
			error("str() takes only 1 argument.\r\n", node->token());
		else
		{
			LVariable result;
			visit(exprs[0], result);
			if (result.type() == LVariable::INT)
			{
				_INT value = result.intValue();
				ostringstream   os;
				os << value;
				res = LVariable(os.str());
				return;
			}
			else if (result.type() == LVariable::FLOAT)
			{
				double value = result.floatValue();
				ostringstream   os;
				os << value;
				res = LVariable(os.str());
				return;
			}
			else if (result.type() == LVariable::POINTER)
			{
				void* value = result.pointerValue();
				ostringstream   os;
				os << value;
				res = LVariable(os.str());
				return;
			}
			else if (result.type() == LVariable::STRING)
			{
				res = result;
				return;
			}
			else
				error("object has no _str()\r\n", node->token());
		}

	}
	else if (node->token().value() == "_int")
	{
		vector<LVariable> results;
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() != 1)
			error("int() takes only 1 argument.\r\n", node->token());
		else
		{
			LVariable result;
			visit(exprs[0], result);
			if (result.type() == LVariable::STRING)
			{
				string value = result.strValue();
				res = LVariable(_ATOI(value.c_str()));
				return;
			}
			else if (result.type() == LVariable::FLOAT)
			{
				double value = result.floatValue();
				ostringstream   os;
				os << value;
				res = LVariable(_ATOI(os.str().c_str()));
				return;
			}
			else if (result.type() == LVariable::INT)
			{
				res = result;
				return;
			}
			else
				error("object has no _int()\r\n", node->token());
		}

	}
	else if (node->token().value() == "_float")
	{
		vector<LVariable> results;
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() != 1)
			error("float() takes only 1 argument.\r\n", node->token());
		else
		{
			LVariable result;
			visit(exprs[0], result);
			if (result.type() == LVariable::STRING)
			{
				string value = result.strValue();
				res = LVariable().setDouble(atof(value.c_str()));
				return;
			}
			else if (result.type() == LVariable::INT)
			{
				_INT value = result.intValue();
				ostringstream   os;
				os << value;
				res = LVariable().setDouble(atof(os.str().c_str()));
				return;
			}
			else if (result.type() == LVariable::FLOAT)
			{
				res = result;
				return;
			}
			else
				error("object has no _float()\r\n", node->token());
		}

	}
	else if (node->token().value() == "_type")
	{
		vector<LVariable> results;
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() != 1)
			error("type() takes only 1 argument.\r\n", node->token());
		else
		{
			LVariable result;
			visit(exprs[0], result);
			if (result.type() == LVariable::INT)
				res = LVariable("INT");
			else if (result.type() == LVariable::FLOAT)
				res = LVariable("FLOAT");
			else if (result.type() == LVariable::STRING)
				res = LVariable("STRING");
			else if (result.type() == LVariable::ARRAY)
				res = LVariable("ARRAY");
			else if (result.type() == LVariable::DICT)
				res = LVariable("DICT");
			else if (result.type() == LVariable::CLASS)
				res = LVariable("CLASS");
			else if (result.type() == LVariable::POINTER)
				res = LVariable("HANDLE");
			else if (result.type() == LVariable::NONE)
				res = LVariable("NULL");
			else if (result.type() == LVariable::EMPTY)
				res = LVariable("UNDEFINED");
			else
				res = LVariable("UNKNOWN");
			return;
		}

	}
	else if (node->token().value() == "sprintf")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() >= 1)
		{
			LVariable fmt;
			visit(exprs[0], fmt);
			if (fmt.type() == LVariable::STRING)
			{
				vector<LVariable> vecArgs;
				for (size_t i = 1; i < exprs.size(); i++)
				{
					LVariable result;
					visit(exprs[i], result);
					vecArgs.push_back(result);
				}
				string format = fmt.strValue();
				string err=Tool::mysprintf(format, vecArgs);
				if (err.size()>0)
					error(err, node->token());
				else
				{
					res = LVariable(format);
					return;
				}
			}

		}
		else
			error("sprintf() arguments number error\r\n", node->token());

		res = LVariable("");//empty str
		return;
	}
	else if (node->token().value() == "printf")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() >= 1)
		{
			LVariable fmt;
			visit(exprs[0], fmt);
			if (fmt.type() == LVariable::STRING)
			{
				vector<LVariable> vecArgs;
				for (size_t i = 1; i < exprs.size(); i++)
				{
					LVariable result;
					visit(exprs[i], result);
					vecArgs.push_back(result);
				}
				string format = fmt.strValue();
				string err;
				if (vecArgs.size()>0)
					err = Tool::mysprintf(format, vecArgs);
				if (err.size()>0)
					error(err, node->token());
				else
				{
					if (m_pFileOut)
						fprintf(m_pFileOut, "%s", format.c_str());
					else
						printf("%s", format.c_str());
				}
			}
			else
				error("printf() argument type error\r\n", node->token());

		}
		else
			error("printf() arguments number error\r\n", node->token());

	}
	else if (node->token().value() == "_getargv")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() >= 0)
		{
			string argvName = ARGV_ARRAY_NAME;
			if (exprs.size() == 1)
			{
				LVariable argValue;
				visit(exprs[0], argValue);
				argvName = argValue.strValue();
			}

			ActivationRecord* ar = NULL;
			ar = &m_callstack.base();
			res = ar->get__global_value(argvName);
			return;
		}
	}
	else if (node->token().value() == "_input")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() == 0)
		{
			char buff[1024];
			buff[0] = 0;
			fgets(buff, 1024, stdin);
			res = LVariable(buff);
			return;
		}
	}
	else if (node->token().value() == "_copy")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() == 1)
		{
			LVariable old;
			visit(exprs[0], old);
			copy_object(old, res);
			return;
		}
	}
	else if (node->token().value() == "_fun")
	{
		LVariable ret;
		ret.setType(LVariable::NONE);
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() >0)
		{
			vector<LVariable> args;
			for (size_t i = 0; i < exprs.size(); i++)
			{
				LVariable result;
				visit(exprs[i], result);
				args.push_back(result);
			}
			Func::call_func(args, ret);
		}
		res = ret;
		return;
	}


	return res.reset();
}
void LInterpreter::visit_Class(FunctionExec* node, LVariable& res)
{
	ClassStatement* cls = (ClassStatement*)node->fun_statement();
	exec_class(cls, node->exprs(), node->token(),res);
}
void LInterpreter::visit_Function(FunctionStatement* node, LVariable& res)
{
	string::size_type pos = node->name().rfind(".");
	if ( pos != string::npos)//class function
	{
		string fun_name = node->name().substr(pos + 1);
		ActivationRecord& ar = m_callstack.peek();

		LVariable& self = ar.get_value("self");
		LVariable &fun=res;
		fun.setType(LVariable::POINTER);
		fun.setPointer(node);
		fun.info()["func_type"] = "member";
		(*self.dictValue())[fun_name] = fun;
	}
	return res.reset();
}

void LInterpreter::visit_FunctionExec(FunctionExec* node, LVariable& res)
{ 
	if (node->is_var())
	{
		LVariable &var=res;
		var.setType(LVariable::POINTER);
		var.setPointer(node->fun_statement());
		var.info()["func_type"] = "func";
		if (node->fun_statement()->type() == AST::CLASS)
			var.info()["func_type"] = "class";

		return;
	}

	if (node->fun_statement()->type() == AST::CLASS)
		return visit_Class(node,res);

	FunctionStatement* fun = (FunctionStatement*)node->fun_statement();
	return exec_function(fun, node->exprs(), node->token(),res);
}

void LInterpreter::exec_function(FunctionStatement* fun, vector<AST*>& exprs, LToken& token, LVariable& res)
{
	if (fun->type() == AST::EMPTY)
		return res.reset();

	LVariable &result = res;
	vector<LVariable> params_pass;
	for (size_t i = 0; i < exprs.size(); i++)
	{
		visit(exprs[i], result);
		if (result.type() == LVariable::EMPTY)
		{
			error("argument value error for " + fun->name() + "\r\n", token);
			return res.reset();
		}
		result.index().clear();
		params_pass.push_back(result);
	}

	vector<string>& params = fun->params();
	if (params.size() < params_pass.size())
	{
		error("argument number error for " + fun->name() + "\r\n", token);
		return res.reset();
	}
	vector<AST*>& params_value = fun->params_value();

	ActivationRecord new_ar(fun->name(), "function", 2);
	for (size_t i = 0; i < params.size(); i++)
	{
		if (i >= 0 && i<params_pass.size())
			new_ar.set_value(params[i], params_pass[i]);
		else
		{
			visit(params_value[i], result);
			if (result.type() == LVariable::EMPTY)
			{
				error("no default argument or default argument value error for " + fun->name() + "\r\n", token);
				return res.reset();
			}
			result.index().clear();
			new_ar.set_value(params[i], result);
		}
	}

	m_callstack.push(new_ar);
	for (size_t i = 0; i < fun->statements().size(); i++)
	{
		visit(fun->statements()[i],result);
		if (result.tag() == LVariable::RETURN)
		{
			m_callstack.pop();
			result.setTag(LVariable::NORMAL);
			return;
		}
		if (result.tag() == LVariable::BREAK || result.tag() == LVariable::CONTINUE)
		{
			error("break/continue is not in loop", fun->statements()[i]->token());
			break;
		}
	}
	m_callstack.pop();

	return res.reset();
}

void LInterpreter::exec_class(ClassStatement* cls, vector<AST*>& exprs, LToken& token, LVariable& res)
{
	LVariable &result=res;
	vector<LVariable> params_pass;
	for (size_t i = 0; i < exprs.size(); i++)
	{
		visit(exprs[i], result);
		if (result.type() == LVariable::EMPTY)
		{
			error("argument value error for " + cls->name() + "\r\n", token);
			return res.reset();
		}
		result.index().clear();
		params_pass.push_back(result);
	}

	vector<string>& params = cls->params();
	if (params.size() < params_pass.size())
	{
		error("argument number error for " + cls->name() + "\r\n", token);
		return res.reset();
	}
	vector<AST*>& params_value = cls->params_value();

	ActivationRecord new_ar(cls->name(), "class", 2);
	for (size_t i = 0; i < params.size(); i++)
	{
		if (i >= 0 && i<params_pass.size())
			new_ar.set_value(params[i], params_pass[i]);
		else
		{
			visit(params_value[i], result);
			if (result.type() == LVariable::EMPTY)
			{
				error("no default argument or default argument value error for " + cls->name() + "\r\n", token);
				return res.reset();
			}
			result.index().clear();
			new_ar.set_value(params[i], result);
		}
	}
	LVariable self;
	self.setDict();
	self.setType(LVariable::CLASS);
	new_ar.set_value("self", self);

	m_callstack.push(new_ar);
	for (size_t i = 0; i < cls->statements().size(); i++)
	{
		visit(cls->statements()[i],result);
		if (result.tag() == LVariable::RETURN)
		{
			//m_callstack.pop();
			result.setTag(LVariable::NORMAL);
			//return result;
			if (result.type() != LVariable::NONE && result.type() != LVariable::EMPTY)
				error("class " + cls->name()+" cannot return non-null type \r\n", cls->token());
			break;
		}
		if (result.tag() == LVariable::BREAK || result.tag() == LVariable::CONTINUE)
		{
			error("break/continue is not in loop", cls->statements()[i]->token());
			break;
		}
	}
	new_ar = m_callstack.peek();
	m_callstack.pop();

	res = new_ar.get_value("self");
}

LVariable LInterpreter::interpret()
{
	AST* tree = m_parser.parse();
	
	if (m_parser.error_msg().size() > 0)
		return m_parser.error_msg();
	
	LVariable result;
	
	visit(tree, result);
	
	if (error_msg().size() > 0)
		return LVariable(error_msg());

	return result;
}

void LInterpreter::visit_Index(AST* obj, AST* idx, LVariable& res)
{
	LVariable var;
	visit(obj, var);
	LVariable index;
	visit(idx, index);
	get_index_value(var, index,res);
}

void LInterpreter::visit_Member(AST* obj, AST* member, LVariable& res)
{
	LVariable &ret=res;
	ret.setType(LVariable::NONE);

	LVariable var;
	visit(obj, var);
	
	Member* memb = (Member*)member;
	vector<AST*>& exprs = memb->exprs();
	vector<LVariable> args;
	for (size_t i = 0; i < memb->exprs().size(); i++)
	{
		LVariable result;
		visit(memb->exprs()[i], result);
		args.push_back(result);
	}

	if (var.type() == LVariable::VARTYPE::STRING)
	{
		if (!m_libString.call_member(memb->token().value(), var, args, ret))
			error(m_libString.err_msg(), memb->token());
	}
	else if (var.type() == LVariable::VARTYPE::ARRAY)
	{
		if (!m_libArray.call_member(memb->token().value(), var, args, ret))
			error(m_libArray.err_msg(), memb->token());
	}
	else if (var.type() == LVariable::VARTYPE::DICT)
	{
		if (!m_libDict.call_member(memb->token().value(), var, args, ret))
			error(m_libDict.err_msg(), memb->token());
	}
	else if (var.type() == LVariable::VARTYPE::CLASS)
	{
		if (m_libClass.call_member(memb->token().value(), var, args, ret))
			return;
		if (m_libClass.err_msg().size() > 0)
		{
			error(m_libClass.err_msg(), memb->token());
			return;
		}

		if (exprs.size() == 0)//member
		{
			LVariable idx = LVariable(memb->token().value());
			LVariable var_value;
			get_index_value(var, idx, var_value);
			bool bSelf = false;
			if (var.info().find("name") != var.info().end() && var.index().size() == 0)
			{
				if (var.info()["name"] == "self")
					bSelf = true;
			}
			if (!bSelf)
			{
				if (idx.strValue().size() > 2 && idx.strValue().substr(0, 2) == "__")//private member
				{
					error("cannot access private member [" + idx.strValue() + "]\r\n", memb->token());
					return;
				}
			}
			if (var_value.type() != LVariable::EMPTY || bSelf)
			{
				res = var_value;
				return;
			}
			else
			{
				error("not found member [" + idx.strValue() + "]\r\n", memb->token());
				return;
			}
		}
		else//function
		{
			AST* save = NULL;
			if (var.dictValue()->find(memb->token().value()) != var.dictValue()->end())
				save = (AST*)(*var.dictValue())[memb->token().value()].pointerValue();
			if (save == NULL)
			{
				error("not found class function name [" + memb->token().value() + "]\r\n", memb->token());
				return;
			}

			bool bSelf = false;
			if (var.info().find("name") != var.info().end() && var.index().size() == 0)
			{
				if (var.info()["name"] == "self")
					bSelf = true;
			}
			if (!bSelf)
			{
				if (memb->token().value().size() > 2 && memb->token().value().substr(0, 2) == "__")//private function
				{
					error("cannot access private function [" + memb->token().value() + "]\r\n", memb->token());
					return;
				}
			}

			FunctionStatement* fun = (FunctionStatement*)save;
			LVariable& fun_value = (*var.dictValue())[memb->token().value()];
			if (fun_value.info().find("func_type") != fun_value.info().end())
			{
				if (fun_value.info()["func_type"] == "func")
					return exec_function(fun, exprs, memb->token(),res);
				if (fun_value.info()["func_type"] == "class")
				{
					ClassStatement* cls = (ClassStatement*)fun;
					return exec_class(cls, exprs, memb->token(),res);
				}
			}

			vector<string>& params = fun->params();
			size_t paramCount = args.size();
			if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
				paramCount = 0;
			if (params.size() < paramCount + 1)
			{
				error("argument number error for [" + fun->name() + "]\r\n", memb->token());
				return;
			}

			vector<AST*>& params_value = fun->params_value();

			ActivationRecord new_ar(fun->name(), "function", 2);
			var.index().clear();
			new_ar.set_value(params[0], var);//self
			for (size_t i = 0; i < params.size()-1; i++)
			{
				if (i >= 0 && i < args.size() && (args.size()>0 && args[0].type()!=LVariable::EMPTY))
				{
					args[i].index().clear();
					new_ar.set_value(params[i + 1], args[i]);
				}
				else
				{
					LVariable result;
					visit(params_value[i], result);
					if (result.type() == LVariable::EMPTY)
					{
						error("no default argument or default argument value error for " + fun->name() + "\r\n", memb->token());
						return;
					}
					result.index().clear();
					new_ar.set_value(params[i + 1], result);
				}

			}

			LVariable result;
			m_callstack.push(new_ar);
			for (size_t i = 0; i < fun->statements().size(); i++)
			{
				visit(fun->statements()[i],result);
				if (result.tag() == LVariable::RETURN)
				{
					m_callstack.pop();
					result.setTag(LVariable::NORMAL);
					res = result;
					return;
				}
				if (result.tag() == LVariable::BREAK || result.tag() == LVariable::CONTINUE)
				{
					error("break/continue is not in loop", fun->statements()[i]->token());
					break;
				}
			}
			m_callstack.pop();

		}
	}
	else
	{
		char buff[512];
		sprintf(buff, "Object [%s] not found function name [%s]\r\n", obj->token().value().c_str(), memb->token().value().c_str());
		error(buff, memb->token());
	}

}

void LInterpreter::visit_Not(AST* node, LVariable& res)
{
	int ret = 0;
	LVariable value;
	visit(node, value);
	if (value.type() == LVariable::INT)
	{
		if (!value.intValue())
			ret = 1;
	}
	else if (value.type() == LVariable::FLOAT)
	{
		if (!value.floatValue())
			ret = 1;
	}
	else if (value.type() == LVariable::NONE || value.type() == LVariable::EMPTY)
	{
		ret = 1;
	}
	else if (value.type() == LVariable::STRING)
	{
		if (value.strValue().size() == 0)
			ret = 1;
	}
	else if (value.type() == LVariable::ARRAY)
	{
		if (value.arrValue()->size() == 0)
			ret = 1;
	}
	else if (value.type() == LVariable::DICT)
	{
		if (value.dictValue()->size() == 0)
			ret = 1;
	}
	else if (value.type() == LVariable::CLASS)
	{
		ret = 0;
	}
	else if (value.type() == LVariable::POINTER)
	{
		if (value.pointerValue() == 0)
			ret = 1;
	}

	res.setInt(ret);
}

void LInterpreter::copy_object(LVariable& object, LVariable& newObj)
{
	if (object.type() == LVariable::ARRAY)
	{
		newObj.setArray(object.arrValue()->size());
		for (size_t i = 0; i < object.arrValue()->size(); i++)
		{
			if ((*object.arrValue())[i].type() == LVariable::ARRAY ||
				(*object.arrValue())[i].type() == LVariable::DICT ||
				(*object.arrValue())[i].type() == LVariable::CLASS
				)
			{
				copy_object((*object.arrValue())[i], (*newObj.arrValue())[i]);
			}
			else
				(*newObj.arrValue())[i] = (*object.arrValue())[i];
		}
		return;
	}
	if (object.type() == LVariable::DICT || object.type() == LVariable::CLASS)
	{
		newObj.setDict();
		newObj.setType(object.type());
		newObj.setPointer(object.pointerValue());
		newObj.setInfo(object.info());
		map<LVariable, LVariable>::iterator iter = object.dictValue()->begin();
		for (; iter != object.dictValue()->end(); iter++)
		{
			if (iter->second.type() == LVariable::ARRAY ||
				iter->second.type() == LVariable::DICT ||
				iter->second.type() == LVariable::CLASS
				)
			{
				copy_object(iter->second, (*newObj.dictValue())[iter->first]);
			}
			else
				(*newObj.dictValue())[iter->first] = iter->second;
		}
		return;

	}

	newObj = object;
}

void LInterpreter::print_object(LVariable& object)
{
	if (object.type() == LVariable::ARRAY)
	{
		re_printf("[");
		for (size_t i = 0; i < object.arrValue()->size(); i++)
		{
			if ((*object.arrValue())[i].type() == LVariable::ARRAY ||
				(*object.arrValue())[i].type() == LVariable::DICT
				)
			{
				print_object((*object.arrValue())[i]);
			}
			else
			{
				LVariable& result = (*object.arrValue())[i];
				if (result.type() == LVariable::INT)
					re_printf(_INTFMT, result.intValue());
				else if (result.type() == LVariable::FLOAT)
					re_printf("%f ", result.floatValue());
				else if (result.type() == LVariable::STRING)
				{
					if (m_pFileOut)
						fprintf(m_pFileOut, "\"%s\" ", result.strValue().c_str());
					else
						printf("\"%s\" ", result.strValue().c_str());
				}
				else if (result.type() == LVariable::POINTER)
					re_printf("%p ", result.pointerValue());
				else if (result.type() == LVariable::CLASS)
				{
					re_printf("<class %p> ", result.dictValue());
				}
				else if (result.type() == LVariable::EMPTY)
				{
					re_printf("<undefined> ");
				}
				else if (result.type() == LVariable::NONE)
				{
					re_printf("<null> ");
				}

			}
			if (i< object.arrValue()->size() - 1)
				re_printf(", ");
		}
		re_printf("] ");
	}
	if (object.type() == LVariable::DICT)
	{
		re_printf("{");
		size_t i = 0;
		size_t count = object.dictValue()->size();
		map<LVariable, LVariable>::iterator iter = object.dictValue()->begin();
		for (; iter != object.dictValue()->end(); iter++)
		{
			const LVariable& result = iter->first;
			if (result.type() == LVariable::INT)
				re_printf(_INTFMT, result.intValue());
			else if (result.type() == LVariable::FLOAT)
				re_printf("%f ", result.floatValue());
			else if (result.type() == LVariable::STRING)
			{
				if (m_pFileOut)
					fprintf(m_pFileOut, "\"%s\" ", result.strValue().c_str());
				else
					printf("\"%s\" ", result.strValue().c_str());
			}
			else if (result.type() == LVariable::POINTER)
				re_printf("%p ", result.pointerValue());
			else if (result.type() == LVariable::ARRAY)
			{
				LVariable key = iter->first;
				re_printf("<array %p> ", key.arrValue());
			}
			else if (result.type() == LVariable::DICT)
			{
				LVariable key = iter->first;
				re_printf("<dict %p> ", key.dictValue());
			}
			else if (result.type() == LVariable::CLASS)
			{
				LVariable key = iter->first;
				re_printf("<class %p> ", key.dictValue());
			}
			else if (result.type() == LVariable::EMPTY)
			{
				re_printf("<undefined> ");
			}
			else if (result.type() == LVariable::NONE)
			{
				re_printf("<null> ");
			}

			re_printf(": ");

			if (iter->second.type() == LVariable::ARRAY ||
				iter->second.type() == LVariable::DICT
				)
			{
				print_object(iter->second);
			}
			else
			{
				LVariable& result2 = iter->second;
				if (result2.type() == LVariable::INT)
					re_printf(_INTFMT, result2.intValue());
				else if (result2.type() == LVariable::FLOAT)
					re_printf("%f ", result2.floatValue());
				else if (result2.type() == LVariable::STRING)
				{
					if (m_pFileOut)
						fprintf(m_pFileOut, "\"%s\" ", result2.strValue().c_str());
					else
						printf("\"%s\" ", result2.strValue().c_str());
				}
				else if (result2.type() == LVariable::POINTER)
					re_printf("%p ", result2.pointerValue());
				else if (result2.type() == LVariable::CLASS)
				{
					re_printf("<class %p> ", result2.dictValue());
				}
				else if (result2.type() == LVariable::EMPTY)
				{
					re_printf("<undefined> ");
				}
				else if (result2.type() == LVariable::NONE)
				{
					re_printf("<null> ");
				}

			}
			if (i<count - 1)
				re_printf(", ");
			i++;
		}
		re_printf("} ");

	}

}