#include "Variable.h"
#include "AutoLock.h"

static LLock _lock;

map<vector<LVariable>*, int> LVariable::_mapArraysRefCount = map<vector<LVariable>*, int>();

map<map<LVariable, LVariable>*, int> LVariable::_mapDictsRefCount = map<map<LVariable, LVariable>*, int>();

map<void*, int> LVariable::_mapPointersRefCount = map<void*, int>();

LVariable::LVariable()
{
	m_tag = NORMAL;
	m_type = EMPTY;
	m_pVecValue = NULL;
	m_pMapValue = NULL;
	m_pPointer = NULL;
}

LVariable::~LVariable()
{
	DecreaseRefCount();
}

LVariable::LVariable(const LVariable& cv)
{
	this->m_tag = cv.m_tag;
	this->m_type = cv.m_type;
	this->m_mapInfo = cv.m_mapInfo;
	this->m_vecIndex = cv.m_vecIndex;
	this->m_strValue = cv.m_strValue;
	this->m_nValue = cv.m_nValue;
	this->m_fValue = cv.m_fValue;
	this->m_pVecValue = cv.m_pVecValue;
	this->m_pMapValue = cv.m_pMapValue;
	this->m_pPointer = cv.m_pPointer;

	IncreaseRefCount();
}

LVariable& LVariable::operator =(const LVariable& cv)
{
	if (this == &cv)
		return *this;

	DecreaseRefCount();
	
	this->m_tag = cv.m_tag;
	this->m_type = cv.m_type;
	this->m_mapInfo = cv.m_mapInfo;
	this->m_vecIndex = cv.m_vecIndex;
	this->m_strValue = cv.m_strValue;
	this->m_nValue = cv.m_nValue;
	this->m_fValue = cv.m_fValue;
	this->m_pVecValue = cv.m_pVecValue;
	this->m_pMapValue = cv.m_pMapValue;
	this->m_pPointer = cv.m_pPointer;

	IncreaseRefCount();

	return *this;
}

LVariable::LVariable(const string& value)
{ 
	m_tag = NORMAL;
	m_type = STRING;
	m_strValue = value; 
	m_pVecValue = NULL;
	m_pMapValue = NULL;
	m_pPointer = NULL;
}

LVariable::LVariable(_INT value)
{ 
	m_tag = NORMAL;
	m_type = INT;
	m_nValue = value; 
	m_pVecValue = NULL;
	m_pMapValue = NULL;
	m_pPointer = NULL;
}

LVariable& LVariable::setDouble(double value)
{
	m_type = FLOAT;
	m_fValue = value;

	return *this;
}

LVariable& LVariable::setError()
{
	m_tag = ERR;
	m_type = EMPTY;

	return *this;
}

void LVariable::reset()
{
	DecreaseRefCount();

	m_tag = NORMAL;
	m_type = EMPTY;
	m_pVecValue = NULL;
	m_pMapValue = NULL;
	m_pPointer = NULL;
	if (m_vecIndex.size() > 0)
		m_vecIndex.clear();
	if (m_mapInfo.size() > 0)
		m_mapInfo.clear();
}

void LVariable::setArray(_INT arrSize, vector<LVariable>* pArray)
{
	m_type = ARRAY;
	if (pArray)
	{
		if (_mapArraysRefCount.find(pArray) != _mapArraysRefCount.end())
		{
			if (_mapArraysRefCount[pArray] == 0)
				return;
		}
		else
			return;
		m_pVecValue = pArray;
		IncreaseRefCount();
		return;
	}

	if (arrSize >= 0)
	{
		m_pVecValue = new vector<LVariable>();
		m_pVecValue->resize(arrSize);
		AutoLock lock(_lock);
		_mapArraysRefCount[m_pVecValue] = 1;
	}
}
void LVariable::setDict(map<LVariable, LVariable>* pDict)
{ 
	m_type = DICT; 
	if (pDict)
	{
		if (_mapDictsRefCount.find(pDict) != _mapDictsRefCount.end())
		{
			if (_mapDictsRefCount[pDict] == 0)
				return;
		}
		else
			return;
		m_pMapValue = pDict;
		IncreaseRefCount();
		return;
	}

	m_pMapValue = new map<LVariable, LVariable>();
	AutoLock lock(_lock);
	_mapDictsRefCount[m_pMapValue] = 1;
}
void LVariable::initPointerRef(void* pPointer)
{
	AutoLock lock(_lock);
	_mapPointersRefCount[m_pPointer] = 1;
}
void LVariable::setInfo(LVecMap& info)
{ 
	if (m_mapInfo.size()==0)
		m_mapInfo = info; 
	else
	{ 
		for (size_t i = 0; i < info.map().size(); i++)
			m_mapInfo[info.map()[i].first] = info.map()[i].second;
	}

}

