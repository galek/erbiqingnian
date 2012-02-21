
#include "PropertyItem.h"
#include "PropertyCtrl.h"
#include "InPlaceEdit.h"
#include "InPlaceComboBox.h"
#include "InPlaceButton.h"
#include "NumberCtrl.h"
#include "FillSliderCtrl.h"
//#include "SplineCtrl.h"
//#include "ColorGradientCtrl.h"
#include "SliderCtrlEx.h"
#include "Settings.h"
#include "UIEnumerations.h"
#include "UIEnumsDatabase.h"
#include "EditorRoot.h"
#include "ColorUtil.h"
#include "CustomColorDialog.h"

#define CMD_ADD_CHILD_ITEM	100
#define CMD_ADD_ITEM		101
#define CMD_DELETE_ITEM		102

#define BUTTON_WIDTH		(16)
#define NUMBER_CTRL_WIDTH	60


class CUndoVariableChange /*: public IUndoObject*/
{
public:
	CUndoVariableChange( IVariable *var,const char *undoDescription )
	{
		assert( var != 0 );
		m_undoDescription = undoDescription;
		m_var = var;
		m_undo = m_var->Clone(false);
	}
protected:
	virtual int GetSize() 
	{
		int size = sizeof(*this);
		if (m_undo)
		{
			size += m_undo->GetSize();
		}
		if (m_redo)
		{
			size += m_redo->GetSize();
		}
		return size;
	}

	virtual const char* GetDescription() 
	{
		return m_undoDescription; 
	}

	virtual void Undo( bool bUndo )
	{
		if (bUndo)
		{
			m_redo = m_var->Clone(false);
		}
		m_var->CopyValue( m_undo );
	}

	virtual void Redo()
	{
		if (m_redo)
		{
			m_var->CopyValue(m_redo);
		}
	}

private:
	CString m_undoDescription;
	TSmartPtr<IVariable> m_undo;
	TSmartPtr<IVariable> m_redo;
	TSmartPtr<IVariable> m_var;
};

namespace 
{
	struct 
	{
		int dataType;
		const char *name;
		PropertyType type;
		int image;
	} s_propertyTypeNames[] =
	{
		{ IVariable::DT_SIMPLE,	"Bool",		ePropertyBool,2 },
		{ IVariable::DT_SIMPLE,	"Int",		ePropertyInt,0 },
		{ IVariable::DT_SIMPLE,	"Float",	ePropertyFloat,0 },
		{ IVariable::DT_SIMPLE,	"Vector",	ePropertyVector,10 },
		{ IVariable::DT_SIMPLE,	"String",	ePropertyString,3 },
		{ IVariable::DT_PERCENT,"Float",	ePropertyInt,13 },
		{ IVariable::DT_BOOLEAN,"Boolean",	ePropertyBool,2 },
		{ IVariable::DT_COLOR,	"Color",	ePropertyColor,1 },
		{ IVariable::DT_ANGLE,	"Angle",	ePropertyAngle,0 },
		{ IVariable::DT_SIMPLE,	"Selection",ePropertySelection,-1 }
	};

	static int NumPropertyTypes = sizeof(s_propertyTypeNames)/sizeof(s_propertyTypeNames[0]);

	const char* DISPLAY_NAME_ATTR	= "DisplayName";
	const char* VALUE_ATTR			= "Value";
	const char* TYPE_ATTR			= "Type";
	const char* TIP_ATTR			= "Tip";
	const char* FILEFILTER_ATTR		= "FileFilters";
};

CPropertyItem::CPropertyItem( CPropertyCtrl* pCtrl )
{
	m_propertyCtrl			= pCtrl;
	m_parent				= NULL;
	m_bExpandable			= false;
	m_bExpanded				= false;
	m_bEditChildren 		= false;
	m_bShowChildren 		= false;
	m_bSelected				= false;
	m_bNoCategory			= false;

	m_pStaticText			= NULL;
	m_cNumber				= NULL;
	m_cNumber1				= NULL;
	m_cNumber2				= NULL;
	m_cFillSlider			= NULL;
	m_cEdit					= NULL;
	m_cCombo				= NULL;
	m_cButton				= NULL;
	m_cButton2				= NULL;
	m_cButton3				= NULL;
	m_cExpandButton 		= NULL;
	m_cCheckBox				= NULL;
	m_cColorButton			= NULL;

	m_image					= -1;
	m_bIgnoreChildsUpdate	= false;
	m_value					= "";
	m_type					= ePropertyInvalid;

	m_modified				= false;
	m_bMoveControls 		= false;
	//m_lastModified			= 0;
	m_valueMultiplier		= 1;
	m_pEnumDBItem			= 0;

	m_nHeight				= 14;

	m_nCategoryPageId		= -1;
}

CPropertyItem::~CPropertyItem()
{
	AddRef();

	DestroyInPlaceControl();

	if (m_pVariable)
		ReleaseVariable();

	for (int i = 0; i < m_childs.size(); i++)
	{
		m_childs[i]->m_parent = 0;
	}
}

