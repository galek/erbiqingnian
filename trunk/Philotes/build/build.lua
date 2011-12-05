
--------------------------------------------
-- 新建个临时的sln，因为premake不能做解决方案文件夹
-- 所以解决方案用固定的，工程用premake生成
solution "_Philotes"
  configurations { "Debug", "Release", "ReleaseSymbols"}
  defines { "__WIN32__","WIN32","_CRT_SECURE_NO_WARNINGS", "_MT", "_WINDOWS", "_USRDLL" }
  location "Solutions"
  linkoptions {"/INCREMENTAL:NO"}
  includedirs { "../source/","../source/common/","../3rdLibs/" }
  configuration "Debug"
	  targetsuffix "_d"
      defines { "_DEBUG" }
	  objdir ("../obj/debug")
	  flags { "Symbols" }
  configuration "Release"
      defines { "NDEBUG" }
	  objdir ("../obj/release")
	  flags { "OptimizeSpeed" }
	  buildoptions {"/Ob2","/Ot"}
  configuration "ReleaseSymbols"
      defines { "NDEBUG" }
	  objdir ("../obj/releaseSymbols")
	  flags { "Symbols" }
	  
project "Common"
  location "Projects"
  language "C++"
  pchheader "PhiloPch.h"
  pchsource "../source/common/common_pch.cpp"
  files { "../source/common/**.c*","../source/common/**.h" }
  kind 'StaticLib'
  flags {"StaticRuntime"}
  buildoptions {[[/FI "PhiloPch.h"]]}
  includedirs { "../source/game/" , "../3rdLibs/zlib/include" , "../3rdLibs/mysql/include" ,"../source/serversuite/include" }
  includedirs { "../source/serversuite/include", "../source/serversuite/PatchServer/inc", "../source/ServerSuite/ServerRunner" }
  configuration "Debug"
	  targetdir ("../libs/debug")
  configuration "Release"
	  targetdir ("../libs/release")
  configuration "ReleaseSymbols"
	  targetdir ("../libs/releaseSymbols")

project "Render"
  location "Projects"
  language "C++"
  pchheader "e_pch.h"
  pchsource "../source/render/source/e_pch.cpp"
  files { "../source/render/source/**.c*","../source/render/source/**.h" }
  excludes { "../source/render/source/Dx10/*.cpp","../source/render/source/Dx10/*.h" }
  kind 'StaticLib'
  flags {"StaticRuntime"}
  defines {"ENGINE_TARGET_DX9"}
  includedirs { "../source/render/source","../source/render/source/dx9","../source/render/source/dxc" }
  includedirs { "../source/game/","../3rdLibs/binkw32","../3rdLibs/mileswin/include","../3rdLibs/DXT_Library/inc" }
  includedirs { "../source/common","../source/game/","../3rdlibs/include"}
  
  configuration "Debug"
	  targetdir ("../libs/debug")
  configuration "Release"
	  targetdir ("../libs/release")
  configuration "ReleaseSymbols"
	  targetdir ("../libs/releaseSymbols")