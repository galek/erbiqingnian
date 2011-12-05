#include "dxdump.h"
#include <direct.h>
#include <string>

using namespace std;

static int initialized = 0;
static int frameNum = 0;
static DWORD MaxTSSNum;
static DWORD MaxRTNum;

void initDxDump(IDirect3DDevice9* p_pd3dDevice) {
	D3DCAPS9 caps;
	p_pd3dDevice->GetDeviceCaps(&caps);
	MaxTSSNum = caps.MaxTextureBlendStages;
	MaxRTNum = caps.NumSimultaneousRTs;

	initialized = 1;
}

void dxDumpAll(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*)) {
	dxDumpRenderStates(p_pd3dDevice, logger);
	dxDumpTextureStageStates(p_pd3dDevice, logger);
	dxDumpSamplerStates(p_pd3dDevice, logger);
	dxDumpAllTextures(p_pd3dDevice, logger);
	dxDumpAllRenderTargets(p_pd3dDevice, logger);
	frameNum++;
}

void logDword(string &msg, const char *name, DWORD value) {
char buf[1024];

	sprintf(buf, "%s = 0x%08X", name, value);
	msg += buf;
	
}

void logInt(string &msg, const char *name, DWORD value) {
char buf[1024];

	sprintf(buf, "%s = %d", name, value);
	msg += buf;
	
}

void logFloat(string &msg, const char *name, DWORD value) {
char buf[1024];

	sprintf(buf, "%s = %f", name, *((float*)&value));
	msg += buf;
	
}
