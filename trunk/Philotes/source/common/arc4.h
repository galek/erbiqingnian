//Robert Donald
//----------------------------------------------------------------------------
// arc4.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//ArcFour encryption
#ifndef _ARC4_H_
#define _ARC4_H_

struct ArcFourEncryptor
{
protected:
	BYTE m_bSBox[256];
	BYTE i;
	BYTE j;

public:
	void Init();

	void ApplyKey(BYTE * pbKey, size_t nBytes);

	void Init(BYTE * pbKey, size_t nBytes)
	{
		Init();
		ApplyKey(pbKey, nBytes);
	}
	ArcFourEncryptor() {Init();}
	ArcFourEncryptor(BYTE * pbKey, size_t nBytes) {Init(pbKey, nBytes);}

	BYTE GetByte();
	BYTE EncryptByte(BYTE);
};

#endif