
#include "UIEnumerations.h"
#include "XmlInterface.h"
#include "EditorRoot.h"

_NAMESPACE_BEGIN

CUIEnumerations& CUIEnumerations::GetUIEnumerationsInstance()
{
	static CUIEnumerations	oGeneralProxy;
	return oGeneralProxy;
}

CUIEnumerations::TDSelectedAnimations&	CUIEnumerations::GetSelectedAnimations()
{
	static TDSelectedAnimations	cSelectedAnimations;
	return cSelectedAnimations;
}

CUIEnumerations::TDValuesContainer&	CUIEnumerations::GetStandardNameContainer()
{
	static TDValuesContainer	cValuesContainer;
	static bool boInit(false);

	if (!boInit)
	{
		boInit=true;

		XmlNodeRef oRootNode;
		XmlNodeRef oEnumaration;
		XmlNodeRef oEnumerationItem;

		int nNumberOfEnumarations(0);
		int nCurrentEnumaration(0);

		int nNumberOfEnumerationItems(0);
		int nCurrentEnumarationItem(0);

		oRootNode = CEditorRoot::Get().LoadXmlFile("Editor\\PropertyEnumerations.xml");
		nNumberOfEnumarations = oRootNode ? oRootNode->GetChildCount():0;

		for (nCurrentEnumaration=0; nCurrentEnumaration<nNumberOfEnumarations; ++nCurrentEnumaration)
		{
			TDValues cValues;
			oEnumaration=oRootNode->GetChild(nCurrentEnumaration);

			nNumberOfEnumerationItems=oEnumaration->GetChildCount();
			for (nCurrentEnumarationItem=0;nCurrentEnumarationItem<nNumberOfEnumerationItems;++nCurrentEnumarationItem)
			{
				oEnumerationItem=oEnumaration->GetChild(nCurrentEnumarationItem);

				const char*	szKey(NULL);
				const char*	szValue(NULL);
				oEnumerationItem->GetAttributeByIndex(0,&szKey,&szValue);

				cValues.push_back(szValue);
			}

			const char*	szKey(NULL);
			const char*	szValue(NULL);
			oEnumaration->GetAttributeByIndex(0,&szKey,&szValue);

			cValuesContainer.insert(TDValuesContainer::value_type(szValue,cValues));
		}
	}

	return cValuesContainer;
}

_NAMESPACE_END