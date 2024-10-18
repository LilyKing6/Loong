#include "Library.h"
#include "Interpreter.h"
#include "Sal.h"

#include <time.h>
#include <algorithm>
#include <regex>
#include <math.h>

#ifdef _WIN32
	#include <fcntl.h>
	#include <io.h>
	#include <Winsock2.h>
	#pragma comment(lib,"WS2_32.lib")
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <netdb.h>   
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <signal.h>
#endif

#define PARAM_INT 1
#define PARAM_STR 2
#define PARAM_DOUBLE 3
#define PARAM_POINTER 4

struct PARAM{
	void* pValue;
	int nSize; 
	int nType;
};

struct PARAMS{
	int nCount;
	int nIndex; 
	PARAM* pParams;
};

LInterpreter* _pInterpreter = NULL;

void dll_callback(PARAMS* pParam)
{
	if (pParam)
	{
		if (pParam->nCount > 0)
		{
			string args;
			vector<LVariable> vecArgv;
			if (pParam->pParams[0].nType == PARAM_POINTER)
			{
				LVariable fun;
				fun.setType(LVariable::POINTER);
				fun.setPointer((void*)(*(unsigned _INT*)pParam->pParams[0].pValue));
				fun.info()["func_type"] = "func";
				vecArgv.push_back(fun);
			}
			for (int i = 1; i < pParam->nCount; i++)
			{
				if (pParam->pParams[i].nType == PARAM_STR)
				{
					string str((char*)pParam->pParams[i].pValue, pParam->pParams[i].nSize);
					vecArgv.push_back(LVariable(str));
					char buff[256];
					sprintf(buff, "_getargv(\"__temp_argv__\")[%d]", i);
					if (args.size() == 0)
						args = buff;
					else
						args = args + "," + buff;
				}
			}

			LVariable globalValue;
			if (_pInterpreter)
			{
				LCallStack& callstack = _pInterpreter->get_callstack();
				ActivationRecord& base = callstack.base();
				globalValue = base.get_value(GLOBAL_DICT_NAME);
			}
			string strCode = "main() { fun=_getargv(\"__temp_argv__\")[0]; fun(" + args + "); }";
			char buff[128];
			sprintf(buff, "temp%d%d", rand(), rand());
			Tool::interpreter(strCode, "", vecArgv, globalValue, buff);
		}
	}
}

typedef void(*callback)(PARAMS* pParam);

void* Dll::loadlib(const string& filename)
{
	void* hDLL = NULL;
#ifdef _WIN32
	string::size_type pos = filename.rfind("\\");
	if (pos != string::npos)
	{
		string strPath = filename.substr(0, pos + 1);
		SetDllDirectoryA(strPath.c_str());
	}

	hDLL = LoadLibraryA(filename.c_str());
#else
	hDLL = dlopen(filename.c_str(), RTLD_LAZY);
#endif

	return hDLL;
}


void Dll::freelib(void* handle)
{
#ifdef _WIN32
	FreeLibrary((HINSTANCE)handle);
#else
	dlclose(handle);
#endif
}

bool Dll::calllib(const vector<LVariable>& args, LVariable& arrResult)
{
	if (args.size() < 2)
		return false;

	void* hDLL = NULL;
	if (args[0].type() != LVariable::POINTER)
		return false;
	hDLL = args[0].pointerValue();
	if (hDLL == NULL)
		return false;

	string fun = args[1].strValue();

	typedef PARAMS*(*pFun)(int count);
	pFun create_params = NULL;
#ifdef _WIN32
	create_params = (pFun)GetProcAddress((HINSTANCE)hDLL, "create_params");
#else
	create_params = (pFun)dlsym(hDLL, "create_params");
#endif

	if (create_params)
	{
		typedef void(*pInt)(PARAMS* pParam, _INT value);
		pInt add_int = NULL;
		typedef void(*pDouble)(PARAMS* pParam, double value);
		pDouble add_double = NULL;
		typedef void(*pString)(PARAMS* pParam, const char* value, _INT size);
		pString add_string = NULL;
		typedef void(*pPointer)(PARAMS* pParam, void* value);
		pPointer add_ptr = NULL;
		typedef PARAMS*(*pCall)(PARAMS* pParam);
		pCall function = NULL;
#ifdef _WIN32
		add_int = (pInt)GetProcAddress((HINSTANCE)hDLL, "add_int");
		add_double = (pDouble)GetProcAddress((HINSTANCE)hDLL, "add_double");
		add_string = (pString)GetProcAddress((HINSTANCE)hDLL, "add_string");
		add_ptr = (pPointer)GetProcAddress((HINSTANCE)hDLL, "add_ptr");
		function = (pCall)GetProcAddress((HINSTANCE)hDLL, fun.c_str());
#else
		add_int = (pInt)dlsym(hDLL, "add_int");
		add_double = (pDouble)dlsym(hDLL, "add_double");
		add_string = (pString)dlsym(hDLL, "add_string");
		add_ptr = (pPointer)dlsym(hDLL, "add_ptr");
		function = (pCall)dlsym(hDLL, fun.c_str());
#endif

		bool bCallback = false;
		for (size_t i = 2; i < args.size(); i++)
		{
			LVariable varPointer = args[i];
			if (varPointer.info().find("func_type") != varPointer.info().end())
			{
				if (varPointer.info()["func_type"] == "func")
				{
					bCallback = true;
					break;
				}
			}
		}

		int nParams = (int)args.size() - 2;
		if (bCallback)
			nParams++;
		PARAMS* pParam = create_params(nParams);

		for (size_t i = 2; i < args.size(); i++)
		{
			if (args[i].type() == LVariable::INT)
			{
				add_int(pParam, args[i].intValue());
			}
			if (args[i].type() == LVariable::FLOAT)
			{
				add_double(pParam, args[i].floatValue());
			}
			if (args[i].type() == LVariable::STRING)
			{
				add_string(pParam, args[i].strValue().c_str(), args[i].strValue().size());
			}
			if (args[i].type() == LVariable::POINTER)
			{
				add_ptr(pParam, args[i].pointerValue());
			}
		}
		if (bCallback)
			add_ptr(pParam, (callback*)(dll_callback));

		PARAMS* pRetParam = function(pParam);
		if (pRetParam)
		{
			int n = pRetParam->nCount;

			arrResult.setArray(n);
			for (int i = 0; i < n; i++)
			{
				if (pRetParam->pParams[i].nType == PARAM_INT)
				{
					(*arrResult.arrValue())[i] = LVariable(*(int*)pRetParam->pParams[i].pValue);
				}
				else if (pRetParam->pParams[i].nType == PARAM_DOUBLE)
				{
					(*arrResult.arrValue())[i] = LVariable().setDouble(*(double*)pRetParam->pParams[i].pValue);
				}
				else if (pRetParam->pParams[i].nType == PARAM_STR)
				{
					string str((char*)pRetParam->pParams[i].pValue, pRetParam->pParams[i].nSize);
					(*arrResult.arrValue())[i] = LVariable(str);
				}
				else if (pRetParam->pParams[i].nType == PARAM_POINTER)
				{
					LVariable result;
					result.setType(LVariable::POINTER);
					result.setPointer((void*)(*(unsigned _INT*)pRetParam->pParams[i].pValue));
					(*arrResult.arrValue())[i] = result;
				}
			}
		}
		else
			arrResult.setArray(0);

		typedef void*(*pDel)(PARAMS* param);
		pDel delete_params = NULL;
#ifdef _WIN32
		delete_params = (pDel)GetProcAddress((HINSTANCE)hDLL, "delete_params");
#else
		delete_params = (pDel)dlsym(hDLL, "delete_params");
#endif
		if (delete_params)
		{
			delete_params(pParam);
			delete_params(pRetParam);
		}
	}

	return true;
}

Lock::Lock()
{
}
Lock::~Lock()
{
	for (size_t i = 0; i < m_vecLocks.size(); i++)
	{
		if (m_vecLocks[i])
			delete m_vecLocks[i];
	}
	m_vecLocks.clear();
}
void* Lock::create_lock()
{
	AutoLock lock(m_lock);

	LLock *pLock = new LLock();
	m_vecLocks.push_back(pLock);

	return pLock;
}
void Lock::lock(void* handle)
{
	if (handle)
		((LLock*)handle)->Lock();
}
void Lock::unlock(void* handle)
{
	if (handle)
		((LLock*)handle)->UnLock();
}

