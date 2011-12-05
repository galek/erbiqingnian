// Implements a one to one mapping of instancing chat channels to
// the particular instanced channel a given client is in.

// For the sake of rapid prototyping, MAP!

#include "stdafx.h"

#if !ISVERSION(SERVER_VERSION)
#include "prime.h"
#include <map>
#include "chatstd.h"

//----------------------------------------------------------------------------
// STATICS
//----------------------------------------------------------------------------

struct INSTANCING_CHANNEL_INFO
{
	WCHAR szName[MAX_INSTANCING_CHANNEL_NAME];
	INSTANCING_CHANNEL_INFO(const WCHAR *name) 
	{
		PStrCopy(szName, name, MAX_INSTANCING_CHANNEL_NAME); 
		PStrLower(szName, MAX_INSTANCING_CHANNEL_NAME);
	}

	bool operator<(const INSTANCING_CHANNEL_INFO &other) const 
	{
		return PStrCmp(this->szName, other.szName) < 0;
	}
};

struct INSTANCED_CHANNEL_INFO
{
	WCHAR szName[MAX_CHAT_CNL_NAME];
	CHANNELID idChannel;
};
//Quick and dirty implementation of instanced channel map: use STL map.
typedef std::map<INSTANCING_CHANNEL_INFO, INSTANCED_CHANNEL_INFO> INSTANCED_CHANNEL_MAP;
static INSTANCED_CHANNEL_MAP sInstancedChannelMap;

//----------------------------------------------------------------------------
// Adds an instanced channel to the map.  Since we can only have one
// instanced channel per instancing channel, we allow ourselves to overwrite
// a given entry.
//----------------------------------------------------------------------------
void AddInstancedChannel(
	const WCHAR *wszInstancingChannelName,
	const WCHAR *wszChannelName,
	CHANNELID idChannel)
{
	INSTANCING_CHANNEL_INFO hInstancingChannel(
		wszInstancingChannelName);

	sInstancedChannelMap[hInstancingChannel].idChannel = idChannel;
	PStrCopy(sInstancedChannelMap[hInstancingChannel].szName,
		wszChannelName, MAX_CHAT_CNL_NAME);
}

//----------------------------------------------------------------------------
// Removes an instancing channel from the map.
//----------------------------------------------------------------------------
void RemoveInstancingChannel(
	const WCHAR *wszInstancingChannelName)
{
	INSTANCING_CHANNEL_INFO hInstancingChannel(
		wszInstancingChannelName);

	sInstancedChannelMap.erase(hInstancingChannel);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CHANNELID GetInstancedChannel(
	const WCHAR *wszInstancingChannelName,
	WCHAR *wszChannelNameOutput)
{
	INSTANCING_CHANNEL_INFO hInstancingChannel(
		wszInstancingChannelName);

	INSTANCED_CHANNEL_MAP::iterator it = 
		sInstancedChannelMap.find(hInstancingChannel);

	if(it != sInstancedChannelMap.end() )
	{
		if(wszChannelNameOutput) 
			PStrCopy(wszChannelNameOutput, it->second.szName, MAX_CHAT_CNL_NAME);

		return it->second.idChannel;
	}
	else
	{
		return INVALID_CHANNELID;
	}
}

BOOL ChannelMapHasInstancingChannel(
	const WCHAR *wszInstancingChannelName)
{
	INSTANCING_CHANNEL_INFO hInstancingChannel(
		wszInstancingChannelName);

	INSTANCED_CHANNEL_MAP::iterator it = 
		sInstancedChannelMap.find(hInstancingChannel);

	return (it != sInstancedChannelMap.end() );
}

#endif //!SERVER_VERSION

