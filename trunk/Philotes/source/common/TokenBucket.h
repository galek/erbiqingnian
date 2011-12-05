//----------------------------------------------------------------------------
// TokenBucket.h
// 
// Modified     : $DateTime: 2007/11/20 09:04:17 $
// by           : $Author: pmason $
//
// Contains a token-bucket object to limit message rates.
//
// Basic principles: http://en.wikipedia.org/wiki/Token_bucket
//
// This implementation is slightly different in that it does not keep a timer,
// instead calculating the number of tokens available when asked.
//
// (C)Copyright 2007, Ping0 LLC. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

class TokenBucket
{
    protected:

        int         m_currentNumberOfMessageTokens;
        int         m_maxMessageTokens;
        int         m_millisecondsPerMessage;
		int			m_remainder;


        DWORD       m_lastCheckTick;

    public:

        // does nothing
        virtual ~TokenBucket(){;}

        // If the rate is < 0, that parameter is not limited.
        TokenBucket(float messageRate, // messages per second allowed
					// messageRate*burstThresholdInSeconds = max messages allowed in a burst.
                    DWORD burstThresholdInSeconds): 
					m_remainder(0)
        {
			ASSERTV(messageRate < 1000, "You should be using TokenBucketEx for rates >= 1000 tps.");
			ASSERTX_RETURN(messageRate > 0, "You may not create a TokenBucket with a message rate of zero!");

			m_millisecondsPerMessage = (int)(1000.0f / messageRate);

            // convert to milliseconds per message
            m_maxMessageTokens = m_currentNumberOfMessageTokens =
                                    burstThresholdInSeconds*1000/m_millisecondsPerMessage;

			m_lastCheckTick = GetTickCount();
        }

        // Returns true if message rate threshold permits
        // this message (or messages to be accepted).
        bool isAllowed(int numMessages)
        {
            DWORD dwTick= GetTickCount();
            DWORD interval =  dwTick - m_lastCheckTick ; //underflow is ok. 
            m_lastCheckTick = dwTick;

            if(m_millisecondsPerMessage > 0)
            {
                m_currentNumberOfMessageTokens  += 
							(interval + m_remainder)/m_millisecondsPerMessage;

				m_remainder = (interval + m_remainder) % m_millisecondsPerMessage;

                m_currentNumberOfMessageTokens = MIN(
                                              m_currentNumberOfMessageTokens,
                                              m_maxMessageTokens);

                int messageTokensAvailable = m_currentNumberOfMessageTokens 
                                                            - numMessages;

				if(messageTokensAvailable > max(numMessages,m_currentNumberOfMessageTokens) )
				{
					messageTokensAvailable = -1;
				}

                m_currentNumberOfMessageTokens = messageTokensAvailable;

                if(messageTokensAvailable < 0)
                {
                    return false;
                }
            }
            return true;
        }

};

// Used for higher token rate counts     -rli
class TokenBucketEx
{
	float m_fTokenPerMSec;
	INT32 m_iMaxTokenCount;
	INT32 m_iCurTokenCount;
	UINT32 m_uLastTick;

public:
	TokenBucketEx(UINT32 uTokenPerSecond, UINT32 uMaxTokenCount)
	{
		m_fTokenPerMSec = uTokenPerSecond / 1000.0f;
		m_iMaxTokenCount = static_cast<INT32>(uMaxTokenCount);
		if (m_iMaxTokenCount < 0)
			m_iMaxTokenCount = INT_MAX;
		m_iCurTokenCount = m_iMaxTokenCount;
		m_uLastTick = GetTickCount();
	}

	INT32 UpdateFromClock(void)
	{ 
		UINT32 uCurTick = GetTickCount();
		UINT32 uTickInterval = uCurTick - m_uLastTick;
		m_uLastTick = uCurTick;
		// we want to increase the token count, so we'll decrement by a negative number
		INT32 iDecrementTokenDelta = -static_cast<INT32>(uTickInterval * m_fTokenPerMSec); 
		if (iDecrementTokenDelta > 0)
			iDecrementTokenDelta = INT_MIN;
		DecrementTokenCount(iDecrementTokenDelta);
		return m_iCurTokenCount;
	} 
		
	INT32 DecrementTokenCount(INT32 iTokenDelta = 1)
	{	
		INT32 iNewCurTokenCount = m_iCurTokenCount - iTokenDelta; 
		if (iTokenDelta > 0 && iNewCurTokenCount > m_iCurTokenCount)
			m_iCurTokenCount = INT_MIN;
		else if (iTokenDelta < 0 && iNewCurTokenCount < m_iCurTokenCount)
			m_iCurTokenCount = INT_MAX;
		else 
			m_iCurTokenCount = iNewCurTokenCount;
		m_iCurTokenCount = min(m_iCurTokenCount, m_iMaxTokenCount);
		return m_iCurTokenCount; 
	}

	bool CanSend(UINT32 uTokenCount = 1, bool bAutoDecrement = false)
	{	
		INT32 iTokenCount = static_cast<INT32>(uTokenCount);
		if (iTokenCount < 0)
			iTokenCount = INT_MAX;
		if (m_iCurTokenCount < iTokenCount)
			UpdateFromClock();
		if (m_iCurTokenCount < iTokenCount)
			return false;
		if (bAutoDecrement) {
			INT32 iDecrementTokenDelta = static_cast<INT32>(uTokenCount);
			if (iDecrementTokenDelta < 0)
				iDecrementTokenDelta = INT_MAX;
			DecrementTokenCount(iDecrementTokenDelta);
		}
		return true;
	}
};

