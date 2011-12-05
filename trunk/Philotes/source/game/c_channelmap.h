#ifndef _C_CHANNELMAP_H_
#define _C_CHANNELMAP_H_

void AddInstancedChannel(
	const WCHAR *wszInstancingChannelName,
	const WCHAR *wszChannelName,
	CHANNELID idChannel);

void RemoveInstancingChannel(
	const WCHAR *wszInstancingChannelName);

CHANNELID GetInstancedChannel(
	const WCHAR *wszInstancingChannelName,
	WCHAR *wszChannelNameOutput = NULL);

BOOL ChannelMapHasInstancingChannel(
	const WCHAR *wszInstancingChannelName);

#endif //_C_CHANNELMAP_H_
