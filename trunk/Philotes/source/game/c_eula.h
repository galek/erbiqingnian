#ifndef _EULA_H_
#define _EULA_H_

#if !ISVERSION(SERVER_VERSION)

enum EULA_STATE
{
	EULA_STATE_NONE,
	EULA_STATE_WAITING_FOR_HASH,
	EULA_STATE_EULA,
	EULA_STATE_TOS,
	EULA_STATE_DONE,
};

void c_EulaStateReset();
BOOL c_EulaStateUpdate();
EULA_STATE c_EulaGetState();

#endif // SERVER_VERSION
#endif //_EULA_H_