void LVariable::IncreaseRefCount()
{
	if (m_pVecValue || m_pMapValue || m_pPointer)
	{
		AutoLock lock(_lock);

		if (m_pVecValue)
		{
			if (_mapArraysRefCount.find(m_pVecValue) != _mapArraysRefCount.end())
				_mapArraysRefCount[m_pVecValue]++;
		}
		if (m_pMapValue)
		{
			if (_mapDictsRefCount.find(m_pMapValue) != _mapDictsRefCount.end())
				_mapDictsRefCount[m_pMapValue]++;
		}
		if (m_pPointer)
		{
			if (_mapPointersRefCount.find(m_pPointer) != _mapPointersRefCount.end())
				_mapPointersRefCount[m_pPointer]++;
		}
	}
}

void LVariable::DecreaseRefCount()
{
	if (m_pVecValue || m_pMapValue || m_pPointer)
	{
		AutoLock lock(_lock);

		if (m_pVecValue)
		{
			map<vector<LVariable>*, int>::iterator iter = _mapArraysRefCount.find(m_pVecValue);
			if (iter != _mapArraysRefCount.end())
			{
				if (_mapArraysRefCount[m_pVecValue] > 0)
				{
					int refcount = _mapArraysRefCount[m_pVecValue] - 1;
					_mapArraysRefCount[m_pVecValue] = refcount;
					if (refcount == 0)
					{
						_mapArraysRefCount.erase(iter);
						delete m_pVecValue;
					}
				}
			}
		}
		if (m_pMapValue)
		{
			map<map<LVariable, LVariable>*, int>::iterator iter = _mapDictsRefCount.find(m_pMapValue);
			if (iter != _mapDictsRefCount.end())
			{
				if (_mapDictsRefCount[m_pMapValue] > 0)
				{
					int refcount = _mapDictsRefCount[m_pMapValue] - 1;
					_mapDictsRefCount[m_pMapValue] = refcount;
					if (refcount == 0)
					{
						_mapDictsRefCount.erase(iter);
						delete m_pMapValue;
					}
				}
			}
		}
		if (m_pPointer)
		{
			map<void*, int>::iterator iter = _mapPointersRefCount.find(m_pPointer);
			if (iter != _mapPointersRefCount.end())
			{
				if (_mapPointersRefCount[m_pPointer] > 0)
				{
					int refcount = _mapPointersRefCount[m_pPointer] - 1;
					_mapPointersRefCount[m_pPointer] = refcount;
					if (refcount == 0)
					{
						_mapPointersRefCount.erase(iter);
						if (this->info().find("reverse_iterator") != this->info().end())
						{
							map<LVariable, LVariable>::reverse_iterator* pIter = (map<LVariable, LVariable>::reverse_iterator*)m_pPointer;
							delete pIter;
						}
						else if (this->info().find("iterator") != this->info().end())
						{
							map<LVariable, LVariable>::iterator* pIter = (map<LVariable, LVariable>::iterator*)m_pPointer;
							delete pIter;
						}
						else if (this->info().find("iterator_set") != this->info().end())
						{
							set<LVariable>::iterator* pIter = (set<LVariable>::iterator*)m_pPointer;
							delete pIter;
						}
					}
				}
			}
		}
	}
}

