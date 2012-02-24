
/********************************************************************
	日期:		2012年2月21日
	文件名: 	PropertyItem.h
	创建者:		王延兴
	描述:		属性元素	
*********************************************************************/

#pragma once

#include <afxstr.h>

#include "ColorCtrl.h"
#include "XmlInterface.h"
#include "Variable.h"

_NAMESPACE_BEGIN

enum PropertyType
{
	ePropertyInvalid	= 0,
	ePropertyTable		= 1,
	ePropertyBool		= 2,
	ePropertyInt,
	ePropertyFloat,
	ePropertyVector,
	ePropertyString,
	ePropertyColor,
	ePropertyAngle,
	ePropertySelection
};

// CPropertyCtrl中的属性元素，每一个属性元素都对应一个XmlNode的值

class CPropertyItem : public CRefCountBase
{
	public:
		typedef std::vector<CString>			TDValues;
		typedef std::map<CString,TDValues>		TDValuesContainer;

	public:
		CPropertyItem( CPropertyCtrl* pCtrl );

		virtual					~CPropertyItem();

		virtual void			SetXmlNode( XmlNodeRef &node );

		virtual void			SetVariable( IVariable *var );

		IVariable*				GetVariable() const { return m_pVariable; }

		virtual int				GetType() { return m_type; }
	  
		virtual CString			GetName() const { return m_name; };

		virtual void			SetName( const char *sName ) { m_name = sName; };

		virtual void			SetSelected( bool selected );

		bool 					IsSelected() const { return m_bSelected; };

		bool 					IsExpanded() const { return m_bExpanded; };

		bool 					IsExpandable() const { return m_bExpandable; };

		bool 					IsNotCategory() const { return m_bNoCategory; };

		bool 					IsBold() const;

		bool 					IsDisabled() const;

		bool 					IsInvisible() const;

		virtual int				GetHeight();
		
		virtual void 			DrawValue( CDC *dc,CRect rect );

		virtual void 			CreateInPlaceControl( CWnd* pWndParent,CRect& rect );

		virtual void 			DestroyInPlaceControl( bool bRecursive=false );

		virtual void 			CreateControls( CWnd* pWndParent,CRect& textRect,CRect& ctrlRect );

		virtual void 			MoveInPlaceControl( const CRect& rect );

		virtual void 			SetFocus();

		virtual void 			SetData( CWnd* pWndInPlaceControl ){};

		virtual void 			OnLButtonDown( UINT nFlags,CPoint point );

		virtual void 			OnRButtonDown( UINT nFlags,CPoint point ) {};

		virtual void 			OnLButtonDblClk( UINT nFlags,CPoint point );

		virtual void 			OnMouseWheel( UINT nFlags,short zDelta,CPoint point );

		virtual void			SetValue( const char* sValue,bool bRecordUndo=true,bool bForceModified=false );

		virtual const char*		GetValue() const;

		XmlNodeRef&				GetXmlNode() { return m_node; };

		CString					GetTip() const;

		int						GetImage() const { return m_image; };

		bool					IsModified() const { return m_modified; }

		bool					HasDefaultValue( bool bChildren = false ) const;

		virtual void 			SetExpanded( bool expanded );
		
		virtual void 			ReloadValues();

		int						GetChildCount() const { return m_childs.size(); };

		CPropertyItem*			GetChild( int index ) const { return m_childs[index]; };

		CPropertyItem*			GetParent() const { return m_parent; };

		void 					AddChild( CPropertyItem *item );

		void 					RemoveChild( CPropertyItem *item );

		void 					RemoveAllChildren();

		CPropertyItem*			FindItemByVar( IVariable *pVar );

		virtual CString 		GetFullName() const;

		CPropertyItem*			FindItemByFullName( const CString &name );

		void					ReceiveFromControl();

		CFillSliderCtrl* const	GetFillSlider()const{return m_cFillSlider;}

	protected:
		
		void 					SendToControl();
		void 					CheckControlActiveColor();

		void 					OnChildChanged( CPropertyItem *child );

		void 					OnEditChanged();
		void 					OnNumberCtrlUpdate( CNumberCtrl *ctrl );
		void 					OnFillSliderCtrlUpdate( CSliderCtrlEx *ctrl );
		void 					OnNumberCtrlBeginUpdate( CNumberCtrl *ctrl ) {};
		void 					OnNumberCtrlEndUpdate( CNumberCtrl *ctrl ) {};

		void 					OnComboSelection();