string Tool::mysprintf(string& format, vector<LVariable>& vecArgs)
{
	const string err1 = "sprintf argument number error\r\n";
	const string err2 = "sprintf argument type error\r\n";
	if (format.size() < 2)
		return err1;

	size_t index = 0;
	size_t pos = 0;
	while (format[pos] != 0)
	{

		if (format[pos] == '%')
		{
			size_t tPos = pos + 1;
			while (format[tPos] != 's' && format[tPos] != 'c'
				&& format[tPos] != 'd' && format[tPos] != 'f'
				&& format[tPos] != 'x' && format[tPos] != 'X'
				&& format[tPos] != 'o'
				&& format[tPos] != 0)
				tPos++;
			if (format[tPos] == 's')
			{
				string fmt = format.substr(pos, tPos - pos + 1);
				if (index >= vecArgs.size())
					return err1;
				if (vecArgs[index].type() != LVariable::STRING)
					return err2;
				if (vecArgs[index].strValue().size() < 0)
					return err1;
				char *buff = new char[vecArgs[index].strValue().size() + 1];
				buff[vecArgs[index].strValue().size()] = 0;
				sprintf(buff, fmt.c_str(), vecArgs[index].strValue().c_str());
				format.replace(pos, fmt.size(), buff);
				pos += strlen(buff);
				delete[] buff;
				index++;
			}
			else if (format[tPos] == 'c')
			{
				string fmt = format.substr(pos, tPos - pos + 1);
				if (index >= vecArgs.size())
					return err1;
				if (vecArgs[index].type() != LVariable::INT && vecArgs[index].type() != LVariable::STRING)
					return err2;
				if (vecArgs[index].type() == LVariable::STRING && vecArgs[index].strValue().size() != 1)
					return err2;
				char buff[1024];
				if (vecArgs[index].type() == LVariable::INT)
					sprintf(buff, fmt.c_str(), vecArgs[index].intValue());
				else if (vecArgs[index].type() == LVariable::STRING)
					sprintf(buff, fmt.c_str(), vecArgs[index].strValue()[0]);
				format.replace(pos, fmt.size(), buff);
				pos += strlen(buff);
				index++;
			}
			else if (format[tPos] == 'd')
			{
				string fmt = format.substr(pos, tPos - pos + 1);
				size_t fmt_size = fmt.size();
				if (sizeof(_INT) == 8)
					fmt.replace(fmt_size - 1, 1, "lld");

				if (index >= vecArgs.size())
					return err1;
				if (vecArgs[index].type() != LVariable::INT && vecArgs[index].type() != LVariable::STRING)
					return err2;
				if (vecArgs[index].type() == LVariable::STRING && vecArgs[index].strValue().size() != 1)
					return err2;
				char buff[1024];
				if (vecArgs[index].type() == LVariable::INT)
					sprintf(buff, fmt.c_str(), vecArgs[index].intValue());
				else if (vecArgs[index].type() == LVariable::STRING)
					sprintf(buff, fmt.c_str(), vecArgs[index].strValue()[0]);
				format.replace(pos, fmt_size, buff);
				pos += strlen(buff);
				index++;
			}
			else if (format[tPos] == 'x' || format[tPos] == 'X' || format[tPos] == 'o')
			{
				string fmt = format.substr(pos, tPos - pos + 1);

				if (index >= vecArgs.size())
					return err1;
				if (vecArgs[index].type() != LVariable::INT && vecArgs[index].type() != LVariable::STRING)
					return err2;
				if (vecArgs[index].type() == LVariable::STRING && vecArgs[index].strValue().size() != 1)
					return err2;
				char buff[1024];
				if (vecArgs[index].type() == LVariable::INT)
					sprintf(buff, fmt.c_str(), vecArgs[index].intValue());
				else if (vecArgs[index].type() == LVariable::STRING)
					sprintf(buff, fmt.c_str(), vecArgs[index].strValue()[0]);
				format.replace(pos, fmt.size(), buff);
				pos += strlen(buff);
				index++;
			}
			else if (format[tPos] == 'f')
			{
				string fmt = format.substr(pos, tPos - pos + 1);
				if (index >= vecArgs.size())
					return err1;
				if (vecArgs[index].type() != LVariable::FLOAT)
					return err2;
				char buff[1024];
				sprintf(buff, fmt.c_str(), vecArgs[index].floatValue());
				format.replace(pos, fmt.size(), buff);
				pos += strlen(buff);
				index++;
			}
			else
				break;
		}
		else
			pos++;

		if (pos + 1 >= format.size())
			break;
	}
	return "";
}


string Tool::readfile(const string& strFilename)
{
	FILE *fp = fopen(strFilename.c_str(), "rb");
	if (fp == NULL)
		return "";

	fseek(fp, 0, SEEK_END);
	_INT size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* buffer = new char[size + 1];
	_INT ret = fread(buffer, 1, sizeof(char) * size, fp);
	string strbuf = string(buffer, ret);
	delete[] buffer;

	fclose(fp);

	return strbuf;
}


char dec2hexChar(short int n) {
	if (0 <= n && n <= 9) {
		return char(short('0') + n);
	}
	else if (10 <= n && n <= 15) {
		return char(short('A') + n - 10);
	}
	else {
		return char(0);
	}
}


short int hexChar2dec(char c) {
	if ('0' <= c && c <= '9') {
		return short(c - '0');
	}
	else if ('a' <= c && c <= 'f') {
		return (short(c - 'a') + 10);
	}
	else if ('A' <= c && c <= 'F') {
		return (short(c - 'A') + 10);
	}
	else {
		return -1;
	}
}



string Tool::url_escape(const string &URL)
{
	string result = "";
	for (size_t i = 0; i < URL.size(); i++) {
		char c = URL[i];
		if (
			('0' <= c && c <= '9') ||
			('a' <= c && c <= 'z') ||
			('A' <= c && c <= 'Z') ||
			c == '/' || c == '.'
			) {
			result += c;
		}
		else {
			int j = (short int)c;
			if (j < 0) {
				j += 256;
			}
			int i1, i0;
			i1 = j / 16;
			i0 = j - i1 * 16;
			result += '%';
			result += dec2hexChar(i1);
			result += dec2hexChar(i0);
		}
	}
	return result;
}


string Tool::url_unescape(const string& URL) {
	string result = "";
	for (size_t i = 0; i < URL.size(); i++) {
		char c = URL[i];
		if (c != '%') {
			result += c;
		}
		else {
			char c1 = URL[++i];
			char c0 = URL[++i];
			int num = 0;
			num += hexChar2dec(c1) * 16 + hexChar2dec(c0);
			result += char(num);
		}
	}
	return result;
}


int Tool::str_count(const string& text, const string& str)
{
	int count = 0;
	string::size_type pos1 = 0;
	string::size_type pos2 = text.find(str, pos1);
	while (pos2 != string::npos)
	{
		count++;
		pos1 = pos2 + str.size();
		pos2 = text.find(str, pos1);
	}
	return count;
}


void Tool::str_replace(string& text, const string& str_old, const string& str_new)
{
	for (string::size_type pos(0); pos != string::npos; pos += str_new.length())
	{
		pos = text.find(str_old, pos);
		if (pos != string::npos)
			text.replace(pos, str_old.length(), str_new);
		else
			break;
	}
}


void Tool::str_split(const string& str, const string& splitstr, vector<string>& vecStr)
{
	string::size_type pos1, pos2;
	pos2 = str.find(splitstr);
	pos1 = 0;
	while (string::npos != pos2)
	{
		vecStr.push_back(str.substr(pos1, pos2 - pos1));

		pos1 = pos2 + splitstr.size();
		pos2 = str.find(splitstr, pos1);
	}
	vecStr.push_back(str.substr(pos1));
}


LVariable Tool::interpreter(const string& code, const string& outputfile, const vector<LVariable>& vecArgv, const LVariable& globalValue, string filename)
{
	FILE *out = NULL;
	if (outputfile.size() > 0)
		out = fopen(outputfile.c_str(), "w");

	string curdir;
#ifdef _WIN32
	string slash = "\\";
#else
	string slash = "/";
#endif		
	string::size_type pos = filename.rfind(slash);
	if (pos != string::npos)
	{
		curdir = filename.substr(0, pos + 1);
		filename = filename.substr(pos + 1);
	}

	LGlobalData globaldata;
	LLexer lexer(code, filename);
	LParser parser = LParser(lexer, &globaldata);
	parser.set_outfile(out);
	parser.set_curdir(curdir);

	LInterpreter interpreter = LInterpreter(parser);
	interpreter.set_outfile(out);
	if (globalValue.type() == LVariable::DICT)
	{
		interpreter.set_globalvalue(globalValue);
		interpreter.set_argvname("__temp_argv__");
	}
	interpreter.set_argv(vecArgv);
	LVariable ret = interpreter.interpret();

	if (out)
		fclose(out);

	return ret;
}


void* File::open(const char *filename, const char *mode)
{
	FILE *fp = fopen(filename, mode);
	return fp;
}


void File::close(void* handle)
{
	FILE* fp = (FILE*)handle;
	if (fp)
		fclose((FILE*)handle);
}


bool File::read(void* handle, _INT size, LVariable& result)
{
	if (size <= 0)
		return false;

	FILE* fp = (FILE*)handle;
	if (fp)
	{
		char* buffer = new char[size + 1];
		_INT ret = fread(buffer, 1, sizeof(char) * size, fp);
		string strbuf = string(buffer, ret);
		delete[] buffer;
		result = LVariable(strbuf);
		return true;
	}
	return false;
}


bool File::write(void* handle, LVariable& content)
{
	FILE* fp = (FILE*)handle;;
	if (fp)
	{
		if (content.type() == LVariable::STRING)
		{
			const char* buffer = content.strValue().c_str();
			_INT size = content.strValue().size();
			fwrite(buffer, 1, sizeof(char) * size, fp);
		}
		else if (content.type() == LVariable::INT)
		{
			_INT value = content.intValue();
			fwrite(&value, 1, sizeof(_INT) * 1, fp);
		}
		else if (content.type() == LVariable::FLOAT)
		{
			double value = content.floatValue();
			fwrite(&value, 1, sizeof(double) * 1, fp);
		}
		return true;
	}
	return false;
}


_INT File::size(void* handle)
{
	FILE* fp = (FILE*)handle;
	if (fp)
	{
#ifdef _WIN64
		_fseeki64(fp, 0, SEEK_END);
		_INT len = _ftelli64(fp);
		return len;
#else
		fseek(fp, 0, SEEK_END);
		_INT len = ftell(fp);
		return len;
#endif
	}

	return -1;
}


bool File::seek(void* handle, _INT pos)
{
	FILE* fp = (FILE*)handle;
	if (fp)
	{
#ifdef _WIN64
		_fseeki64(fp, pos, SEEK_SET);
#else
		fseek(fp, (long)pos, SEEK_SET);
#endif
		return true;
	}

	return false;
}


