
/********************************************************************
	日期:		2012年2月20日
	文件名: 	TemplateMessage.h
	创建者:		王延兴
	描述:		对MFC一些消息的封装	
*********************************************************************/

#pragma once

#define ARGS2(arg1, arg2)                     arg1, arg2
#define ARGS3(arg1, arg2, arg3)               arg1, arg2, arg3
#define ARGS4(arg1, arg2, arg3, arg4)         arg1, arg2, arg3, arg4
#define ARGS5(arg1, arg2, arg3, arg4, arg5)   arg1, arg2, arg3, arg4, arg5


#define DECLARE_TEMPLATE_MESSAGE_MAP() DECLARE_MESSAGE_MAP()

#if _MFC_VER >= 0x0800
//	MFC 8 (VS 2005)

#define BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(theTemplArgs, theClass, baseClass)\
	PTM_WARNING_DISABLE                                                     \
	template <theTemplArgs>                                                 \
	const AFX_MSGMAP* theClass::GetMessageMap() const                       \
			{ return GetThisMessageMap(); }	                                \
			template <theTemplArgs>                                                 \
			const AFX_MSGMAP* PASCAL theClass::GetThisMessageMap()                  \
		{																	\
		typedef theClass ThisClass;							                \
		typedef baseClass TheBaseClass;										\
		static const AFX_MSGMAP_ENTRY _messageEntries[] =					\
		{

#elif defined(WIN64) && _MFC_VER < 0x700

#ifdef _AFXDLL
#define BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(theTemplArgs, theClass, baseClass) \
	template <theTemplArgs> \
	const AFX_MSGMAP* PASCAL theClass::_GetBaseMessageMap() \
			{ return &baseClass::messageMap; } \
			template <theTemplArgs> \
			const AFX_MSGMAP* theClass::GetMessageMap() const \
			{ return &theClass::messageMap; } \
			template <theTemplArgs> \
			/*AFX_COMDAT AFX_DATADEF*/ const AFX_MSGMAP theClass::messageMap = \
		{ &theClass::_GetBaseMessageMap, &theClass::_messageEntries[0] }; \
		template <theTemplArgs> \
		/*AFX_COMDAT */const AFX_MSGMAP_ENTRY theClass::_messageEntries[] = \
		{ 
#else
#define BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(theTemplArgs, theClass, baseClass) \
	template <theTemplArgs> \
	const AFX_MSGMAP* theClass::GetMessageMap() const \
			{ return &theClass::messageMap; } \
			template <theTemplArgs> \
			/*AFX_COMDAT */AFX_DATADEF const AFX_MSGMAP theClass::messageMap = \
		{ &baseClass::messageMap, &theClass::_messageEntries[0] }; \
		template <theTemplArgs> \
		/*AFX_COMDAT */const AFX_MSGMAP_ENTRY theClass::_messageEntries[] = \
		{ 

#endif // _AFXDLL

#else

#ifdef _AFXDLL

#define BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(theTemplArgs, theClass, baseClass) \
	template <theTemplArgs> \
	const AFX_MSGMAP* PASCAL theClass::GetThisMessageMap() \
			{ return &theClass::messageMap; } \
			template <theTemplArgs> \
			const AFX_MSGMAP* theClass::GetMessageMap() const \
			{ return &theClass::messageMap; } \
			template <theTemplArgs> \
			/*AFX_COMDAT AFX_DATADEF*/ const AFX_MSGMAP theClass::messageMap = \
		{ &baseClass::GetThisMessageMap, &theClass::_messageEntries[0] }; \
		template <theTemplArgs> \
		/*AFX_COMDAT */const AFX_MSGMAP_ENTRY theClass::_messageEntries[] = \
		{ \

#else
#define BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(theTemplArgs, theClass, baseClass) \
	template <theTemplArgs> \
	const AFX_MSGMAP* theClass::GetMessageMap() const \
			{ return &theClass::messageMap; } \
			template <theTemplArgs> \
			/*AFX_COMDAT */AFX_DATADEF const AFX_MSGMAP theClass::messageMap = \
		{ &baseClass::messageMap, &theClass::_messageEntries[0] }; \
		template <theTemplArgs> \
		/*AFX_COMDAT */const AFX_MSGMAP_ENTRY theClass::_messageEntries[] = \
		{ \

#endif // _AFXDLL

#endif //WIN64

#define END_TEMPLATE_MESSAGE_MAP_CUSTOM() END_MESSAGE_MAP()

