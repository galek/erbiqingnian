//
// Modified $Date: 2008/05/15 $
//       by $Author: builder $
//

// DO NOT CHANGE ANY VERSION NUMBER IN THIS FILE !!! 
// Versions should only be generated  on the build servers.
//


// These are placeholder values. The build server will replace these with
// appropriate values for each branch.
#pragma once
#include <winver.h>
#include "buildversion.h"
#include "legal.h"
#define TITLE_VERSION                  CURRENT_BUILD_VERSION

#define PRODUCTION_BRANCH_VERSION 	0

#define RC_BRANCH_VERSION	0

#define MAIN_LINE_VERSION	4577

#define COMPANY_NAME_STRING            "Flagship Studios"

#ifdef _SERVER
#define ORIGINAL_FILENAME_STRING       "WatchdogClient.exe"
#define PRODUCT_NAME_STRING            "WatchdogClient"
#else //!SERVER

#ifdef TUGBOAT
#define ORIGINAL_FILENAME_STRING       "mythos.exe"
#define PRODUCT_NAME_STRING            "Mythos"
#else //HELLGATE
#define ORIGINAL_FILENAME_STRING       "hellgate.exe"
#define PRODUCT_NAME_STRING            "Hellgate: London"
#endif 

#endif//!SERVER

#define FILE_DESCRIPTION_STRING        PRODUCT_NAME_STRING

#define FILE_VERSION_STRING	"4577_ML"

#define PRODUCT_VERSION_STRING	FILE_VERSION_STRING

#define PRIVATE_BUILD_STRING	"Built by builder@BUILD119"

#ifndef RC_INVOKED
#define VERSION_AS_ULONGLONG  (( (ULONGLONG)(TITLE_VERSION <<16 |\
                                        PRODUCTION_BRANCH_VERSION) << 32) |\
                          ((ULONGLONG)(RC_BRANCH_VERSION <<16 |\
                                        MAIN_LINE_VERSION) << 32) )

#define VER_QUOTE(a) #a

#define VERSION_AS_STRING_(a,b,c,d) VER_QUOTE(a) "." VER_QUOTE(b) "." VER_QUOTE(c) "." VER_QUOTE(d)

#define VERSION_AS_STRING VER_VERSION_AS_STRING_(TITLE_VERSION,PRODUCTION_BRANCH_VERSION,RC_BRANCH_VERSION,MAIN_LINE_VERSION)
#endif