bool Lib::compare(const LVariable& a, const LVariable& b) {
	
	if (a.type() != b.type())
	{
		if (a.type() < b.type())
			return true;
	}
	else
	{
		if (a.type() == LVariable::INT)
		{
			if (a.intValue() < b.intValue())
				return true;
		}
		if (a.type() == LVariable::FLOAT)
		{
			if (a.floatValue() < b.floatValue())
				return true;
		}
		if (a.type() == LVariable::STRING)
		{
			if (a.strValue() < b.strValue())
				return true;
		}
	}

	return false;
}


Lib_String::Lib_String()
{
	srand((unsigned)time(NULL));
	m_mapMembers["substr"] = SUBSTR;
	m_mapMembers["find"] = FIND;
	m_mapMembers["rfind"] = RFIND;
	m_mapMembers["replace"] = REPLACE;
	m_mapMembers["size"] = SIZE;
	m_mapMembers["split"] = SPLIT;
	m_mapMembers["insert"] = INSERT;
	m_mapMembers["erase"] = ERASE;
	m_mapMembers["trim"] = TRIM;
	m_mapMembers["ltrim"] = LTRIM;
	m_mapMembers["rtrim"] = RTRIM;
	m_mapMembers["lower"] = LOWER;
	m_mapMembers["upper"] = UPPER;
}

Lib_String::~Lib_String()
{
}

bool Lib_String::call_member(const string& name, LVariable& var, const vector<LVariable>& args, LVariable& ret)
{
	map<string, LIBMEMBER>::const_iterator itr = m_mapMembers.find(name);
	if (itr == m_mapMembers.end())
	{
		error("member " + name + " not found.\r\n");
		return false;
	}

	if (itr->second == SUBSTR)
	{
		if (args.size() == 1)
		{
			LVariable pos = args[0];
			if (pos.type() == LVariable::INT)
			{
				if (pos.intValue() >= 0)
				{
					string str = var.strValue().substr(pos.intValue());
					ret = LVariable(str);
				}
			}
			return true;
		}
		if (args.size() == 2)
		{
			LVariable pos = args[0];
			LVariable len = args[1];
			if (pos.type() == LVariable::INT && len.type() == LVariable::INT)
			{
				if (pos.intValue() >= 0 && len.intValue() >= 0)
				{
					string str = var.strValue().substr(pos.intValue(), len.intValue());
					ret = LVariable(str);
				}
			}
			return true;
		}
		else
			error("substr() arguments number error\r\n");
	}

	if (itr->second == FIND)
	{
		if (args.size() == 1)
		{
			if (var.type() == LVariable::STRING)
			{
				LVariable key = args[0];
				_INT pos = -1;
				if (key.type() == LVariable::STRING)
					pos = var.strValue().find(key.strValue());
				ret = LVariable(pos);
			}
			return true;
		}
		else if (args.size() == 2)
		{
			if (var.type() == LVariable::STRING)
			{
				LVariable key = args[0];
				LVariable start = args[1];
				_INT pos = -1;
				if (key.type() == LVariable::STRING)
					pos = var.strValue().find(key.strValue(), start.intValue());
				ret = LVariable(pos);
			}
			return true;
		}
		else
			error("find() arguments number error\r\n");
	}
	if (itr->second == RFIND)
	{
		if (args.size() == 1)
		{
			if (var.type() == LVariable::STRING)
			{
				LVariable key = args[0];
				_INT pos = -1;
				if (key.type() == LVariable::STRING)
					pos = var.strValue().rfind(key.strValue());
				ret = LVariable(pos);
			}
			return true;
		}
		else if (args.size() == 2)
		{
			if (var.type() == LVariable::STRING)
			{
				LVariable key = args[0];
				LVariable start = args[1];
				_INT pos = -1;
				if (key.type() == LVariable::STRING)
					pos = var.strValue().rfind(key.strValue(), start.intValue());
				ret = LVariable(pos);
			}
			return true;
		}
		else
			error("rfind() arguments number error\r\n");
	}

	if (itr->second == REPLACE)
	{
		if (args.size() == 2)
		{
			if (var.type() == LVariable::STRING)
			{
				string str = var.strValue();
				LVariable oldstr = args[0];
				LVariable newstr = args[1];
				if (oldstr.type() == LVariable::STRING && newstr.type() == LVariable::STRING)
				{
					string before = oldstr.strValue();
					string after = newstr.strValue();
					for (string::size_type pos(0); pos != string::npos; pos += after.length())
					{
						pos = str.find(before, pos);
						if (pos != string::npos)
							str.replace(pos, before.length(), after);
						else
							break;
					}
					ret = LVariable(str);
				}
			}
			return true;
		}
		else if (args.size() == 3)
		{
			if (var.type() == LVariable::STRING)
			{
				string str = var.strValue();
				LVariable pos = args[0];
				LVariable len = args[1];
				LVariable dest = args[2];
				if (pos.type() == LVariable::INT && len.type() == LVariable::INT && dest.type() == LVariable::STRING)
				{
					if (pos.intValue() >= 0)
						str.replace(pos.intValue(), len.intValue(), dest.strValue());
					ret = LVariable(str);
				}
			}
			return true;
		}
		else
			error("replace() arguments number error\r\n");
	}

	if (itr->second == SPLIT)
	{
		if (args.size() == 1)
		{
			if (var.type() == LVariable::STRING && args[0].type() == LVariable::STRING)
			{
				string str = var.strValue();
				string splitstr = args[0].strValue();
				
				LVariable vartemp;
				vartemp.setArray(0);

				string::size_type pos1, pos2;
				pos2 = str.find(splitstr);
				pos1 = 0;
				while (string::npos != pos2)
				{
					vartemp.arrValue()->push_back(str.substr(pos1, pos2 - pos1));

					pos1 = pos2 + splitstr.size();
					pos2 = str.find(splitstr, pos1);
				}
				vartemp.arrValue()->push_back(str.substr(pos1));
				ret = vartemp;
			}
			return true;
		}
		else
			error("split() arguments number error\r\n");
	}

	if (itr->second == SIZE)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.type() == LVariable::STRING)
			{
				ret = LVariable((_INT)var.strValue().size());
			}
			return true;
		}
		else
			error("size() arguments number error\r\n");
	}

	if (itr->second == INSERT)
	{
		if (args.size() == 2)
		{
			if (var.type() == LVariable::STRING)
			{
				LVariable pos = args[0];
				LVariable str = args[1];
				string temp = var.strValue();
				if (pos.type() == LVariable::INT && pos.intValue() >= 0 && str.type() == LVariable::STRING)
					temp.insert(pos.intValue(), str.strValue());
				ret = LVariable(temp);
			}
			return true;
		}
		else
			error("insert() arguments number error\r\n");
	}
	if (itr->second == ERASE)
	{
		if (args.size() == 2)
		{
			if (var.type() == LVariable::STRING)
			{
				LVariable pos = args[0];
				LVariable len = args[1];
				string temp = var.strValue();
				if (pos.type() == LVariable::INT && pos.intValue() >= 0 && len.type() == LVariable::INT)
					temp.erase(pos.intValue(), len.intValue());
				ret = LVariable(temp);
			}
			return true;
		}
		else
			error("erase() arguments number error\r\n");
	}

	if (itr->second == TRIM)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.type() == LVariable::STRING)
			{
				string value = var.strValue();
				string::size_type i = 0;
				for (i = 0; i < value.size(); i++) {
					if (value[i] != ' ' &&
						value[i] != '\t' &&
						value[i] != '\n' &&
						value[i] != '\r')
						break;
				}
				value = value.substr(i);
				for (i = value.size() - 1; i >= 0; i--) {
					if (value[i] != ' ' &&
						value[i] != '\t' &&
						value[i] != '\n' &&
						value[i] != '\r')
						break;
				}
				ret = LVariable(value.substr(0, i + 1));
			}
			return true;
		}
		else
			error("trim() arguments number error\r\n");
	}
	if (itr->second == LTRIM)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.type() == LVariable::STRING)
			{
				string::size_type i = 0;
				for (i = 0; i < var.strValue().size(); i++) {
					if (var.strValue()[i] != ' ' &&
						var.strValue()[i] != '\t' &&
						var.strValue()[i] != '\n' &&
						var.strValue()[i] != '\r')
						break;
				}
				ret = LVariable(var.strValue().substr(i));
			}
			return true;
		}
		else
			error("ltrim() arguments number error\r\n");
	}
	if (itr->second == RTRIM)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.type() == LVariable::STRING)
			{
				string::size_type i = 0;
				for (i = var.strValue().size() - 1; i >= 0; i--) {
					if (var.strValue()[i] != ' ' &&
						var.strValue()[i] != '\t' &&
						var.strValue()[i] != '\n' &&
						var.strValue()[i] != '\r')
						break;
				}
				ret = LVariable(var.strValue().substr(0, i + 1));
			}
			return true;
		}
		else
			error("rtrim() arguments number error\r\n");
	}

	if (itr->second == LOWER)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.type() == LVariable::STRING)
			{
				string newvalue(var.strValue());
				for (string::size_type i = 0; i < newvalue.size(); i++)
					newvalue[i] = tolower(newvalue[i]);
				ret = LVariable(newvalue);
			}
			return true;
		}
		else
			error("lower() arguments number error\r\n");
	}
	if (itr->second == UPPER)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.type() == LVariable::STRING)
			{
				string newvalue(var.strValue());
				for (string::size_type i = 0; i < newvalue.size(); i++)
					newvalue[i] = toupper(newvalue[i]);
				ret = LVariable(newvalue);
			}
			return true;
		}
		else
			error("upper() arguments number error\r\n");
	}

	return false;
}

