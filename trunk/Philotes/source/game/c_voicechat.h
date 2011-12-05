//----------------------------------------------------------------------------
// c_voicechat.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_VOICECHAT_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _VOICECHAT_H_

// init/cleanup functions
void c_VoiceChatInit();
void c_VoiceChatClose();

// called every so often to process events
void c_VoiceChatUpdate();

// login functions
void c_VoiceChatLogin(const WCHAR * uszName, const WCHAR * uszPassword);
void c_VoiceChatLogout();
BOOL c_VoiceChatIsLoggedIn();

// room creation
void c_VoiceChatCreateRoom();
void c_VoiceChatJoinRoom(const WCHAR *uszRoomID);
void c_VoiceChatJoinRoom(BYTE pRoomID[21]);
void c_VoiceChatLeaveRoom();
void c_VoiceChatIgnoreUser(DWORD dwXfireUserId, bool bIgnoreUser);

// start/stop voice chat
/* console only */ void c_VoiceChatStartHost();
/* console only */ void c_VoiceChatJoinHost(const WCHAR *uszRoomId);
void c_VoiceChatStart();
void c_VoiceChatStop();

// utility functions
BOOL c_VoiceChatIsChatting();
BOOL c_VoiceChatIsEnabled();
BOOL c_VoiceChatIsInRoom();
int c_VoiceChatGetLoggedInUserId();
const WCHAR *c_VoiceChatGetLoggedUsername();


#endif