LVariable LVariable::operator +(const LVariable & right)
{
	if (this->type() == INT && right.type() == INT)
	{
		_INT value = this->m_nValue + right.m_nValue;
		return LVariable(value);
	}
	if (this->type() == FLOAT && right.type() == FLOAT)
	{
		double value = this->m_fValue + right.m_fValue;
		return LVariable().setDouble(value);
	}
	if (this->type() == STRING &&  right.type() == STRING)
	{
		string value = this->m_strValue + right.m_strValue;
		return LVariable(value);
	}

	if (this->type() == INT && right.type() == FLOAT)
	{
		double value = this->m_nValue + right.m_fValue;
		return LVariable().setDouble(value);
	}
	if (this->type() == FLOAT && right.type() == INT)
	{
		double value = this->m_fValue + right.m_nValue;
		return LVariable().setDouble(value);
	}
	if (this->type() == INT &&  right.type() == STRING)
	{
		ostringstream   os;
		os << this->m_nValue;
		string value = os.str() + right.m_strValue;
		return LVariable(value);
	}
	if (this->type() == STRING &&  right.type() == INT)
	{
		ostringstream   os;
		os << right.m_nValue;
		string value = this->m_strValue + os.str();
		return LVariable(value);
	}
	if (this->type() == FLOAT &&  right.type() == STRING)
	{
		ostringstream   os;
		os << this->m_fValue;
		string value = os.str() + right.m_strValue;
		return LVariable(value);
	}
	if (this->type() == STRING &&  right.type() == FLOAT)
	{
		ostringstream   os;
		os << right.m_fValue;
		string value = this->m_strValue + os.str();
		return LVariable(value);
	}

	return LVariable().setError();
}

LVariable LVariable::operator -(const LVariable & right)
{
	if (this->type() == INT && right.type() == INT)
	{
		_INT value = this->m_nValue - right.m_nValue;
		return LVariable(value);
	}
	if (this->type() == FLOAT && right.type() == FLOAT)
	{
		double value = this->m_fValue - right.m_fValue;
		return LVariable().setDouble(value);
	}

	if (this->type() == INT && right.type() == FLOAT)
	{
		double value = this->m_nValue - right.m_fValue;
		return LVariable().setDouble(value);
	}
	if (this->type() == FLOAT && right.type() == INT)
	{
		double value = this->m_fValue - right.m_nValue;
		return LVariable().setDouble(value);
	}

	return LVariable().setError();

}

LVariable LVariable::operator *(const LVariable & right)
{
	if (this->type() == INT && right.type() == INT)
	{
		_INT value = this->m_nValue * right.m_nValue;
		return LVariable(value);
	}
	if (this->type() == FLOAT && right.type() == FLOAT)
	{
		double value = this->m_fValue * right.m_fValue;
		return LVariable().setDouble(value);
	}

	if (this->type() == INT && right.type() == FLOAT)
	{
		double value = this->m_nValue * right.m_fValue;
		return LVariable().setDouble(value);
	}
	if (this->type() == FLOAT && right.type() == INT)
	{
		double value = this->m_fValue * right.m_nValue;
		return LVariable().setDouble(value);
	}

	return LVariable().setError();
}

LVariable LVariable::operator /(const LVariable & right)
{
	if (this->type() == INT && right.type() == INT)
	{
		if(right.m_nValue == 0)
		{
			this->m_tag = ERR_DIVZERO;
			this->m_type = NONE;
			return *this;
		}
			
		_INT value = this->m_nValue / right.m_nValue;
		return LVariable(value);
	}

	if (this->type() == FLOAT && right.type() == FLOAT)
	{
		if(right.m_fValue == 0)
		{
			this->m_tag = ERR_DIVZERO;
			this->m_type = NONE;
			return *this;
		}
		double value = this->m_fValue / right.m_fValue;
		return LVariable().setDouble(value);
	}

	if (this->type() == INT && right.type() == FLOAT)
	{
		if(right.m_fValue == 0)
		{
			this->m_tag = ERR_DIVZERO;
			this->m_type = NONE;
			return *this;
		}
		double value = this->m_nValue / right.m_fValue;
		return LVariable().setDouble(value);
	}

	if (this->type() == FLOAT && right.type() == INT)
	{
		if(right.m_nValue == 0)
		{
			this->m_tag = ERR_DIVZERO;
			this->m_type = NONE;
			return *this;
		}
		double value = this->m_fValue / right.m_nValue;
		return LVariable().setDouble(value);
	}

	return LVariable().setError();
}

LVariable LVariable::operator %(const LVariable & right)
{
	if (this->type() == INT && right.type() == INT)
	{
		_INT value = this->m_nValue % right.m_nValue;
		return LVariable(value);
	}
	return LVariable().setError();
}