Lib_Array::Lib_Array()
{
	m_mapMembers["append"] = APPEND;
	m_mapMembers["size"] = SIZE;
	m_mapMembers["resize"] = RESIZE;
	m_mapMembers["clear"] = CLEAR;
	m_mapMembers["erase"] = ERASE;
	m_mapMembers["insert"] = INSERT;
	m_mapMembers["_getptr"] = GETPTR;
	m_mapMembers["_restore"] = RESTORE;
	m_mapMembers["sort"] = SORT;
	m_mapMembers["swap"] = SWAP;
	m_mapMembers["create2d"] = CREATE2D;
	m_mapMembers["create3d"] = CREATE3D;
}

Lib_Array::~Lib_Array()
{
}

bool Lib_Array::call_member(const string& name, LVariable& var, vector<LVariable>& args, LVariable& ret)
{
	map<string, LIBMEMBER>::const_iterator itr = m_mapMembers.find(name);
	if (itr == m_mapMembers.end())
	{
		error("member " + name + " not found.\r\n");
		return false;
	}

	if (itr->second == APPEND)
	{
		if (args.size() == 1)
		{
			if (var.arrValue())
			{
				var.arrValue()->push_back(args[0]);
			}
			return true;
		}
		else
			error("push_back() arguments number error\r\n");
	}

	if (itr->second == SIZE)
	{
		if (args.size() == 1 && args[0].type()==LVariable::EMPTY)
		{
			if (var.arrValue())
			{
				ret = LVariable((_INT)var.arrValue()->size());
			}
			return true;
		}
		else
			error("size() arguments number error\r\n");
	}

	if (itr->second == CLEAR)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.arrValue())
			{
				var.arrValue()->clear();
			}
			return true;
		}
		else
			error("clear() arguments number error\r\n");
	}

	if (itr->second == RESIZE)
	{
		if (args.size() == 1 || args.size() == 2)
		{
			if (var.arrValue())
			{
				if (args[0].type() == LVariable::INT)
				{
					_INT size = args[0].intValue();
					if (args.size() == 1)
						var.arrValue()->resize(size);
					else
						var.arrValue()->resize(size,args[1]);
				}
			}
			return true;
		}
		else
			error("resize() arguments number error\r\n");
	}

	if (itr->second == ERASE)
	{
		if (args.size() == 1 || args.size() == 2)
		{
			if (var.arrValue())
			{
				if (args[0].type() == LVariable::INT)
				{
					_INT index = args[0].intValue();
					if (index >= 0 && (size_t)index < var.arrValue()->size())
					{
						if (args.size() == 2)
						{
							_INT index2 = args[1].intValue();
							if (index2 >= index && (size_t)index2 < var.arrValue()->size())
							{
								var.arrValue()->erase(var.arrValue()->begin() + index, var.arrValue()->begin() + index2);
								ret = LVariable(1);
							}
						}
						else
						{
							var.arrValue()->erase(var.arrValue()->begin() + index);
							ret = LVariable(1);
						}
					}
					else
						ret = LVariable(0);
				}
			}
			return true;
		}
		else
			error("erase() arguments number error\r\n");
	}

	if (itr->second == INSERT)
	{
		if (args.size() == 2)
		{
			if (var.arrValue())
			{
				if (args[0].type() == LVariable::INT)
				{
					_INT index = args[0].intValue();
					if (index >= 0 && (size_t)index < var.arrValue()->size())
					{
						var.arrValue()->insert(var.arrValue()->begin() + index,args[1]);
						ret = LVariable(1);
					}
					else
						ret = LVariable(0);
				}
			}
			return true;
		}
		else
			error("insert() arguments number error\r\n");
	}
	if (itr->second == GETPTR)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.arrValue())
			{
				ret.setType(LVariable::POINTER);
				ret.setPointer(var.arrValue());
			}
			return true;
		}
		else
			error("getptr() arguments number error\r\n");
	}
	if (itr->second == RESTORE)
	{
		if (args.size() == 1)
		{
			if (var.arrValue())
			{
				ret.setArray(0,(vector<LVariable>*)args[0].pointerValue());
				if (ret.arrValue() == NULL)
					ret.setType(LVariable::NONE);
			}
			return true;
		}
		else
			error("restore() arguments number error\r\n");
	}
	if (itr->second == SORT)
	{
		if (args.size() == 1)
		{
			if (var.arrValue())
			{
				sort(var.arrValue()->begin(), var.arrValue()->end(), compare);
				ret = LVariable(1);
			}
			return true;
		}
		else
			error("restore() arguments number error\r\n");
	}
	if (itr->second == SWAP)
	{
		if (args.size() == 1)
		{
			if (var.arrValue() && args[0].arrValue())
			{
				var.arrValue()->swap(*args[0].arrValue());
			}
			return true;
		}
		else
			error("restore() arguments number error\r\n");
	}
	if (itr->second == CREATE2D)
	{
		if (args.size() >= 2)
		{
			if (var.arrValue())
			{
				_INT d1 = args[0].intValue();
				_INT d2 = args[1].intValue();
				if (d1 >= 0 && d2 >= 0)
				{
					var.arrValue()->resize(d1);
					for (_INT i = 0; i < d1; i++)
					{
						LVariable vartemp;
						if (args.size() == 2)
							vartemp.setArray(d2);
						else
						{
							vartemp.setArray(0);
							vartemp.arrValue()->resize(d2, args[2]);
						}
						(*var.arrValue())[i] = vartemp;
					}
				}

			}
			return true;
		}
		else
			error("restore() arguments number error\r\n");
	}
	if (itr->second == CREATE3D)
	{
		if (args.size() >= 3)
		{
			if (var.arrValue())
			{
				_INT d1 = args[0].intValue();
				_INT d2 = args[1].intValue();
				_INT d3 = args[2].intValue();
				if (d1 >= 0 && d2 >= 0 && d3 >= 0)
				{
					var.arrValue()->resize(d1);
					for (_INT i = 0; i < d1; i++)
					{
						LVariable vartemp;
						vartemp.setArray(d2);
						(*var.arrValue())[i] = vartemp;
						for (_INT j = 0; j < d2; j++)
						{
							LVariable vartemp2;
							if (args.size() == 3)
								vartemp2.setArray(d3);
							else
							{
								vartemp2.setArray(0);
								vartemp2.arrValue()->resize(d3, args[3]);
							}
							(*(*var.arrValue())[i].arrValue())[j] = vartemp2;

						}

					}
				}

			}
			return true;
		}
		else
			error("restore() arguments number error\r\n");
	}

	return false;
}

Lib_Dict::Lib_Dict()
{
	m_mapMembers["find"] = FIND;
	m_mapMembers["size"] = SIZE;
	m_mapMembers["begin"] = BEGIN;
	m_mapMembers["end"] = END;
	m_mapMembers["rbegin"] = RBEGIN;
	m_mapMembers["rend"] = REND;
	m_mapMembers["next"] = NEXT;
	m_mapMembers["get"] = GET;
	m_mapMembers["erase"] = ERASE;
	m_mapMembers["insert"] = INSERT;
	m_mapMembers["clear"] = CLEAR;
	m_mapMembers["_getptr"] = GETPTR;
	m_mapMembers["_restore"] = RESTORE;

}
Lib_Dict::~Lib_Dict()
{
}

