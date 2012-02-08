///PHILOTES Source Code.  (C)2012 PhiloLabs

#pragma once

#include "core/config.h"

namespace Philo {

class Bitwise {
public:

    static __forceinline unsigned int mostSignificantBitSet(unsigned int value)
    {
        unsigned int result = 0;
        while (value != 0) {
            ++result;
            value >>= 1;
        }
        return result-1;
    }

    static __forceinline uint32 firstPO2From(uint32 n)
    {
        --n;            
        n |= n >> 16;
        n |= n >> 8;
        n |= n >> 4;
        n |= n >> 2;
        n |= n >> 1;
        ++n;
        return n;
    }

    template<typename T>
    static __forceinline bool isPO2(T n)
    {
        return (n & (n-1)) == 0;
    }

	template<typename T>
    static __forceinline unsigned int getBitShift(T mask)
	{
		if (mask == 0)
			return 0;

		unsigned int result = 0;
		while ((mask & 1) == 0) {
			++result;
			mask >>= 1;
		}
		return result;
	}

	template<typename SrcT, typename DestT>
    static inline DestT convertBitPattern(SrcT srcValue, SrcT srcBitMask, DestT destBitMask)
	{
		srcValue = srcValue & srcBitMask;

		const unsigned int srcBitShift = getBitShift(srcBitMask);
		srcValue >>= srcBitShift;

		const SrcT srcMax = srcBitMask >> srcBitShift;

		const unsigned int destBitShift = getBitShift(destBitMask);
		const DestT destMax = destBitMask >> destBitShift;

		DestT destValue = (srcValue * destMax) / srcMax;
		return (destValue << destBitShift);
	}

    static inline unsigned int fixedToFixed(uint32 value, unsigned int n, unsigned int p) 
    {
        if(n > p) 
        {
            value >>= n-p;
        } 
        else if(n < p)
        {
            if(value == 0)
                    value = 0;
            else if(value == (static_cast<unsigned int>(1)<<n)-1)
                    value = (1<<p)-1;
            else    value = value*(1<<p)/((1<<n)-1);
        }
        return value;    
    }

    static inline unsigned int floatToFixed(const float value, const unsigned int bits)
    {
        if(value <= 0.0f) return 0;
        else if (value >= 1.0f) return (1<<bits)-1;
        else return (unsigned int)(value * (1<<bits));     
    }

    static inline float fixedToFloat(unsigned value, unsigned int bits)
    {
        return (float)value/(float)((1<<bits)-1);
    }

    static inline void intWrite(void *dest, const int n, const unsigned int value)
    {
        switch(n) {
            case 1:
                ((uint8*)dest)[0] = (uint8)value;
                break;
            case 2:
                ((uint16*)dest)[0] = (uint16)value;
                break;
            case 3:
#ifdef PHILO_BIG_ENDIAN
                ((uint8*)dest)[0] = (uint8)((value >> 16) & 0xFF);
                ((uint8*)dest)[1] = (uint8)((value >> 8) & 0xFF);
                ((uint8*)dest)[2] = (uint8)(value & 0xFF);
#else
                ((uint8*)dest)[2] = (uint8)((value >> 16) & 0xFF);
                ((uint8*)dest)[1] = (uint8)((value >> 8) & 0xFF);
                ((uint8*)dest)[0] = (uint8)(value & 0xFF);
#endif
                break;
            case 4:
                ((uint32*)dest)[0] = (uint32)value;                
                break;                
        }        
    }

    static inline unsigned int intRead(const void *src, int n) {
        switch(n) {
            case 1:
                return ((uint8*)src)[0];
            case 2:
                return ((uint16*)src)[0];
            case 3:
#ifdef PHILO_BIG_ENDIAN
                return ((uint32)((uint8*)src)[0]<<16)|
                        ((uint32)((uint8*)src)[1]<<8)|
                        ((uint32)((uint8*)src)[2]);
#else
                return ((uint32)((uint8*)src)[0])|
                        ((uint32)((uint8*)src)[1]<<8)|
                        ((uint32)((uint8*)src)[2]<<16);
#endif
            case 4:
                return ((uint32*)src)[0];
        } 
        return 0; // ?
    }


    static inline uint16 floatToHalf(float i)
    {
        union { float f; uint32 i; } v;
        v.f = i;
        return floatToHalfI(v.i);
    }

    static inline uint16 floatToHalfI(uint32 i)
    {
        register int s =  (i >> 16) & 0x00008000;
        register int e = ((i >> 23) & 0x000000ff) - (127 - 15);
        register int m =   i        & 0x007fffff;
    
        if (e <= 0)
        {
            if (e < -10)
            {
                return 0;
            }
            m = (m | 0x00800000) >> (1 - e);
    
            return static_cast<uint16>(s | (m >> 13));
        }
        else if (e == 0xff - (127 - 15))
        {
            if (m == 0) // Inf
            {
                return static_cast<uint16>(s | 0x7c00);
            } 
            else    // NAN
            {
                m >>= 13;
                return static_cast<uint16>(s | 0x7c00 | m | (m == 0));
            }
        }
        else
        {
            if (e > 30) // Overflow
            {
                return static_cast<uint16>(s | 0x7c00);
            }
    
            return static_cast<uint16>(s | (e << 10) | (m >> 13));
        }
    }
    
    static inline float halfToFloat(uint16 y)
    {
        union { float f; uint32 i; } v;
        v.i = halfToFloatI(y);
        return v.f;
    }

    static inline uint32 halfToFloatI(uint16 y)
    {
        register int s = (y >> 15) & 0x00000001;
        register int e = (y >> 10) & 0x0000001f;
        register int m =  y        & 0x000003ff;
    
        if (e == 0)
        {
            if (m == 0) // Plus or minus zero
            {
                return s << 31;
            }
            else // Denormalized number -- renormalize it
            {
                while (!(m & 0x00000400))
                {
                    m <<= 1;
                    e -=  1;
                }
    
                e += 1;
                m &= ~0x00000400;
            }
        }
        else if (e == 31)
        {
            if (m == 0) // Inf
            {
                return (s << 31) | 0x7f800000;
            }
            else // NaN
            {
                return (s << 31) | 0x7f800000 | (m << 13);
            }
        }
    
        e = e + (127 - 15);
        m = m << 13;
    
        return (s << 31) | (e << 23) | m;
    }

};

}
