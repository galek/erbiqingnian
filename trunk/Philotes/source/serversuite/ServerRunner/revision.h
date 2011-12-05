//----------------------------------------------------------------------------
// revision.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef	_REVISION_H_
#define _REVISION_H_


//	Revision Month Character defines: month abbreviation decoding "arrays".
//			     a   b   c   d   e   f   g   h   i   j   k   l   m   n   o   p   q   r   s   t   u   v   w   x   y   z
#define RMC0 "\x03\x00\x00\x0b\x00\x01\x00\x00\x00\x00\x00\x00\x02\x0a\x09\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00"
#define RMC1 "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00"
#define RMC2 "\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x01\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x03\x00"

//	Revision Day Character define for optional 10s place day val
//			   SP   !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /   0   1   2   3   4   5   6   7   8   9
#define RDC "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09"

//	revision time/date encoding macros
//	__DATE__ expands to a literal ascii Mmm dd yyyy.																					//	bits used ( value range ) [ bit positions ]
#define REV_YEAR		(      (__DATE__[7]-'0')*1000 +      (__DATE__[8]-'0')*100 +      (__DATE__[9]-'0')*10 + (__DATE__[10]-'0') )	//	will be 9 bits with start year subtraction, ( 0 to 511 ) [31-23], years from given start year
#define REV_MONTH		( RMC0[(__DATE__[0]-'A')]     + RMC1[(__DATE__[1]-'a')]    + RMC2[(__DATE__[2] -'a')] )							//	4 bits ( 0 to 12 )  [22-19]
#define REV_DAY			( RDC [(__DATE__[4]-' ')]*10  +      (__DATE__[5]-'0') )														//	5 bits ( 1 to 31 )  [20-16]
//	__TIME__ expands to a literal ascii hh:mm:ss.						//	bits used ( value range ) [ bit positions ]
#define REV_HOUR		( (__TIME__[0]-'0')*10 + (__TIME__[1] -'0') )	//	5 bits ( 0 to 24 )  [15-11]
#define REV_MIN			( (__TIME__[3]-'0')*10 + (__TIME__[4] -'0') )	//	6 bits ( 0 to 59 )  [10- 5]
#define REV_10SOFSEC	  (__TIME__[6]-'0')								//	3 bits ( 0 to 5 )   [ 4- 2], build versions shouldn't need more accuracy than tens of seconds.


//----------------------------------------------------------------------------
//	UINT32 revision number based on compilation date and time.
//	NOTE: Assign in .cpp files for fixed at compile time in static libraries, updated every re-compile for exe.
//----------------------------------------------------------------------------
#define TIMESTAMP_REVISION(startYear)	((UINT32)( ( ((REV_YEAR)- startYear ) << 23 ) | \
												     ( REV_MONTH			  << 19 ) | \
												     ( REV_DAY				  << 14 ) | \
												     ( REV_HOUR				  <<  9 ) | \
												     ( REV_MIN				  <<  3 ) | \
												     ( REV_10SOFSEC ) ))


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int TimestampRevisionToString(
				char *	dest,
				int		destLen,
				UINT32	revision,
				UINT32  startYear )
{
	static const char * months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
									 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	return PStrPrintf(
			dest,
			destLen,
			"%s %.2d, %d %.2d:%.2d:%.2d",
			months[(( revision >> 19 ) & 0xF ) - 1 ],
			        ( revision >> 14 ) & 0x1F,
					( revision >> 23 ) + startYear,
					( revision >> 9 ) & 0x1F,
					( revision >> 3 ) & 0x3F,
					( revision & 0x7 ) * 10 );
}


#endif	//	_REVISION_H_
