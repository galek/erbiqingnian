#ifndef _S_LOADBALANCENETWORK_H_
#define _S_LOADBALANCENETWORK_H_

BOOL GameServerToGameLoadBalanceServerSendMessage(
	MSG_STRUCT * msg,
	NET_CMD		 command);

BOOL SendGameLoadBalanceHeartBeat(
	GameServerContext *pServerContext);

BOOL SendGameLoadBalanceLogin(
	GameServerContext *pServerContext, 
	NETCLT_USER *pClientContext);

BOOL SendGameLoadBalanceLogout(
	GameServerContext *pServerContext, 
	NETCLT_USER *pClientContext);

#endif