void CPropertyItem::SetXmlNode( XmlNodeRef &node )
{
	m_node = node;
	//GetIEditor()->SuspendUndo();
	ParseXmlNode();
	//GetIEditor()->ResumeUndo();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::ParseXmlNode( bool bRecursive/* =true  */)
{
	if (!m_node->getAttr( DISPLAY_NAME_ATTR,m_name ))
	{
		m_name = m_node->getTag();
	}

	CString value;
	bool bHasValue=m_node->getAttr( VALUE_ATTR,value );

	CString type;
	m_node->getAttr( TYPE_ATTR,type);

	m_tip = "";
	m_node->getAttr( TIP_ATTR,m_tip );

	m_image = -1;
	m_type = ePropertyInvalid;
	for (int i = 0; i < NumPropertyTypes; i++)
	{
		if (stricmp(type,s_propertyTypeNames[i].name) == 0)
		{
			m_type = s_propertyTypeNames[i].type;
			m_image = s_propertyTypeNames[i].image;
			break;
		}
	}

	m_rangeMin = 0;
	m_rangeMax = 100;
	m_step = 0.1f;
	m_bHardMin = m_bHardMax = false;
	if (m_type == ePropertyFloat || m_type == ePropertyInt)
	{
		if (m_node->getAttr( "Min", m_rangeMin ))
			m_bHardMin = true;
		if (m_node->getAttr( "Max", m_rangeMax ))
			m_bHardMax = true;

		int nPrecision;
		if (!m_node->getAttr( "Precision", nPrecision))
			nPrecision = std::max(3 - int(log(m_rangeMax - m_rangeMin) / log(10.f)), 0);
		m_step = powf(10.f, -nPrecision);
	}

	if (bHasValue)
		SetValue( value,false );

	m_bNoCategory = false;

	if (m_type == ePropertyVector && !m_propertyCtrl->IsExtenedUI())
	{
		bRecursive = false;

		m_childs.clear();
		Float3 vec;
		m_node->getAttr( VALUE_ATTR,vec );
		
		XmlNodeRef x = m_node->createNode( "X" );
		XmlNodeRef y = m_node->createNode( "Y" );
		XmlNodeRef z = m_node->createNode( "Z" );
		x->setAttr( TYPE_ATTR,"Float" );
		y->setAttr( TYPE_ATTR,"Float" );
		z->setAttr( TYPE_ATTR,"Float" );

		x->setAttr( VALUE_ATTR,vec.x );
		y->setAttr( VALUE_ATTR,vec.y );
		z->setAttr( VALUE_ATTR,vec.z );

		m_bIgnoreChildsUpdate = true;

		CPropertyItemPtr itemX = new CPropertyItem( m_propertyCtrl );
		itemX->SetXmlNode( x );
		AddChild( itemX );

		CPropertyItemPtr itemY = new CPropertyItem( m_propertyCtrl );
		itemY->SetXmlNode( y );
		AddChild( itemY );

		CPropertyItemPtr itemZ = new CPropertyItem( m_propertyCtrl );
		itemZ->SetXmlNode( z );
		AddChild( itemZ );

		m_bNoCategory = true;
		m_bExpandable = true;

		m_bIgnoreChildsUpdate = false;
	}
	else if (bRecursive)
	{
		m_bExpandable = false;
		
		for (int i=0; i < m_node->getChildCount(); i++)
		{
			m_bIgnoreChildsUpdate = true;

			XmlNodeRef child = m_node->getChild(i);
			CPropertyItemPtr item = new CPropertyItem( m_propertyCtrl );
			item->SetXmlNode( m_node->getChild(i) );
			AddChild( item );
			m_bExpandable = true;

			m_bIgnoreChildsUpdate = false;
		}
	}
	m_modified = false;
}

void CPropertyItem::SetVariable( IVariable *var )
{
	if (var == m_pVariable)
		return;

	TSmartPtr<IVariable> pInputVar = var;

	if (m_pVariable)
	{
		ReleaseVariable();
	}

	m_pVariable = pInputVar;
	assert( m_pVariable );

	m_pVariable->AddOnSetCallback( functor(*this,&CPropertyItem::OnVariableChange) );

	m_tip = "";
	m_name = m_pVariable->GetHumanName();

	int dataType = m_pVariable->GetDataType();

	m_image = -1;
	m_type = ePropertyInvalid;
	int i;

	if (dataType != IVariable::DT_SIMPLE)
	{
		for (i = 0; i < NumPropertyTypes; i++)
		{
			if (dataType == s_propertyTypeNames[i].dataType)
			{
				m_type = s_propertyTypeNames[i].type;
				m_image = s_propertyTypeNames[i].image;
				break;
			}
		}
	}

	m_enumList = m_pVariable->GetEnumList();
	if (m_enumList)
	{
		m_type = ePropertySelection;
	}

	if (m_type == ePropertyInvalid)
	{
		switch(m_pVariable->GetType()) 
		{
		case IVariable::INT:
			m_type = ePropertyInt;
			break;
		case IVariable::BOOL:
			m_type = ePropertyBool;
			break;
		case IVariable::FLOAT:
			m_type = ePropertyFloat;
			break;
		case IVariable::VECTOR:
			m_type = ePropertyVector;
			break;
		case IVariable::STRING:
			m_type = ePropertyString;
			break;
		}
		for (i = 0; i < NumPropertyTypes; i++)
		{
			if (m_type == s_propertyTypeNames[i].type)
			{
				m_image = s_propertyTypeNames[i].image;
				break;
			}
		}
	}

	m_valueMultiplier = 1;
	m_rangeMin = 0;
	m_rangeMax = 100;
	m_step = 0.f;
	m_bHardMin = m_bHardMax = false;

	m_pVariable->GetLimits( m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax );

	if (dataType == IVariable::DT_PERCENT)
	{
		m_valueMultiplier = 100;
	}
	else if (dataType == IVariable::DT_ANGLE)
	{
		m_valueMultiplier = RAD2DEG(1);
		m_rangeMin = -360;
		m_rangeMax =  360;
	}
	else if (dataType == IVariable::DT_UIENUM)
	{
		m_pEnumDBItem = EditorRoot::Get().GetUIEnumsDatabase()->FindEnum(m_name);
	}

	int nPrec = std::max(3 - int(log(m_rangeMax - m_rangeMin) / log(10.f)), 0);
	m_step = std::max(m_step, powf(10.f, -nPrec));

	//////////////////////////////////////////////////////////////////////////
	VarToValue();

	m_bNoCategory = false;

	if (m_type == ePropertyVector && !m_propertyCtrl->IsExtenedUI())
	{
		m_bEditChildren = true;
		m_childs.clear();

		Float3 vec;
		m_pVariable->Get( vec );
		IVariable *pVX = new CVariable<float>;
		pVX->SetName( "x" );
		pVX->SetLimits(m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax);
		pVX->Set( vec.x );
		IVariable *pVY = new CVariable<float>;
		pVY->SetName( "y" );
		pVY->SetLimits(m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax);
		pVY->Set( vec.y );
		IVariable *pVZ = new CVariable<float>;
		pVZ->SetName( "z" );
		pVZ->SetLimits(m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax);
		pVZ->Set( vec.z );

		m_bIgnoreChildsUpdate = true;

		CPropertyItemPtr itemX = new CPropertyItem( m_propertyCtrl );
		itemX->SetVariable( pVX );
		AddChild( itemX );

		CPropertyItemPtr itemY = new CPropertyItem( m_propertyCtrl );
		itemY->SetVariable( pVY );
		AddChild( itemY );

		CPropertyItemPtr itemZ = new CPropertyItem( m_propertyCtrl );
		itemZ->SetVariable( pVZ );
		AddChild( itemZ );

		m_bNoCategory = true;
		m_bExpandable = true;

		m_bIgnoreChildsUpdate = false;
	}

	if (m_pVariable->NumChildVars() > 0)
	{
		if (m_type == ePropertyInvalid)
			m_type = ePropertyTable;
		m_bExpandable = true;
		if (m_pVariable->GetFlags() & IVariable::UI_SHOW_CHILDREN)
			m_bShowChildren = true;
		m_bIgnoreChildsUpdate = true;
		for (i = 0; i < m_pVariable->NumChildVars(); i++)
		{
			CPropertyItemPtr item = new CPropertyItem( m_propertyCtrl );
			item->SetVariable(m_pVariable->GetChildVar(i));
			AddChild( item );
		}
		m_bIgnoreChildsUpdate = false;
	}
	m_modified = false;
}

void CPropertyItem::ReleaseVariable()
{
	if (m_pVariable)
	{
		m_pVariable->RemoveOnSetCallback(functor(*this,&CPropertyItem::OnVariableChange));
	}
	m_pVariable = 0;
}

CPropertyItem* CPropertyItem::FindItemByVar( IVariable *pVar )
{
	if (m_pVariable == pVar)
		return this;
	for (int i = 0; i < m_childs.size(); i++)
	{
		CPropertyItem* pFound = m_childs[i]->FindItemByVar(pVar);
		if (pFound)
			return pFound;
	}
	return 0;
}

CString CPropertyItem::GetFullName() const
{
	if (m_parent)
	{
		return m_parent->GetFullName() + "::" + m_name;
	}
	else
		return m_name;
}

CPropertyItem* CPropertyItem::FindItemByFullName( const CString &name )
{
	if (GetFullName() == name)
	{
		return this;
	}

	for (int i = 0; i < m_childs.size(); i++)
	{
		CPropertyItem* pFound = m_childs[i]->FindItemByFullName(name);
		if (pFound)
			return pFound;
	}
	return 0;
}

void CPropertyItem::AddChild( CPropertyItem *item )
{
	assert(item);
	m_bExpandable = true;
	item->m_parent = this;
	m_childs.push_back(item);
}

void CPropertyItem::RemoveChild( CPropertyItem *item )
{
	for (int i = 0; i < m_childs.size(); i++)
	{
		if (item == m_childs[i])
		{
			m_childs.erase( m_childs.begin() + i );
			break;
		}
	}
}

void CPropertyItem::RemoveAllChildren()
{
	m_childs.clear();
}

void CPropertyItem::OnChildChanged( CPropertyItem *child )
{
	if (m_bIgnoreChildsUpdate)
		return;

	if (m_bEditChildren)
	{
		CString sValue;

		for (int i = 0; i < m_childs.size(); i++)
		{
			if (i>0)
				sValue += ", ";
			sValue += m_childs[i]->GetValue();
		}

		SetValue( sValue, false );
	}
	if (m_bEditChildren || m_bShowChildren)
	{
		SendToControl();
	}
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::SetSelected( bool selected )
{
	m_bSelected = selected;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::SetExpanded( bool expanded )
{
	if (IsDisabled())
	{
		m_bExpanded = false;
	}
	else
	{
		m_bExpanded = expanded;
	}
}

bool CPropertyItem::IsDisabled() const
{
	if (m_pVariable)
	{
		return m_pVariable->GetFlags() & IVariable::UI_DISABLED;
	}
	return false;
}

bool CPropertyItem::IsInvisible() const
{
	if (m_pVariable)
	{
		return m_pVariable->GetFlags() & IVariable::UI_INVISIBLE;
	}
	return false;
}


bool CPropertyItem::IsBold() const
{
	if (m_pVariable)
	{
		return m_pVariable->GetFlags() & IVariable::UI_BOLD;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::CreateInPlaceControl( CWnd* pWndParent,CRect& ctrlRect )
{
	if (IsDisabled())
	{
		return;
	}

	m_bMoveControls = true;
	CRect nullRc(0,0,0,0);
	switch (m_type)
	{
	case ePropertyFloat:
	case ePropertyInt:
	case ePropertyAngle:
		{
			if (m_pEnumDBItem)
			{
				m_cCombo = new CInPlaceComboBox();
				m_cCombo->SetReadOnly( true );
				m_cCombo->Create( NULL,"",WS_CHILD|WS_VISIBLE|LBS_SORT, nullRc, pWndParent,2 );
				m_cCombo->SetUpdateCallback( functor(*this,&CPropertyItem::OnComboSelection) );

				for (int i = 0; i < m_pEnumDBItem->strings.size(); i++)
				{
					m_cCombo->AddString( m_pEnumDBItem->strings[i] );
				}
			}
			else
			{
				m_cNumber = new CNumberCtrl;
				m_cNumber->SetUpdateCallback( functor(*this,&CPropertyItem::OnNumberCtrlUpdate) );
				m_cNumber->EnableUndo( m_name + " Modified" );

				if(m_type == ePropertyFloat)
				{
					m_cNumber->SetStep( m_step );
				}

				if (m_type == ePropertyInt)
				{
					m_cNumber->SetInteger(true);
				}
				m_cNumber->SetMultiplier( m_valueMultiplier );
				m_cNumber->SetRange( m_bHardMin ? m_rangeMin : -FLT_MAX, m_bHardMax ? m_rangeMax : FLT_MAX );

				m_cNumber->Create( pWndParent,nullRc,1,CNumberCtrl::NOBORDER|CNumberCtrl::LEFTALIGN );
				m_cNumber->SetLeftAlign( true );

				m_cFillSlider = new CFillSliderCtrl;
				m_cFillSlider->EnableUndo( m_name + " Modified" );
				m_cFillSlider->Create( WS_VISIBLE|WS_CHILD,nullRc,pWndParent,2);
				m_cFillSlider->SetUpdateCallback( functor(*this,&CPropertyItem::OnFillSliderCtrlUpdate) );
				m_cFillSlider->SetRangeFloat( m_rangeMin/m_valueMultiplier, m_rangeMax/m_valueMultiplier, m_step/m_valueMultiplier );
			}
		}
		break;

	case ePropertyTable:
		if (!m_bEditChildren)
		{
			break;
		}

	case ePropertyString:
		if (m_pEnumDBItem)
		{
			m_cCombo = new CInPlaceComboBox();
			m_cCombo->SetReadOnly( true );
			m_cCombo->Create( NULL,"",WS_CHILD|WS_VISIBLE|LBS_SORT, nullRc, pWndParent,2 );
			m_cCombo->SetUpdateCallback( functor(*this,&CPropertyItem::OnComboSelection) );

			for (int i = 0; i < m_pEnumDBItem->strings.size(); i++)
			{
				m_cCombo->AddString( m_pEnumDBItem->strings[i] );
			}
			break;
		}
		else
		{
			TDValues* pcValue = (m_pVariable) ? GetEnumValues(m_pVariable->GetName()) : NULL;

			if (pcValue)
			{
				m_cCombo = new CInPlaceComboBox();
				m_cCombo->SetReadOnly( true );
				m_cCombo->Create( NULL,"",WS_CHILD|WS_VISIBLE|LBS_SORT, nullRc, pWndParent,2 );
				m_cCombo->SetUpdateCallback( functor(*this,&CPropertyItem::OnComboSelection) );

				for (int i = 0; i < pcValue->size(); i++)
				{
					m_cCombo->AddString( (*pcValue)[i] );
				}
				break;
			}				
		}

	case ePropertyVector:
		{
			m_cEdit = new CInPlaceEdit( m_value,functor(*this,&CPropertyItem::OnEditChanged) );
			m_cEdit->Create( WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_LEFT, nullRc, pWndParent,2 );
		}
		break;

	case ePropertySelection:
		{
			m_cCombo = new CInPlaceComboBox();
			m_cCombo->SetReadOnly( true );
			DWORD nSorting = !m_enumList || m_enumList->NeedsSort() ? LBS_SORT : 0;
			m_cCombo->Create( NULL,"",WS_CHILD|WS_VISIBLE | nSorting, nullRc, pWndParent,2 );
			m_cCombo->SetUpdateCallback( functor(*this,&CPropertyItem::OnComboSelection) );

			if (m_enumList)
			{
				for (int i = 0; i < m_enumList->GetItemsCount(); i++)
				{
					m_cCombo->AddString( m_enumList->GetItemName(i) );
				}
			}
			else
			{
				m_cCombo->SetReadOnly( false );
			}
		}
		break;

	case ePropertyColor:
		{
			m_cEdit = new CInPlaceEdit( m_value,functor(*this,&CPropertyItem::OnEditChanged) );
			m_cEdit->Create( WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_LEFT, nullRc, pWndParent,2 );

			m_cButton = new CInPlaceButton( functor(*this,&CPropertyItem::OnColorBrowseButton) );
			m_cButton->Create( "",WS_CHILD|WS_VISIBLE,nullRc,pWndParent,4 );
			m_cButton->SetXButtonStyle( BS_XT_SEMIFLAT,FALSE );
		}
		break;
	}

	MoveInPlaceControl( ctrlRect );
	SendToControl();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::CreateControls( CWnd* pWndParent,CRect& textRect,CRect& ctrlRect )
{
	m_bMoveControls = false;
	CRect nullRc(0,0,0,0);

	if (IsExpandable() && !m_propertyCtrl->IsCategory(this))
	{
		m_cExpandButton  = new CInPlaceColorButton( functor(*this,&CPropertyItem::OnExpandButton) );
		const char *sText = "+";
		if (IsExpanded())
		{
			sText = "-";
		}
		m_cExpandButton->Create( sText,WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_OWNERDRAW,
			CRect(textRect.left-10,textRect.top+3,textRect.left,textRect.top+15),
			pWndParent,5 );
		m_cExpandButton->SetFont(m_propertyCtrl->GetFont());
	}

	if (m_type != ePropertyBool)
	{
		m_pStaticText = new CColorCtrl<CStatic>;
		m_pStaticText->SetTextColor( RGB(0,0,0) );
		m_pStaticText->Create( GetName(),WS_CHILD|WS_VISIBLE|SS_RIGHT|SS_ENDELLIPSIS,textRect,pWndParent );
		m_pStaticText->SetFont( CFont::FromHandle((HFONT)(EditorSettings::Get().Gui.hSystemFont)) );
	}

	switch (m_type)
	{
	case ePropertyBool:
		{
			m_cCheckBox = new CInPlaceCheckBox( functor(*this,&CPropertyItem::OnCheckBoxButton) );
			m_cCheckBox->Create( GetName(),WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|WS_TABSTOP,ctrlRect,pWndParent,0 );
			m_cCheckBox->SetFont(m_propertyCtrl->GetFont());
		}
		break;
	case ePropertyFloat:
	case ePropertyInt:
	case ePropertyAngle:
		{
			m_cNumber = new CNumberCtrl;
			m_cNumber->SetUpdateCallback( functor(*this,&CPropertyItem::OnNumberCtrlUpdate) );
			m_cNumber->EnableUndo( m_name + " Modified" );

			if(m_type == ePropertyFloat)
			{
				m_cNumber->SetStep( m_step );
			}

			if (m_type == ePropertyInt)
			{
				m_cNumber->SetInteger(true);
			}
			m_cNumber->SetMultiplier( m_valueMultiplier );
			m_cNumber->SetRange( m_bHardMin ? m_rangeMin : -FLT_MAX, m_bHardMax ? m_rangeMax : FLT_MAX );

			m_cNumber->Create( pWndParent,nullRc,1 );
			m_cNumber->SetLeftAlign( true );

			m_cFillSlider = new CFillSliderCtrl;
			m_cFillSlider->SetFilledLook(false);
			m_cFillSlider->EnableUndo( m_name + " Modified" );
			m_cFillSlider->Create( WS_VISIBLE|WS_CHILD,nullRc,pWndParent,2);
			m_cFillSlider->SetUpdateCallback( functor(*this,&CPropertyItem::OnFillSliderCtrlUpdate) );
			m_cFillSlider->SetRangeFloat( m_rangeMin/m_valueMultiplier, m_rangeMax/m_valueMultiplier, m_step/m_valueMultiplier );
		}
		break;

	case ePropertyVector:
		{
			CNumberCtrl** cNumber[3] = { &m_cNumber, &m_cNumber1, &m_cNumber2 };
			for (int a = 0; a < 3; a++)
			{
				CNumberCtrl* pNumber = *cNumber[a] = new CNumberCtrl;
				pNumber->Create( pWndParent,nullRc,1 );
				pNumber->SetUpdateCallback( functor(*this, &CPropertyItem::OnNumberCtrlUpdate) );
				pNumber->EnableUndo( m_name + " Modified" );
				pNumber->SetMultiplier( m_valueMultiplier );
				pNumber->SetRange( m_bHardMin ? m_rangeMin : -FLT_MAX, m_bHardMax ? m_rangeMax : FLT_MAX );
			}
		}
		break;

	case ePropertyTable:
		if (!m_bEditChildren)
		{
			break;
		}
	case ePropertyString:
		{
			m_cEdit = new CInPlaceEdit( m_value,functor(*this,&CPropertyItem::OnEditChanged) );
			m_cEdit->Create( WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_LEFT|WS_TABSTOP, nullRc, pWndParent,2 );
		}
		break;

	case ePropertySelection:
		{
			m_cCombo = new CInPlaceComboBox();
			m_cCombo->SetReadOnly( true );
			DWORD nSorting = !m_enumList || m_enumList->NeedsSort() ? LBS_SORT : 0;
			m_cCombo->Create( NULL, "", WS_CHILD|WS_VISIBLE|WS_TABSTOP | nSorting, nullRc, pWndParent,2 );
			m_cCombo->SetUpdateCallback( functor(*this,&CPropertyItem::OnComboSelection) );

			if (m_enumList)
			{
				for (int i = 0; i < m_enumList->GetItemsCount(); i++)
				{
					m_cCombo->AddString( m_enumList->GetItemName(i) );
				}
			}
			else
			{
				m_cCombo->SetReadOnly( false );
			}
		}
		break;
	case ePropertyColor:
		{
			m_cEdit = new CInPlaceEdit( m_value,functor(*this,&CPropertyItem::OnEditChanged) );
			m_cEdit->Create( WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_LEFT|WS_TABSTOP, nullRc, pWndParent,2 );

			m_cColorButton = new CInPlaceColorButton( functor(*this,&CPropertyItem::OnColorBrowseButton) );
			m_cColorButton->Create( "",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_OWNERDRAW,nullRc,pWndParent,4 );
		}
		break;
	}

	if (m_cEdit)
	{
		m_cEdit->ModifyStyleEx( 0,WS_EX_CLIENTEDGE );
	}
	if (m_cCombo)
	{
		m_cCombo->ModifyStyleEx( 0,WS_EX_CLIENTEDGE );
	}
	if (m_cButton && m_type != ePropertyColor)
	{
		m_cButton->SetXButtonStyle( BS_XT_WINXP_COMPAT,FALSE );
	}

	MoveInPlaceControl( ctrlRect );
	SendToControl();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::DestroyInPlaceControl( bool bRecursive )
{
	if (!m_propertyCtrl->IsExtenedUI())
	{
		ReceiveFromControl();
	}

	SAFE_DELETE(m_pStaticText);
	SAFE_DELETE(m_cNumber);
	SAFE_DELETE(m_cNumber1);
	SAFE_DELETE(m_cNumber2);
	SAFE_DELETE(m_cFillSlider);
	SAFE_DELETE(m_cEdit);
	SAFE_DELETE(m_cCombo);
	SAFE_DELETE(m_cButton);
	SAFE_DELETE(m_cButton2);
	SAFE_DELETE(m_cButton3);
	SAFE_DELETE(m_cExpandButton);
	SAFE_DELETE(m_cCheckBox);
	SAFE_DELETE(m_cColorButton);

	if (bRecursive)
	{
		int num = GetChildCount();
		for (int i = 0; i < num; i++)
		{
			GetChild(i)->DestroyInPlaceControl(bRecursive);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::MoveInPlaceControl( const CRect& rect )
{
	int nButtonWidth = BUTTON_WIDTH;
	if (m_propertyCtrl->IsExtenedUI())
	{
		nButtonWidth += 10;
	}

	CRect rc = rect;
	if (m_cButton)
	{
		if (m_type == ePropertyColor)
		{
			rc.right = rc.left + nButtonWidth + 2;
			m_cButton->MoveWindow( rc,FALSE );
			rc = rect;
			rc.left += nButtonWidth + 2 + 4;
		}
		else
		{
			rc.left = rc.right - nButtonWidth;
			m_cButton->MoveWindow( rc,FALSE );
			rc = rect;
			rc.right -= nButtonWidth;
		}
		m_cButton->SetFont(m_propertyCtrl->GetFont());
	}
	if (m_cColorButton)
	{
		rc.right = rc.left + nButtonWidth + 2;
		m_cColorButton->MoveWindow( rc,FALSE );
		rc = rect;
		rc.left += nButtonWidth + 2 + 4;
	}
	if (m_cButton2)
	{
		CRect brc(rc);
		brc.left = brc.right - nButtonWidth-2;
		m_cButton2->MoveWindow( brc,FALSE );
		rc.right -= nButtonWidth+4;
		m_cButton2->SetFont(m_propertyCtrl->GetFont());
	}
	if (m_cButton3)
	{
		CRect brc(rc);
		brc.left = brc.right - nButtonWidth-2;
		m_cButton3->MoveWindow( brc,FALSE );
		rc.right -= nButtonWidth+4;
		m_cButton2->SetFont(m_propertyCtrl->GetFont());
	}


	if (m_cNumber)
	{
		CRect rcn = rc;
		if (m_cNumber1 && m_cNumber2)
		{
			int x = NUMBER_CTRL_WIDTH+4;
			CRect rc0,rc1,rc2;
			rc0 = CRect( rc.left,rc.top,rc.left+NUMBER_CTRL_WIDTH,rc.bottom );
			rc1 = CRect( rc.left+x,rc.top,rc.left+x+NUMBER_CTRL_WIDTH,rc.bottom );
			rc2 = CRect( rc.left+x*2,rc.top,rc.left+x*2+NUMBER_CTRL_WIDTH,rc.bottom );
			m_cNumber->MoveWindow( rc0,FALSE );
			m_cNumber1->MoveWindow( rc1,FALSE );
			m_cNumber2->MoveWindow( rc2,FALSE );
		}
		else
		{
			if (rcn.Width() > NUMBER_CTRL_WIDTH)
			{
				rcn.right = rc.left + NUMBER_CTRL_WIDTH;
			}
			m_cNumber->MoveWindow( rcn,FALSE );
		}
		m_cNumber->SetFont(m_propertyCtrl->GetFont());
		if (m_cFillSlider)
		{
			CRect rcSlider = rc;
			rcSlider.left = rcn.right+1;
			m_cFillSlider->MoveWindow(rcSlider,FALSE);
		}
	}
	if (m_cEdit)
	{
		m_cEdit->MoveWindow( rc,FALSE );
		m_cEdit->SetFont(m_propertyCtrl->GetFont());
	}
	if (m_cCombo)
	{
		m_cCombo->MoveWindow( rc,FALSE );
		m_cCombo->SetFont(m_propertyCtrl->GetFont());
	}

}

void CPropertyItem::SetFocus()
{
	if (m_cNumber)
	{
		m_cNumber->SetFocus();
	}
	if (m_cEdit)
	{
		m_cEdit->SetFocus();
	}
	if (m_cCombo)
	{
		m_cCombo->SetFocus();
	}
	if (!m_cNumber && !m_cEdit && !m_cCombo)
		m_propertyCtrl->SetFocus();
}

void CPropertyItem::ReceiveFromControl()
{
	if (IsDisabled())
		return;
	if (m_cEdit)
	{
		CString str;
		m_cEdit->GetWindowText( str );
		//if (!CUndo::IsRecording())
		//{

		//	CUndo undo( GetName() + " Modified" );
		//	SetValue( str );
		//}
		//else
		SetValue( str );
	}
	if (m_cCombo)
	{
		//if (!CUndo::IsRecording())
		//{
		//	CUndo undo( GetName() + " Modified" );
		//	if (m_pEnumDBItem)
		//		SetValue( m_pEnumDBItem->NameToValue(m_cCombo->GetSelectedString()) );
		//	else
		//		SetValue( m_cCombo->GetSelectedString() );
		//}
		//else
		{
			if (m_pEnumDBItem)
				SetValue( m_pEnumDBItem->NameToValue(m_cCombo->GetSelectedString()) );
			else
				SetValue( m_cCombo->GetSelectedString() );
		}
	}
	if (m_cNumber)
	{
		if (m_cNumber1 && m_cNumber2)
		{
			CString val;
			val.Format( "%g,%g,%g",m_cNumber->GetValue(),m_cNumber1->GetValue(),m_cNumber2->GetValue() );
			SetValue( val );
		}
		else
			SetValue( m_cNumber->GetValueAsString() );
	}
}

void CPropertyItem::SendToControl()
{
	bool bInPlaceCtrl = m_propertyCtrl->IsExtenedUI();
	if (m_cButton)
	{
		if (m_type == ePropertyColor)
		{
			COLORREF clr = StringToColor(m_value);
			m_cButton->SetColorFace( clr );
			m_cButton->RedrawWindow();
			bInPlaceCtrl = true;
		}
		m_cButton->Invalidate();
	}
	if (m_cColorButton)
	{
		COLORREF clr = StringToColor(m_value);
		m_cColorButton->SetColor( clr );
		m_cColorButton->Invalidate();
		bInPlaceCtrl = true;
	}
	if (m_cCheckBox)
	{
		m_cCheckBox->SetCheck( GetBoolValue() ? BST_CHECKED : BST_UNCHECKED );
		m_cCheckBox->Invalidate();
	}
	if (m_cEdit)
	{
		m_cEdit->SetText(m_value);
		bInPlaceCtrl = true;
		m_cEdit->Invalidate();
	}
	if (m_cCombo)
	{
		if (m_type == ePropertyBool)
			m_cCombo->SetCurSel( GetBoolValue()?0:1 );
		else
		{
			if (m_pEnumDBItem)
				m_cCombo->SelectString(-1,m_pEnumDBItem->ValueToName(m_value));
			else
				m_cCombo->SelectString(-1,m_value);
		}
		bInPlaceCtrl = true;
		m_cCombo->Invalidate();
	}
	if (m_cNumber)
	{
		if (m_cNumber1 && m_cNumber2)
		{
			float x,y,z;
			sscanf( m_value,"%f,%f,%f",&x,&y,&z );
			m_cNumber->SetValue( x );
			m_cNumber1->SetValue( y );
			m_cNumber2->SetValue( z );
		}
		else
		{
			m_cNumber->SetValue( atof(m_value) );
		}
		bInPlaceCtrl = true;
		m_cNumber->Invalidate();
	}
	if (m_cFillSlider)
	{
		m_cFillSlider->SetValue( atof(m_value) );
		bInPlaceCtrl = true;
		m_cFillSlider->Invalidate();
	}
	if (!bInPlaceCtrl)
	{
		CRect rc;
		m_propertyCtrl->GetItemRect( this,rc );
		m_propertyCtrl->InvalidateRect( rc,TRUE );
		m_propertyCtrl->SetFocus();
	}

	CheckControlActiveColor();
}

//////////////////////////////////////////////////////////////////////////
bool CPropertyItem::HasDefaultValue( bool bChildren ) const
{
	if (!bChildren)
	{
		if (m_pVariable)
		{
			switch(m_pVariable->GetType()) 
			{
			case IVariable::INT:
				{
					int v = 0;
					m_pVariable->Get(v);
					if (v != 0)
						return false;
				};
				break;
			case IVariable::BOOL:
				{
					bool v = 0;
					m_pVariable->Get(v);
					if (v != 0)
						return false;
				};
				break;
			case IVariable::FLOAT:
				{
					float v = 0;
					m_pVariable->Get(v);
					if (v != 0)
						return false;
				};
				break;
			case IVariable::VECTOR:
				{
					Float3 v(0,0,0);
					m_pVariable->Get(v);

					if (!MathUtil::IsFloat3Zero(v))
					{
						return false;
					}
				};
				break;
			case IVariable::STRING:
				{
					CString v;
					m_pVariable->Get(v);
					if (!v.IsEmpty())
						return false;
				};
				break;
			default:
				if (!m_value.IsEmpty())
					return false;
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < m_childs.size(); i++)
		{
			if (!m_childs[i]->HasDefaultValue(false) || !m_childs[i]->HasDefaultValue(true))
			{
				return false;
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::CheckControlActiveColor()
{
	if (m_pStaticText)
	{
		COLORREF nTextColor = HasDefaultValue(false) ? ::GetSysColor(COLOR_BTNTEXT) : ::GetSysColor(COLOR_HIGHLIGHT);
		if (m_pStaticText->GetTextColor() != nTextColor)
		{
			m_pStaticText->SetTextColor( nTextColor );
		}
	}

	if (m_cExpandButton)
	{
		static COLORREF nDefColor = ::GetSysColor( COLOR_BTNFACE );
		static COLORREF nDefTextColor = ::GetSysColor( COLOR_BTNTEXT );
		static COLORREF nNondefColor = RGB( GetRValue(nDefColor)/2, GetGValue(nDefColor)/2, GetBValue(nDefColor) );
		static COLORREF nNondefTextColor = RGB( 255,255,0 );
		COLORREF nColor = HasDefaultValue(true) ? nDefColor : nNondefColor;
		if (m_cExpandButton->GetColor() != nColor)
		{
			m_cExpandButton->SetColor(nColor);
			if (nColor != nDefColor)
			{
				m_cExpandButton->SetTextColor(nNondefTextColor);
			}
			else
			{
				m_cExpandButton->SetTextColor(nDefTextColor);
			}
		}
	}
}

void CPropertyItem::OnComboSelection()
{
	ReceiveFromControl();
	SendToControl();
}

void CPropertyItem::DrawValue( CDC *dc,CRect rect )
{
	dc->SetBkMode( TRANSPARENT );

	CString val = GetDrawValue();

	if (m_type == ePropertyBool)
	{
		CPoint p1( rect.left,rect.top+1 );
		int sz = rect.bottom - rect.top - 1;
		CRect rc(p1.x,p1.y,p1.x+sz,p1.y+sz );
		dc->DrawFrameControl( rc,DFC_BUTTON,DFCS_BUTTON3STATE );
		if (GetBoolValue())
			m_propertyCtrl->m_icons.Draw( dc,12,p1,ILD_TRANSPARENT );
		rect.left += 15;
	}
	else if (m_type == ePropertyColor)
	{
		CRect rc( CPoint(rect.left,rect.top+1),CSize(BUTTON_WIDTH+2,rect.bottom-rect.top-2) );
		CPen pen( PS_SOLID,1,RGB(0,0,0));
		CBrush brush( StringToColor(val) );
		CPen *pOldPen = dc->SelectObject( &pen );
		CBrush *pOldBrush = dc->SelectObject( &brush );
		dc->Rectangle( rc );
		dc->SelectObject( pOldPen );
		dc->SelectObject( pOldBrush );
		rect.left = rect.left + BUTTON_WIDTH + 2 + 4;
	}

	CRect textRc;
	textRc = rect;
	::DrawTextEx( dc->GetSafeHdc(), val.GetBuffer(), val.GetLength(),
		textRc, DT_END_ELLIPSIS|DT_LEFT|DT_SINGLELINE|DT_VCENTER, NULL );
}

COLORREF CPropertyItem::StringToColor( const CString &value )
{
	float r,g,b;
	int res = 0;
	if (res != 3)
		res = sscanf( value,"%f,%f,%f",&r,&g,&b );
	if (res != 3)
		res = sscanf( value,"R:%f,G:%f,B:%f",&r,&g,&b );
	if (res != 3)
		res = sscanf( value,"R:%f G:%f B:%f",&r,&g,&b );
	if (res != 3)
		res = sscanf( value,"%f %f %f",&r,&g,&b );
	if (res != 3)
	{
		sscanf( value,"%f",&r );
		return r;
	}
	int ir = r;
	int ig = g;
	int ib = b;

	return RGB(ir,ig,ib);
}

bool CPropertyItem::GetBoolValue()
{
	if (stricmp(m_value,"true")==0 || atoi(m_value) != 0)
		return true;
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////
const char* CPropertyItem::GetValue() const
{
	return m_value;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::SetValue( const char* sValue,bool bRecordUndo,bool bForceModified )
{
	if (bRecordUndo && IsDisabled())
	{
		return;
	}

	TSmartPtr<CPropertyItem> holder = this;

	CString value = sValue;

	switch (m_type)
	{

	case ePropertyBool:
		if (stricmp(value,"true")==0 || atof(value) != 0)
			value = "1";
		else
			value = "0";
		break;

	case ePropertyVector:
		if (value.Find(',') < 0)
			value = value + ", " + value + ", " + value;
		break;
	}

	bool bModified = bForceModified || m_value.Compare(value) != 0;
	m_value = value;

	if (m_pVariable)
	{
		if (bModified)
		{
			//float currTime = GetIEditor()->GetSystem()->GetITimer()->GetCurrTime();
			//if (currTime > m_lastModified+1) // At least one second between undo stores.
			{
// 				if (bRecordUndo && !CUndo::IsRecording())
// 				{
// 					if (!CUndo::IsSuspended())
// 					{
// 						CUndo undo( GetName() + " Modified" );
// 						undo.Record( new CUndoVariableChange(m_pVariable,"PropertyChange") );
// 					}
// 				}
// 				else if (bRecordUndo)
// 				{
// 					GetIEditor()->RecordUndo( new CUndoVariableChange(m_pVariable,"PropertyChange") );
// 				}
			}

			ValueToVar();
			//m_lastModified = currTime;
		}
	}
	else
	{
		if (m_node)
		{
			m_node->setAttr( VALUE_ATTR,m_value );
		}

		SendToControl();

		if (bModified)
		{
			m_modified = true;
			if (m_bEditChildren)
			{
				int pos = 0;
				for (int i = 0; i < m_childs.size(); i++)
				{
					CString elem = m_value.Tokenize(", ", pos);
					m_childs[i]->m_value = elem;
					m_childs[i]->SendToControl();
				}
			}

			if (m_parent)
				m_parent->OnChildChanged( this );
			m_propertyCtrl->OnItemChange( this );
		}
	}
}

void CPropertyItem::VarToValue()
{
	assert( m_pVariable );

	if (m_type == ePropertyColor)
	{
		if (m_pVariable->GetType() == IVariable::VECTOR)
		{
			Float3 v(0,0,0);
			m_pVariable->Get(v);
			COLORREF col = ColorUtil::LinearToGamma( ColorValue( v.x, v.y, v.z ,1 ) );
			m_value.Format( "%d,%d,%d",GetRValue(col),GetGValue(col),GetBValue(col) );
		}
		else
		{
			int col(0);
			m_pVariable->Get(col);
			m_value.Format( "%d,%d,%d",GetRValue((UINT)col),GetGValue((UINT)col),GetBValue((UINT)col) );
		}
		return;
	}

	m_value = m_pVariable->GetDisplayValue();
}

CString CPropertyItem::GetDrawValue()
{
	CString value = m_value;

	if (m_pEnumDBItem)
		value = m_pEnumDBItem->ValueToName(value);

	if (m_valueMultiplier != 1.f)
	{
		float f = atof(m_value) * m_valueMultiplier;
		if (m_type == ePropertyInt)
			value.Format("%d", MathUtil::IntRound(f));
		else
			value.Format("%g", f);
		if (m_valueMultiplier == 100)
			value += "%";
	}
	else if (m_type == ePropertyBool)
	{
		return GetBoolValue() ? "True" : "False";	
	}

	if (m_bShowChildren && !m_bExpanded)
	{
		int iLast = 0;
		for (int i = 0; i < m_childs.size(); i++)
		{
			CString childVal = m_childs[i]->GetDrawValue();
			if ( childVal != "" && childVal != "0" && childVal != "0%" && childVal != "0,0,0" )
			{
				while (iLast <= i)
				{
					value += ", ";
					iLast++;
				}
				value += childVal;
			}
		}
	}

	return value;
}

void CPropertyItem::ValueToVar()
{
	assert( m_pVariable);

	TSmartPtr<CPropertyItem> holder = this;

	if (m_type == ePropertyColor)
	{
		COLORREF col = StringToColor(m_value);
		if (m_pVariable->GetType() == IVariable::VECTOR)
		{
			ColorValue colLin = ColorUtil::GammaToLinear( col );
			m_pVariable->Set( Float3( colLin.r, colLin.g, colLin.b ) );
		}
		else
			m_pVariable->Set((int)col);
	}
	else if (m_type != ePropertyInvalid)
	{
		m_pVariable->SetDisplayValue(m_value);
	}
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnVariableChange( IVariable* pVar )
{
	assert(pVar != 0 && pVar == m_pVariable);

	m_modified = true;

	VarToValue();

	if (m_type == ePropertySelection)  
	{
		m_enumList = m_pVariable->GetEnumList();
	}

	SendToControl();

	if (m_bEditChildren)
	{
		bool bPrevIgnore = m_bIgnoreChildsUpdate;
		m_bIgnoreChildsUpdate = true;

		int pos = 0;
		for (int i = 0; i < m_childs.size(); i++)
		{
			CString sElem = pos >= 0 ? m_value.Tokenize(", ", pos) : CString();
			m_childs[i]->SetValue(sElem,false);
		}
		m_bIgnoreChildsUpdate = bPrevIgnore;
	}  

	if (m_parent)
	{
		m_parent->OnChildChanged( this );
	}

	m_propertyCtrl->OnItemChange( this );
}
//////////////////////////////////////////////////////////////////////////
CPropertyItem::TDValues*	CPropertyItem::GetEnumValues(const CString& strPropertyName)
{
	TDValuesContainer::iterator	itIterator;
	TDValuesContainer& cEnumContainer = CUIEnumerations::GetUIEnumerationsInstance().GetStandardNameContainer();

	itIterator=cEnumContainer.find(strPropertyName);
	if (itIterator==cEnumContainer.end())
	{
		return NULL;
	}

	return &itIterator->second;
}

void CPropertyItem::OnMouseWheel( UINT nFlags,short zDelta,CPoint point )
{
	if (m_propertyCtrl->IsReadOnly())
		return;

	if (m_cCombo)
	{
		int sel = m_cCombo->GetCurSel();
		if (zDelta > 0)
		{
			sel++;
			if (m_cCombo->SetCurSel( sel ) == CB_ERR)
				m_cCombo->SetCurSel( 0 );
		}
		else
		{
			sel--;
			if (m_cCombo->SetCurSel( sel ) == CB_ERR)
				m_cCombo->SetCurSel( m_cCombo->GetCount()-1 );
		}
	}
	else if (m_cNumber)
	{
		if (zDelta > 0)
		{
			m_cNumber->SetValue( m_cNumber->GetValue() + m_cNumber->GetStep() );
		}
		else
		{
			m_cNumber->SetValue( m_cNumber->GetValue() - m_cNumber->GetStep() );
		}
		ReceiveFromControl();
	}
}

void CPropertyItem::OnLButtonDblClk( UINT nFlags,CPoint point )
{
	if (m_propertyCtrl->IsReadOnly())
	{
		return;
	}

	if (IsDisabled())
	{
		return;
	}

	if (m_type == ePropertyBool)
	{
		if (GetBoolValue())
		{
			SetValue( "0" );
		}
		else
		{
			SetValue( "1" );
		}
	}
	else
	{
		if (m_cButton)
		{
			m_cButton->Click();
		}
	}
}

void CPropertyItem::OnLButtonDown( UINT nFlags,CPoint point )
{
	if (m_propertyCtrl->IsReadOnly())
	{
		return;
	}

	if (m_type == ePropertyBool)
	{
		CRect rect;
		m_propertyCtrl->GetItemRect( this,rect );
		rect = m_propertyCtrl->GetItemValueRect( rect );

		CPoint p( rect.left-2,rect.top+1 );
		int sz = rect.bottom - rect.top;
		rect = CRect(p.x,p.y,p.x+sz,p.y+sz );

		if (rect.PtInRect(point))
		{
			if (GetBoolValue())
			{
				SetValue( "0" );
			}
			else
			{
				SetValue( "1" );
			}
		}
	}
}

void CPropertyItem::OnCheckBoxButton()
{
	if (m_cCheckBox)
		if (m_cCheckBox->GetCheck() == BST_CHECKED)
			SetValue( "1" );
		else
			SetValue( "0" );
}

void CPropertyItem::OnColorBrowseButton()
{
	if (IsDisabled())
	{
		return;
	}

	COLORREF orginalColor = StringToColor(m_value);
	COLORREF clr = orginalColor;
	CCustomColorDialog dlg( orginalColor,CC_FULLOPEN,m_propertyCtrl );
	dlg.SetColorChangeCallback( functor(*this,&CPropertyItem::OnColorChange) );
	if (dlg.DoModal() == IDOK)
	{
		clr = dlg.GetColor();
		if (clr != orginalColor)
		{
			int r,g,b;
			CString val;
			r = GetRValue(orginalColor);
			g = GetGValue(orginalColor);
			b = GetBValue(orginalColor);
			val.Format( "%d,%d,%d",r,g,b );
			SetValue( val,false );

			//CUndo undo( CString(GetName()) + " Modified" );
			r = GetRValue(clr);
			g = GetGValue(clr);
			b = GetBValue(clr);
			val.Format( "%d,%d,%d",r,g,b );
			SetValue( val );
			m_propertyCtrl->InvalidateCtrl();
		}
	}
	else
	{
		if (StringToColor(m_value) != orginalColor)
		{
			int r,g,b;
			r = GetRValue(clr);
			g = GetGValue(clr);
			b = GetBValue(clr);
			CString val;
			val.Format( "%d,%d,%d",r,g,b );
			SetValue( val,false );
			m_propertyCtrl->InvalidateCtrl();
		}
	}
	if (m_cButton)
		m_cButton->Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnColorChange( COLORREF clr )
{
	//GetIEditor()->SuspendUndo();
	int r,g,b;
	r = GetRValue(clr);
	g = GetGValue(clr);
	b = GetBValue(clr);
	CString val;
	val.Format( "%d,%d,%d",r,g,b );
	SetValue( val );
	//GetIEditor()->UpdateViews( eRedrawViewports );
	m_propertyCtrl->InvalidateCtrl();
	//GetIEditor()->ResumeUndo();

	if (m_cButton)
	{
		m_cButton->Invalidate();
	}
}

void CPropertyItem::ReloadValues()
{
	m_modified = false;
	if (m_node)
		ParseXmlNode(false);
	if (m_pVariable)
		SetVariable(m_pVariable);
	for (int i = 0; i < GetChildCount(); i++)
	{
		GetChild(i)->ReloadValues();
	}
	SendToControl();
}

CString CPropertyItem::GetTip() const
{
	if (!m_tip.IsEmpty())
		return m_tip;

	CString type;
	for (int i = 0; i < NumPropertyTypes; i++)
	{
		if (m_type == s_propertyTypeNames[i].type)
		{
			type = s_propertyTypeNames[i].name;
			break;
		}
	}

	CString tip;
	tip = CString("[")+type+"] ";
	tip += m_name + ": " + m_value;

	if (m_pVariable)
	{
		CString description = m_pVariable->GetDescription();
		if (!description.IsEmpty())
		{
			tip += CString("\r\n") + description;
		}
	}
	return tip;
}

void CPropertyItem::OnEditChanged()
{
	ReceiveFromControl();
}

void CPropertyItem::OnNumberCtrlUpdate( CNumberCtrl *ctrl )
{
	ReceiveFromControl();
}

void CPropertyItem::OnFillSliderCtrlUpdate( CSliderCtrlEx *ctrl )
{
	if (m_cFillSlider)
	{
		float fValue = m_cFillSlider->GetValue();

		if (m_step != 0.f)
		{
			float fRound = pow(10.f, floor(log(m_step) / log(10.f))) / m_valueMultiplier;
			fValue = MathUtil::IntRound(fValue / fRound) * fRound;
		}
		fValue =  MathUtil::ClampTpl( fValue, m_rangeMin, m_rangeMax );

		CString val;
		val.Format("%g", fValue);
		SetValue( val );
	}
}

void CPropertyItem::OnExpandButton()
{
	m_propertyCtrl->Expand( this,!IsExpanded(),true );
}

//////////////////////////////////////////////////////////////////////////
int	CPropertyItem::GetHeight()
{
	if (m_propertyCtrl->IsExtenedUI())
	{
		//switch (m_type) 
		//{
		//case ePropertyFloatCurve:
		//	return 52;
		//	break;
		//case ePropertyColorCurve:
		//	return 36;
		//}
		return 20;
	}
	return m_nHeight;
}
