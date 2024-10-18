#pragma once
#include "Variable.h"
#include "AutoLock.h"
#include <string>
#include <map>

using namespace std;

#ifdef _WIN32
	#include<windows.h>
#else
	#include<dlfcn.h>
	#include <cstring>
	#define sprintf sprintf
#endif

class Tool
{
public:
    
	static string mysprintf(string& format, vector<LVariable>& vecArgs);
	
	static string readfile(const string& strFilename);
	
	static string url_escape(const string& URL);

	static string url_unescape(const string& URL);

	static int str_count(const string& text, const string& str);

	static void str_replace(string& text, const string& str_old, const string& str_new);

	static void str_split(const string& str, const string& splitstr, vector<string>& vecStr);

	static LVariable interpreter(const string& code, const string& outputfile, const vector<LVariable>& vecArgv, const LVariable& globalValue, string filename);
};

class File
{
public:
    
	void* open(const char *filename, const char *mode);

	void close(void* handle);
	
	bool read(void* handle, _INT size, LVariable& result);
	
	bool write(void* handle, LVariable& content);
	
	bool seek(void* handle, _INT pos);
	
	_INT size(void* handle);
};

class Dll
{
public:
    
	void* loadlib(const string& filename);
	
	void freelib(void* handle);
	
	bool calllib(const vector<LVariable>& args, LVariable& arrResult);
};

class Lock
{
public:
    
	Lock();
	~Lock();
	
	void* create_lock();
	
	void lock(void* handle);
	
	void unlock(void* handle);
private:
	vector<LLock*> m_vecLocks;  
	LLock m_lock;               
};

class Lib
{
public:
    
	enum LIBMEMBER 
	{ 
		SUBSTR,  
		FIND,    
		REPLACE, 
		SIZE,    
		RESIZE,  
		APPEND,  
		BEGIN,   
		END,     
		RBEGIN,  
		REND,    
		NEXT,    
		GET,     
		ERASE,   
		INSERT,  
		CLEAR,   
		SPLIT,   
		GETPTR,  
		RESTORE, 
		RFIND,   
		TRIM,    
		LTRIM,   
		RTRIM,   
		LOWER,   
		UPPER,   
		SORT,    
		SWAP,    
		CREATE2D,
		CREATE3D 
	};

	Lib(){}
	~Lib(){}

	virtual bool call_member(const string& name, LVariable& var, const vector<LVariable>& args, LVariable& ret) { return false; }
	
	void error(string err) { m_err = err; }
	
	string err_msg() { return m_err; }
	
	static bool compare(const LVariable& a, const LVariable& b);
private:
	string m_err;               
protected:
	map<string, LIBMEMBER> m_mapMembers;  
};

class Lib_String : public Lib
{
public:
    
	Lib_String();
	~Lib_String();
	
	bool call_member(const string& name, LVariable& var, const vector<LVariable>& args, LVariable& ret);
};

class Lib_Array : public Lib
{
public:
    
	Lib_Array();
	~Lib_Array();
	
	bool call_member(const string& name, LVariable& var, vector<LVariable>& args, LVariable& ret);
};

class Lib_Dict : public Lib
{
public:
    
	Lib_Dict();
	~Lib_Dict();
	
	bool call_member(const string& name, LVariable& var, const vector<LVariable>& args, LVariable& ret);
};

class Lib_Class : public Lib
{
public:
    
	Lib_Class();
	~Lib_Class();
	
	bool call_member(const string& name, LVariable& var, const vector<LVariable>& args, LVariable& ret);
};

class Func
{
public:
    
	static bool call_func(const vector<LVariable>& args, LVariable& ret);
	
	static bool math_func(const vector<LVariable>& args, LVariable& ret);
	
	static bool set_func(const vector<LVariable>& args, LVariable& ret);
};