LVariable LVariable::operator ==(const LVariable & right)
{
	if (this->type() != right.type())
	{
		if (this->type() == INT && right.type() == FLOAT)
		{
			int value = 0;
			if (this->m_nValue == right.m_fValue)
				value = 1;
			return LVariable(value);
		}
		if (this->type() == FLOAT && right.type() == INT)
		{
			int value = 0;
			if (this->m_fValue == right.m_nValue)
				value = 1;
			return LVariable(value);
		}

		return LVariable(0);//false
	}

	if (this->type() == INT)
	{
		int value = 0;
		if (this->m_nValue == right.m_nValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == FLOAT)
	{
		int value = 0;
		if (this->m_fValue == right.m_fValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == STRING)
	{
		int value = 0;
		if (this->m_strValue == right.m_strValue)
			value = 1;
		return LVariable(value);
	}
	if (this->type() == POINTER)
	{
		int value = 0;
		if (this->m_pPointer == right.m_pPointer)
			value = 1;
		return LVariable(value);
	}
	if (this->type() == ARRAY)
	{
		int value = 0;
		if (this->m_pVecValue == right.m_pVecValue)
			value = 1;
		return LVariable(value);
	}
	if (this->type() == DICT || this->type() == CLASS)
	{
		int value = 0;
		if (this->m_pMapValue == right.m_pMapValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == NONE)
		return LVariable(1);
	if (this->type() == EMPTY)
		return LVariable(1);

	return LVariable(0);//false
}

LVariable LVariable::operator !=(const LVariable & right)
{
	if (this->type() != right.type())
	{
		if (this->type() == INT && right.type() == FLOAT)
		{
			int value = 0;
			if (this->m_nValue != right.m_fValue)
				value = 1;
			return LVariable(value);
		}
		if (this->type() == FLOAT && right.type() == INT)
		{
			int value = 0;
			if (this->m_fValue != right.m_nValue)
				value = 1;
			return LVariable(value);
		}

		return LVariable(1);//true
	}

	if (this->type() == INT)
	{
		int value = 1;
		if (this->m_nValue == right.m_nValue)
			value = 0;
		return LVariable(value);
	}

	if (this->type() == FLOAT)
	{
		int value = 1;
		if (this->m_fValue == right.m_fValue)
			value = 0;
		return LVariable(value);
	}

	if (this->type() == STRING)
	{
		int value = 1;
		if (this->m_strValue == right.m_strValue)
			value = 0;
		return LVariable(value);
	}
	if (this->type() == POINTER)
	{
		int value = 1;
		if (this->m_pPointer == right.m_pPointer)
			value = 0;
		return LVariable(value);
	}
	if (this->type() == ARRAY)
	{
		int value = 1;
		if (this->m_pVecValue == right.m_pVecValue)
			value = 0;
		return LVariable(value);
	}
	if (this->type() == DICT || this->type() == CLASS)
	{
		int value = 1;
		if (this->m_pMapValue == right.m_pMapValue)
			value = 0;
		return LVariable(value);
	}


	if (this->type() == NONE)
		return LVariable(0);
	if (this->type() == EMPTY)
		return LVariable(0);

	return LVariable(1);//true
}

LVariable LVariable::operator >=(const LVariable & right)
{
	if (this->type() != right.type())
	{
		if (this->type() == INT && right.type() == FLOAT)
		{
			int value = 0;
			if (this->m_nValue >= right.m_fValue)
				value = 1;
			return LVariable(value);
		}
		if (this->type() == FLOAT && right.type() == INT)
		{
			int value = 0;
			if (this->m_fValue >= right.m_nValue)
				value = 1;
			return LVariable(value);
		}

		return LVariable().setError();//false
	}

	if (this->type() == INT)
	{
		int value = 0;
		if (this->m_nValue >= right.m_nValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == FLOAT)
	{
		int value = 0;
		if (this->m_fValue >= right.m_fValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == STRING)
	{
		int value = 0;
		if (this->m_strValue >= right.m_strValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == POINTER)
	{
		int value = 0;
		if (this->m_pPointer >= right.m_pPointer)
			value = 1;
		return LVariable(value);
	}
	if (this->type() == ARRAY)
	{
		int value = 0;
		if (this->m_pVecValue >= right.m_pVecValue)
			value = 1;
		return LVariable(value);
	}
	if (this->type() == DICT || this->type() == CLASS)
	{
		int value = 0;
		if (this->m_pMapValue >= right.m_pMapValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == NONE)
		return LVariable(1);
	if (this->type() == EMPTY)
		return LVariable(1);


	return LVariable().setError();//false
}

LVariable LVariable::operator <=(const LVariable & right)
{
	if (this->type() != right.type())
	{
		if (this->type() == INT && right.type() == FLOAT)
		{
			int value = 0;
			if (this->m_nValue <= right.m_fValue)
				value = 1;
			return LVariable(value);
		}
		if (this->type() == FLOAT && right.type() == INT)
		{
			int value = 0;
			if (this->m_fValue <= right.m_nValue)
				value = 1;
			return LVariable(value);
		}

		return LVariable().setError();//false
	}

	if (this->type() == INT)
	{
		int value = 0;
		if (this->m_nValue <= right.m_nValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == FLOAT)
	{
		int value = 0;
		if (this->m_fValue <= right.m_fValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == STRING)
	{
		int value = 0;
		if (this->m_strValue <= right.m_strValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == ARRAY)
	{
		int value = 0;
		if (this->m_pVecValue <= right.m_pVecValue)
			value = 1;
		return LVariable(value);
	}
	if (this->type() == DICT || this->type() == CLASS)
	{
		int value = 0;
		if (this->m_pMapValue <= right.m_pMapValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == NONE)
		return LVariable(1);
	if (this->type() == EMPTY)
		return LVariable(1);

	return LVariable().setError();//false
}

LVariable LVariable::operator >(const LVariable & right)
{
	if (this->type() != right.type())
	{
		if (this->type() == INT && right.type() == FLOAT)
		{
			int value = 0;
			if (this->m_nValue > right.m_fValue)
				value = 1;
			return LVariable(value);
		}
		if (this->type() == FLOAT && right.type() == INT)
		{
			int value = 0;
			if (this->m_fValue > right.m_nValue)
				value = 1;
			return LVariable(value);
		}

		return LVariable().setError();//false
	}

	if (this->type() == INT)
	{
		int value = 0;
		if (this->m_nValue > right.m_nValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == FLOAT)
	{
		int value = 0;
		if (this->m_fValue > right.m_fValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == STRING)
	{
		int value = 0;
		if (this->m_strValue > right.m_strValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == ARRAY)
	{
		int value = 0;
		if (this->m_pVecValue > right.m_pVecValue)
			value = 1;
		return LVariable(value);
	}
	if (this->type() == DICT || this->type() == CLASS)
	{
		int value = 0;
		if (this->m_pMapValue > right.m_pMapValue)
			value = 1;
		return LVariable(value);
	}

	return LVariable().setError();//false
}

LVariable LVariable::operator <(const LVariable & right)
{
	if (this->type() != right.type())
	{
		if (this->type() == INT && right.type() == FLOAT)
		{
			int value = 0;
			if (this->m_nValue < right.m_fValue)
				value = 1;
			return LVariable(value);
		}
		if (this->type() == FLOAT && right.type() == INT)
		{
			int value = 0;
			if (this->m_fValue < right.m_nValue)
				value = 1;
			return LVariable(value);
		}

		return LVariable().setError();//false
	}

	if (this->type() == INT)
	{
		int value = 0;
		if (this->m_nValue < right.m_nValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == FLOAT)
	{
		int value = 0;
		if (this->m_fValue < right.m_fValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == STRING)
	{
		int value = 0;
		if (this->m_strValue < right.m_strValue)
			value = 1;
		return LVariable(value);
	}

	if (this->type() == ARRAY)
	{
		int value = 0;
		if (this->m_pVecValue < right.m_pVecValue)
			value = 1;
		return LVariable(value);
	}
	if (this->type() == DICT || this->type() == CLASS)
	{
		int value = 0;
		if (this->m_pMapValue < right.m_pMapValue)
			value = 1;
		return LVariable(value);
	}

	return LVariable().setError();//false
}

LVariable LVariable::operator &&(const LVariable & right)
{
	_INT value1 = 0;
	_INT value2 = 0;
	if (this->type() == INT)
		value1 = this->m_nValue;
	else if (this->type() == FLOAT && this->m_fValue!=0.0)
		value1 = 1;
	else if (this->type() == STRING)
		value1 = this->m_strValue.size();
	else if (this->type() == DICT)
		value1 = this->m_pMapValue->size();
	else if (this->type() == ARRAY)
		value1 = this->m_pVecValue->size();
	else if (this->type() == POINTER && this->m_pPointer)
		value1 = 1;
	else if (this->type() == CLASS)
		value1 = 1;

	if (right.type() == INT)
		value2 = right.m_nValue;
	else if (right.type() == FLOAT && right.m_fValue != 0.0)
		value2 = 1;
	else if (right.type() == STRING)
		value2 = right.m_strValue.size();
	else if (right.type() == DICT)
		value2 = right.m_pMapValue->size();
	else if (right.type() == ARRAY)
		value2 = right.m_pVecValue->size();
	else if (right.type() == POINTER && right.m_pPointer)
		value2 = 1;
	else if (right.type() == CLASS)
		value2 = 1;

	int value = value1 && value2;
	return LVariable(value);
}

LVariable LVariable::operator ||(const LVariable & right)
{
	_INT value1 = 0;
	_INT value2 = 0;
	if (this->type() == INT)
		value1 = this->m_nValue;
	else if (this->type() == FLOAT && this->m_fValue != 0.0)
		value1 = 1;
	else if (this->type() == STRING)
		value1 = this->m_strValue.size();
	else if (this->type() == DICT)
		value1 = this->m_pMapValue->size();
	else if (this->type() == ARRAY)
		value1 = this->m_pVecValue->size();
	else if (this->type() == POINTER && this->m_pPointer)
		value1 = 1;
	else if (this->type() == CLASS)
		value1 = 1;

	if (right.type() == INT)
		value2 = right.m_nValue;
	else if (right.type() == FLOAT && right.m_fValue != 0.0)
		value2 = 1;
	else if (right.type() == STRING)
		value2 = right.m_strValue.size();
	else if (right.type() == DICT)
		value2 = right.m_pMapValue->size();
	else if (right.type() == ARRAY)
		value2 = right.m_pVecValue->size();
	else if (right.type() == POINTER && right.m_pPointer)
		value2 = 1;
	else if (right.type() == CLASS)
		value2 = 1;

	int value = value1 || value2;
	return LVariable(value);
}

LVariable LVariable::operator &(const LVariable & right)
{
    if (this->type() == INT && right.type() == INT)
    {
        _INT value = this->m_nValue & right.m_nValue;
        return LVariable(value);
    }
    return LVariable().setError();
}

LVariable LVariable::operator |(const LVariable & right)
{
    if (this->type() == INT && right.type() == INT)
    {
        _INT value = this->m_nValue | right.m_nValue;
        return LVariable(value);
    }
    return LVariable().setError();
}

LVariable LVariable::operator ^(const LVariable & right)
{
    if (this->type() == INT && right.type() == INT)
    {
        _INT value = this->m_nValue ^ right.m_nValue;
        return LVariable(value);
    }
    return LVariable().setError();
}

LVariable LVariable::operator ~()
{
    if (this->type() == INT)
    {
        _INT value = ~this->m_nValue;
        return LVariable(value);
    }
    return LVariable().setError();
}

LVariable LVariable::operator <<(const LVariable & right)
{
    if (this->type() == INT && right.type() == INT)
    {
        _INT value = this->m_nValue << right.m_nValue;
        return LVariable(value);
    }
    return LVariable().setError();
}

LVariable LVariable::operator >>(const LVariable & right)
{
    if (this->type() == INT && right.type() == INT)
    {
        _INT value = this->m_nValue >> right.m_nValue;
        return LVariable(value);
    }
    return LVariable().setError();
}

bool LVariable::operator<(const LVariable& right) const
{
	if (this->type() != right.type())
	{
		if (this->type() < right.type())
			return true;
	}
	else
	{
		if (this->type() == INT)
		{
			if (this->intValue() < right.intValue())
				return true;
		}
		if (this->type() == FLOAT)
		{
			if (this->m_fValue < right.m_fValue)
				return true;
		}
		if (this->type() == STRING)
		{
			if (this->m_strValue < right.m_strValue)
				return true;
		}
		if (this->type() == ARRAY)
		{
			if (this->m_pVecValue < right.m_pVecValue)
				return true;
		}
		if (this->type() == DICT || this->type() == CLASS)
		{
			if (this->m_pMapValue < right.m_pMapValue)
				return true;
		}
		if (this->type() == POINTER)
		{
			if (this->m_pPointer < right.m_pPointer)
				return true;
		}
	}

	return false;
}
