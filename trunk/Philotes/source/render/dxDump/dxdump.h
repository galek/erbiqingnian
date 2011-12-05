/*

dxDump 0.1
----------
Simple DirectX debugging tool


Dumps render states, Sampler states, Texture stage states to log file
Captures textures and render targets into file
Translates DirectX error codes to error names and explanations

Tested with DirectX 9.0b and 9.0c
Most of the code generated automatically, code generator included

Author: kovacsp@sch.bme.hu


You should know what to pass as parameters ;) ... except one: logger
It's a pointer to a function that can write a const char*. You decide where does it write the output. 
You can write to the screen, a log, or you may have a more complex debugging module that receives the output like we have.
This is why I added this king of flexibility, you can write your own logger funcion and make dxDump use it.
The funcion must accept a const char* and return nothing (void) like this:

void Msg(const char* str)

Morever, it must be either global or static, because you cannot pass the pointer of a simple member function this way.


Example:
This is how you include dxDump into the Meshes tutorial (from DX SDK tutorials)
add dxdump.h and dxdump.cpp to the project
include it in meshes.cpp and write a simple logger function:

#include "dxdump.h"
FILE *f;

void logger(const char* s) {
		fprintf(f, "%s\n", s);
} 

At the end of InitD3D, you init dxDump and create the file to log in:
	initDxDump(g_pd3dDevice);
	f = fopen ("log.txt", "wt");
	

Suppose you want to know everything after rendering the tiger, so in Render you put this after the DrawSubset:
dxDumpAll(g_pd3dDevice, logger);

You close the file in Cleanup.

After doing this and running the program (which will be slow of course), you will get a log.txt in the directory 
where you have the .exe, and get a dxDump directory on c: containing directories for all frames, and having the 
actual textures and rendertargets dumped in bmps.
*/

#include <D3D9.h>
#include <d3dx9tex.h>
#include <string>

#define dirname "dxDump"

/*
Initializes dxDump

Initailizes internal TSS and render target count 
Only needed before dumping texture stage states, textures and render targets
*/
void initDxDump(IDirect3DDevice9* p_pd3dDevice);

/*
Dump everything
*/
void dxDumpAll(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*));


void dxDumpRenderStates(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*));
void dxDumpTextureStageStates(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*));
void dxDumpSamplerStates(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*));
void dxDumpNextFrame(void);
void dxDumpTexture(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*), int stageNum);
void dxDumpAllTextures(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*));
void dxDumpRenderTarget(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*), int targetNum);
void dxDumpAllRenderTargets(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*));
std::string dxError2Errname(HRESULT p_hr);
std::string dxError2Explanation(HRESULT p_hr);