bool Lib_Dict::call_member(const string& name, LVariable& var, const vector<LVariable>& args, LVariable& ret)
{
	map<string, LIBMEMBER>::const_iterator itr = m_mapMembers.find(name);
	if (itr == m_mapMembers.end())
	{
		error("member " + name + " not found.\r\n");
		return false;
	}

	if (itr->second == FIND)
	{
		if (args.size() == 1)
		{
			if (var.dictValue())
			{
				if (var.dictValue()->find(args[0]) != var.dictValue()->end())
				{
					ret = (*var.dictValue())[args[0]];
				}
			}
			return true;
		}
		else
			error("find() arguments number error\r\n");
	}

	if (itr->second == ERASE)
	{
		if (args.size() == 1)
		{
			if (var.dictValue())
			{
				int nArgType = 0;
				if (args[0].type() == LVariable::POINTER)
				{
					LVariable arg0 = args[0];
					if (arg0.info().find("reverse_iterator") != arg0.info().end())
						nArgType = 1;
					else if (arg0.info().find("iterator") != arg0.info().end())
						nArgType = 2;
				}

				if (nArgType>0)
				{
					if (nArgType==1)
					{
						map<LVariable, LVariable>::reverse_iterator iter = *(map<LVariable, LVariable>::reverse_iterator*)args[0].pointerValue();
						var.dictValue()->erase((++iter).base());
						ret = args[0];
					}
					else
					{
						map<LVariable, LVariable>::iterator iter = *(map<LVariable, LVariable>::iterator*)args[0].pointerValue();
						var.dictValue()->erase(iter++);
						map<LVariable, LVariable>::iterator* pIter = (map<LVariable, LVariable>::iterator*)args[0].pointerValue();
						*pIter = iter;
						ret = args[0];
					}
				}
				else
				{
					map<LVariable, LVariable>::iterator iter = var.dictValue()->find(args[0]);
					if (iter != var.dictValue()->end())
					{
						var.dictValue()->erase(iter);
						ret = LVariable(1);
					}
					else
						ret = LVariable(0);
				}
			}
			return true;
		}
		else
			error("find() arguments number error\r\n");
	}
	if (itr->second == INSERT)
	{
		if (args.size() == 2)
		{
			if (var.dictValue())
			{
				pair<map<LVariable, LVariable>::iterator, bool> result = var.dictValue()->insert(pair<LVariable, LVariable>(args[0],args[1]));
				if (result.second == true)
					ret = LVariable(1);
				else
					ret = LVariable(0);
			}
			return true;
		}
		else
			error("insert() arguments number error\r\n");
	}
	if (itr->second == SIZE)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.dictValue())
			{
				ret = LVariable((_INT)var.dictValue()->size());
			}
			return true;
		}
		else
			error("size() arguments number error\r\n");
	}
	if (itr->second == CLEAR)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.dictValue())
			{
				var.dictValue()->clear();
			}
			return true;
		}
		else
			error("clear() arguments number error\r\n");
	}
	if (itr->second == GETPTR)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.dictValue())
			{
				ret.setType(LVariable::POINTER);
				ret.setPointer(var.dictValue());
			}
			return true;
		}
		else
			error("getptr() arguments number error\r\n");
	}
	if (itr->second == RESTORE)
	{
		if (args.size() == 1)
		{
			if (var.dictValue())
			{
				ret.setDict((map<LVariable, LVariable>*)args[0].pointerValue());
				if (ret.dictValue() == NULL)
					ret.setType(LVariable::NONE);
			}
			return true;
		}
		else
			error("restore() arguments number error\r\n");
	}

	if (itr->second == BEGIN)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.dictValue())
			{
				LVariable vartemp;
				vartemp.setType(LVariable::POINTER);
				vartemp.info()["iterator"] = "1";
				map<LVariable, LVariable>::iterator iter = var.dictValue()->begin();
				map<LVariable, LVariable>::iterator* pIter = new map<LVariable, LVariable>::iterator;
				*pIter = iter;
				vartemp.setPointer(pIter);
				vartemp.initPointerRef(pIter);
				ret = vartemp;
			}
			return true;
		}
		else
			error("begin() arguments number error\r\n");
	}
	if (itr->second == END)
	{
		if (args.size() == 1 && args[0].type() == LVariable::POINTER)
		{
			if (var.dictValue())
			{
				LVariable vartemp(0);
				map<LVariable, LVariable>::iterator iter = *(map<LVariable, LVariable>::iterator*)args[0].pointerValue();
				if (iter == var.dictValue()->end())
					vartemp = LVariable(1);
				ret = vartemp;
			}
			return true;
		}
		else
			error("end() arguments number error\r\n");
	}
	if (itr->second == NEXT)
	{
		if (args.size() == 1 && args[0].type() == LVariable::POINTER)
		{
			if (var.dictValue())
			{
				LVariable arg0 = args[0];
				if (arg0.info().find("reverse_iterator") != arg0.info().end())
				{
					map<LVariable, LVariable>::reverse_iterator iter = *(map<LVariable, LVariable>::reverse_iterator*)args[0].pointerValue();
					iter++;
					map<LVariable, LVariable>::reverse_iterator* pIter = (map<LVariable, LVariable>::reverse_iterator*)args[0].pointerValue();
					*pIter = iter;
					ret = args[0];
				}
				else
				{
					map<LVariable, LVariable>::iterator iter = *(map<LVariable, LVariable>::iterator*)args[0].pointerValue();
					iter++;
					map<LVariable, LVariable>::iterator* pIter = (map<LVariable, LVariable>::iterator*)args[0].pointerValue();
					*pIter = iter;
					ret = args[0];
				}
			}
			return true;
		}
		else
			error("next() arguments number error\r\n");
	}
	if (itr->second == RBEGIN)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.dictValue())
			{
				LVariable vartemp;
				vartemp.setType(LVariable::POINTER);
				vartemp.info()["reverse_iterator"] = "1";
				map<LVariable, LVariable>::reverse_iterator iter = var.dictValue()->rbegin();
				map<LVariable, LVariable>::reverse_iterator* pIter = new map<LVariable, LVariable>::reverse_iterator;
				*pIter = iter;
				vartemp.setPointer(pIter);
				vartemp.initPointerRef(pIter);
				ret = vartemp;
			}
			return true;
		}
		else
			error("rbegin() arguments number error\r\n");
	}
	if (itr->second == REND)
	{
		if (args.size() == 1 && args[0].type() == LVariable::POINTER)
		{
			if (var.dictValue())
			{
				LVariable vartemp(0);
				map<LVariable, LVariable>::reverse_iterator  iter = *(map<LVariable, LVariable>::reverse_iterator*)args[0].pointerValue();
				if (iter == var.dictValue()->rend())
					vartemp = LVariable(1);
				ret = vartemp;
			}
			return true;
		}
		else
			error("rend() arguments number error\r\n");
	}
	if (itr->second == GET)
	{
		if (args.size() == 1)
		{
			if (var.dictValue())
			{
				LVariable vartemp;
				vartemp.setArray(2);
				int nArgType = 0;
				if (args[0].type() == LVariable::POINTER)
				{
					LVariable arg0 = args[0];
					if (arg0.info().find("reverse_iterator") != arg0.info().end())
						nArgType = 1;
					else if (arg0.info().find("iterator") != arg0.info().end())
						nArgType = 2;
				}

				if (nArgType>0)
				{
					if (nArgType==1)
					{
						map<LVariable, LVariable>::reverse_iterator iter = *(map<LVariable, LVariable>::reverse_iterator*)args[0].pointerValue();
						if (iter != var.dictValue()->rend())
						{
							(*vartemp.arrValue())[0] = iter->first;
							(*vartemp.arrValue())[1] = iter->second;
							ret = vartemp;
						}
					}
					else
					{
						map<LVariable, LVariable>::iterator iter = *(map<LVariable, LVariable>::iterator*)args[0].pointerValue();
						if (iter != var.dictValue()->end())
						{
							(*vartemp.arrValue())[0] = iter->first;
							(*vartemp.arrValue())[1] = iter->second;
							ret = vartemp;
						}
					}
				}
				else
				{
					map<LVariable, LVariable>::iterator iter = var.dictValue()->find(args[0]);
					if (iter != var.dictValue()->end())
					{
						(*vartemp.arrValue())[0] = iter->first;
						(*vartemp.arrValue())[1] = iter->second;
						ret = vartemp;
					}
				}
			}
			return true;
		}
		else
			error("get() arguments number error\r\n");
	}


	return false;
}


Lib_Class::Lib_Class()
{
	m_mapMembers["_getptr"] = GETPTR;
	m_mapMembers["_restore"] = RESTORE;
}


Lib_Class::~Lib_Class()
{
}

bool Lib_Class::call_member(const string& name, LVariable& var, const vector<LVariable>& args, LVariable& ret)
{
	map<string, LIBMEMBER>::const_iterator itr = m_mapMembers.find(name);
	if (itr == m_mapMembers.end())
	{
		//error("member " + name + " not found.\r\n");
		return false;
	}

	if (itr->second == GETPTR)
	{
		if (args.size() == 1 && args[0].type() == LVariable::EMPTY)
		{
			if (var.dictValue())
			{
				ret.setType(LVariable::POINTER);
				ret.setPointer(var.dictValue());
				return true;
			}
		}
		else
			error("getptr() arguments number error\r\n");
	}
	if (itr->second == RESTORE)
	{
		if (args.size() == 1)
		{
			if (var.dictValue())
			{
				ret.setDict((map<LVariable, LVariable>*)args[0].pointerValue());
				ret.setType(LVariable::CLASS);
				if (ret.dictValue() == NULL)
					ret.setType(LVariable::NONE);
				return true;
			}
		}
		else
			error("restore() arguments number error\r\n");
	}

	return false;
}

