#pragma once
//------------------------------------------------------------------------------
/**
    @class Guid
    
    Implements a GUID.
    
    (C) 2006 Radon Labs GmbH
*/
#include "core/config.h"
#if __WIN32__
#include "util/win32/win32guid.h"
namespace Philo
{
typedef Win32::Win32Guid Guid;
}
#elif __XBOX360__
#include "util/xbox360/xbox360guid.h"
namespace Philo
{
typedef Xbox360::Xbox360Guid Guid;
}
#elif __WII__
#include "util/wii/wiiguid.h"
namespace Philo
{
typedef Wii::WiiGuid Guid;
}
#elif __PS3__
#include "util/ps3/ps3guid.h"
namespace Philo
{
typedef PS3::PS3Guid Guid;
}
#elif __OSX__
#include "util/osx/osxguid.h"
namespace Philo
{
typedef OSX::OSXGuid Guid;
}
#else
#error "Guid not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
    