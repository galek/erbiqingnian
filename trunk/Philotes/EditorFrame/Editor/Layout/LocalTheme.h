
/********************************************************************
	����:		2012��2��22��
	�ļ���: 	LocalTheme.h
	������:		������
	����:		�༭������	
*********************************************************************/

#pragma once

class CLocalTheme : public CXTPNativeXPTheme
{
public:
	CLocalTheme();

	virtual ~CLocalTheme();

	virtual void DrawImage(CDC* pDC, CPoint pt, CSize sz, CXTPImageManagerIcon* pImage,
		BOOL bSelected, BOOL bPressed, BOOL bEnabled = TRUE,
		BOOL bChecked = FALSE, BOOL bPopuped = FALSE, BOOL bToolBarImage = TRUE);
};
