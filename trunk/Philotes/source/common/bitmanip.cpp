//-------------------------------------------------------------------------------------------------
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// INCLUDE
//-------------------------------------------------------------------------------------------------



//-------------------------------------------------------------------------------------------------
// CONSTANTS
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// i: OFFSET to start at (0 is high bit in byte)
// j: number of bits to mask off 
//-------------------------------------------------------------------------------------------------
const BYTE gbStartLenBitMask[8][32] =
{
	//  1     2     3      4     5     6     7     8    9     10    11    12    13    14    15    16    17    18    19    20    21    22    23    24    25    26    27    28    29    30    31    32
	{ 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
	{ 0x40, 0x60, 0x70, 0x78, 0x7c, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f },
	{ 0x20, 0x30, 0x38, 0x3c, 0x3e, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f },
	{ 0x10, 0x18, 0x1c, 0x1e, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f },
	{ 0x08, 0x0c, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f },
	{ 0x04, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07 },
	{ 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 },
	{ 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 }
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const int gnBitCountTable[256] =
{
	0,  1,  1,  2,  1,  2,  2,  3,  1,  2,  2,  3,  2,  3,  3,  4, 
	1,  2,  2,  3,  2,  3,  3,  4,  2,  3,  3,  4,  3,  4,  4,  5, 
	1,  2,  2,  3,  2,  3,  3,  4,  2,  3,  3,  4,  3,  4,  4,  5, 
	2,  3,  3,  4,  3,  4,  4,  5,  3,  4,  4,  5,  4,  5,  5,  6, 
	1,  2,  2,  3,  2,  3,  3,  4,  2,  3,  3,  4,  3,  4,  4,  5, 
	2,  3,  3,  4,  3,  4,  4,  5,  3,  4,  4,  5,  4,  5,  5,  6, 
	2,  3,  3,  4,  3,  4,  4,  5,  3,  4,  4,  5,  4,  5,  5,  6, 
	3,  4,  4,  5,  4,  5,  5,  6,  4,  5,  5,  6,  5,  6,  6,  7, 
	1,  2,  2,  3,  2,  3,  3,  4,  2,  3,  3,  4,  3,  4,  4,  5, 
	2,  3,  3,  4,  3,  4,  4,  5,  3,  4,  4,  5,  4,  5,  5,  6, 
	2,  3,  3,  4,  3,  4,  4,  5,  3,  4,  4,  5,  4,  5,  5,  6, 
	3,  4,  4,  5,  4,  5,  5,  6,  4,  5,  5,  6,  5,  6,  6,  7, 
	2,  3,  3,  4,  3,  4,  4,  5,  3,  4,  4,  5,  4,  5,  5,  6, 
	3,  4,  4,  5,  4,  5,  5,  6,  4,  5,  5,  6,  5,  6,  6,  7, 
	3,  4,  4,  5,  4,  5,  5,  6,  4,  5,  5,  6,  5,  6,  6,  7, 
	4,  5,  5,  6,  5,  6,  6,  7,  5,  6,  6,  7,  6,  7,  7,  8, 
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
void ObsTest(
	void)
{
	RAND rand;
	RandInit(rand, 1);

	struct TEST_OBS
	{
		union
		{
			unsigned int		uval;
			int					ival;
		};
		unsigned int		t;
		unsigned int		bytes;
		unsigned int		cur;
	};

	const unsigned int BUF_LEN = 4091;
	DWORD obuffer[BUF_LEN];
	for (int ii = 0; ii < BUF_LEN; ++ii)
	{
		obuffer[ii] = ii;
	}

	DWORD buffer[BUF_LEN];
	BYTE_BUF_OBFUSCATED buf(buffer, BUF_LEN * sizeof(DWORD), 69069, 666);
	buf.PushBuf(obuffer, BUF_LEN * sizeof(DWORD), 0);
	buf.SetCursor(0);
	for (int ii = 0; ii < BUF_LEN; ++ii)
	{
		DWORD dw;
		ASSERT(buf.PopUInt(&dw, sizeof(DWORD)));
		ASSERT(dw == obuffer[ii]);
	}
	buf.SetCursor(0);

	TEST_OBS testArray[BUF_LEN];
	for (int ii = 0; ii < BUF_LEN; ++ii)
	{
		testArray[ii].cur = buf.GetCursor();
		testArray[ii].t = RandGetNum(rand, 0, 3);
		switch (testArray[ii].t)
		{
		case 0:	// PushUInt
			{
				testArray[ii].bytes = RandGetNum(rand, 0, 2);
				switch (testArray[ii].bytes)
				{
				case 0:	// byte
					{
						testArray[ii].uval = RandGetNum(rand, 0, 0xff);
						BYTE b = BYTE(testArray[ii].uval);
						ASSERT(buf.PushUInt(b));
						break;
					}
				case 1: // word
					{
						testArray[ii].uval = RandGetNum(rand, 0, 0xffff);
						WORD w = WORD(testArray[ii].uval);
						ASSERT(buf.PushUInt(w));
						break;
					}
				case 2: // dword
					{
						testArray[ii].uval = RandGetNum(rand);
						DWORD dw = testArray[ii].uval;
						ASSERT(buf.PushUInt(dw));
						break;
					}
				default: ASSERT(0);
				}
				break;
			}
		case 1:	// PushUInt, len
			{
				testArray[ii].bytes = RandGetNum(rand, 0, 2) * 16;
				switch (testArray[ii].bytes / 16)
				{
				case 0:	// byte
					{
						testArray[ii].bytes += 1;
						testArray[ii].uval = RandGetNum(rand, 0, 0xff);
						BYTE b = BYTE(testArray[ii].uval);
						ASSERT(buf.PushUInt(b, 1));
						break;
					}
				case 1: // word
					{
						testArray[ii].bytes += RandGetNum(rand, 1, 2);
						switch (testArray[ii].bytes & 15)
						{
						case 1:		testArray[ii].uval = RandGetNum(rand, 0, 0xff); break;
						case 2:		testArray[ii].uval = RandGetNum(rand, 0, 0xffff); break;
						default:	ASSERT(0);
						}
						WORD w = WORD(testArray[ii].uval);
						ASSERT(buf.PushUInt(w, (testArray[ii].bytes & 15)));
						break;
					}
				case 2: // dword
					{
						testArray[ii].bytes += RandGetNum(rand, 1, 4);
						switch (testArray[ii].bytes & 15)
						{
						case 1:		testArray[ii].uval = RandGetNum(rand, 0, 0xff); break;
						case 2:		testArray[ii].uval = RandGetNum(rand, 0, 0xffff); break;
						case 3:		testArray[ii].uval = RandGetNum(rand, 0, 0xffffff); break;
						case 4:		testArray[ii].uval = RandGetNum(rand);  break;
						default:	ASSERT(0);
						}
						DWORD dw = testArray[ii].uval;
						ASSERT(buf.PushUInt(dw, (testArray[ii].bytes & 15)));
						break;
					}
				default: ASSERT(0);
				}
				break;
			}
		case 2:	// PushInt
			{
				testArray[ii].bytes = RandGetNum(rand, 0, 2);
				switch (testArray[ii].bytes)
				{
				case 0:	// char
					{
						testArray[ii].ival = (int)RandGetNum(rand, -128, 127);
						char c = (char)testArray[ii].ival;
						ASSERT(buf.PushInt(c));
						break;
					}
				case 1: // short
					{
						testArray[ii].ival = (int)RandGetNum(rand, -32768, 32767);
						short s = (short)testArray[ii].ival;
						ASSERT(buf.PushInt(s));
						break;
					}
				case 2: // int
					{
						testArray[ii].ival = (int)RandGetNum(rand);
						int i = testArray[ii].ival;
						ASSERT(buf.PushInt(i));
						break;
					}
				default: ASSERT(0);
				}
				break;
			}
		case 3:	// PushInt, len
			{
				testArray[ii].bytes = RandGetNum(rand, 0, 2) * 16;
				switch (testArray[ii].bytes / 16)
				{
				case 0:	// char
					{
						testArray[ii].bytes += 1;
						testArray[ii].ival = RandGetNum(rand, -128, 127);
						char b = char(testArray[ii].ival);
						ASSERT(buf.PushInt(b, 1));
						break;
					}
				case 1: // word
					{
						testArray[ii].bytes += RandGetNum(rand, 1, 2);
						switch (testArray[ii].bytes & 15)
						{
						case 1:		testArray[ii].ival = RandGetNum(rand, -128, 127); break;
						case 2:		testArray[ii].ival = RandGetNum(rand, -32768, 32767); break;
						default:	ASSERT(0);
						}
						short s = short(testArray[ii].ival);
						ASSERT(buf.PushInt(s, (testArray[ii].bytes & 15)));
						break;
					}
				case 2: // dword
					{
						testArray[ii].bytes += RandGetNum(rand, 1, 4);
						switch (testArray[ii].bytes & 15)
						{
						case 1:		testArray[ii].ival = RandGetNum(rand, -128, 127); break;
						case 2:		testArray[ii].ival = RandGetNum(rand, -32768, 32767); break;
						case 3:		testArray[ii].ival = RandGetNum(rand, -(1 << 23), (1 << 23) - 1); break;
						case 4:		testArray[ii].ival = RandGetNum(rand); break;
						default:	ASSERT(0);
						}
						int dw = testArray[ii].ival;
						ASSERT(buf.PushInt(dw, (testArray[ii].bytes & 15)));
						break;
					}
				default: ASSERT(0);
				}
				break;
			}
		}
	}

	buf.SetCursor(0);
	for (int ii = 0; ii < BUF_LEN; ++ii)
	{
		ASSERT(testArray[ii].cur == buf.GetCursor());
		switch (testArray[ii].t)
		{
		case 0:	// PushUInt
			{
				switch (testArray[ii].bytes)
				{
				case 0:	// byte
					{
						BYTE b;
						ASSERT(buf.PopUInt(&b));
						ASSERT(b == testArray[ii].uval);
						break;
					}
				case 1: // word
					{
						WORD w;
						ASSERT(buf.PopUInt(&w));
						ASSERT(w == testArray[ii].uval);
						break;
					}
				case 2: // dword
					{
						DWORD dw;
						ASSERT(buf.PopUInt(&dw));
						ASSERT(dw == testArray[ii].uval);
						break;
					}
				default: ASSERT(0);
				}
				break;
			}
		case 1:	// PushUInt, len
			{
				switch (testArray[ii].bytes / 16)
				{
				case 0:	// byte
					{
						BYTE b;
						ASSERT(buf.PopUInt(&b, 1));
						ASSERT(b == testArray[ii].uval);
						break;
					}
				case 1: // word
					{
						WORD w;
						ASSERT(buf.PopUInt(&w, (testArray[ii].bytes & 15)));
						ASSERT(w == testArray[ii].uval);
						break;
					}
				case 2: // dword
					{
						DWORD dw;
						ASSERT(buf.PopUInt(&dw, (testArray[ii].bytes & 15)));
						ASSERT(dw == testArray[ii].uval);
						break;
					}
				default: ASSERT(0);
				}
				break;
			}
		case 2:	// PushInt
			{
				switch (testArray[ii].bytes)
				{
				case 0:	// char
					{
						char c;
						ASSERT(buf.PopInt(&c));
						ASSERT(c == testArray[ii].ival);
						break;
					}
				case 1: // short
					{
						short s;
						ASSERT(buf.PopInt(&s));
						ASSERT(s == testArray[ii].ival);
						break;
					}
				case 2: // int
					{
						int i;
						ASSERT(buf.PopInt(&i));
						ASSERT(i == testArray[ii].ival);
						break;
					}
				default: ASSERT(0);
				}
				break;
			}
		case 3:	// PushInt, len
			{
				switch (testArray[ii].bytes / 16)
				{
				case 0:	// char
					{
						char c;
						ASSERT(buf.PopInt(&c, 1));
						ASSERT(c == testArray[ii].ival);
						break;
					}
				case 1: // short
					{
						short s;
						ASSERT(buf.PopInt(&s, (testArray[ii].bytes & 15)));
						ASSERT(s == testArray[ii].ival);
						break;
					}
				case 2: // dword
					{
						int i;
						ASSERT(buf.PopInt(&i, (testArray[ii].bytes & 15)));
						ASSERT(i == testArray[ii].ival);
						break;
					}
				default: ASSERT(0);
				}
				break;
			}
		}
	}
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Dec2Bin64(
	char * str,
	UINT64 val)
{
	for (unsigned int kk = 0; kk < 64; ++kk)
	{
		str[63-kk] = ((UINT64)val & (UINT64)(1ULL << (UINT64)kk)) ? '1' : '0';
	}
	str[64] = 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
void FFSTest(
	void)
{
	RAND rand;
	RandInit(rand, 1, 1);

	trace("%64s  %3s\n", "x", "ffs");

	{
		UINT64 x = 0;
		unsigned int ffs = LEAST_SIGNIFICANT_BIT(x);
		unsigned int ffs64 = LEAST_SIGNIFICANT_BIT((UINT64)x);
		ASSERT(ffs == -1 && ffs64 == -1);

		char bin[65];
		Dec2Bin64(bin, 0);
		trace("%64s  %3d\n", bin, ffs64);
	}

	// test Find First Set Bit 32
	for (unsigned int ii = 0; ii < 64; ++ii)
	{
		UINT64 x = 1ULL << (63 - ii);
		unsigned int loops = (ii > 5 ? 32 : (2 << ii));
		for (unsigned int jj = 0; jj < loops; ++jj)
		{
			UINT64 y = x;
			if (loops < 32)
			{
				y += (UINT64)jj * (1ULL << (63 - ii + 1));
			}
			else if (jj > 0)
			{
				y += (Rand64(rand) << (64 - ii));
			}
			unsigned int ffs64 = LEAST_SIGNIFICANT_BIT(UINT64(y));
			if (ii < 32)
			{
				unsigned int ffs = LEAST_SIGNIFICANT_BIT(y);
				ASSERT(ffs == ffs64);
			}
#pragma warning(suppress : 6292) //ill-defined for loop
			for (unsigned int kk = 0; kk > ffs64; ++kk)
			{
				ASSERT((y & (1ULL << kk)) == 0);
			}
			ASSERT((y & (1ULL << ffs64)) != 0);

			char bin[65];
			Dec2Bin64(bin, y);
			trace("%64s  %3d\n", bin, ffs64);
		}
	}
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
unsigned int BST_Search_InsertionPoint(
	unsigned int * list,
	unsigned int count,
	unsigned int key)
{
	unsigned int * min = list;
	unsigned int * max = list + count;
	unsigned int * ii;
	while (min < max)
	{
		ii = min + (max - min) / 2;
		if (*ii > key)
		{
			max = ii;
		}
		else if (*ii < key)
		{
			min = ii + 1;
		}
		else
		{
			return (unsigned int)(ii - list);
		}
	}
	if (max < list + count)
	{
		return (unsigned int)(max - list);
	}
	return count;
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
unsigned int BST_Search_Smaller(
	unsigned int * list,
	unsigned int count,
	unsigned int key)
{
	unsigned int * min = list;
	unsigned int * max = list + count;
	unsigned int * ii;
	while (min < max)
	{
		ii = min + (max - min) / 2;
		if (*ii > key)
		{
			max = ii;
		}
		else if (*ii < key)
		{
			min = ii + 1;
		}
		else
		{
			return (unsigned int)(ii - list);
		}
	}
	if (max > list)
	{
		return (unsigned int)((max - 1) - list);
	}
	return count;
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
unsigned int BST_Search_Exact(
	unsigned int * list,
	unsigned int count,
	unsigned int key)
{
	unsigned int * min = list;
	unsigned int * max = list + count;
	unsigned int * ii;
	while (min < max)
	{
		ii = min + (max - min) / 2;
		if (*ii > key)
		{
			max = ii;
		}
		else if (*ii < key)
		{
			min = ii + 1;
		}
		else
		{
			return (unsigned int)(ii - list);
		}
	}
	return count;
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
void BST_Shuffle(
	RAND & rand,
	unsigned int * list,
	unsigned int count)
{
	unsigned int ii = 0;
	do
	{
		unsigned int remaining = count - ii;
		if (remaining <= 1)
		{
			break;
		}
		unsigned int jj = ii + RandGetNum(rand, remaining);
		unsigned int temp = list[jj];
		list[jj] = list[ii];
		list[ii] = temp;
	} while (++ii);
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
void BST_Trace(
	unsigned int * list,
	unsigned int count)
{
	for (unsigned int ii = 0; ii < count - 1; ++ii)
	{
		trace("%d, ", list[ii]);
	}
	trace("%d\n", list[count - 1]);
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
void BSearchTest(
	void)
{
	const unsigned int ITER = 250;
	const unsigned int SIZE = 20;

	unsigned int listA[SIZE];
	unsigned int listB[SIZE];
	unsigned int curlist = 0;
	unsigned int * list[2] = { listA, listB };

	RAND rand;
	RandInit(rand, 1, 1);

	for (unsigned int ii = 0; ii < SIZE; ++ii)
	{
		listA[ii] = ii;
		listB[ii] = ii;
	}

	for (unsigned int ii = 0; ii < ITER; ++ii)
	{
		unsigned int othlist = (curlist + 1) % 2;
		for (unsigned int jj = 0; jj < SIZE; ++jj)
		{
			(list[othlist])[jj] = (unsigned int)-1;
		}

		trace(" start: ");
		BST_Shuffle(rand, list[curlist], SIZE);
		BST_Trace(list[curlist], SIZE);

		for (unsigned int jj = 0; jj < SIZE; ++jj)
		{
			unsigned int elem = (list[curlist])[jj];

			unsigned int exact = BST_Search_Exact(list[othlist], jj, elem);
			ASSERT(exact == jj);

			unsigned int smaller = BST_Search_Smaller(list[othlist], jj, elem);
			if (smaller == jj)
			{
				ASSERT(jj == 0 || (list[othlist])[jj - 1] > elem);
			}
			else
			{
				ASSERT((list[othlist])[smaller] <= elem);
				ASSERT(smaller >= jj - 1 || (list[othlist])[smaller + 1] > elem);
			}

			unsigned int idx = BST_Search_InsertionPoint(list[othlist], jj, elem);
			if (jj > idx)
			{
				memmove(list[othlist] + idx + 1, list[othlist] + idx, sizeof(unsigned int) * (jj - idx));
			}
			(list[othlist])[idx] = elem;

			exact = BST_Search_Exact(list[othlist], jj + 1, elem);
			ASSERT((list[othlist])[exact] == elem);
		}

		for (unsigned int jj = 0; jj < SIZE; ++jj)
		{
			ASSERT((list[othlist])[jj] == jj);
		}

		trace("result: ");
		BST_Trace(list[othlist], SIZE);

		curlist = othlist;
	}
}
#endif