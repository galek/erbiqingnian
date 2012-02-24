
/********************************************************************
日期:		2012年2月20日
文件名: 	MemDC.h
创建者:		王延兴
描述:		对内存DC的封装	
*********************************************************************/

#pragma 

#ifndef MemDCh
#define MemDCh
#endif

_NAMESPACE_BEGIN

class CMemDC : public CDC 
{
public:
	CMemDC(CPaintDC& dc, CBitmap* pBmp = NULL) : isMemDC(!dc.IsPrinting()) 
	{
		Rect = dc.m_ps.rcPaint;
		Set(&dc, Paint, pBmp);
	}

	CMemDC(CDC* pDC, CBitmap* pBmp = NULL) 
	{
		isMemDC=!pDC->IsPrinting();
		if(isMemDC) 
		{
			pDC->GetClipBox(&Rect); //For Views
			pDC->LPtoDP(&Rect);
		}
		Set(pDC, Draw, pBmp);
	}

	CMemDC(LPDRAWITEMSTRUCT lpDrawItemStruct, CBitmap* pBmp=NULL) : isMemDC(true)
	{
		Rect=lpDrawItemStruct->rcItem;
		Set(CDC::FromHandle(lpDrawItemStruct->hDC), DrawItem, pBmp);
	}

	~CMemDC() 
	{
		if(isMemDC) 
		{
			pDC->BitBlt(Rect.left, Rect.top, Rect.Width(), Rect.Height(), 
				this, Rect.left, Rect.top, SRCCOPY);
			SelectObject(OldBitmap);
		}
	}

	CMemDC* operator->() {return this;}
	operator CMemDC*()   {return this;}

private:
	enum dcType {Paint,Draw,DrawItem};

	void Set(CDC* _pDC, dcType Type, CBitmap* pBmp = NULL) 
	{
		ASSERT_VALID(_pDC);
		pDC = _pDC;
		OldBitmap = NULL;
		if(isMemDC) 
		{
			CreateCompatibleDC(pDC);
			if(pBmp!=NULL) OldBitmap = SelectObject(pBmp);
			else 
			{
				CRect rc;
				pDC->GetWindow()->GetClientRect(rc);
				Bitmap.CreateCompatibleBitmap(pDC, rc.Width(), rc.Height());
				OldBitmap = SelectObject(&Bitmap);
			}
			if(Type == Draw) 
			{
				SetMapMode(pDC->GetMapMode());
				pDC->DPtoLP(&Rect);
				SetWindowOrg(Rect.left, Rect.top);
			}
		}
		else
		{
			m_bPrinting	= pDC->m_bPrinting;
			m_hDC		= pDC->m_hDC;
			m_hAttribDC	= pDC->m_hAttribDC;
		}
	}

	CBitmap  Bitmap;    
	CBitmap* OldBitmap; 
	CDC*     pDC;       
	CRect    Rect;      
	bool     isMemDC;   
};

_NAMESPACE_END