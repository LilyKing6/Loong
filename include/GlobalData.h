#pragma once
#include "AST.h"
#include "Library.h"

class LGlobalData
{
public:
	LGlobalData();
	~LGlobalData();
	
	map<string, AST*>& functions(){ return m_functions; }
	
	map<string, bool>& globals(){ return m_globals; }
	
	vector<AST*>& all_nodes(){ return m_vecNodes; }
	
	Lock& lock(){ return m_lock; }
	void clear_all_nodes();
	void clear_globals();
private:
	
	map<string, AST*> m_functions;
	
	map<string, bool> m_globals;
	
	vector<AST*> m_vecNodes;
	
	Lock m_lock;
};


class LGlobalCheck
{
public:
	
	vector<AST*>& nodes(){ return m_global_nodes; }
	
	map<string, bool>& params(){ return m_param_vars; }
	
	map<string, bool>& assigns(){ return m_assign_vars; }

private:
	
	vector<AST*> m_global_nodes;
	
	map<string, bool> m_param_vars;
	
	map<string, bool> m_assign_vars;
};


class LCheckStack
{
public:
	
	void push();
	
	void pop();
	
	LGlobalCheck& top();
	
	void add_node(AST* node);
	
	void add_param(string name);
	
	void add_assign(string name);
private:
	
	vector<LGlobalCheck> m_stack;
};
