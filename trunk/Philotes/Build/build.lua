
--------------------------------------------
-- 新建个临时的sln，因为premake不能做解决方案文件夹
-- 所以解决方案用固定的，工程用premake生成
solution "_Philotes"
  configurations { "Debug", "Release", "ReleaseSymbols"}
  defines { "__WIN32__","WIN32","_CRT_SECURE_NO_WARNINGS", "_MT", "_WINDOWS", "_USRDLL" }
  location "Solutions"
  configuration "Debug"
	  targetsuffix "_d"
      defines { "_DEBUG" }
	  flags { "Symbols" }
  configuration "Release"
      defines { "NDEBUG" }
	  flags { "OptimizeSpeed" }
	  buildoptions {"/Ob2","/Ot"}
  configuration "ReleaseSymbols"
	  targetsuffix "_s"
      defines { "NDEBUG" }
	  
--------------------------------- test programs -----------------------------------
	  
project "TestConsole"
  location "Projects"
  language "C++"
  files { "../test/consoleTest/**.c*","../test/consoleTest/**.h" }
  includedirs {"../common/"}
  kind 'ConsoleApp'
  objdir ("../obj")
  targetdir ("../bin")
  libdirs { "../lib/" }
  --debugdir "../bin"
  links { "dbghelp","dxguid","wsock32","rpcrt4","wininet","d3d9","d3dx9","dinput8","xinput" }
  configuration "Debug"
      linkoptions {"/PDB:../../bin/TestConsole_d.pdb"}
	  links { "common_d" }
  configuration "Release"
	  links { "common" }
  configuration "ReleaseSymbols"
      flags {"Symbols"}
      linkoptions {"/PDB:../../bin/TestConsole.pdb"}
	  links { "common" }
	  

--------------------------------- common module -----------------------------------
project "Common"
  location "Projects"
  language "C++"
  libdirs { "../lib/"}
  pchheader "CommonPch.h"
  pchsource "../common/commonPch.cpp"
  files { "../common/**.c*","../common/**.h" }
  kind 'StaticLib'
  objdir ("../obj")
  targetdir ("../lib")
  includedirs { "../common/","../3rdLibs/" }
  buildoptions {[[/FI "CommonPch.h"]]}

project "PhiloRender"
  location "Projects"
  language "C++"
  libdirs { "../lib/"}
  pchheader "RendererPch.h"
  includedirs {"../common/","../render"}
  pchsource "../Render/RendererPch.cpp"
  files { "../Render/**.c*","../Render/**.h" }
  kind 'StaticLib'
  objdir ("../obj")
  targetdir ("../lib")
  includedirs { "../common/","../3rdLibs/" }
  buildoptions {[[/FI "RendererPch.h"]]}