bool Func::call_func(const vector<LVariable>& args, LVariable& ret)
{
	if (args.size() == 0)
		return false;

	if (args[0].strValue() == "PrintColor")
	{
		if (args.size() == 3)
		{
			LVariable str = args[1];
			LVariable color = args[2];
			if (str.type() == LVariable::STRING && color.type() == LVariable::INT)
			{
				int nColor = (int)color.intValue();
				if (nColor < 0 || nColor > 15) return false;
				enum Color color_enum = (Color)nColor;
				PrintColor(str.strValue().c_str(), color_enum);
				return true;
			}
		}
		return false;
	}

	if (args[0].strValue() == "fopen")
	{
		if (args.size() == 3)
		{
			LVariable filename = args[1];
			LVariable mode = args[2];
			if (filename.type() == LVariable::STRING && mode.type() == LVariable::STRING)
			{
				File file;
				void* handle = file.open(filename.strValue().c_str(), mode.strValue().c_str());
				if (handle)
				{
					ret.setType(LVariable::POINTER);
					ret.setPointer(handle);
					return true;
				}
			}
		}
		return false;
	}
	if (args[0].strValue() == "fclose")
	{
		if (args.size()==2)
		{
			LVariable handle = args[1];
			if (handle.type() == LVariable::POINTER)
			{
				File file;
				file.close(handle.pointerValue());
			}
			return true;
		}
		return false;
	}
	if (args[0].strValue() == "fremove")
	{
		if (args.size() == 2)
		{
			LVariable filename = args[1];
			if (filename.strValue().size() >0)
			{
				int res=remove(filename.strValue().c_str());
				ret = LVariable(res);
			}
			return true;
		}
		return false;
	}
	if (args[0].strValue() == "frename")
	{
		if (args.size() == 3)
		{
			LVariable filename_old = args[1];
			LVariable filename_new = args[2];
			if (filename_old.strValue().size() >0 && filename_new.strValue().size() >0)
			{
				int res = rename(filename_old.strValue().c_str(), filename_new.strValue().c_str());
				ret = LVariable(res);
			}
			return true;
		}
		return false;
	}
	if (args[0].strValue() == "fsize")
	{
		if (args.size() == 2)
		{
			LVariable handle = args[1];
			if (handle.type() == LVariable::POINTER)
			{
				File file;
				_INT size = file.size(handle.pointerValue());
				ret= LVariable(size);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "fseek")
	{
		if (args.size() == 3)
		{
			LVariable handle = args[1];
			LVariable pos = args[2];
			if (handle.type() == LVariable::POINTER && pos.type() == LVariable::INT)
			{
				File file;
				file.seek(handle.pointerValue(), pos.intValue());
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "fread")
	{
		if (args.size() == 3)
		{
			LVariable handle = args[1];
			LVariable size = args[2];
			if (handle.type() == LVariable::POINTER && size.type() == LVariable::INT)
			{
				File file;
				file.read(handle.pointerValue(), size.intValue(), ret);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "fwrite")
	{
		if (args.size() == 3)
		{
			LVariable handle = args[1];
			LVariable content = args[2];
			if (handle.type() == LVariable::POINTER)
			{
				File file;
				bool bret = file.write(handle.pointerValue(), content);
				if (bret)
					ret = LVariable(1);
				else
					ret = LVariable(0);
				return true;
			}
		}
		return false;
	}

#ifdef _WIN32
	if (args[0].strValue() == "sock_listen")
	{
		if (args.size() == 2)
		{
			LVariable port = args[1];
			if (port.type() == LVariable::INT)
			{
				_INT nPort = port.intValue();
				if (nPort < 0)
					return false;

				WORD wVersionRequested;
				WSADATA wsaData;
				int err;

				wVersionRequested = MAKEWORD(1, 1);

				err = WSAStartup(wVersionRequested, &wsaData);
				if (err != 0) {
					return false;
				}

				if (LOBYTE(wsaData.wVersion) != 1 ||
					HIBYTE(wsaData.wVersion) != 1) {
					WSACleanup();
					return false;
				}
				SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);

				SOCKADDR_IN addrSrv;
				addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
				addrSrv.sin_family = AF_INET;
				addrSrv.sin_port = htons((u_short)nPort);

				bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));

				int nRet = listen(sockSrv, 5);
				if (nRet == SOCKET_ERROR)
				{
					WSACleanup();
					return false;
				}

				ret.setType(LVariable::POINTER);
				ret.setPointer((void*)sockSrv);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_close")
	{
		if (args.size() == 2)
		{
			LVariable socket = args[1];
			if (socket.type() == LVariable::POINTER)
			{
				SOCKET sock = (SOCKET)socket.pointerValue();
				closesocket(sock);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_accept")
	{
		if (args.size() == 2)
		{
			LVariable socket = args[1];
			if (socket.type() == LVariable::POINTER)
			{
				SOCKET sockSrv = (SOCKET)socket.pointerValue();
				SOCKADDR_IN addrClient;
				int len = sizeof(SOCKADDR);
				SOCKET sockClient = accept(sockSrv, (SOCKADDR*)&addrClient, &len);

				ret.setType(LVariable::POINTER);
				ret.setPointer((void*)sockClient);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_recv")
	{
		if (args.size() == 3)
		{
			LVariable socket = args[1];
			if (socket.type() == LVariable::POINTER)
			{
				SOCKET sock = (SOCKET)socket.pointerValue();
				int buffsize = 4096;
				buffsize = (int)args[2].intValue();
				if (buffsize <= 0)
					buffsize = 4096;
				char *recvBuf = new char[buffsize];
				int iResult = recv(sock, recvBuf, buffsize, 0);
				if (iResult < 0)
					iResult = 0;
				ret = LVariable(string(recvBuf, iResult));
				delete[]recvBuf;
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_send")
	{
		if (args.size() == 3)
		{
			LVariable socket = args[1];
			LVariable data = args[2];
			if (socket.type() == LVariable::POINTER)
			{
				SOCKET sock = (SOCKET)socket.pointerValue();
				int iResult = send(sock, data.strValue().c_str(), (int)data.strValue().size(), 0);
				ret = LVariable(iResult);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_connect")
	{
		if (args.size() == 3)
		{
			LVariable server = args[1];
			LVariable port = args[2];
			if (server.type() == LVariable::STRING && port.type() == LVariable::INT)
			{
				WORD wVersionRequested;
				WSADATA wsaData;
				int err;

				wVersionRequested = MAKEWORD(1, 1);

				err = WSAStartup(wVersionRequested, &wsaData);
				if (err != 0) {
					return false;
				}

				if (LOBYTE(wsaData.wVersion) != 1 ||
					HIBYTE(wsaData.wVersion) != 1) {
					WSACleanup();
					return false;
				}
				SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);

				hostent* hostInfo = gethostbyname(server.strValue().c_str());
				if (NULL == hostInfo)
					return false;

				SOCKADDR_IN addrSrv;
				memcpy(&addrSrv.sin_addr, &(*hostInfo->h_addr_list[0]), hostInfo->h_length);
				addrSrv.sin_family = AF_INET;
				addrSrv.sin_port = htons((u_short)port.intValue());
				if (connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) == SOCKET_ERROR)
				{
					closesocket(sockClient);
					WSACleanup();
					return false;
				}
				ret.setType(LVariable::POINTER);
				ret.setPointer((void*)sockClient);
				return true;
			}
		}
	}
	if (args[0].strValue() == "sock_remoteaddr")
	{
		if (args.size() == 2)
		{
			LVariable socket = args[1];
			if (socket.type() == LVariable::POINTER)
			{
				SOCKET sock = (SOCKET)socket.pointerValue();
				struct sockaddr_in sa;
				int len = sizeof(sa);
				if(!getpeername(sock, (struct sockaddr *)&sa, &len))
				{
					ret.setArray(0);
					ret.arrValue()->push_back(LVariable(inet_ntoa(sa.sin_addr)));
					ret.arrValue()->push_back(LVariable(ntohs(sa.sin_port)));
					return true;
				}
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_timeout")
	{
		if (args.size() == 3)
		{
			LVariable socket = args[1];
			LVariable timeout = args[2];
			if (socket.type() == LVariable::POINTER)
			{
				SOCKET sock = (SOCKET)socket.pointerValue();
				int nTimeout = (int)timeout.intValue()*1000;
				setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&nTimeout, sizeof(int));
				setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&nTimeout, sizeof(int));
			 }
		}
		return false;
	}
	if (args[0].strValue() == "sock_shutdown")
	{
		if (args.size() == 3)
		{
			LVariable socket = args[1];
			LVariable how = args[2];
			if (socket.type() == LVariable::POINTER)
			{
				SOCKET sock = (SOCKET)socket.pointerValue();
				int nHow = (int)how.intValue();
				shutdown(sock, nHow);
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_ipaddr")
	{
		if (args.size() == 2)
		{
			LVariable domain = args[1];
			if (domain.type() == LVariable::STRING)
			{

				WORD wVersionRequested;
				WSADATA wsaData;
				int err;

				wVersionRequested = MAKEWORD(1, 1);

				err = WSAStartup(wVersionRequested, &wsaData);
				if (err != 0) {
					return false;
				}

				if (LOBYTE(wsaData.wVersion) != 1 ||
					HIBYTE(wsaData.wVersion) != 1) {
					WSACleanup();
					return false;
				}
				hostent* hostInfo = gethostbyname(domain.strValue().c_str());
				if (NULL == hostInfo)
					return false;

				ret.setArray(0);
				char buff[1024];
				for (int i = 0; hostInfo->h_addr_list[i]; i++){
					sprintf(buff,"%s", inet_ntoa(*(struct in_addr*)hostInfo->h_addr_list[i]));
					ret.arrValue()->push_back(LVariable(buff));
				}
				return true;
			}
		}
		return false;
	}
#else
	if (args[0].strValue() == "sock_listen")
	{
		if (args.size() == 2)
		{
			LVariable port = args[1];
			if (port.type() == LVariable::INT)
			{
				_INT nPort = port.intValue();
				if (nPort < 0)
					return false;

				int sockSrv = socket(AF_INET, SOCK_STREAM, 0);

				struct sockaddr_in addrSrv;
				memset(&addrSrv, 0, sizeof(addrSrv));
				addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
				addrSrv.sin_family = AF_INET;
				addrSrv.sin_port = htons(nPort);

				if (::bind(sockSrv, (struct sockaddr*)&addrSrv, sizeof(addrSrv)) < 0)
				{
					return false;
				}
				int nRet = listen(sockSrv, 5);
				if (nRet < 0)
				{
					return false;
				}
				ret = LVariable(sockSrv);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_close")
	{
		if (args.size() == 2)
		{
			LVariable socket = args[1];
			if (socket.type() == LVariable::INT)
			{
				signal(SIGPIPE, SIG_IGN);
				int sock = (int)socket.intValue();
				::close(sock);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_accept")
	{
		if (args.size() == 2)
		{
			LVariable socket = args[1];
			if (socket.type() == LVariable::INT)
			{
				int sockSrv = (int)socket.intValue();
				struct sockaddr_in addrClient;
				socklen_t len = sizeof(addrClient);

				int sockClient = accept(sockSrv, (struct sockaddr*)&addrClient, &len);

				ret = LVariable(sockClient);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_recv")
	{
		if (args.size() == 3)
		{
			LVariable socket = args[1];
			if (socket.type() == LVariable::INT)
			{
				signal(SIGPIPE, SIG_IGN);
				int sock = (int)socket.intValue();
				int buffsize = 4096;
				buffsize = args[2].intValue();
				if (buffsize <= 0)
					buffsize = 4096;
				char *recvBuf = new char[buffsize];
				ssize_t iResult = recv(sock, recvBuf, buffsize, 0);
				if (iResult < 0)
					iResult = 0;
				ret = LVariable(string(recvBuf, iResult));
				delete[]recvBuf;
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_send")
	{
		if (args.size() == 3)
		{
			LVariable socket = args[1];
			LVariable data = args[2];
			if (socket.type() == LVariable::INT)
			{
				signal(SIGPIPE, SIG_IGN);
				int sock = (int)socket.intValue();
				#ifdef __linux__
					ssize_t iResult = send(sock, data.strValue().c_str(), data.strValue().size(), MSG_NOSIGNAL);
				#else
					int set=1;
					setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
					ssize_t iResult = send(sock, data.strValue().c_str(), data.strValue().size(), 0);
				#endif
				ret=LVariable(iResult);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_connect")
	{
		if (args.size() == 3)
		{
			LVariable server = args[1];
			LVariable port = args[2];
			if (server.type() == LVariable::STRING && port.type() == LVariable::INT)
			{
				int sockClient = socket(AF_INET, SOCK_STREAM, 0);

				hostent* hostInfo = gethostbyname(server.strValue().c_str());
				if (NULL == hostInfo)
					return false;

				struct sockaddr_in addrSrv;
				memcpy(&addrSrv.sin_addr, &(*hostInfo->h_addr_list[0]), hostInfo->h_length);
				addrSrv.sin_family = AF_INET;
				addrSrv.sin_port = htons((u_short)port.intValue());
				if (connect(sockClient, (struct sockaddr*)&addrSrv, sizeof(addrSrv)) != 0)
				{
					::close(sockClient);
					return false;
				}
				ret = LVariable(sockClient);
				return true;
			}
		}
	}
	if (args[0].strValue() == "sock_remoteaddr")
	{
		if (args.size() == 2)
		{
			LVariable socket = args[1];
			if (socket.type() == LVariable::INT)
			{
				int sock = (int)socket.intValue();
				struct sockaddr_in sa;
				socklen_t len = sizeof(sa);
				if (!getpeername(sock, (struct sockaddr *)&sa, &len))
				{
					ret.setArray(0);
					ret.arrValue()->push_back(LVariable(inet_ntoa(sa.sin_addr)));
					ret.arrValue()->push_back(LVariable(ntohs(sa.sin_port)));
					return true;
				}
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_shutdown")
	{
		if (args.size() == 3)
		{
			LVariable socket = args[1];
			LVariable how = args[2];
			if (socket.type() == LVariable::INT)
			{
				int sock = (int)socket.intValue();
				int nHow = (int)how.intValue();
				shutdown(sock, nHow);
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_ipaddr")
	{
		if (args.size() == 2)
		{
			LVariable domain = args[1];
			if (domain.type() == LVariable::STRING)
			{

				hostent* hostInfo = gethostbyname(domain.strValue().c_str());
				if (NULL == hostInfo)
					return false;

				ret.setArray(0);
				char buff[1024];
				for (int i = 0; hostInfo->h_addr_list[i]; i++){
					sprintf(buff, "%s", inet_ntoa(*(struct in_addr*)hostInfo->h_addr_list[i]));
					ret.arrValue()->push_back(LVariable(buff));
				}
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_timeout")
	{
		if (args.size() == 3)
		{
			LVariable socket = args[1];
			LVariable timeout = args[2];
			if (socket.type() == LVariable::POINTER)
			{
				#ifdef __linux__
					int sock = (int)socket.intValue();
					int nTimeout = (int)timeout.intValue();
					struct timeval timeout={nTimeout,0};
					setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
					setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
				#endif
			}
		}
		return false;
	}
#endif

	if (args[0].strValue() == "run")
	{
		if (args.size() >= 3)
		{
			const LVariable& text = args[1];
			string outputFile = args[2].strValue();
			vector<LVariable> vecArgv;
			for (size_t i = 3; i < args.size(); i++)
			{
				vecArgv.push_back(args[i]);
				vecArgv[vecArgv.size() - 1].index().clear();
			}

			char buff[128];
			sprintf(buff, "temp%d%d", rand(), rand());
			LVariable result = Tool::interpreter(text.strValue(), outputFile, vecArgv, LVariable(), buff);
			if (result.type() != LVariable::EMPTY)
				ret = result;
			ret.setTag(LVariable::NORMAL);
			return true;
		 }
	}
	if (args[0].strValue() == "url_unescape")
	{
		if (args.size() == 2)
		{
			LVariable str = args[1];
			size_t nSize = str.strValue().size();
			if (nSize>0)
			{
				string strRet = Tool::url_unescape(str.strValue());
				ret = LVariable(strRet);
				return true;
			}
		}
	}
	if (args[0].strValue() == "url_escape")
	{
		if (args.size() == 2)
		{
			LVariable str = args[1];
			size_t nSize = str.strValue().size();
			if (nSize>0)
			{
				string strRet = Tool::url_escape(str.strValue());
				ret = LVariable(strRet);
				return true;
			}
		}
	}
	if (args[0].strValue() == "time_clock")
	{
		if (args.size() == 1)
		{
			_INT t = clock();
			ret = LVariable(t);
			return true;
		}
	}
	if (args[0].strValue() == "time_now")
	{
		if (args.size() == 1)
		{
			time_t now = time(0);
			char* dt = ctime(&now);
			if (dt)
			{
				size_t dtlen = strlen(dt);
				if (dtlen > 0 && dt[dtlen - 1]=='\n')
					dt[dtlen - 1] = 0;
			}
			tm *ltm = localtime(&now);
			LVariable cvar;
			cvar.setDict();
			(*cvar.dictValue())[LVariable("time")] = LVariable((_INT)now);
			(*cvar.dictValue())[LVariable("time_str")] = LVariable(dt);
			(*cvar.dictValue())[LVariable("year")] = LVariable(1900+ltm->tm_year);
			(*cvar.dictValue())[LVariable("mon")] = LVariable(1+ltm->tm_mon);
			(*cvar.dictValue())[LVariable("day")] = LVariable(ltm->tm_mday);
			(*cvar.dictValue())[LVariable("hour")] = LVariable(ltm->tm_hour);
			(*cvar.dictValue())[LVariable("min")] = LVariable(ltm->tm_min);
			(*cvar.dictValue())[LVariable("sec")] = LVariable(ltm->tm_sec);
			ret = cvar;
			return true;
		}
	}
	if (args[0].strValue() == "time_rand")
	{
		if (args.size() == 1)
		{
			int a = rand();
			ret = LVariable(a);
			return true;
		}
	}
	if (args[0].strValue() == "time_sleep")
	{
		if (args.size() == 2)
		{
			int msec = (int)args[1].intValue();
			if (msec < 0)
				msec = 0;
			#ifdef _WIN32
				Sleep(msec);
			#else
				usleep(msec*1000);
			#endif
			return true;
		}
	}
	if (args[0].strValue() == "system")
	{
		if (args.size() == 2)
		{
			const string& text = args[1].strValue();
			system(text.c_str());
			return true;
		}
	}
	if (args[0].strValue() == "encrypt")
	{
		if (args.size() == 3)
		{
			string text = args[1].strValue();
			string key = args[2].strValue();
			if (key.size() > 0)
			{
				for (size_t i = 0; i < text.size(); i++)
					text[i] = text[i] ^ key[i % key.size()];
			}
			ret=LVariable(text);
			return true;
		}
	}
	if (args[0].strValue() == "os_platform")
	{
		if (args.size() == 1)
		{
			#ifdef _WIN32
				string platform = "WIN";
			#else
				#ifdef __linux__
					string platform = "LINUX";
				#else
					string platform = "MAC";
				#endif
			#endif
			if (sizeof(_INT) == 8)
				platform += " x64";
			ret = LVariable(platform);
			return true;
		}
	}
	if (args[0].strValue() == "curdir")
	{
		if (args.size() == 1)
		{
			if (_pInterpreter)
				ret = LVariable(_pInterpreter->get_parser().curdir());
			return true;
		}
	}
	if (args[0].strValue() == "getenv")
	{
		if (args.size() == 2)
		{
			const string& text = args[1].strValue();
			char* pBuf = getenv(text.c_str());
			if (pBuf)
				ret = LVariable(string(pBuf, strlen(pBuf)));
			return true;
		}
	}
	if (args[0].strValue() == "getstdin")
	{
		if (args.size() == 2)
		{
			int nLen = (int)args[1].intValue();
			if (nLen > 0)
			{
#ifdef _WIN32
				_setmode(_fileno(stdin), _O_BINARY);
#endif
				char* pBuf = new char[nLen + 1];
				int i = 0;
				while (i < nLen)
				{
					int x = fgetc(stdin);
					if(feof(stdin))
						break;
					pBuf[i++] = x;
				}
				pBuf[i] = 0;
				if (pBuf)
					ret = LVariable(string(pBuf, i));
				delete[]pBuf;
				return true;
			}
		}
	}
	if (args[0].strValue() == "putstdin")
	{
		if (args.size() == 2)
		{
			const string& strBuf = args[1].strValue();
			if (strBuf.size() > 0)
			{
#ifdef _WIN32
				_setmode(_fileno(stdout), _O_BINARY);
#endif
				for (size_t i = 0; i < strBuf.size(); i++)
				{
					fputc(strBuf[i], stdout);
				}
				
				return true;
			}
		}
	}
	if (args[0].strValue().substr(0, 5) == "math_")
	{
		return math_func(args, ret);
	}
	if (args[0].strValue().substr(0, 4) == "set_")
	{
		return set_func(args, ret);
	}

	return false;
}

bool Func::math_func(const vector<LVariable>& args, LVariable& ret)
{
	if (args[0].strValue() == "math_sqrt")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = sqrt(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_acos")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = acos(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_asin")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = asin(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_atan")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = atan(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_atan2")
	{
		if (args.size() == 3)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			double dVal2 = 0;
			if (args[2].type() == LVariable::INT)
				dVal2 = (double)args[2].intValue();
			else if (args[2].type() == LVariable::FLOAT)
				dVal2 = args[2].floatValue();
			else
				return false;
			dVal = atan2(dVal, dVal2);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_cos")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = cos(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_cosh")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = cosh(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_sin")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = sin(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_sinh")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = sinh(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_tanh")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = tanh(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_exp")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = exp(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_log")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = log(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_frexp")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			int e;
			dVal = frexp(dVal,&e);
			ret.setArray(0);
			ret.arrValue()->push_back(LVariable().setDouble(dVal));
			ret.arrValue()->push_back(LVariable(e));
			return true;
		}
	}
	if (args[0].strValue() == "math_ldexp")
	{
		if (args.size() == 3)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			int nVal2 = 0;
			if (args[2].type() == LVariable::INT)
				nVal2 = (int)args[2].intValue();
			else if (args[2].type() == LVariable::FLOAT)
				nVal2 = (int)args[2].floatValue();
			else
				return false;
			dVal = ldexp(dVal, nVal2);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_log10")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = log10(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_modf")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			double i;
			dVal = modf(dVal, &i);
			ret.setArray(0);
			ret.arrValue()->push_back(LVariable().setDouble(i));
			ret.arrValue()->push_back(LVariable().setDouble(dVal));
			return true;
		}
	}
	if (args[0].strValue() == "math_pow")
	{
		if (args.size() == 3)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			double dVal2 = 0;
			if (args[2].type() == LVariable::INT)
				dVal2 = (double)args[2].intValue();
			else if (args[2].type() == LVariable::FLOAT)
				dVal2 = args[2].floatValue();
			else
				return false;
			dVal = pow(dVal, dVal2);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_ceil")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = ceil(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_fabs")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = fabs(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_floor")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = floor(dVal);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_fmod")
	{
		if (args.size() == 3)
		{
			double dVal = 0;
			if (args[1].type() == LVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == LVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			double dVal2 = 0;
			if (args[2].type() == LVariable::INT)
				dVal2 = (double)args[2].intValue();
			else if (args[2].type() == LVariable::FLOAT)
				dVal2 = args[2].floatValue();
			else
				return false;
			dVal = fmod(dVal, dVal2);
			ret = LVariable().setDouble(dVal);
			return true;
		}
	}

	return false;
}


bool Func::set_func(const vector<LVariable>& args, LVariable& ret)
{
	if (args[0].strValue() == "set_create")
	{
		if (args.size() >= 1)
		{
			set<LVariable>* pSet = new set<LVariable>();
			ret.setType(LVariable::POINTER);
			ret.setPointer(pSet);
			for (size_t i = 1; i < args.size(); i++)
			{
				pSet->insert(args[i]);
			}

			return true;
		}
	}
	if (args[0].strValue() == "set_free")
	{
		if (args.size() == 2)
		{
			LVariable handle = args[1];
			if (handle.type() == LVariable::POINTER)
			{
				set<LVariable>* pSet = (set<LVariable>*)handle.pointerValue();
				delete pSet;
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_size")
	{
		if (args.size() == 2)
		{
			LVariable handle = args[1];
			if (handle.type() == LVariable::POINTER)
			{
				set<LVariable>* pSet = (set<LVariable>*)handle.pointerValue();
				ret = LVariable((_INT)pSet->size());
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_clear")
	{
		if (args.size() == 2)
		{
			LVariable handle = args[1];
			if (handle.type() == LVariable::POINTER)
			{
				set<LVariable>* pSet = (set<LVariable>*)handle.pointerValue();
				pSet->clear();
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_insert")
	{
		if (args.size() == 3)
		{
			LVariable handle = args[1];
			if (handle.type() == LVariable::POINTER)
			{
				set<LVariable>* pSet = (set<LVariable>*)handle.pointerValue();
				pair<set<LVariable>::iterator, bool> result = pSet->insert(args[2]);
				if (result.second)
					ret = LVariable(1);
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_erase")
	{
		if (args.size() == 3)
		{
			LVariable handle = args[1];
			if (handle.type() == LVariable::POINTER)
			{
				set<LVariable>* pSet = (set<LVariable>*)handle.pointerValue();
				int nArgType = 0;
				LVariable item = args[2];
				if (item.type() == LVariable::POINTER)
				{
					if (item.info().find("iterator_set") != item.info().end())
						nArgType = 1;
				}
				if (nArgType > 0)
				{
					set<LVariable>::iterator iter = *(set<LVariable>::iterator*)item.pointerValue();
					if (iter != pSet->end())
					{
						pSet->erase(iter++);
						set<LVariable>::iterator* pIter = (set<LVariable>::iterator*)item.pointerValue();
						*pIter = iter;
						ret = item;
					}
				}
				else
				{
					set<LVariable>::iterator iter = pSet->find(item);
					if (iter != pSet->end())
					{
						pSet->erase(iter);
						ret = LVariable(1);
					}
				}

			}
			return true;
		}
	}
	if (args[0].strValue() == "set_begin")
	{
		if (args.size() == 2)
		{
			LVariable handle = args[1];
			if (handle.type() == LVariable::POINTER)
			{
				set<LVariable>* pSet = (set<LVariable>*)handle.pointerValue();
				set<LVariable>::iterator iter = pSet->begin();
				if (iter != pSet->end())
				{
					LVariable vartemp;
					vartemp.setType(LVariable::POINTER);
					vartemp.info()["iterator_set"] = "1";
					set<LVariable>::iterator* pIter = new set<LVariable>::iterator;
					*pIter = iter;
					vartemp.setPointer(pIter);
					vartemp.initPointerRef(pIter);
					ret = vartemp;
				}
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_next")
	{
		if (args.size() == 3)
		{
			LVariable handle = args[1];
			if (handle.type() == LVariable::POINTER)
			{
				set<LVariable>* pSet = (set<LVariable>*)handle.pointerValue();

				if (args[2].type() == LVariable::POINTER && args[2].pointerValue())
				{
					set<LVariable>::iterator iter = *(set<LVariable>::iterator*)args[2].pointerValue();
					if (iter != pSet->end())
						iter++;
					if (iter != pSet->end())
					{
						set<LVariable>::iterator* pIter = (set<LVariable>::iterator*)args[2].pointerValue();
						*pIter = iter;
						ret = args[2];
					}
				}
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_get")
	{
		if (args.size() == 3)
		{
			LVariable handle = args[1];
			LVariable item = args[2];
			if (handle.type() == LVariable::POINTER && item.type() == LVariable::POINTER)
			{
				set<LVariable>* pSet = (set<LVariable>*)handle.pointerValue();
				set<LVariable>::iterator iter = *(set<LVariable>::iterator*)item.pointerValue();
				if (iter != pSet->end())
					ret = *iter;
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_find")
	{
		if (args.size() == 3)
		{
			LVariable handle = args[1];
			LVariable item = args[2];
			if (handle.type() == LVariable::POINTER)
			{
				set<LVariable>* pSet = (set<LVariable>*)handle.pointerValue();
				set<LVariable>::iterator iter = pSet->find(item);
				if (iter != pSet->end())
					ret = *iter;
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_union")
	{
		if (args.size() == 3)
		{
			LVariable set1 = args[1];
			LVariable set2 = args[2];
			if (set1.type() == LVariable::POINTER && set2.type() == LVariable::POINTER)
			{
				set<LVariable>* pSet1 = (set<LVariable>*)set1.pointerValue();
				set<LVariable>* pSet2 = (set<LVariable>*)set2.pointerValue();
				//ret.setArray(0);
				//set_union(pSet1->begin(), pSet1->end(), pSet2->begin(), pSet2->end(), back_inserter(*ret.arrValue()));

				set<LVariable>* pSet = new set<LVariable>();
				ret.setType(LVariable::POINTER);
				ret.setPointer(pSet);
				set_union(pSet1->begin(), pSet1->end(), pSet2->begin(), pSet2->end(), inserter(*pSet, pSet->begin()));
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_intersection")
	{
		if (args.size() == 3)
		{
			LVariable set1 = args[1];
			LVariable set2 = args[2];
			if (set1.type() == LVariable::POINTER && set2.type() == LVariable::POINTER)
			{
				set<LVariable>* pSet1 = (set<LVariable>*)set1.pointerValue();
				set<LVariable>* pSet2 = (set<LVariable>*)set2.pointerValue();
				//ret.setArray(0);
				//set_intersection(pSet1->begin(), pSet1->end(), pSet2->begin(), pSet2->end(), back_inserter(*ret.arrValue()));

				set<LVariable>* pSet = new set<LVariable>();
				ret.setType(LVariable::POINTER);
				ret.setPointer(pSet);
				set_intersection(pSet1->begin(), pSet1->end(), pSet2->begin(), pSet2->end(), inserter(*pSet, pSet->begin()));
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_difference")
	{
		if (args.size() == 3)
		{
			LVariable set1 = args[1];
			LVariable set2 = args[2];
			if (set1.type() == LVariable::POINTER && set2.type() == LVariable::POINTER)
			{
				set<LVariable>* pSet1 = (set<LVariable>*)set1.pointerValue();
				set<LVariable>* pSet2 = (set<LVariable>*)set2.pointerValue();
				//ret.setArray(0);
				//set_difference(pSet1->begin(), pSet1->end(), pSet2->begin(), pSet2->end(), back_inserter(*ret.arrValue()));

				set<LVariable>* pSet = new set<LVariable>();
				ret.setType(LVariable::POINTER);
				ret.setPointer(pSet);
				set_difference(pSet1->begin(), pSet1->end(), pSet2->begin(), pSet2->end(), inserter(*pSet, pSet->begin()));
			}
			return true;
		}
	}

	return false;
}