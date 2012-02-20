
#include "InPlaceButton.h"

// CInPlaceButton

IMPLEMENT_DYNAMIC(CInPlaceButton, CXTButton)

BEGIN_MESSAGE_MAP(CInPlaceButton, CXTButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnBnClicked)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CInPlaceCheckBox, CButton)
BEGIN_MESSAGE_MAP(CInPlaceCheckBox, CButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnBnClicked)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CInPlaceColorButton, CColorButton)
BEGIN_MESSAGE_MAP(CInPlaceColorButton, CColorButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnBnClicked)
END_MESSAGE_MAP()
