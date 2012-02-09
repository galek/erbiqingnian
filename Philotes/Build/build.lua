
--------------------------------------------
-- 新建个临时的sln，因为premake不能做解决方案文件夹
-- 所以解决方案用固定的，工程用premake生成
solution "Philotes"
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
	  
project "TestRender"
  location "Projects"
  language "C++"
  files { "../test/RenderTest/**.c*","../test/RenderTest/**.h" }
  includedirs {"../common/","../render","../gears"}
  kind 'WindowedApp'
  objdir ("../obj")
  targetdir ("../bin")
  libdirs { "../lib/","../3rdLibs/cg/lib/" }
  --debugdir "../bin"
  links { "dbghelp","dxguid","rpcrt4","d3d9","d3dx9","dinput8","xinput","cg","cgD3D9" }
  configuration "Debug"
      linkoptions {"/PDB:../../bin/TestRender_d.pdb"}
	  links { "common_d","PhiloGears_d","PhiloRender_d" }
  configuration "Release"
	  links { "common","PhiloGears","PhiloRender" }
  configuration "ReleaseSymbols"
      flags {"Symbols"}
      linkoptions {"/PDB:../../bin/TestRender.pdb"}
	  links { "common","PhiloGears_s","PhiloRender_s" }
	  

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
  pchheader "renderPch.h"
  includedirs {"../3rdLibs/" ,"../common/","../render", "../gears"}
  pchsource "../Render/renderPch.cpp"
  files { "../Render/**.c*","../Render/**.h" }
  kind 'StaticLib'
  objdir ("../obj")
  targetdir ("../lib")
  buildoptions {[[/FI "renderPch.h"]]}
  
project "PhiloGears"
  location "Projects"
  language "C++"
  libdirs { "../lib/"}
  pchheader "gearsPch.h"
  includedirs {"../3rdLibs/cg/include","../3rdLibs/rapidxml/" ,"../common/","../render", "../gears", "../gears/addon"}
  pchsource "../Gears/gearsPch.cpp"
  files { "../Gears/**.c*","../Gears/**.h" }
  kind 'StaticLib'
  objdir ("../obj")
  targetdir ("../lib")
  buildoptions {[[/FI "gearsPch.h"]]}