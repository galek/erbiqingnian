
#include "exception.h"

namespace Philo
{
	Exception::Exception( ErrorType type,const String &error_text )
	{
		m_Desc = error_text;
		m_ErrType = type;
		m_Line = 0;
	}

	Exception::Exception( ErrorType type,const String &error_text ,int line, const char* file )
	{
		m_Desc = error_text;
		m_ErrType = type;
		m_Line = line;
		m_File = file;
	}

	String Exception::getDesc()
	{
		String desc;
		desc.Format("%s : %s",getTypeDesc(m_ErrType),m_Desc.c_str());
		return desc;
	}

	void Exception::errorMessage()
	{
		char text[1024];
		if (m_Line != 0)
		{
			sprintf(text, "Type : %s\n\nDescribe : %s\n\nLine : %d\n\nFile : %s",
				getTypeDesc(m_ErrType),m_Desc.c_str(),m_Line,m_File.c_str());
		}
		else
		{
			sprintf(text, "Type : %s\n\nDescribe : %s\n",
				getTypeDesc(m_ErrType),m_Desc.c_str());
		}
		
		MessageBoxA(NULL, text, "Fable Exception", MB_ICONERROR);

		#ifdef _DEBUG
				__asm int 3
		#endif
	}

	char* Exception::getTypeDesc( ErrorType type )
	{
		switch(type) 
		{
		case ERR_MEMORY:
			return "Memory Error";
		case ERR_DATASTRUCT:
			return "Data Struct Error";
		case ERR_LOGIC:
			return "Logic Error";
		case ERR_NETWORK:
			return "Network Error";
		case ERR_UNKNOW:
			return "Unknow Error";
		case ERR_INPUT:
			return "Input Error";
		default:
			return "Unknow Error";
		}
	}

}