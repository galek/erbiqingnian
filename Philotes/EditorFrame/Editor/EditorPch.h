
/********************************************************************
	日期:		2012年2月20日
	文件名: 	EditorPch.h
	创建者:		王延兴
	描述:		编辑器预编译头	
*********************************************************************/

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN			
#endif


#ifndef WINVER					
#define WINVER 0x0400			
#endif

#ifndef _WIN32_WINNT			
#define _WIN32_WINNT 0x0400     
#endif

#ifndef _WIN32_WINDOWS			
#define _WIN32_WINDOWS 0x0410	
#endif

#ifndef _WIN32_IE				
#define _WIN32_IE 0x0400		
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

#define _AFX_ALL_WARNINGS



#include <afxwin.h>    
#include <afxext.h>    
#include <afxdtctl.h>
#include <afxcmn.h>    

#include <XTToolkitPro.h>

#ifdef max
#undef max
#endif 

#ifdef min
#undef min
#endif 

#include <assert.h>
#include <float.h>
#include <math.h>
#include <vector>
#include <string>
#include <list>
#include <utility>
#include <algorithm>
#include <stack>
#include <deque>
#include <map>
#include <xutility>

#include <d3d9.h>
#include <d3dx9.h>

#include "Resource.h"
#include "EditorDefines.h"
#include "EditorPrerequisites.h"
#include "RefCountBase.h"
#include "EditorUtils.h"
#include "GuidUtil.h"
#include "StlUtil.h"
#include "PathUtil.h"
#include "MathUtil.h"
#include "ColorUtil.h"
#include "SmartPtr.h"

