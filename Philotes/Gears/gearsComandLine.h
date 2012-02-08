
#pragma once

_NAMESPACE_BEGIN

class GearCommandLine
{
public:
	GearCommandLine(unsigned int argc, const char *const* argv);

	GearCommandLine(const char *args);

	~GearCommandLine(void);

	//! has a given command-line switch?
	//  e.g. s=="foo" checks for -foo
	bool hasSwitch(const char *s, const unsigned int argNum = INVALID_ARG_NUM) const;

	//! gets the value of a switch... 
	//  e.g. s="foo" returns "bar" if '-foo=bar' is in the commandline.
	const char* getValue(const char *s, unsigned int argNum = INVALID_ARG_NUM) const;

	// return how many command line arguments there are
	const unsigned int getNumArgs(void) const;

	// what is the program name
	const char* getProgramName(void) const;

	// get the string that contains the unsued args
	const unsigned int unusedArgsBufSize(void) const;

	// get the string that contains the unsued args
	const char* getUnusedArgs(char *buf, unsigned int bufSize) const;

	//! if the first argument is the given command.
	bool isCommand(const char *s) const;

	//! get the first argument assuming it isn't a switch.
	//  e.g. for the command-line "myapp.exe editor -foo" it will return "editor".
	const char *getCommand(void) const;

	//! get the raw command-line argument list...
	unsigned int      getArgC(void) const { return m_argc; }

	const char *const*getArgV(void) const { return m_argv; }

private:
	GearCommandLine(const GearCommandLine&) 
	{
		ph_assert(0); 
	}
	
	GearCommandLine(void) 
	{
		ph_assert(0); 
	}
	
	GearCommandLine operator = (const GearCommandLine&) 
	{
		ph_assert(0); 
	}
	
	unsigned int		m_argc;
	
	const char *const	*m_argv;
	
	void				*m_freeme;
	
	bool*				m_argUsed;

	static const uint32 INVALID_ARG_NUM;
};

_NAMESPACE_END