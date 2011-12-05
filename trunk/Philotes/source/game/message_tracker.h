//Per-gameclient c_message and s_message rate tracking
//In future, will return false if we exceed the desired rate of a given message,
// so we can check it and disconnect the flooder.

//Presently, just notes the message rate to help us with our bot user model.

#ifndef _MESSAGE_TRACKER_H_
#define _MESSAGE_TRACKER_H_
#define CLIENT_PING_INTERVAL 5*GAME_TICKS_PER_SECOND
#define SERVER_PING_INTERVAL 53*GAME_TICKS_PER_SECOND
//TODO very soon: pull defines for above from a common location for c_network and s_network.

#define TRACE_MESSAGE_RATE //Activates message rate tracing in game logic

class MessageTracker
{
	public:
		inline void Reset()
		{
			structclear(*this);
		}

		void InitMessageTrackerForCCMDRates();

		BOOL AddMessage(DWORD dwTick, NET_CMD cmd, unsigned int size = 0);
		void TraceMessageRate(WCHAR *);
		static BOOL AddClientMessageToTrackerForCCMD(
			struct GAMECLIENT *pClient, 
			DWORD dwTick, NET_CMD cmd, unsigned int size = 0);

	protected:
		DWORD dwTickStart;
		DWORD dwTickLast;
		DWORD dwTickAssert;
		int nAsserts;
		unsigned int messagecount[256]; //255 is max possible NET_CMD
		unsigned int sizecount[256]; //total sum of hdr.size for each message

		BOOL MessageRateIsAllowed(NET_CMD cmd, float fProportionalTolerance, DWORD dwTimeTolerance, const struct RateLimit &tRateLimit);
};

inline void MessageTrackerReset(MessageTracker &tracker)
{
	tracker.Reset();
}

inline BOOL MessageTrackerAddMessage(MessageTracker &tracker, DWORD dwTick, NET_CMD cmd)
{
	return tracker.AddMessage(dwTick, cmd);
}

void TraceMessageRateForClient(struct GAME * pGame, struct GAMECLIENT *pClient);

#endif