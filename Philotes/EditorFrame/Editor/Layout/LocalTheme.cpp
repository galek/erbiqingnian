
#include "LocalTheme.h"


CLocalTheme::CLocalTheme()
{
}

CLocalTheme::~CLocalTheme()
{
}


void CLocalTheme::DrawImage(CDC* pDC, CPoint pt, CSize sz, CXTPImageManagerIcon* pImage, 
							 BOOL bSelected, BOOL bPressed, BOOL bEnabled,
							 BOOL bChecked, BOOL bPopuped, BOOL  )
{
	if (!bEnabled)
	{
		pImage->Draw(pDC, pt, pImage->GetIcon(xtpImageDisabled),sz);
	}
	else if (bPopuped || bChecked) 
	{
		pImage->Draw(pDC, pt, pImage->GetIcon(bChecked && (bSelected || bPressed) ? xtpImageHot : xtpImageNormal),sz);
	}
	else
	{
		pImage->Draw(pDC, pt, pImage->GetIcon(bSelected || bPressed ? xtpImageHot : xtpImageNormal),sz);
	}
}