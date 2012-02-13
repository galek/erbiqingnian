
#ifndef Exception_h__
#define Exception_h__

#include <exception>

namespace Philo
{
	enum ErrorType 
	{
		ERR_MEMORY = 1,
		ERR_DATASTRUCT,
		ERR_RENDER,
		ERR_LOGIC,
		ERR_NETWORK,
		ERR_INPUT,
		ERR_UNKNOW
	};

	class Exception  : public std::exception
	{
	public:

		Exception( ErrorType type,const String &error_text );

		Exception( ErrorType type,const String &error_text ,int line, const char* file);

		String			getDesc();

		void			errorMessage();

		static	char*	getTypeDesc(ErrorType type);

	protected:

		String			m_Desc;

		ErrorType		m_ErrType;

		int				m_Line;

		String			m_File;
	};

	#define PH_EXCEPT(type,desc) throw Philo::Exception(type,desc,__LINE__,__FILE__);
}

#endif // Exception_h__