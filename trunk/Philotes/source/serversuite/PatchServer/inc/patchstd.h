

#pragma once
#ifndef _PATCH_STD_H_
#define _PATCH_STD_H_


//----------------------------------------------------------------------------
// INCLUDES FROM COMMON
//----------------------------------------------------------------------------
#include "stdCommonIncludes.h"
#include <hash_map>
#include <algorithm>

#include "appcommon.h"
#pragma warning(push,3)
#include <prime.h>
#pragma warning(pop)
#include <filepaths.h>
#include <list.h>

#include <id.h>
#include <fileio.h>
#include <definition_common.h>
#include <definition_priv.h>

#include <servers.h>
#include <ServerConfiguration.h>
#include <ServerRunnerApi.h>
#include <ServerSpecification.h>
#include "svrdebugtrace.h"
#include "patch_info.h"
#include "sha1.h"
#include "pakfile.h"
#include "simple_hash.h"
#include "file_info.h"
#include "file_manage.h"
#include "PatchServer.h"
#include "PatchServerMsg.h"
#include "runnerdebug.h"

#endif