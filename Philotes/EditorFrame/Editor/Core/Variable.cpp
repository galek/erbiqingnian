
#include "Variable.h"
#include "UIEnumsDatabase.h"
#include "EditorRoot.h"

_NAMESPACE_BEGIN

CVarBlock::~CVarBlock()
{

}

CVarBlock* CVarBlock::Clone( bool bRecursive ) const
{
	CVarBlock *vb = new CVarBlock;
	for (Variables::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
	{
		IVariable *var = *it;
		vb->AddVariable( var->Clone(bRecursive) );
	}
	return vb;
}

void CVarBlock::CopyValues( CVarBlock *fromVarBlock )
{
	int numSrc = fromVarBlock->GetVarsCount();
	int numTrg = GetVarsCount();
	for (int i = 0; i < numSrc && i < numTrg; i++)
	{
		GetVariable(i)->CopyValue( fromVarBlock->GetVariable(i) );
	}
}

void CVarBlock::CopyValuesByName( CVarBlock *fromVarBlock )
{
	XmlNodeRef node = CEditorRoot::Get().CreateXmlNode( "Temp" );
	fromVarBlock->Serialize( node,false );
	Serialize( node,true );
}

void CVarBlock::OnSetValues()
{
	for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
	{
		IVariable *var = *it;
		var->OnSetValue(true);
	}
}

void CVarBlock::AddVariable( IVariable *var )
{
	m_vars.push_back(var);
}

void CVarBlock::AddVariable( IVariable *pVar,const char *varName,unsigned char dataType )
{
	if (varName)
		pVar->SetName(varName);
	pVar->SetDataType(dataType);
	AddVariable(pVar);
}

void CVarBlock::AddVariable( CVariableBase &var,const char *varName,unsigned char dataType )
{
	if (varName)
		var.SetName(varName);
	var.SetDataType(dataType);
	AddVariable(&var);
}

void CVarBlock::RemoveVariable( IVariable *var )
{
	stl::FindAndErase(m_vars,var);
}

bool CVarBlock::IsContainVariable( IVariable *pVar,bool bRecursive ) const
{
	for (Variables::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
	{
		if ((*it) == pVar)
		{
			return true;
		}
	}
	if (bRecursive)
	{
		for (Variables::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
		{
			if (ContainChildVar(*it,pVar))
			{
				return true;
			}
		}
	}
	return false;
}

IVariable* CVarBlock::FindVariable( const char *name,bool bRecursive ) const
{
	for (Variables::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
	{
		IVariable *var = *it;
		if (strcmp(var->GetName(),name) == 0)
		{
			return var;
		}
	}
	if (bRecursive)
	{
		for (Variables::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
		{
			IVariable *var = *it;
			IVariable *found = FindChildVar(name,var);
			if (found)
				return found;
		}
	}
	return 0;
}

IVariable* CVarBlock::FindChildVar( const char *name,IVariable *pParentVar ) const
{
	if (strcmp(pParentVar->GetName(),name) == 0)
	{
		return pParentVar;
	}
	int numSubVar = pParentVar->NumChildVars();
	for (int i = 0; i < numSubVar; i++)
	{
		IVariable *var = FindChildVar( name,pParentVar->GetChildVar(i) );
		if (var)
		{
			return var;
		}
	}
	return 0;
}

bool CVarBlock::ContainChildVar( IVariable *pParentVar,IVariable *pVar ) const
{
	int numSubVar = pParentVar->NumChildVars();
	for (int i = 0; i < numSubVar; i++)
	{
		IVariable *pChild = pParentVar->GetChildVar(i);
		if (pChild == pVar)
		{
			return true;
		}
		if (ContainChildVar(pChild,pVar))
		{
			return true;
		}
	}
	return false;
}

void CVarBlock::Serialize( XmlNodeRef &vbNode,bool load )
{
	if (load)
	{
		CString name;
		for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
		{
			IVariable *var = *it;
			if (var->NumChildVars())
			{
				XmlNodeRef child = vbNode->FindChild(var->GetName());
				if (child)
					var->Serialize( child,load );
			}
			else
				var->Serialize( vbNode,load );
		}
	}
	else
	{
		for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
		{
			IVariable *var = *it;
			if (var->NumChildVars())
			{
				XmlNodeRef child = vbNode->NewChild(var->GetName());
				var->Serialize( child,load );
			}
			else
				var->Serialize( vbNode,load );
		}
	}
}

void CVarBlock::ReserveNumVariables( int numVars )
{
	m_vars.reserve( numVars );
}

void CVarBlock::WireVar( IVariable *src,IVariable *trg,bool bWire )
{
	if (bWire)
		src->Wire(trg);
	else
		src->Unwire(trg);
	int numSrcVars = src->NumChildVars();
	if (numSrcVars > 0)
	{
		int numTrgVars = trg->NumChildVars();
		for (int i = 0; i < numSrcVars && i < numTrgVars; i++)
		{
			WireVar(src->GetChildVar(i),trg->GetChildVar(i),bWire);
		}
	}
}

void CVarBlock::Wire( CVarBlock *toVarBlock )
{
	Variables::iterator tit = toVarBlock->m_vars.begin();
	Variables::iterator sit = m_vars.begin();
	for (; sit != m_vars.end() && tit != toVarBlock->m_vars.end(); ++sit,++tit)
	{
		IVariable *src = *sit;
		IVariable *trg = *tit;
		WireVar(src,trg,true);
	}
}

void CVarBlock::Unwire( CVarBlock *toVarBlock )
{
	Variables::iterator tit = toVarBlock->m_vars.begin();
	Variables::iterator sit = m_vars.begin();
	for (; sit != m_vars.end() && tit != toVarBlock->m_vars.end(); ++sit,++tit)
	{
		IVariable *src = *sit;
		IVariable *trg = *tit;
		WireVar(src,trg,false);
	}
}

void CVarBlock::AddOnSetCallback( IVariable::OnSetCallback func )
{
	for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
	{
		IVariable *var = *it;
		SetCallbackToVar( func,var,true );
	}
}

void CVarBlock::RemoveOnSetCallback( IVariable::OnSetCallback func )
{
	for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
	{
		IVariable *var = *it;
		SetCallbackToVar( func,var,false );
	}
}

void CVarBlock::SetCallbackToVar( IVariable::OnSetCallback func,IVariable *pVar,bool bAdd )
{
	if (bAdd)
		pVar->AddOnSetCallback(func);
	else
		pVar->RemoveOnSetCallback(func);
	int numVars = pVar->NumChildVars();
	if (numVars > 0)
	{
		for (int i = 0; i < numVars; i++)
		{
			SetCallbackToVar( func,pVar->GetChildVar(i),bAdd );
		}
	}
}

void CVarBlock::GatherUsedResources( CUsedResources &resources )
{
	for (int i = 0; i < GetVarsCount(); i++)
	{
		IVariable *pVar = GetVariable(i);
		GatherUsedResourcesInVar( pVar,resources );
	}
}
void CVarBlock::EnableUpdateCallbacks(bool boEnable)
{
	for (int i = 0; i < GetVarsCount(); i++)
	{
		IVariable *pVar = GetVariable(i);
		pVar->EnableUpdateCallbacks(boEnable);
	}
}
void CVarBlock::GatherUsedResourcesInVar( IVariable *pVar,CUsedResources &resources )
{
	int type = pVar->GetDataType();

	for (int i = 0; i < pVar->NumChildVars(); i++)
	{
		GatherUsedResourcesInVar( pVar->GetChildVar(i),resources );
	}
}

CVarObject::CVarObject()
{}

CVarObject::~CVarObject()
{}

void CVarObject::AddVariable( CVariableBase &var,const CString &varName,VarOnSetCallback cb,unsigned char dataType )
{
	if (!m_vars)
		m_vars = new CVarBlock;
	var.AddRef(); // Variables are local and must not be released by CVarBlock.
	var.SetName(varName);
	var.SetDataType(dataType);
	if (cb)
		var.AddOnSetCallback(cb);
	m_vars->AddVariable(&var);
}

void CVarObject::AddVariable( CVariableBase &var,const CString &varName,const CString &varHumanName,VarOnSetCallback cb,unsigned char dataType )
{
	if (!m_vars)
	{
		m_vars = new CVarBlock;
	}
	var.AddRef();
	var.SetName(varName);
	var.SetHumanName(varHumanName);
	var.SetDataType(dataType);
	if (cb)
		var.AddOnSetCallback(cb);
	m_vars->AddVariable(&var);
}

void CVarObject::AddVariable( CVariableArray &table,CVariableBase &var,const CString &varName,const CString &varHumanName,VarOnSetCallback cb,unsigned char dataType )
{
	if (!m_vars)
	{
		m_vars = new CVarBlock;
	}
	var.AddRef();
	var.SetName(varName);
	var.SetHumanName(varHumanName);
	var.SetDataType(dataType);
	if (cb)
		var.AddOnSetCallback(cb);
	table.AddChildVar(&var);
}

void CVarObject::RemoveVariable( IVariable *var )
{
	if(m_vars)
	{
		m_vars->RemoveVariable(var);
	}
}

void CVarObject::EnableUpdateCallbacks(bool boEnable)
{
	if(m_vars)
	{
		m_vars->EnableUpdateCallbacks(boEnable);
	}
}

void CVarObject::OnSetValues()
{
	if(m_vars)
	{
		m_vars->OnSetValues();
	}
}

void CVarObject::ReserveNumVariables( int numVars )
{
	if(m_vars)
	{
		m_vars->ReserveNumVariables( numVars );
	}
}

void CVarObject::CopyVariableValues( CVarObject *sourceObject )
{
	assert( GetRuntimeClass() == sourceObject->GetRuntimeClass() );
	if (m_vars  && sourceObject->m_vars )
	{
		m_vars->CopyValues(sourceObject->m_vars);
	}
}

void CVarObject::Serialize( XmlNodeRef node,bool load )
{
	if (m_vars)
	{
		m_vars->Serialize( node,load );
	}
}

CVarGlobalEnumList::CVarGlobalEnumList(const CString& enumName)
{
	m_pEnum = CEditorRoot::Get().GetUIEnumsDatabase()->FindEnum(enumName);
}

int CVarGlobalEnumList::GetItemsCount() 
{
	if (m_pEnum)
	{
		return m_pEnum->strings.size();
	}
	else
	{
		return 0;
	}
}

const char* CVarGlobalEnumList::GetItemName( int index )
{
	if (m_pEnum == 0)
	{
		return "";
	}
	assert( index >= 0 && index < (int)m_pEnum->strings.size() );
	return m_pEnum->strings[index];
}

CString CVarGlobalEnumList::NameToValue(const CString& name)
{
	if (m_pEnum)
	{
		return m_pEnum->NameToValue(name);
	}
	else
	{
		return name;
	}
}

CString CVarGlobalEnumList::ValueToName(const CString& value)
{
	if (m_pEnum)
	{
		return m_pEnum->ValueToName(value);

	}
	else
	{
		return value;
	}
}

_NAMESPACE_END