		void 					OnCheckBoxButton();
		void 					OnColorBrowseButton();
		void 					OnColorChange( COLORREF col );
		void 					OnFileBrowseButton();
		void 					OnTextureBrowseButton();
		void 					OnTextureApplyButton();
		void 					OnAnimationBrowseButton();
		void 					OnAnimationApplyButton();
		void 					OnShaderBrowseButton();
		void 					OnAIBehaviorBrowseButton();
		void 					OnAIAnchorBrowseButton();
		void 					OnAICharacterBrowseButton();
		void 					OnAIPFPropertiesListBrowseButton();
		void 					OnEquipBrowseButton();
		void 					OnEAXPresetBrowseButton();
		void 					OnMaterialBrowseButton();
		void 					OnMaterialLookupButton();
		void 					OnMaterialPickSelectedButton();
		void 					OnGameTokenBrowseButton();
		void 					OnSequenceBrowseButton();
		void 					OnMissionObjButton();
		void 					OnUserBrowseButton();
		void 					OnLocalStringBrowseButton();
		void 					OnExpandButton();

		void 					OnSOClassBrowseButton();
		void 					OnSOClassesBrowseButton();
		void 					OnSOStateBrowseButton();
		void 					OnSOStatesBrowseButton();
		void 					OnSOStatePatternBrowseButton();
		void 					OnSOActionBrowseButton();
		void 					OnSOHelperBrowseButton();
		void 					OnSONavHelperBrowseButton();
		void 					OnSOAnimHelperBrowseButton();
		void 					OnSOEventBrowseButton();
		void 					OnSOTemplateBrowseButton();

		void 					ParseXmlNode( bool bRecursive=true );

		COLORREF				StringToColor( const CString &value );
		bool					GetBoolValue();

		void					VarToValue();
		CString					GetDrawValue();

		void 					ValueToVar();

		void 					ReleaseVariable();
		void 					OnVariableChange( IVariable* var );

		TDValues*				GetEnumValues(const CString& strPropertyName);

		void					OnTextureDoubleClickCallback();

	private:
		
		CString					m_name;

		PropertyType			m_type;

		CString					m_value;

	
		unsigned int 			m_bSelected				: 1;
		unsigned int 			m_bExpanded				: 1;
		unsigned int 			m_bExpandable			: 1;
		unsigned int 			m_bEditChildren 		: 1;
		unsigned int 			m_bShowChildren 		: 1;
		unsigned int 			m_bNoCategory			: 1;
		unsigned int 			m_bIgnoreChildsUpdate	: 1;
		unsigned int 			m_bMoveControls			: 1;
		unsigned int 			m_modified				: 1;

		// number控件用
		float 					m_rangeMin;
		float 					m_rangeMax;
		float 					m_step;
		bool  					m_bHardMin, m_bHardMax;	// 实际值限制
		int   					m_nHeight;

		XmlNodeRef				m_node;

		// 指向变量
		TSmartPtr<IVariable>	m_pVariable;

		// 控件
		CColorCtrl<CStatic>*	m_pStaticText;
		CNumberCtrl* 			m_cNumber;
		CNumberCtrl* 			m_cNumber1;
		CNumberCtrl* 			m_cNumber2;
		CFillSliderCtrl*		m_cFillSlider;
		CInPlaceEdit*			m_cEdit;
		CInPlaceComboBox*		m_cCombo;
		CInPlaceButton*			m_cButton;
		CInPlaceButton*			m_cButton2;
		CInPlaceButton*			m_cButton3;
		CInPlaceColorButton*	m_cExpandButton;
		CInPlaceCheckBox*		m_cCheckBox;
		CInPlaceColorButton*	m_cColorButton;

		CPropertyCtrl*			m_propertyCtrl;

		CPropertyItem*			m_parent;

		IVarEnumListPtr			m_enumList;

		CUIEnumsDatabase_SEnum* m_pEnumDBItem;

		CString					m_tip;
		int						m_image;

		//float					m_lastModified;

		float					m_valueMultiplier;

		// 子节点
		typedef std::vector<TSmartPtr<CPropertyItem> > Childs;
		Childs m_childs;

		//////////////////////////////////////////////////////////////////////////
		friend class CPropertyCtrlEx;
		int						m_nCategoryPageId;
		
		void 					AddChildrenForPFProperties();
		void 					PopulateAITerritoriesList();
		void 					PopulateAIWavesList();
};

typedef TSmartPtr<CPropertyItem> CPropertyItemPtr;

_NAMESPACE_END