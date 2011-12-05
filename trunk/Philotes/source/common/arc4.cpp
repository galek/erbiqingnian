//Robert Donald
//----------------------------------------------------------------------------
// arc4.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//ArcFour encryption
//TODO: add a debug lock around GetByte that asserts if there is ever any
//lock contention.  If threads are fighting for the lock, we are already
//screwed since RC4 must always receive in the correct order.


#include "arc4.h"


void ArcFourEncryptor::Init()
{
	for(int ii = 0; ii < 256; ii++)
	{
		m_bSBox[ii] = BYTE(ii);
	}
	i = 0;
	j = 0;
}

void ArcFourEncryptor::ApplyKey(BYTE * pbKey, size_t nBytes)
{
	ASSERT_RETURN(pbKey);
	ASSERT_RETURN(nBytes > 0);

	unsigned int ii;

	BYTE jj = 0;

	for(ii = 0; ii < 256; ii++)
	{
		jj += m_bSBox[ii] + pbKey[ii % nBytes];
		SWAP(m_bSBox[ii], m_bSBox[jj]);
	}
	
}

BYTE ArcFourEncryptor::GetByte()
{
	i++;
	j = BYTE(j+m_bSBox[i]);
	SWAP(m_bSBox[i], m_bSBox[j]);
	BYTE t = m_bSBox[i] + m_bSBox[j];
	return m_bSBox[t];
}

BYTE ArcFourEncryptor::EncryptByte(BYTE bByte)
{
	return bByte ^ GetByte();
}

//----------------------------------------------------------------------------
// TEST CODE
//----------------------------------------------------------------------------
void ARC4Test()
{
	char key[] = "Key";
	char plaintext[] = "Plaintext";

	ArcFourEncryptor Keyed( (BYTE*)(key), 3);
	for(int i = 0; i < 9; i++) trace("%02x",  Keyed.EncryptByte(BYTE(plaintext[i])) );
	trace("\n");
}