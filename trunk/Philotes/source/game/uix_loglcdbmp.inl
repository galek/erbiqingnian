
#define _ 0x00,
#define X 0xff,

static const int scnNumeralW = 4;
static const int scnNumeralScanW = scnNumeralW * 10;
static const int scnNumeralH = 6;
const unsigned char sccNumeral[] = 
{
	_ X X _  _ _ X _  _ X X _   _ X X _   _ _ X _   _ X X X   _ X X _   _ X X X   _ X X _   _ X X _
	X _ _ X	 _ X X _  X _ _ X   X _ _ X   _ X X _   _ X _ _   X _ _ _   _ _ _ X   X _ _ X   X _ _ X
	X _ _ X	 _ _ X _  _ _ _ X   _ _ X _   _ X X _   _ X X _   X X X _   _ _ X _   _ X X _   _ X X X
	X _ _ X	 _ _ X _  _ X X _   _ _ _ X   X _ X _   _ _ _ X   X _ _ X   _ _ X _   X _ _ X   _ _ _ X
	X _ _ X	 _ _ X _  X _ _ _   X _ _ X   X X X X   _ _ _ X   X _ _ X   _ _ X _   X _ _ X   X _ _ X
	_ X X _	 _ _ X _  X X X X   _ X X _   _ _ X _   _ X X _   _ X X _   _ _ X _   _ X X _   _ X X _
};
			