
#include "gearsComandLine.h"

_NAMESPACE_BEGIN

static char **CommandLineToArgvA(const char *cmdLine, unsigned int &_argc)
{
	char        **argv     = 0;
	char         *_argv    = 0;
	unsigned int  len      = 0;
	unsigned int  argc     = 0;
	char          a        = 0;
	unsigned int  i        = 0;
	unsigned int  j        = 0;
	bool          in_QM    = false;
	bool          in_TEXT  = false;
	bool          in_SPACE = true;

	len   = (unsigned int)strlen(cmdLine);
	i     = ((len+2)/2)*sizeof(char*) + sizeof(char*);
	argv  = (char **)malloc(i + (len+2)*sizeof(char));
	_argv = (char *)(((char *)argv)+i);
	argv[0] = _argv;

	i = 0;

	a = cmdLine[i];
	while(a)
	{
		if(in_QM)
		{
			if(a == '\"')
			{
				in_QM = false;
			}
			else
			{
				_argv[j] = a;
				j++;
			}
		}
		else
		{
			switch(a)
			{
			case '\"':
				in_QM = true;
				in_TEXT = true;
				if(in_SPACE)
				{
					argv[argc] = _argv+j;
					argc++;
				}
				in_SPACE = false;
				break;
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				if(in_TEXT)
				{
					_argv[j] = '\0';
					j++;
				}
				in_TEXT = false;
				in_SPACE = true;
				break;
			default:
				in_TEXT = true;
				if(in_SPACE)
				{
					argv[argc] = _argv+j;
					argc++;
				}
				_argv[j] = a;
				j++;
				in_SPACE = false;
				break;
			}
		}
		i++;
		a = cmdLine[i];
	}
	_argv[j] = '\0';
	argv[argc] = 0;
	_argc = argc;
	return argv;
}

static bool isSwitchChar(char c)
{
	return (c == '-' || c == '/');
}

GearCommandLine::GearCommandLine(unsigned int argc, const char *const* argv)
{
	m_argc   = argc;
	m_argv   = argv;
	m_freeme = 0;
	m_argUsed = new bool[m_argc];
	for(unsigned int i = 0; i < m_argc; i++)
	{
		m_argUsed[i] = false;
	}
}

GearCommandLine::GearCommandLine(const char *args)
{
	m_argUsed  = 0;
	m_argc   = 0;
	m_argv   = 0;
	m_freeme = 0;
	unsigned int argc = 0;
	char       **argv = CommandLineToArgvA(args, argc);
	ph_assert(argv);
	if(argv)
	{
		m_argc   = argc;
		m_argv   = argv;
		m_freeme = argv;
	}
	m_argUsed = new bool[m_argc];
	for(unsigned int i = 0; i < m_argc; i++)
	{
		m_argUsed[i] = false;
	}
}

GearCommandLine::~GearCommandLine(void)
{
	if(m_freeme)
	{
		free(m_freeme);
		m_freeme = NULL;
	}
	if(m_argUsed)
	{
		delete[] m_argUsed;
		m_argUsed = NULL;
	}
}

const unsigned int GearCommandLine::getNumArgs(void) const
{
	return(m_argc);
}

const char * GearCommandLine::getProgramName(void) const
{
	return(m_argv[0]);
}

const unsigned int GearCommandLine::unusedArgsBufSize(void) const
{
	unsigned int bufLen = 0;
	for(unsigned int i = 1; i < m_argc; i++)
	{
		if((!m_argUsed[i]) && isSwitchChar(m_argv[i][0]))
		{
			bufLen += (unsigned int) strlen(m_argv[i]) + 1;	// + 1 is for the space between unused args
		}
	}
	if(bufLen != 0)
	{
		bufLen++;	// if there are unused args add a space for the '\0' char.
	}
	return(bufLen);
}

const char* GearCommandLine::getUnusedArgs(char *buf, unsigned int bufSize) const
{
	if(bufSize != 0)
	{
		buf[0] = '\0';
		for(unsigned int i = 1; i < m_argc; i++)
		{
			if((!m_argUsed[i]) && isSwitchChar(m_argv[i][0]))
			{
				strcat_s(buf, bufSize, m_argv[i]);
				strcat_s(buf, bufSize, " ");
			}
		}
	}
	return(buf);
}

//! has a given command-line switch?
//  e.g. s=="foo" checks for -foo
bool GearCommandLine::hasSwitch(const char *s, unsigned int argNum) const
{
	bool has = false;
	ph_assert(*s);
	if(*s)
	{
		unsigned int n = (unsigned int) strlen(s);
		unsigned int firstArg;
		unsigned int lastArg;
		if(argNum != INVALID_ARG_NUM)
		{
			firstArg = argNum;
			lastArg  = argNum;
		}
		else
		{
			firstArg = 1;
			lastArg  = m_argc - 1;
		}
		for(unsigned int i=firstArg; i<=lastArg; i++)
		{
			const char *arg = m_argv[i];
			// allow switches of '/', '-', and double character versions of both.
			if( (isSwitchChar(*arg) && !strnicmp(arg+1, s, n) && ((arg[n+1]=='\0')||(arg[n+1]=='='))) ||
				(isSwitchChar(*(arg+1)) && !strnicmp(arg+2, s, n) && ((arg[n+2]=='\0')||(arg[n+2]=='='))) )
			{
				m_argUsed[i] = true;
				has = true;
				break;
			}
		}
	}
	return has;
}

//! gets the value of a switch... 
//  e.g. s="foo" returns "bar" if '-foo=bar' is in the commandline.
const char *GearCommandLine::getValue(const char *s,  unsigned int argNum) const
{
	const char *value = 0;
	ph_assert(*s);
	if(*s)
	{
		unsigned int firstArg;
		unsigned int lastArg;
		if(argNum != INVALID_ARG_NUM)
		{
			firstArg = argNum;
			lastArg  = argNum;
		}
		else
		{
			firstArg = 1;
			lastArg  = m_argc - 1;
		}
		for(unsigned int i=firstArg; i<=lastArg; i++)
		{
			const char *arg = m_argv[i];
			if(isSwitchChar(*arg))
			{
				arg++;
				if(isSwitchChar(*arg))	// is it a double dash arg?  '--'
				{
					arg++;
				}
				const char *st=s;
				for(; *st && *arg && toupper(*st)==toupper(*arg) && *arg!='='; st++,arg++)
				{
				}
				if(!*st && *arg=='=')
				{
					m_argUsed[i] = true;
					value = arg+1;
					break;
				}
				if(!*st && !*arg && ((i+1)<m_argc) && (!isSwitchChar(*m_argv[i+1])))
				{
					m_argUsed[i] = true;
					value = m_argv[i+1];
					break;
				}
			}
		}
	}
	return value;
}

//! if the first argument is the given command.
bool GearCommandLine::isCommand(const char *s) const
{
	bool has = false;
	const char *command = getCommand();
	if(command && !stricmp(command, s))
	{
		has = true;
	}
	return has;
}

//! get the first argument assuming it isn't a switch.
//  e.g. for the command-line "myapp.exe editor -foo" it will return "editor".
const char *GearCommandLine::getCommand(void) const
{
	const char *command = 0;
	if(m_argc > 1 && !isSwitchChar(*m_argv[1]))
	{
		command = m_argv[1];
	}
	return command;
}

const uint32 GearCommandLine::INVALID_ARG_NUM = 0xFFFFFFFFU;

_NAMESPACE_END