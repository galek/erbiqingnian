
/********************************************************************
	����:		2012��2��20��
	�ļ���: 	EditorUtils.h
	������:		������
	����:		�༭�����õĹ��ߺ���	
*********************************************************************/

#pragma once

_NAMESPACE_BEGIN

#define TOOLBAR_TRANSPARENT_COLOR RGB(0xC0,0xC0,0xC0)

struct CMFCUtils
{
	static BOOL LoadTrueColorImageList( CImageList &imageList,UINT nIDResource,int nIconWidth,COLORREF colMaskColor );

	static void TransparentBlt( HDC hdcDest, int nXDest, int nYDest, int nWidth,int nHeight, HBITMAP hBitmap, int nXSrc, int nYSrc,
		COLORREF colorTransparent );
};

//////////////////////////////////////////////////////////////////////////

struct HeapCheck
{
	static void Check( const char *file,int line );
};

#ifdef _DEBUG
#define HEAP_CHECK HeapCheck::Check( __FILE__,__LINE__ );
#else
#define HEAP_CHECK
#endif


_NAMESPACE_END