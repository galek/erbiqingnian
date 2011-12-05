//----------------------------------------------------------------------------
// Prime v2.0
//
// voicechat.cpp
// 
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#if !ISVERSION(SERVER_VERSION)
#include "c_voicechat.h"
#include "console.h"
#include "svrstd.h"
#include "c_chatNetwork.h"
#include "UserChatCommunication.h"
#include "..\3rd Party\XFire\xfiresdk\xfireapi.h"
#include "uix_components.h"
#include "uix_options.h"
#include "prime.h"

#include <map>

static XFIREAPI * sgpXFireAPI = NULL;
static XFIRE_ROOMID sgtXFireHostedRoomID;
static BOOL sgbHosting = FALSE;
static BOOL sgbVoiceChatting = FALSE;
static BOOL sgbEnabled = FALSE;
static BOOL sbgInUpdateState = FALSE;
static XFIRE_API_USER_INFO sgtXFireUserInfo;
static std::map<DWORD, bool> sgIgnoredUserId;

#define ENABLE_VOICE_CHAT

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sStrToRoomID(const WCHAR* szStr, XFIRE_ROOMID tRoomID) 
{
	if (PStrLen(szStr)!=2*sizeof(XFIRE_ROOMID))
		return;
	for (int i = 0; i< sizeof(XFIRE_ROOMID); i++)
	{
		unsigned short c1 = *(szStr++);
		if (c1>='0' && c1<='9')		 {c1 = c1 - '0'; }
		else if (c1>='a' && c1<='f') {c1 = c1 - 'a' + 10; }
		else if (c1>='A' && c1<='F') {c1 = c1 - 'A' + 10; }

		unsigned short c2 = *(szStr++);
		if (c2>='0' && c2<='9')		 {c2 = c2 - '0'; }
		else if (c2>='a' && c2<='f') {c2 = c2 - 'a' + 10; }
		else if (c2>='A' && c2<='F') {c2 = c2 - 'A' + 10; }

		tRoomID[i] = (unsigned char)((c1<<4) | c2);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sBroadcastUserid() 
{
	if (AppGetChatChannel())
	{
		CHAT_REQUEST_MSG_SET_CLIENT_SET_DATA msgUserInfoMsg;
		msgUserInfoMsg.ClientSpecefiedData.XfireId = c_VoiceChatGetLoggedInUserId();
		c_ChatNetSendMessage(&msgUserInfoMsg, USER_REQUEST_SET_CLIENT_SET_DATA);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sBroadcastRoomid(XFIRE_ROOMID roomid) 
{
	if (AppGetChatChannel())
	{
		CHAT_REQUEST_MSG_UPDATE_PARTY_CLIENT_DATA msgRoomMsg;
		MemoryCopy(msgRoomMsg.PartyClientData.XFireRoomId, sizeof(XFIRE_ROOMID), roomid, sizeof(XFIRE_ROOMID));
		c_ChatNetSendMessage(&msgRoomMsg, USER_REQUEST_UPDATE_PARTY_CLIENT_DATA);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define XFIRE_LOGGING_ENABLED 1
static void c_sXFireLogVoiceChatError(
	XFIRE_API_ERROR eError,
	const char * pszMessage)
{
#if XFIRE_LOGGING_ENABLED
	char pszError[256];
	WCHAR uszError[256];
	memclear(uszError, 256 * sizeof(WCHAR));
	memclear(pszError, 256 * sizeof(char));
	XfireGetErrorString(eError, uszError, 256);
	PStrCvt(pszError, uszError, 256);
	LogMessage("%s - %s", pszMessage, pszError);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sXFireCallbackPrintUserInfo(
	XFIRE_API_USER_INFO * pUserInfo)
{
	ASSERT_RETURN(pUserInfo);

	LogMessage("User Info: id[%d] %S-%S", pUserInfo->userid, pUserInfo->username, pUserInfo->nickname);
	switch(pUserInfo->update_type)
	{
	case XFIRE_USER_JOINED_ROOM:
		LogMessage("  User Joined Room");
		break;
	case XFIRE_USER_LEFT_ROOM:
		LogMessage("  User Left Room");
		break;
	case XFIRE_USER_MUTE_ON:
		LogMessage("  User Has Been Muted");
		break;
	case XFIRE_USER_MUTE_OFF:
		LogMessage("  User Has Been DeMuted");
		break;
	case XFIRE_USER_START_VOICE:
		LogMessage("  User Started Speaking");
		break;
	case XFIRE_USER_STOP_VOICE:
		LogMessage("  User Stopped Speaking");
		break;
	default:
		LogMessage("  User has done something unknown! - %d", pUserInfo->update_type);
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void XFIREFUNC c_sXFireCallback(
	void *user_data,
	XFIRE_API_CALLBACK_TYPE message_id,
	void *message_data)
{
	switch(message_id)
	{
	case XFIRE_CBTYPE_UPDATE_RECV:
		{
			XfireDestroy(sgpXFireAPI);
			sgpXFireAPI = NULL;
			sbgInUpdateState = TRUE;
		}
		break;
	case XFIRE_CBTYPE_GET_USER_INFO:
		{
			XFIRE_API_USER_INFO tUserInfo;
			XFIRE_API_ERROR eError = sgpXFireAPI->ParseGetCurrentUserInfo(sgpXFireAPI, message_data, tUserInfo);
			if(eError == XFIRE_ERROR_SUCCESS)
			{
				LogMessage("Logged in user info:");
				c_sXFireCallbackPrintUserInfo(&tUserInfo);
				MemoryCopy(&sgtXFireUserInfo, sizeof(sgtXFireUserInfo), &tUserInfo, sizeof(tUserInfo));

				c_sBroadcastUserid();
			}
			else
			{
				c_sXFireLogVoiceChatError(eError, "Error parsing current user info");
			}
		}
		break;
	case XFIRE_CBTYPE_ROOM_UPDATE:
		{
			XFIRE_API_ROOM_INFO tRoomInfo;
			XFIRE_API_ERROR eError = sgpXFireAPI->ParseRoomUpdate(sgpXFireAPI, message_data, tRoomInfo);
			if(eError == XFIRE_ERROR_SUCCESS)
			{
				switch(tRoomInfo.update_type)
				{
				case XFIRE_ROOM_JOINED:
					LogMessage("Room Joined");
					if (sgbHosting)
					{
						MemoryCopy(sgtXFireHostedRoomID, sizeof(XFIRE_ROOMID), tRoomInfo.roomid, sizeof(XFIRE_ROOMID));
						c_VoiceChatStartHost();

						// we need to update the client data
						c_sBroadcastRoomid(tRoomInfo.roomid);

						c_sBroadcastUserid();

						if (!sgIgnoredUserId.empty())
						{
							std::map<DWORD, bool>::iterator iter;
							for (iter = sgIgnoredUserId.begin(); iter != sgIgnoredUserId.end(); iter++)
							{
								c_VoiceChatIgnoreUser((*iter).first, true);
							}
							sgIgnoredUserId.clear();
						}
					}
					break;
				case XFIRE_ROOM_LEFT:
					LogMessage("Room Left");
					{
						if (memcmp(sgtXFireHostedRoomID, tRoomInfo.roomid, sizeof(XFIRE_ROOMID)) == 0)
						{
							// don't allow us to leave until called by game
							sgpXFireAPI->SendJoinRoom(sgpXFireAPI, sgtXFireHostedRoomID, false, XFIRE_PLAYBACKTYPE_NONE, 0);
							c_sBroadcastUserid();
						}
					}
					break;
				case XFIRE_ROOM_REFLECTOR:
					{
						char tszRoomString[64] = { 0 };
						for (int i = 0; i < sizeof(sgtXFireHostedRoomID); i++)
							PStrPrintf(tszRoomString + 2*i, sizeof(tszRoomString) - 2*i, "%.2x", sgtXFireHostedRoomID[i]);

						LogMessage("Room Chat Host Changed (%s)", tszRoomString);
						if (memcmp(sgtXFireHostedRoomID, tRoomInfo.roomid, sizeof(XFIRE_ROOMID)) == 0)
						{
							if (sgbEnabled)
								c_VoiceChatStart();
						}
						if (tRoomInfo.reflector_uid == 0 && sgbHosting)
							c_VoiceChatStartHost();

						c_sBroadcastUserid();
					}
					break;
				default:
					LogMessage("Unknown Room Update type! - %d", tRoomInfo.update_type);
					break;
				}
				LogMessage("UID: %d - %S:%S", tRoomInfo.reflector_uid, tRoomInfo.roomname, tRoomInfo.motd);
			}
			else
			{
				c_sXFireLogVoiceChatError(eError, "Error parsing room update");
			}
		}
		break;
	case XFIRE_CBTYPE_USER_UPDATE_IN_ROOM:
		{
			XFIRE_ROOMID tRoomId;
			XFIRE_API_USER_INFO * pUserInfoArray = NULL;
			int nUserInfoSize = 0;
			XFIRE_API_ERROR eError = sgpXFireAPI->ParseUserUpdateInRoom(sgpXFireAPI, message_data, tRoomId, &pUserInfoArray, nUserInfoSize);
			if(eError == XFIRE_ERROR_SUCCESS)
			{
				ASSERT_BREAK(pUserInfoArray);
				LogMessage("User Update in Room: User Count: %d", nUserInfoSize);

				for(int i=0; i<nUserInfoSize; i++)
				{
					XFIRE_API_USER_INFO * pUserInfo = &pUserInfoArray[i];
					ASSERT_CONTINUE(pUserInfo);

					c_sXFireCallbackPrintUserInfo(pUserInfo);
				}
			}
			else
			{
				c_sXFireLogVoiceChatError(eError, "Error parsing user update in room");
			}
		}
		break;
	case XFIRE_CBTYPE_CLIENT_STATUS:
		{
			XFIRE_API_CLIENT_STATUS_TYPE eStatus;
			XFIRE_API_ERROR eError = sgpXFireAPI->ParseClientStatus(sgpXFireAPI, message_data, eStatus);
			if(eError == XFIRE_ERROR_SUCCESS)
			{
				sgpXFireAPI->SendGetCurrentUserInfo(sgpXFireAPI);
				switch(eStatus)
				{
				case XFIRE_STATUSTYPE_SUCCESS:
					LogMessage("Client Status Success");
					UIOptionsXfireSetStatus(TRUE, FALSE);
					break;
				case XFIRE_STATUSTYPE_FAILURE:
					LogMessage("Client Status Failure");
					UIOptionsXfireSetStatus(FALSE, TRUE);
					break;
				case XFIRE_STATUSTYPE_LOGOFF:
					LogMessage("Client Status Log Off");
					memclear(sgtXFireHostedRoomID, sizeof(XFIRE_ROOMID));
					sgbVoiceChatting = FALSE;
					UIOptionsXfireSetStatus(FALSE, FALSE);
					memclear(&sgtXFireUserInfo, sizeof(sgtXFireUserInfo));
					break;
				case XFIRE_STATUSTYPE_EXIT:
					LogMessage("Client Status Exit");
					UIOptionsXfireSetStatus(FALSE, FALSE);
					break;
				default:
					LogMessage("Client Status Unknown! - %d", eStatus);
					break;
				}
			}
			else
			{
				c_sXFireLogVoiceChatError(eError, "Error parsing client status");
			}
		}
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static bool g_XFire_Installed = false;
void c_VoiceChatInit()
{
#ifdef ENABLE_VOICE_CHAT
	g_XFire_Installed = XfireIsInstalled();

	if (sgpXFireAPI == NULL)
	{
		memclear(sgtXFireHostedRoomID, sizeof(XFIRE_ROOMID));
		XFIRE_API_ERROR eError = XfireCreate(DEFAULT_XFIRE_GAMEID, DEFAULT_XFIRE_VERSION_API, &sgpXFireAPI);
		if(eError != XFIRE_ERROR_SUCCESS)
		{
			//c_sXFireLogVoiceChatError(eError, "Error creating XFire interface");
			sgpXFireAPI = NULL;
			return;
		}
		sgpXFireAPI->SetCallback(sgpXFireAPI, c_sXFireCallback, NULL);
		sbgInUpdateState = FALSE;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatClose()
{
#ifdef ENABLE_VOICE_CHAT
	if(sgpXFireAPI)
	{
		c_VoiceChatLeaveRoom();
		XfireDestroy(sgpXFireAPI);
		sgpXFireAPI = NULL;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatUpdate()
{
#ifdef ENABLE_VOICE_CHAT
	if(sgpXFireAPI)
	{
		BOOL bConnected;
		sgpXFireAPI->IsConnected(sgpXFireAPI, &bConnected );
		if( bConnected )
		{
			sgpXFireAPI->CheckForEvents(sgpXFireAPI);
		}
	}

	// we only try to recreate if we know xfire is installed
	if (g_XFire_Installed && sgpXFireAPI == NULL)
	{
		// check to see if we can recreate
		c_VoiceChatInit();
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatLogin(const WCHAR * uszName, const WCHAR * uszPassword)
{
#ifdef ENABLE_VOICE_CHAT
	if(sgpXFireAPI)
	{
		BOOL bIsConnected = FALSE;
		sgpXFireAPI->IsConnected(sgpXFireAPI, &bIsConnected);
		if (!bIsConnected)
		{
			StartXfire(uszName, uszPassword);
		}
		else
		{
			sgpXFireAPI->SendLogIn(sgpXFireAPI, uszName, uszPassword);
		}
		PStrCvt(sgtXFireUserInfo.username, uszName, arrsize(sgtXFireUserInfo.username));
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatLogout()
{
#ifdef ENABLE_VOICE_CHAT
	if(sgpXFireAPI)
	{
		sgpXFireAPI->SendLogOut(sgpXFireAPI);
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_VoiceChatIsLoggedIn()
{
#ifdef ENABLE_VOICE_CHAT
	if(sgpXFireAPI)
	{
		BOOL retval;
		sgpXFireAPI->IsLoggedIn(sgpXFireAPI, &retval);
		return retval;
	}
#endif
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatCreateRoom()
{
#ifdef ENABLE_VOICE_CHAT
	c_VoiceChatLeaveRoom();

	if(sgpXFireAPI && sgtXFireHostedRoomID[0] == 0)
	{
#ifdef _DEBUG
		sgpXFireAPI->SendCreateRoom(sgpXFireAPI, 0);
#else 
		sgpXFireAPI->SendCreateRoom(sgpXFireAPI, XFIRE_ROOM_INVISIBLE);
#endif
		sgbHosting = TRUE;

		c_sBroadcastUserid();
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatJoinRoom(const WCHAR *uszRoomID)
{
#ifdef ENABLE_VOICE_CHAT
	c_VoiceChatLeaveRoom();

	if(sgpXFireAPI)
	{
		XFIRE_ROOMID tXfireRoomID;
		c_sStrToRoomID(uszRoomID, tXfireRoomID);
		MemoryCopy(sgtXFireHostedRoomID, sizeof(XFIRE_ROOMID), tXfireRoomID, sizeof(XFIRE_ROOMID));
		sgpXFireAPI->SendJoinRoom(sgpXFireAPI, tXfireRoomID, false, XFIRE_PLAYBACKTYPE_NONE, 0);

		c_sBroadcastUserid();
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatJoinRoom(BYTE pRoomID[21])
{
#ifdef ENABLE_VOICE_CHAT
	if(sgpXFireAPI)
	{
		if (sgtXFireHostedRoomID[0] == 0)
		{
			if (memcmp(sgtXFireHostedRoomID, pRoomID, sizeof(XFIRE_ROOMID)) != 0)
				c_VoiceChatLeaveRoom();
		}

		if (pRoomID[0] != 0 && sgtXFireHostedRoomID[0] == 0)
		{
			MemoryCopy(sgtXFireHostedRoomID, sizeof(XFIRE_ROOMID), pRoomID, sizeof(XFIRE_ROOMID));
			sgpXFireAPI->SendJoinRoom(sgpXFireAPI, pRoomID, false, XFIRE_PLAYBACKTYPE_NONE, 0);

			c_sBroadcastUserid();
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatLeaveRoom()
{
#ifdef ENABLE_VOICE_CHAT
	sgbHosting = FALSE;
	sgbVoiceChatting = FALSE;
	if(sgpXFireAPI)
	{
		if (sgtXFireHostedRoomID[0] != 0)
		{
			sgpXFireAPI->SendLeaveRoom(sgpXFireAPI, sgtXFireHostedRoomID);
			memclear(sgtXFireHostedRoomID, sizeof(sgtXFireHostedRoomID));
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatIgnoreUser(DWORD dwXfireUserId, bool bIgnoreUser)
{
	if(sgpXFireAPI)
	{
		if (sgtXFireHostedRoomID[0] != 0)
		{
			sgpXFireAPI->SendIgnoreUser(sgpXFireAPI, sgtXFireHostedRoomID, dwXfireUserId, bIgnoreUser);
		}
		else
		{
			// keep ignore user list around
			if (bIgnoreUser)
			{
				sgIgnoredUserId[dwXfireUserId] = bIgnoreUser;
			}
			else
			{
				std::map<DWORD,bool>::iterator iter = sgIgnoredUserId.find(dwXfireUserId);
				if (iter != sgIgnoredUserId.end())
					sgIgnoredUserId.erase(iter);
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatStartHost()
{
#ifdef ENABLE_VOICE_CHAT
	if(sgpXFireAPI)
	{
		sgbHosting = TRUE;
		sgpXFireAPI->SendHostGroupVoiceChat(sgpXFireAPI, sgtXFireHostedRoomID, XFIRE_QUALITY_MEDIUM, 100, XFIRE_PLAYBACKTYPE_DIRECTSOUND, GetCurrentProcessId());

		// we need to update the client data
		c_sBroadcastRoomid(sgtXFireHostedRoomID);
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatJoinHost(const WCHAR *uszRoomID)
{
#ifdef ENABLE_VOICE_CHAT
	if(sgpXFireAPI)
	{
		XFIRE_ROOMID tXfireRoomID;
		if (uszRoomID == NULL || uszRoomID[0] == 0)
		{
			MemoryCopy(tXfireRoomID, sizeof(XFIRE_ROOMID), sgtXFireHostedRoomID, sizeof(XFIRE_ROOMID));
		}
		else
		{
			c_sStrToRoomID(uszRoomID, tXfireRoomID);
		}
		sgpXFireAPI->SendJoinGroupVoiceChat(sgpXFireAPI, sgtXFireHostedRoomID, XFIRE_PLAYBACKTYPE_DIRECTSOUND, GetCurrentProcessId());

		sgbVoiceChatting = TRUE;
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatStart()
{
#ifdef ENABLE_VOICE_CHAT
	sgbEnabled = TRUE;
	if (sgpXFireAPI && sgtXFireHostedRoomID[0] != 0)
	{
		sgpXFireAPI->SendJoinGroupVoiceChat(sgpXFireAPI, sgtXFireHostedRoomID, XFIRE_PLAYBACKTYPE_DIRECTSOUND, GetCurrentProcessId());
		sgbVoiceChatting = TRUE;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_VoiceChatStop()
{
#ifdef ENABLE_VOICE_CHAT
	sgbEnabled = FALSE;
	if (sgpXFireAPI)
	{
		sgpXFireAPI->SendLeaveGroupVoiceChat(sgpXFireAPI, sgtXFireHostedRoomID);
		sgbVoiceChatting = FALSE;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_VoiceChatIsChatting()
{
	return sgbVoiceChatting;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_VoiceChatIsEnabled()
{
	return sgbEnabled;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_VoiceChatIsInRoom()
{
	return sgtXFireHostedRoomID[0] != 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_VoiceChatGetLoggedInUserId()
{
	return sgtXFireUserInfo.userid;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *c_VoiceChatGetLoggedUsername()
{
	return sgtXFireUserInfo.username;
}


#endif //!SERVER_VERSION