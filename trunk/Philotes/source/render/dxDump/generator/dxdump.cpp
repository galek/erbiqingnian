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
void dxDumpRenderStates(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*)) {
string msg;
logger("================================ dxDumpRenderStates start ========================");
logger("RenderStates");

	msg = "\t";
	DWORD _D3DRS_ZENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_ZENABLE, &_D3DRS_ZENABLE);
	msg += "D3DRS_ZENABLE = ";
	switch(_D3DRS_ZENABLE) {
		case D3DZB_FALSE:
			msg += "D3DZB_FALSE";
			break;
		case D3DZB_TRUE:
			msg += "D3DZB_TRUE";
			break;
		case D3DZB_USEW:
			msg += "D3DZB_USEW";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_FILLMODE;
	p_pd3dDevice->GetRenderState(D3DRS_FILLMODE, &_D3DRS_FILLMODE);
	msg += "D3DRS_FILLMODE = ";
	switch(_D3DRS_FILLMODE) {
		case D3DFILL_POINT:
			msg += "D3DFILL_POINT";
			break;
		case D3DFILL_WIREFRAME:
			msg += "D3DFILL_WIREFRAME";
			break;
		case D3DFILL_SOLID:
			msg += "D3DFILL_SOLID";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_SHADEMODE;
	p_pd3dDevice->GetRenderState(D3DRS_SHADEMODE, &_D3DRS_SHADEMODE);
	msg += "D3DRS_SHADEMODE = ";
	switch(_D3DRS_SHADEMODE) {
		case D3DSHADE_FLAT:
			msg += "D3DSHADE_FLAT";
			break;
		case D3DSHADE_GOURAUD:
			msg += "D3DSHADE_GOURAUD";
			break;
		case D3DSHADE_PHONG:
			msg += "D3DSHADE_PHONG";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_ZWRITEENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_ZWRITEENABLE, &_D3DRS_ZWRITEENABLE);
	msg += "D3DRS_ZWRITEENABLE = ";
	switch(_D3DRS_ZWRITEENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_ALPHATESTENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_ALPHATESTENABLE, &_D3DRS_ALPHATESTENABLE);
	msg += "D3DRS_ALPHATESTENABLE = ";
	switch(_D3DRS_ALPHATESTENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_LASTPIXEL;
	p_pd3dDevice->GetRenderState(D3DRS_LASTPIXEL, &_D3DRS_LASTPIXEL);
	msg += "D3DRS_LASTPIXEL = ";
	switch(_D3DRS_LASTPIXEL) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_SRCBLEND;
	p_pd3dDevice->GetRenderState(D3DRS_SRCBLEND, &_D3DRS_SRCBLEND);
	msg += "D3DRS_SRCBLEND = ";
	switch(_D3DRS_SRCBLEND) {
		case D3DBLEND_ZERO:
			msg += "D3DBLEND_ZERO";
			break;
		case D3DBLEND_ONE:
			msg += "D3DBLEND_ONE";
			break;
		case D3DBLEND_SRCCOLOR:
			msg += "D3DBLEND_SRCCOLOR";
			break;
		case D3DBLEND_INVSRCCOLOR:
			msg += "D3DBLEND_INVSRCCOLOR";
			break;
		case D3DBLEND_SRCALPHA:
			msg += "D3DBLEND_SRCALPHA";
			break;
		case D3DBLEND_INVSRCALPHA:
			msg += "D3DBLEND_INVSRCALPHA";
			break;
		case D3DBLEND_DESTALPHA:
			msg += "D3DBLEND_DESTALPHA";
			break;
		case D3DBLEND_INVDESTALPHA:
			msg += "D3DBLEND_INVDESTALPHA";
			break;
		case D3DBLEND_DESTCOLOR:
			msg += "D3DBLEND_DESTCOLOR";
			break;
		case D3DBLEND_INVDESTCOLOR:
			msg += "D3DBLEND_INVDESTCOLOR";
			break;
		case D3DBLEND_SRCALPHASAT:
			msg += "D3DBLEND_SRCALPHASAT";
			break;
		case D3DBLEND_BOTHSRCALPHA:
			msg += "D3DBLEND_BOTHSRCALPHA";
			break;
		case D3DBLEND_BOTHINVSRCALPHA:
			msg += "D3DBLEND_BOTHINVSRCALPHA";
			break;
		case D3DBLEND_BLENDFACTOR:
			msg += "D3DBLEND_BLENDFACTOR";
			break;
		case D3DBLEND_INVBLENDFACTOR:
			msg += "D3DBLEND_INVBLENDFACTOR";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_DESTBLEND;
	p_pd3dDevice->GetRenderState(D3DRS_DESTBLEND, &_D3DRS_DESTBLEND);
	msg += "D3DRS_DESTBLEND = ";
	switch(_D3DRS_DESTBLEND) {
		case D3DBLEND_ZERO:
			msg += "D3DBLEND_ZERO";
			break;
		case D3DBLEND_ONE:
			msg += "D3DBLEND_ONE";
			break;
		case D3DBLEND_SRCCOLOR:
			msg += "D3DBLEND_SRCCOLOR";
			break;
		case D3DBLEND_INVSRCCOLOR:
			msg += "D3DBLEND_INVSRCCOLOR";
			break;
		case D3DBLEND_SRCALPHA:
			msg += "D3DBLEND_SRCALPHA";
			break;
		case D3DBLEND_INVSRCALPHA:
			msg += "D3DBLEND_INVSRCALPHA";
			break;
		case D3DBLEND_DESTALPHA:
			msg += "D3DBLEND_DESTALPHA";
			break;
		case D3DBLEND_INVDESTALPHA:
			msg += "D3DBLEND_INVDESTALPHA";
			break;
		case D3DBLEND_DESTCOLOR:
			msg += "D3DBLEND_DESTCOLOR";
			break;
		case D3DBLEND_INVDESTCOLOR:
			msg += "D3DBLEND_INVDESTCOLOR";
			break;
		case D3DBLEND_SRCALPHASAT:
			msg += "D3DBLEND_SRCALPHASAT";
			break;
		case D3DBLEND_BOTHSRCALPHA:
			msg += "D3DBLEND_BOTHSRCALPHA";
			break;
		case D3DBLEND_BOTHINVSRCALPHA:
			msg += "D3DBLEND_BOTHINVSRCALPHA";
			break;
		case D3DBLEND_BLENDFACTOR:
			msg += "D3DBLEND_BLENDFACTOR";
			break;
		case D3DBLEND_INVBLENDFACTOR:
			msg += "D3DBLEND_INVBLENDFACTOR";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_CULLMODE;
	p_pd3dDevice->GetRenderState(D3DRS_CULLMODE, &_D3DRS_CULLMODE);
	msg += "D3DRS_CULLMODE = ";
	switch(_D3DRS_CULLMODE) {
		case D3DCULL_NONE:
			msg += "D3DCULL_NONE";
			break;
		case D3DCULL_CW:
			msg += "D3DCULL_CW";
			break;
		case D3DCULL_CCW:
			msg += "D3DCULL_CCW";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_ZFUNC;
	p_pd3dDevice->GetRenderState(D3DRS_ZFUNC, &_D3DRS_ZFUNC);
	msg += "D3DRS_ZFUNC = ";
	switch(_D3DRS_ZFUNC) {
		case D3DCMP_NEVER:
			msg += "D3DCMP_NEVER";
			break;
		case D3DCMP_LESS:
			msg += "D3DCMP_LESS";
			break;
		case D3DCMP_EQUAL:
			msg += "D3DCMP_EQUAL";
			break;
		case D3DCMP_LESSEQUAL:
			msg += "D3DCMP_LESSEQUAL";
			break;
		case D3DCMP_GREATER:
			msg += "D3DCMP_GREATER";
			break;
		case D3DCMP_NOTEQUAL:
			msg += "D3DCMP_NOTEQUAL";
			break;
		case D3DCMP_GREATEREQUAL:
			msg += "D3DCMP_GREATEREQUAL";
			break;
		case D3DCMP_ALWAYS:
			msg += "D3DCMP_ALWAYS";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_ALPHAREF;
	p_pd3dDevice->GetRenderState(D3DRS_ALPHAREF, &_D3DRS_ALPHAREF);
	logDword(msg, "D3DRS_ALPHAREF", _D3DRS_ALPHAREF);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_ALPHAFUNC;
	p_pd3dDevice->GetRenderState(D3DRS_ALPHAFUNC, &_D3DRS_ALPHAFUNC);
	msg += "D3DRS_ALPHAFUNC = ";
	switch(_D3DRS_ALPHAFUNC) {
		case D3DCMP_NEVER:
			msg += "D3DCMP_NEVER";
			break;
		case D3DCMP_LESS:
			msg += "D3DCMP_LESS";
			break;
		case D3DCMP_EQUAL:
			msg += "D3DCMP_EQUAL";
			break;
		case D3DCMP_LESSEQUAL:
			msg += "D3DCMP_LESSEQUAL";
			break;
		case D3DCMP_GREATER:
			msg += "D3DCMP_GREATER";
			break;
		case D3DCMP_NOTEQUAL:
			msg += "D3DCMP_NOTEQUAL";
			break;
		case D3DCMP_GREATEREQUAL:
			msg += "D3DCMP_GREATEREQUAL";
			break;
		case D3DCMP_ALWAYS:
			msg += "D3DCMP_ALWAYS";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_DITHERENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_DITHERENABLE, &_D3DRS_DITHERENABLE);
	msg += "D3DRS_DITHERENABLE = ";
	switch(_D3DRS_DITHERENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_ALPHABLENDENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &_D3DRS_ALPHABLENDENABLE);
	msg += "D3DRS_ALPHABLENDENABLE = ";
	switch(_D3DRS_ALPHABLENDENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_FOGENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_FOGENABLE, &_D3DRS_FOGENABLE);
	msg += "D3DRS_FOGENABLE = ";
	switch(_D3DRS_FOGENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_SPECULARENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_SPECULARENABLE, &_D3DRS_SPECULARENABLE);
	msg += "D3DRS_SPECULARENABLE = ";
	switch(_D3DRS_SPECULARENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_FOGCOLOR;
	p_pd3dDevice->GetRenderState(D3DRS_FOGCOLOR, &_D3DRS_FOGCOLOR);
	logDword(msg, "D3DRS_FOGCOLOR", _D3DRS_FOGCOLOR);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_FOGTABLEMODE;
	p_pd3dDevice->GetRenderState(D3DRS_FOGTABLEMODE, &_D3DRS_FOGTABLEMODE);
	msg += "D3DRS_FOGTABLEMODE = ";
	switch(_D3DRS_FOGTABLEMODE) {
		case D3DFOG_NONE:
			msg += "D3DFOG_NONE";
			break;
		case D3DFOG_EXP:
			msg += "D3DFOG_EXP";
			break;
		case D3DFOG_EXP2:
			msg += "D3DFOG_EXP2";
			break;
		case D3DFOG_LINEAR:
			msg += "D3DFOG_LINEAR";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_FOGSTART;
	p_pd3dDevice->GetRenderState(D3DRS_FOGSTART, &_D3DRS_FOGSTART);
	logFloat(msg, "D3DRS_FOGSTART", _D3DRS_FOGSTART);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_FOGEND;
	p_pd3dDevice->GetRenderState(D3DRS_FOGEND, &_D3DRS_FOGEND);
	logFloat(msg, "D3DRS_FOGEND", _D3DRS_FOGEND);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_FOGDENSITY;
	p_pd3dDevice->GetRenderState(D3DRS_FOGDENSITY, &_D3DRS_FOGDENSITY);
	logFloat(msg, "D3DRS_FOGDENSITY", _D3DRS_FOGDENSITY);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_RANGEFOGENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_RANGEFOGENABLE, &_D3DRS_RANGEFOGENABLE);
	msg += "D3DRS_RANGEFOGENABLE = ";
	switch(_D3DRS_RANGEFOGENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_STENCILENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_STENCILENABLE, &_D3DRS_STENCILENABLE);
	msg += "D3DRS_STENCILENABLE = ";
	switch(_D3DRS_STENCILENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_STENCILFAIL;
	p_pd3dDevice->GetRenderState(D3DRS_STENCILFAIL, &_D3DRS_STENCILFAIL);
	msg += "D3DRS_STENCILFAIL = ";
	switch(_D3DRS_STENCILFAIL) {
		case D3DSTENCILOP_KEEP:
			msg += "D3DSTENCILOP_KEEP";
			break;
		case D3DSTENCILOP_ZERO:
			msg += "D3DSTENCILOP_ZERO";
			break;
		case D3DSTENCILOP_REPLACE:
			msg += "D3DSTENCILOP_REPLACE";
			break;
		case D3DSTENCILOP_INCRSAT:
			msg += "D3DSTENCILOP_INCRSAT";
			break;
		case D3DSTENCILOP_DECRSAT:
			msg += "D3DSTENCILOP_DECRSAT";
			break;
		case D3DSTENCILOP_INVERT:
			msg += "D3DSTENCILOP_INVERT";
			break;
		case D3DSTENCILOP_INCR:
			msg += "D3DSTENCILOP_INCR";
			break;
		case D3DSTENCILOP_DECR:
			msg += "D3DSTENCILOP_DECR";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_STENCILZFAIL;
	p_pd3dDevice->GetRenderState(D3DRS_STENCILZFAIL, &_D3DRS_STENCILZFAIL);
	msg += "D3DRS_STENCILZFAIL = ";
	switch(_D3DRS_STENCILZFAIL) {
		case D3DSTENCILOP_KEEP:
			msg += "D3DSTENCILOP_KEEP";
			break;
		case D3DSTENCILOP_ZERO:
			msg += "D3DSTENCILOP_ZERO";
			break;
		case D3DSTENCILOP_REPLACE:
			msg += "D3DSTENCILOP_REPLACE";
			break;
		case D3DSTENCILOP_INCRSAT:
			msg += "D3DSTENCILOP_INCRSAT";
			break;
		case D3DSTENCILOP_DECRSAT:
			msg += "D3DSTENCILOP_DECRSAT";
			break;
		case D3DSTENCILOP_INVERT:
			msg += "D3DSTENCILOP_INVERT";
			break;
		case D3DSTENCILOP_INCR:
			msg += "D3DSTENCILOP_INCR";
			break;
		case D3DSTENCILOP_DECR:
			msg += "D3DSTENCILOP_DECR";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_STENCILPASS;
	p_pd3dDevice->GetRenderState(D3DRS_STENCILPASS, &_D3DRS_STENCILPASS);
	msg += "D3DRS_STENCILPASS = ";
	switch(_D3DRS_STENCILPASS) {
		case D3DSTENCILOP_KEEP:
			msg += "D3DSTENCILOP_KEEP";
			break;
		case D3DSTENCILOP_ZERO:
			msg += "D3DSTENCILOP_ZERO";
			break;
		case D3DSTENCILOP_REPLACE:
			msg += "D3DSTENCILOP_REPLACE";
			break;
		case D3DSTENCILOP_INCRSAT:
			msg += "D3DSTENCILOP_INCRSAT";
			break;
		case D3DSTENCILOP_DECRSAT:
			msg += "D3DSTENCILOP_DECRSAT";
			break;
		case D3DSTENCILOP_INVERT:
			msg += "D3DSTENCILOP_INVERT";
			break;
		case D3DSTENCILOP_INCR:
			msg += "D3DSTENCILOP_INCR";
			break;
		case D3DSTENCILOP_DECR:
			msg += "D3DSTENCILOP_DECR";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_STENCILFUNC;
	p_pd3dDevice->GetRenderState(D3DRS_STENCILFUNC, &_D3DRS_STENCILFUNC);
	msg += "D3DRS_STENCILFUNC = ";
	switch(_D3DRS_STENCILFUNC) {
		case D3DCMP_NEVER:
			msg += "D3DCMP_NEVER";
			break;
		case D3DCMP_LESS:
			msg += "D3DCMP_LESS";
			break;
		case D3DCMP_EQUAL:
			msg += "D3DCMP_EQUAL";
			break;
		case D3DCMP_LESSEQUAL:
			msg += "D3DCMP_LESSEQUAL";
			break;
		case D3DCMP_GREATER:
			msg += "D3DCMP_GREATER";
			break;
		case D3DCMP_NOTEQUAL:
			msg += "D3DCMP_NOTEQUAL";
			break;
		case D3DCMP_GREATEREQUAL:
			msg += "D3DCMP_GREATEREQUAL";
			break;
		case D3DCMP_ALWAYS:
			msg += "D3DCMP_ALWAYS";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_STENCILREF;
	p_pd3dDevice->GetRenderState(D3DRS_STENCILREF, &_D3DRS_STENCILREF);
	logInt(msg, "D3DRS_STENCILREF", _D3DRS_STENCILREF);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_STENCILMASK;
	p_pd3dDevice->GetRenderState(D3DRS_STENCILMASK, &_D3DRS_STENCILMASK);
	logDword(msg, "D3DRS_STENCILMASK", _D3DRS_STENCILMASK);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_STENCILWRITEMASK;
	p_pd3dDevice->GetRenderState(D3DRS_STENCILWRITEMASK, &_D3DRS_STENCILWRITEMASK);
	logDword(msg, "D3DRS_STENCILWRITEMASK", _D3DRS_STENCILWRITEMASK);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_TEXTUREFACTOR;
	p_pd3dDevice->GetRenderState(D3DRS_TEXTUREFACTOR, &_D3DRS_TEXTUREFACTOR);
	logDword(msg, "D3DRS_TEXTUREFACTOR", _D3DRS_TEXTUREFACTOR);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP0;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP0, &_D3DRS_WRAP0);
	msg += "D3DRS_WRAP0 =";
	if ( _D3DRS_WRAP0 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP0 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP0 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP1;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP1, &_D3DRS_WRAP1);
	msg += "D3DRS_WRAP1 =";
	if ( _D3DRS_WRAP1 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP1 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP1 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP2;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP2, &_D3DRS_WRAP2);
	msg += "D3DRS_WRAP2 =";
	if ( _D3DRS_WRAP2 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP2 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP2 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP3;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP3, &_D3DRS_WRAP3);
	msg += "D3DRS_WRAP3 =";
	if ( _D3DRS_WRAP3 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP3 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP3 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP4;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP4, &_D3DRS_WRAP4);
	msg += "D3DRS_WRAP4 =";
	if ( _D3DRS_WRAP4 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP4 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP4 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP5;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP5, &_D3DRS_WRAP5);
	msg += "D3DRS_WRAP5 =";
	if ( _D3DRS_WRAP5 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP5 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP5 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP6;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP6, &_D3DRS_WRAP6);
	msg += "D3DRS_WRAP6 =";
	if ( _D3DRS_WRAP6 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP6 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP6 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP7;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP7, &_D3DRS_WRAP7);
	msg += "D3DRS_WRAP7 =";
	if ( _D3DRS_WRAP7 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP7 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP7 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP8;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP8, &_D3DRS_WRAP8);
	msg += "D3DRS_WRAP8 =";
	if ( _D3DRS_WRAP8 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP8 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP8 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP9;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP9, &_D3DRS_WRAP9);
	msg += "D3DRS_WRAP9 =";
	if ( _D3DRS_WRAP9 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP9 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP9 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP10;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP10, &_D3DRS_WRAP10);
	msg += "D3DRS_WRAP10 =";
	if ( _D3DRS_WRAP10 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP10 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP10 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP11;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP11, &_D3DRS_WRAP11);
	msg += "D3DRS_WRAP11 =";
	if ( _D3DRS_WRAP11 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP11 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP11 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP12;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP12, &_D3DRS_WRAP12);
	msg += "D3DRS_WRAP12 =";
	if ( _D3DRS_WRAP12 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP12 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP12 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP13;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP13, &_D3DRS_WRAP13);
	msg += "D3DRS_WRAP13 =";
	if ( _D3DRS_WRAP13 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP13 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP13 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP14;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP14, &_D3DRS_WRAP14);
	msg += "D3DRS_WRAP14 =";
	if ( _D3DRS_WRAP14 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP14 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP14 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_WRAP15;
	p_pd3dDevice->GetRenderState(D3DRS_WRAP15, &_D3DRS_WRAP15);
	msg += "D3DRS_WRAP15 =";
	if ( _D3DRS_WRAP15 & D3DWRAP_U)
		msg += " D3DWRAP_U";
	if ( _D3DRS_WRAP15 & D3DWRAP_V)
		msg += " D3DWRAP_V";
	if ( _D3DRS_WRAP15 & D3DWRAP_W)
		msg += " D3DWRAP_W";
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_CLIPPING;
	p_pd3dDevice->GetRenderState(D3DRS_CLIPPING, &_D3DRS_CLIPPING);
	msg += "D3DRS_CLIPPING = ";
	switch(_D3DRS_CLIPPING) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_LIGHTING;
	p_pd3dDevice->GetRenderState(D3DRS_LIGHTING, &_D3DRS_LIGHTING);
	msg += "D3DRS_LIGHTING = ";
	switch(_D3DRS_LIGHTING) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_AMBIENT;
	p_pd3dDevice->GetRenderState(D3DRS_AMBIENT, &_D3DRS_AMBIENT);
	logDword(msg, "D3DRS_AMBIENT", _D3DRS_AMBIENT);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_FOGVERTEXMODE;
	p_pd3dDevice->GetRenderState(D3DRS_FOGVERTEXMODE, &_D3DRS_FOGVERTEXMODE);
	msg += "D3DRS_FOGVERTEXMODE = ";
	switch(_D3DRS_FOGVERTEXMODE) {
		case D3DFOG_NONE:
			msg += "D3DFOG_NONE";
			break;
		case D3DFOG_EXP:
			msg += "D3DFOG_EXP";
			break;
		case D3DFOG_EXP2:
			msg += "D3DFOG_EXP2";
			break;
		case D3DFOG_LINEAR:
			msg += "D3DFOG_LINEAR";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_COLORVERTEX;
	p_pd3dDevice->GetRenderState(D3DRS_COLORVERTEX, &_D3DRS_COLORVERTEX);
	msg += "D3DRS_COLORVERTEX = ";
	switch(_D3DRS_COLORVERTEX) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_LOCALVIEWER;
	p_pd3dDevice->GetRenderState(D3DRS_LOCALVIEWER, &_D3DRS_LOCALVIEWER);
	msg += "D3DRS_LOCALVIEWER = ";
	switch(_D3DRS_LOCALVIEWER) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_NORMALIZENORMALS;
	p_pd3dDevice->GetRenderState(D3DRS_NORMALIZENORMALS, &_D3DRS_NORMALIZENORMALS);
	msg += "D3DRS_NORMALIZENORMALS = ";
	switch(_D3DRS_NORMALIZENORMALS) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_DIFFUSEMATERIALSOURCE;
	p_pd3dDevice->GetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, &_D3DRS_DIFFUSEMATERIALSOURCE);
	msg += "D3DRS_DIFFUSEMATERIALSOURCE = ";
	switch(_D3DRS_DIFFUSEMATERIALSOURCE) {
		case D3DMCS_MATERIAL:
			msg += "D3DMCS_MATERIAL";
			break;
		case D3DMCS_COLOR1:
			msg += "D3DMCS_COLOR1";
			break;
		case D3DMCS_COLOR2:
			msg += "D3DMCS_COLOR2";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_SPECULARMATERIALSOURCE;
	p_pd3dDevice->GetRenderState(D3DRS_SPECULARMATERIALSOURCE, &_D3DRS_SPECULARMATERIALSOURCE);
	msg += "D3DRS_SPECULARMATERIALSOURCE = ";
	switch(_D3DRS_SPECULARMATERIALSOURCE) {
		case D3DMCS_MATERIAL:
			msg += "D3DMCS_MATERIAL";
			break;
		case D3DMCS_COLOR1:
			msg += "D3DMCS_COLOR1";
			break;
		case D3DMCS_COLOR2:
			msg += "D3DMCS_COLOR2";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_AMBIENTMATERIALSOURCE;
	p_pd3dDevice->GetRenderState(D3DRS_AMBIENTMATERIALSOURCE, &_D3DRS_AMBIENTMATERIALSOURCE);
	msg += "D3DRS_AMBIENTMATERIALSOURCE = ";
	switch(_D3DRS_AMBIENTMATERIALSOURCE) {
		case D3DMCS_MATERIAL:
			msg += "D3DMCS_MATERIAL";
			break;
		case D3DMCS_COLOR1:
			msg += "D3DMCS_COLOR1";
			break;
		case D3DMCS_COLOR2:
			msg += "D3DMCS_COLOR2";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_EMISSIVEMATERIALSOURCE;
	p_pd3dDevice->GetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, &_D3DRS_EMISSIVEMATERIALSOURCE);
	msg += "D3DRS_EMISSIVEMATERIALSOURCE = ";
	switch(_D3DRS_EMISSIVEMATERIALSOURCE) {
		case D3DMCS_MATERIAL:
			msg += "D3DMCS_MATERIAL";
			break;
		case D3DMCS_COLOR1:
			msg += "D3DMCS_COLOR1";
			break;
		case D3DMCS_COLOR2:
			msg += "D3DMCS_COLOR2";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_VERTEXBLEND;
	p_pd3dDevice->GetRenderState(D3DRS_VERTEXBLEND, &_D3DRS_VERTEXBLEND);
	msg += "D3DRS_VERTEXBLEND = ";
	switch(_D3DRS_VERTEXBLEND) {
		case D3DVBF_DISABLE:
			msg += "D3DVBF_DISABLE";
			break;
		case D3DVBF_1WEIGHTS:
			msg += "D3DVBF_1WEIGHTS";
			break;
		case D3DVBF_2WEIGHTS:
			msg += "D3DVBF_2WEIGHTS";
			break;
		case D3DVBF_3WEIGHTS:
			msg += "D3DVBF_3WEIGHTS";
			break;
		case D3DVBF_TWEENING:
			msg += "D3DVBF_TWEENING";
			break;
		case D3DVBF_0WEIGHTS:
			msg += "D3DVBF_0WEIGHTS";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_CLIPPLANEENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_CLIPPLANEENABLE, &_D3DRS_CLIPPLANEENABLE);
	logDword(msg, "D3DRS_CLIPPLANEENABLE", _D3DRS_CLIPPLANEENABLE);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_POINTSIZE;
	p_pd3dDevice->GetRenderState(D3DRS_POINTSIZE, &_D3DRS_POINTSIZE);
	logFloat(msg, "D3DRS_POINTSIZE", _D3DRS_POINTSIZE);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_POINTSIZE_MIN;
	p_pd3dDevice->GetRenderState(D3DRS_POINTSIZE_MIN, &_D3DRS_POINTSIZE_MIN);
	logFloat(msg, "D3DRS_POINTSIZE_MIN", _D3DRS_POINTSIZE_MIN);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_POINTSPRITEENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_POINTSPRITEENABLE, &_D3DRS_POINTSPRITEENABLE);
	msg += "D3DRS_POINTSPRITEENABLE = ";
	switch(_D3DRS_POINTSPRITEENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_POINTSCALEENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_POINTSCALEENABLE, &_D3DRS_POINTSCALEENABLE);
	msg += "D3DRS_POINTSCALEENABLE = ";
	switch(_D3DRS_POINTSCALEENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_POINTSCALE_A;
	p_pd3dDevice->GetRenderState(D3DRS_POINTSCALE_A, &_D3DRS_POINTSCALE_A);
	logFloat(msg, "D3DRS_POINTSCALE_A", _D3DRS_POINTSCALE_A);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_POINTSCALE_B;
	p_pd3dDevice->GetRenderState(D3DRS_POINTSCALE_B, &_D3DRS_POINTSCALE_B);
	logFloat(msg, "D3DRS_POINTSCALE_B", _D3DRS_POINTSCALE_B);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_POINTSCALE_C;
	p_pd3dDevice->GetRenderState(D3DRS_POINTSCALE_C, &_D3DRS_POINTSCALE_C);
	logFloat(msg, "D3DRS_POINTSCALE_C", _D3DRS_POINTSCALE_C);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_MULTISAMPLEANTIALIAS;
	p_pd3dDevice->GetRenderState(D3DRS_MULTISAMPLEANTIALIAS, &_D3DRS_MULTISAMPLEANTIALIAS);
	msg += "D3DRS_MULTISAMPLEANTIALIAS = ";
	switch(_D3DRS_MULTISAMPLEANTIALIAS) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_MULTISAMPLEMASK;
	p_pd3dDevice->GetRenderState(D3DRS_MULTISAMPLEMASK, &_D3DRS_MULTISAMPLEMASK);
	logDword(msg, "D3DRS_MULTISAMPLEMASK", _D3DRS_MULTISAMPLEMASK);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_PATCHEDGESTYLE;
	p_pd3dDevice->GetRenderState(D3DRS_PATCHEDGESTYLE, &_D3DRS_PATCHEDGESTYLE);
	msg += "D3DRS_PATCHEDGESTYLE = ";
	switch(_D3DRS_PATCHEDGESTYLE) {
		case D3DPATCHEDGE_DISCRETE:
			msg += "D3DPATCHEDGE_DISCRETE";
			break;
		case D3DPATCHEDGE_CONTINUOUS:
			msg += "D3DPATCHEDGE_CONTINUOUS";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_DEBUGMONITORTOKEN;
	p_pd3dDevice->GetRenderState(D3DRS_DEBUGMONITORTOKEN, &_D3DRS_DEBUGMONITORTOKEN);
	msg += "D3DRS_DEBUGMONITORTOKEN = ";
	switch(_D3DRS_DEBUGMONITORTOKEN) {
		case D3DDMT_ENABLE:
			msg += "D3DDMT_ENABLE";
			break;
		case D3DDMT_DISABLE:
			msg += "D3DDMT_DISABLE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_POINTSIZE_MAX;
	p_pd3dDevice->GetRenderState(D3DRS_POINTSIZE_MAX, &_D3DRS_POINTSIZE_MAX);
	logFloat(msg, "D3DRS_POINTSIZE_MAX", _D3DRS_POINTSIZE_MAX);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_INDEXEDVERTEXBLENDENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, &_D3DRS_INDEXEDVERTEXBLENDENABLE);
	msg += "D3DRS_INDEXEDVERTEXBLENDENABLE = ";
	switch(_D3DRS_INDEXEDVERTEXBLENDENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_COLORWRITEENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_COLORWRITEENABLE, &_D3DRS_COLORWRITEENABLE);
	logDword(msg, "D3DRS_COLORWRITEENABLE", _D3DRS_COLORWRITEENABLE);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_TWEENFACTOR;
	p_pd3dDevice->GetRenderState(D3DRS_TWEENFACTOR, &_D3DRS_TWEENFACTOR);
	logFloat(msg, "D3DRS_TWEENFACTOR", _D3DRS_TWEENFACTOR);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_BLENDOP;
	p_pd3dDevice->GetRenderState(D3DRS_BLENDOP, &_D3DRS_BLENDOP);
	msg += "D3DRS_BLENDOP = ";
	switch(_D3DRS_BLENDOP) {
		case D3DBLENDOP_ADD:
			msg += "D3DBLENDOP_ADD";
			break;
		case D3DBLENDOP_SUBTRACT:
			msg += "D3DBLENDOP_SUBTRACT";
			break;
		case D3DBLENDOP_REVSUBTRACT:
			msg += "D3DBLENDOP_REVSUBTRACT";
			break;
		case D3DBLENDOP_MIN:
			msg += "D3DBLENDOP_MIN";
			break;
		case D3DBLENDOP_MAX:
			msg += "D3DBLENDOP_MAX";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_POSITIONDEGREE;
	p_pd3dDevice->GetRenderState(D3DRS_POSITIONDEGREE, &_D3DRS_POSITIONDEGREE);
	msg += "D3DRS_POSITIONDEGREE = ";
	switch(_D3DRS_POSITIONDEGREE) {
		case D3DDEGREE_LINEAR:
			msg += "D3DDEGREE_LINEAR";
			break;
		case D3DDEGREE_QUADRATIC:
			msg += "D3DDEGREE_QUADRATIC";
			break;
		case D3DDEGREE_CUBIC:
			msg += "D3DDEGREE_CUBIC";
			break;
		case D3DDEGREE_QUINTIC:
			msg += "D3DDEGREE_QUINTIC";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_NORMALDEGREE;
	p_pd3dDevice->GetRenderState(D3DRS_NORMALDEGREE, &_D3DRS_NORMALDEGREE);
	msg += "D3DRS_NORMALDEGREE = ";
	switch(_D3DRS_NORMALDEGREE) {
		case D3DDEGREE_LINEAR:
			msg += "D3DDEGREE_LINEAR";
			break;
		case D3DDEGREE_QUADRATIC:
			msg += "D3DDEGREE_QUADRATIC";
			break;
		case D3DDEGREE_CUBIC:
			msg += "D3DDEGREE_CUBIC";
			break;
		case D3DDEGREE_QUINTIC:
			msg += "D3DDEGREE_QUINTIC";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_SCISSORTESTENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_SCISSORTESTENABLE, &_D3DRS_SCISSORTESTENABLE);
	msg += "D3DRS_SCISSORTESTENABLE = ";
	switch(_D3DRS_SCISSORTESTENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_SLOPESCALEDEPTHBIAS;
	p_pd3dDevice->GetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, &_D3DRS_SLOPESCALEDEPTHBIAS);
	logFloat(msg, "D3DRS_SLOPESCALEDEPTHBIAS", _D3DRS_SLOPESCALEDEPTHBIAS);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_ANTIALIASEDLINEENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_ANTIALIASEDLINEENABLE, &_D3DRS_ANTIALIASEDLINEENABLE);
	msg += "D3DRS_ANTIALIASEDLINEENABLE = ";
	switch(_D3DRS_ANTIALIASEDLINEENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_MINTESSELLATIONLEVEL;
	p_pd3dDevice->GetRenderState(D3DRS_MINTESSELLATIONLEVEL, &_D3DRS_MINTESSELLATIONLEVEL);
	logFloat(msg, "D3DRS_MINTESSELLATIONLEVEL", _D3DRS_MINTESSELLATIONLEVEL);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_MAXTESSELLATIONLEVEL;
	p_pd3dDevice->GetRenderState(D3DRS_MAXTESSELLATIONLEVEL, &_D3DRS_MAXTESSELLATIONLEVEL);
	logFloat(msg, "D3DRS_MAXTESSELLATIONLEVEL", _D3DRS_MAXTESSELLATIONLEVEL);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_ADAPTIVETESS_X;
	p_pd3dDevice->GetRenderState(D3DRS_ADAPTIVETESS_X, &_D3DRS_ADAPTIVETESS_X);
	logFloat(msg, "D3DRS_ADAPTIVETESS_X", _D3DRS_ADAPTIVETESS_X);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_ADAPTIVETESS_Y;
	p_pd3dDevice->GetRenderState(D3DRS_ADAPTIVETESS_Y, &_D3DRS_ADAPTIVETESS_Y);
	logFloat(msg, "D3DRS_ADAPTIVETESS_Y", _D3DRS_ADAPTIVETESS_Y);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_ADAPTIVETESS_Z;
	p_pd3dDevice->GetRenderState(D3DRS_ADAPTIVETESS_Z, &_D3DRS_ADAPTIVETESS_Z);
	logFloat(msg, "D3DRS_ADAPTIVETESS_Z", _D3DRS_ADAPTIVETESS_Z);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_ADAPTIVETESS_W;
	p_pd3dDevice->GetRenderState(D3DRS_ADAPTIVETESS_W, &_D3DRS_ADAPTIVETESS_W);
	logFloat(msg, "D3DRS_ADAPTIVETESS_W", _D3DRS_ADAPTIVETESS_W);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_ENABLEADAPTIVETESSELLATION;
	p_pd3dDevice->GetRenderState(D3DRS_ENABLEADAPTIVETESSELLATION, &_D3DRS_ENABLEADAPTIVETESSELLATION);
	msg += "D3DRS_ENABLEADAPTIVETESSELLATION = ";
	switch(_D3DRS_ENABLEADAPTIVETESSELLATION) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_TWOSIDEDSTENCILMODE;
	p_pd3dDevice->GetRenderState(D3DRS_TWOSIDEDSTENCILMODE, &_D3DRS_TWOSIDEDSTENCILMODE);
	msg += "D3DRS_TWOSIDEDSTENCILMODE = ";
	switch(_D3DRS_TWOSIDEDSTENCILMODE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_CCW_STENCILFAIL;
	p_pd3dDevice->GetRenderState(D3DRS_CCW_STENCILFAIL, &_D3DRS_CCW_STENCILFAIL);
	msg += "D3DRS_CCW_STENCILFAIL = ";
	switch(_D3DRS_CCW_STENCILFAIL) {
		case D3DSTENCILOP_KEEP:
			msg += "D3DSTENCILOP_KEEP";
			break;
		case D3DSTENCILOP_ZERO:
			msg += "D3DSTENCILOP_ZERO";
			break;
		case D3DSTENCILOP_REPLACE:
			msg += "D3DSTENCILOP_REPLACE";
			break;
		case D3DSTENCILOP_INCRSAT:
			msg += "D3DSTENCILOP_INCRSAT";
			break;
		case D3DSTENCILOP_DECRSAT:
			msg += "D3DSTENCILOP_DECRSAT";
			break;
		case D3DSTENCILOP_INVERT:
			msg += "D3DSTENCILOP_INVERT";
			break;
		case D3DSTENCILOP_INCR:
			msg += "D3DSTENCILOP_INCR";
			break;
		case D3DSTENCILOP_DECR:
			msg += "D3DSTENCILOP_DECR";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_CCW_STENCILZFAIL;
	p_pd3dDevice->GetRenderState(D3DRS_CCW_STENCILZFAIL, &_D3DRS_CCW_STENCILZFAIL);
	msg += "D3DRS_CCW_STENCILZFAIL = ";
	switch(_D3DRS_CCW_STENCILZFAIL) {
		case D3DSTENCILOP_KEEP:
			msg += "D3DSTENCILOP_KEEP";
			break;
		case D3DSTENCILOP_ZERO:
			msg += "D3DSTENCILOP_ZERO";
			break;
		case D3DSTENCILOP_REPLACE:
			msg += "D3DSTENCILOP_REPLACE";
			break;
		case D3DSTENCILOP_INCRSAT:
			msg += "D3DSTENCILOP_INCRSAT";
			break;
		case D3DSTENCILOP_DECRSAT:
			msg += "D3DSTENCILOP_DECRSAT";
			break;
		case D3DSTENCILOP_INVERT:
			msg += "D3DSTENCILOP_INVERT";
			break;
		case D3DSTENCILOP_INCR:
			msg += "D3DSTENCILOP_INCR";
			break;
		case D3DSTENCILOP_DECR:
			msg += "D3DSTENCILOP_DECR";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_CCW_STENCILPASS;
	p_pd3dDevice->GetRenderState(D3DRS_CCW_STENCILPASS, &_D3DRS_CCW_STENCILPASS);
	msg += "D3DRS_CCW_STENCILPASS = ";
	switch(_D3DRS_CCW_STENCILPASS) {
		case D3DSTENCILOP_KEEP:
			msg += "D3DSTENCILOP_KEEP";
			break;
		case D3DSTENCILOP_ZERO:
			msg += "D3DSTENCILOP_ZERO";
			break;
		case D3DSTENCILOP_REPLACE:
			msg += "D3DSTENCILOP_REPLACE";
			break;
		case D3DSTENCILOP_INCRSAT:
			msg += "D3DSTENCILOP_INCRSAT";
			break;
		case D3DSTENCILOP_DECRSAT:
			msg += "D3DSTENCILOP_DECRSAT";
			break;
		case D3DSTENCILOP_INVERT:
			msg += "D3DSTENCILOP_INVERT";
			break;
		case D3DSTENCILOP_INCR:
			msg += "D3DSTENCILOP_INCR";
			break;
		case D3DSTENCILOP_DECR:
			msg += "D3DSTENCILOP_DECR";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_CCW_STENCILFUNC;
	p_pd3dDevice->GetRenderState(D3DRS_CCW_STENCILFUNC, &_D3DRS_CCW_STENCILFUNC);
	msg += "D3DRS_CCW_STENCILFUNC = ";
	switch(_D3DRS_CCW_STENCILFUNC) {
		case D3DCMP_NEVER:
			msg += "D3DCMP_NEVER";
			break;
		case D3DCMP_LESS:
			msg += "D3DCMP_LESS";
			break;
		case D3DCMP_EQUAL:
			msg += "D3DCMP_EQUAL";
			break;
		case D3DCMP_LESSEQUAL:
			msg += "D3DCMP_LESSEQUAL";
			break;
		case D3DCMP_GREATER:
			msg += "D3DCMP_GREATER";
			break;
		case D3DCMP_NOTEQUAL:
			msg += "D3DCMP_NOTEQUAL";
			break;
		case D3DCMP_GREATEREQUAL:
			msg += "D3DCMP_GREATEREQUAL";
			break;
		case D3DCMP_ALWAYS:
			msg += "D3DCMP_ALWAYS";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_COLORWRITEENABLE1;
	p_pd3dDevice->GetRenderState(D3DRS_COLORWRITEENABLE1, &_D3DRS_COLORWRITEENABLE1);
	logDword(msg, "D3DRS_COLORWRITEENABLE1", _D3DRS_COLORWRITEENABLE1);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_COLORWRITEENABLE2;
	p_pd3dDevice->GetRenderState(D3DRS_COLORWRITEENABLE2, &_D3DRS_COLORWRITEENABLE2);
	logDword(msg, "D3DRS_COLORWRITEENABLE2", _D3DRS_COLORWRITEENABLE2);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_COLORWRITEENABLE3;
	p_pd3dDevice->GetRenderState(D3DRS_COLORWRITEENABLE3, &_D3DRS_COLORWRITEENABLE3);
	logDword(msg, "D3DRS_COLORWRITEENABLE3", _D3DRS_COLORWRITEENABLE3);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_BLENDFACTOR;
	p_pd3dDevice->GetRenderState(D3DRS_BLENDFACTOR, &_D3DRS_BLENDFACTOR);
	logDword(msg, "D3DRS_BLENDFACTOR", _D3DRS_BLENDFACTOR);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_DEPTHBIAS;
	p_pd3dDevice->GetRenderState(D3DRS_DEPTHBIAS, &_D3DRS_DEPTHBIAS);
	logFloat(msg, "D3DRS_DEPTHBIAS", _D3DRS_DEPTHBIAS);
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_SEPARATEALPHABLENDENABLE;
	p_pd3dDevice->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, &_D3DRS_SEPARATEALPHABLENDENABLE);
	msg += "D3DRS_SEPARATEALPHABLENDENABLE = ";
	switch(_D3DRS_SEPARATEALPHABLENDENABLE) {
		case TRUE:
			msg += "TRUE";
			break;
		case FALSE:
			msg += "FALSE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_SRCBLENDALPHA;
	p_pd3dDevice->GetRenderState(D3DRS_SRCBLENDALPHA, &_D3DRS_SRCBLENDALPHA);
	msg += "D3DRS_SRCBLENDALPHA = ";
	switch(_D3DRS_SRCBLENDALPHA) {
		case D3DBLEND_ZERO:
			msg += "D3DBLEND_ZERO";
			break;
		case D3DBLEND_ONE:
			msg += "D3DBLEND_ONE";
			break;
		case D3DBLEND_SRCCOLOR:
			msg += "D3DBLEND_SRCCOLOR";
			break;
		case D3DBLEND_INVSRCCOLOR:
			msg += "D3DBLEND_INVSRCCOLOR";
			break;
		case D3DBLEND_SRCALPHA:
			msg += "D3DBLEND_SRCALPHA";
			break;
		case D3DBLEND_INVSRCALPHA:
			msg += "D3DBLEND_INVSRCALPHA";
			break;
		case D3DBLEND_DESTALPHA:
			msg += "D3DBLEND_DESTALPHA";
			break;
		case D3DBLEND_INVDESTALPHA:
			msg += "D3DBLEND_INVDESTALPHA";
			break;
		case D3DBLEND_DESTCOLOR:
			msg += "D3DBLEND_DESTCOLOR";
			break;
		case D3DBLEND_INVDESTCOLOR:
			msg += "D3DBLEND_INVDESTCOLOR";
			break;
		case D3DBLEND_SRCALPHASAT:
			msg += "D3DBLEND_SRCALPHASAT";
			break;
		case D3DBLEND_BOTHSRCALPHA:
			msg += "D3DBLEND_BOTHSRCALPHA";
			break;
		case D3DBLEND_BOTHINVSRCALPHA:
			msg += "D3DBLEND_BOTHINVSRCALPHA";
			break;
		case D3DBLEND_BLENDFACTOR:
			msg += "D3DBLEND_BLENDFACTOR";
			break;
		case D3DBLEND_INVBLENDFACTOR:
			msg += "D3DBLEND_INVBLENDFACTOR";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_DESTBLENDALPHA;
	p_pd3dDevice->GetRenderState(D3DRS_DESTBLENDALPHA, &_D3DRS_DESTBLENDALPHA);
	msg += "D3DRS_DESTBLENDALPHA = ";
	switch(_D3DRS_DESTBLENDALPHA) {
		case D3DBLEND_ZERO:
			msg += "D3DBLEND_ZERO";
			break;
		case D3DBLEND_ONE:
			msg += "D3DBLEND_ONE";
			break;
		case D3DBLEND_SRCCOLOR:
			msg += "D3DBLEND_SRCCOLOR";
			break;
		case D3DBLEND_INVSRCCOLOR:
			msg += "D3DBLEND_INVSRCCOLOR";
			break;
		case D3DBLEND_SRCALPHA:
			msg += "D3DBLEND_SRCALPHA";
			break;
		case D3DBLEND_INVSRCALPHA:
			msg += "D3DBLEND_INVSRCALPHA";
			break;
		case D3DBLEND_DESTALPHA:
			msg += "D3DBLEND_DESTALPHA";
			break;
		case D3DBLEND_INVDESTALPHA:
			msg += "D3DBLEND_INVDESTALPHA";
			break;
		case D3DBLEND_DESTCOLOR:
			msg += "D3DBLEND_DESTCOLOR";
			break;
		case D3DBLEND_INVDESTCOLOR:
			msg += "D3DBLEND_INVDESTCOLOR";
			break;
		case D3DBLEND_SRCALPHASAT:
			msg += "D3DBLEND_SRCALPHASAT";
			break;
		case D3DBLEND_BOTHSRCALPHA:
			msg += "D3DBLEND_BOTHSRCALPHA";
			break;
		case D3DBLEND_BOTHINVSRCALPHA:
			msg += "D3DBLEND_BOTHINVSRCALPHA";
			break;
		case D3DBLEND_BLENDFACTOR:
			msg += "D3DBLEND_BLENDFACTOR";
			break;
		case D3DBLEND_INVBLENDFACTOR:
			msg += "D3DBLEND_INVBLENDFACTOR";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t";
	DWORD _D3DRS_BLENDOPALPHA;
	p_pd3dDevice->GetRenderState(D3DRS_BLENDOPALPHA, &_D3DRS_BLENDOPALPHA);
	msg += "D3DRS_BLENDOPALPHA = ";
	switch(_D3DRS_BLENDOPALPHA) {
		case D3DBLENDOP_ADD:
			msg += "D3DBLENDOP_ADD";
			break;
		case D3DBLENDOP_SUBTRACT:
			msg += "D3DBLENDOP_SUBTRACT";
			break;
		case D3DBLENDOP_REVSUBTRACT:
			msg += "D3DBLENDOP_REVSUBTRACT";
			break;
		case D3DBLENDOP_MIN:
			msg += "D3DBLENDOP_MIN";
			break;
		case D3DBLENDOP_MAX:
			msg += "D3DBLENDOP_MAX";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

logger("================================ dxDumpRenderStates end ========================");
}


void dxDumpTextureStageStates(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*)) {
string msg;
logger("================================ dxDumpTextureStageStates start ========================");
logger("TextureStageStates");
if (!initialized) logger ("dxDump wasn't initialized! unable to dump texture stage states!");
	DWORD _D3DTSS_COLOROP;
	DWORD _D3DTSS_COLORARG1;
	DWORD _D3DTSS_COLORARG2;
	DWORD _D3DTSS_ALPHAOP;
	DWORD _D3DTSS_ALPHAARG1;
	DWORD _D3DTSS_ALPHAARG2;
	DWORD _D3DTSS_BUMPENVMAT00;
	DWORD _D3DTSS_BUMPENVMAT01;
	DWORD _D3DTSS_BUMPENVMAT10;
	DWORD _D3DTSS_BUMPENVMAT11;
	DWORD _D3DTSS_TEXCOORDINDEX;
	DWORD _D3DTSS_BUMPENVLSCALE;
	DWORD _D3DTSS_BUMPENVLOFFSET;
	DWORD _D3DTSS_TEXTURETRANSFORMFLAGS;
	DWORD _D3DTSS_COLORARG0;
	DWORD _D3DTSS_ALPHAARG0;
	DWORD _D3DTSS_RESULTARG;


DWORD TSSNum = 0;
int wasDisabled = 0;
while (TSSNum < MaxTSSNum && !wasDisabled) {
	char buf[1024];
	sprintf(buf, "	TextureStage %d", TSSNum);
	logger(buf);

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_COLOROP, &_D3DTSS_COLOROP);
	msg += "D3DTSS_COLOROP = ";
	switch(_D3DTSS_COLOROP) {
		case D3DTOP_DISABLE:
			msg += "D3DTOP_DISABLE";
			break;
		case D3DTOP_SELECTARG1:
			msg += "D3DTOP_SELECTARG1";
			break;
		case D3DTOP_SELECTARG2:
			msg += "D3DTOP_SELECTARG2";
			break;
		case D3DTOP_MODULATE:
			msg += "D3DTOP_MODULATE";
			break;
		case D3DTOP_MODULATE2X:
			msg += "D3DTOP_MODULATE2X";
			break;
		case D3DTOP_MODULATE4X:
			msg += "D3DTOP_MODULATE4X";
			break;
		case D3DTOP_ADD:
			msg += "D3DTOP_ADD";
			break;
		case D3DTOP_ADDSIGNED:
			msg += "D3DTOP_ADDSIGNED";
			break;
		case D3DTOP_ADDSIGNED2X:
			msg += "D3DTOP_ADDSIGNED2X";
			break;
		case D3DTOP_SUBTRACT:
			msg += "D3DTOP_SUBTRACT";
			break;
		case D3DTOP_ADDSMOOTH:
			msg += "D3DTOP_ADDSMOOTH";
			break;
		case D3DTOP_BLENDDIFFUSEALPHA:
			msg += "D3DTOP_BLENDDIFFUSEALPHA";
			break;
		case D3DTOP_BLENDTEXTUREALPHA:
			msg += "D3DTOP_BLENDTEXTUREALPHA";
			break;
		case D3DTOP_BLENDFACTORALPHA:
			msg += "D3DTOP_BLENDFACTORALPHA";
			break;
		case D3DTOP_BLENDTEXTUREALPHAPM:
			msg += "D3DTOP_BLENDTEXTUREALPHAPM";
			break;
		case D3DTOP_BLENDCURRENTALPHA:
			msg += "D3DTOP_BLENDCURRENTALPHA";
			break;
		case D3DTOP_PREMODULATE:
			msg += "D3DTOP_PREMODULATE";
			break;
		case D3DTOP_MODULATEALPHA_ADDCOLOR:
			msg += "D3DTOP_MODULATEALPHA_ADDCOLOR";
			break;
		case D3DTOP_MODULATECOLOR_ADDALPHA:
			msg += "D3DTOP_MODULATECOLOR_ADDALPHA";
			break;
		case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
			msg += "D3DTOP_MODULATEINVALPHA_ADDCOLOR";
			break;
		case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
			msg += "D3DTOP_MODULATEINVCOLOR_ADDALPHA";
			break;
		case D3DTOP_BUMPENVMAP:
			msg += "D3DTOP_BUMPENVMAP";
			break;
		case D3DTOP_BUMPENVMAPLUMINANCE:
			msg += "D3DTOP_BUMPENVMAPLUMINANCE";
			break;
		case D3DTOP_DOTPRODUCT3:
			msg += "D3DTOP_DOTPRODUCT3";
			break;
		case D3DTOP_MULTIPLYADD:
			msg += "D3DTOP_MULTIPLYADD";
			break;
		case D3DTOP_LERP:
			msg += "D3DTOP_LERP";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_COLORARG1, &_D3DTSS_COLORARG1);
	msg += "D3DTSS_COLORARG1 =";
	switch(_D3DTSS_COLORARG1 & D3DTA_SELECTMASK) {
		case D3DTA_DIFFUSE:
			msg += " D3DTA_DIFFUSE";
			break;
		case D3DTA_CURRENT:
			msg += " D3DTA_CURRENT";
			break;
		case D3DTA_TEXTURE:
			msg += " D3DTA_TEXTURE";
			break;
		case D3DTA_TFACTOR:
			msg += " D3DTA_TFACTOR";
			break;
		case D3DTA_SPECULAR:
			msg += " D3DTA_SPECULAR";
			break;
		case D3DTA_TEMP:
			msg += " D3DTA_TEMP";
			break;
		case D3DTA_CONSTANT:
			msg += " D3DTA_CONSTANT";
			break;
		default:
			msg += "?";
			break;
	}
	if (_D3DTSS_COLORARG1 & D3DTA_ALPHAREPLICATE)
		msg += " D3DTA_ALPHAREPLICATE";				
	if (_D3DTSS_COLORARG1 & D3DTA_COMPLEMENT)
		msg += " D3DTA_COMPLEMENT";				
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_COLORARG2, &_D3DTSS_COLORARG2);
	msg += "D3DTSS_COLORARG2 =";
	switch(_D3DTSS_COLORARG2 & D3DTA_SELECTMASK) {
		case D3DTA_DIFFUSE:
			msg += " D3DTA_DIFFUSE";
			break;
		case D3DTA_CURRENT:
			msg += " D3DTA_CURRENT";
			break;
		case D3DTA_TEXTURE:
			msg += " D3DTA_TEXTURE";
			break;
		case D3DTA_TFACTOR:
			msg += " D3DTA_TFACTOR";
			break;
		case D3DTA_SPECULAR:
			msg += " D3DTA_SPECULAR";
			break;
		case D3DTA_TEMP:
			msg += " D3DTA_TEMP";
			break;
		case D3DTA_CONSTANT:
			msg += " D3DTA_CONSTANT";
			break;
		default:
			msg += "?";
			break;
	}
	if (_D3DTSS_COLORARG2 & D3DTA_ALPHAREPLICATE)
		msg += " D3DTA_ALPHAREPLICATE";				
	if (_D3DTSS_COLORARG2 & D3DTA_COMPLEMENT)
		msg += " D3DTA_COMPLEMENT";				
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_ALPHAOP, &_D3DTSS_ALPHAOP);
	msg += "D3DTSS_ALPHAOP = ";
	switch(_D3DTSS_ALPHAOP) {
		case D3DTOP_DISABLE:
			msg += "D3DTOP_DISABLE";
			break;
		case D3DTOP_SELECTARG1:
			msg += "D3DTOP_SELECTARG1";
			break;
		case D3DTOP_SELECTARG2:
			msg += "D3DTOP_SELECTARG2";
			break;
		case D3DTOP_MODULATE:
			msg += "D3DTOP_MODULATE";
			break;
		case D3DTOP_MODULATE2X:
			msg += "D3DTOP_MODULATE2X";
			break;
		case D3DTOP_MODULATE4X:
			msg += "D3DTOP_MODULATE4X";
			break;
		case D3DTOP_ADD:
			msg += "D3DTOP_ADD";
			break;
		case D3DTOP_ADDSIGNED:
			msg += "D3DTOP_ADDSIGNED";
			break;
		case D3DTOP_ADDSIGNED2X:
			msg += "D3DTOP_ADDSIGNED2X";
			break;
		case D3DTOP_SUBTRACT:
			msg += "D3DTOP_SUBTRACT";
			break;
		case D3DTOP_ADDSMOOTH:
			msg += "D3DTOP_ADDSMOOTH";
			break;
		case D3DTOP_BLENDDIFFUSEALPHA:
			msg += "D3DTOP_BLENDDIFFUSEALPHA";
			break;
		case D3DTOP_BLENDTEXTUREALPHA:
			msg += "D3DTOP_BLENDTEXTUREALPHA";
			break;
		case D3DTOP_BLENDFACTORALPHA:
			msg += "D3DTOP_BLENDFACTORALPHA";
			break;
		case D3DTOP_BLENDTEXTUREALPHAPM:
			msg += "D3DTOP_BLENDTEXTUREALPHAPM";
			break;
		case D3DTOP_BLENDCURRENTALPHA:
			msg += "D3DTOP_BLENDCURRENTALPHA";
			break;
		case D3DTOP_PREMODULATE:
			msg += "D3DTOP_PREMODULATE";
			break;
		case D3DTOP_MODULATEALPHA_ADDCOLOR:
			msg += "D3DTOP_MODULATEALPHA_ADDCOLOR";
			break;
		case D3DTOP_MODULATECOLOR_ADDALPHA:
			msg += "D3DTOP_MODULATECOLOR_ADDALPHA";
			break;
		case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
			msg += "D3DTOP_MODULATEINVALPHA_ADDCOLOR";
			break;
		case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
			msg += "D3DTOP_MODULATEINVCOLOR_ADDALPHA";
			break;
		case D3DTOP_BUMPENVMAP:
			msg += "D3DTOP_BUMPENVMAP";
			break;
		case D3DTOP_BUMPENVMAPLUMINANCE:
			msg += "D3DTOP_BUMPENVMAPLUMINANCE";
			break;
		case D3DTOP_DOTPRODUCT3:
			msg += "D3DTOP_DOTPRODUCT3";
			break;
		case D3DTOP_MULTIPLYADD:
			msg += "D3DTOP_MULTIPLYADD";
			break;
		case D3DTOP_LERP:
			msg += "D3DTOP_LERP";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_ALPHAARG1, &_D3DTSS_ALPHAARG1);
	msg += "D3DTSS_ALPHAARG1 =";
	switch(_D3DTSS_ALPHAARG1 & D3DTA_SELECTMASK) {
		case D3DTA_DIFFUSE:
			msg += " D3DTA_DIFFUSE";
			break;
		case D3DTA_CURRENT:
			msg += " D3DTA_CURRENT";
			break;
		case D3DTA_TEXTURE:
			msg += " D3DTA_TEXTURE";
			break;
		case D3DTA_TFACTOR:
			msg += " D3DTA_TFACTOR";
			break;
		case D3DTA_SPECULAR:
			msg += " D3DTA_SPECULAR";
			break;
		case D3DTA_TEMP:
			msg += " D3DTA_TEMP";
			break;
		case D3DTA_CONSTANT:
			msg += " D3DTA_CONSTANT";
			break;
		default:
			msg += "?";
			break;
	}
	if (_D3DTSS_ALPHAARG1 & D3DTA_ALPHAREPLICATE)
		msg += " D3DTA_ALPHAREPLICATE";				
	if (_D3DTSS_ALPHAARG1 & D3DTA_COMPLEMENT)
		msg += " D3DTA_COMPLEMENT";				
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_ALPHAARG2, &_D3DTSS_ALPHAARG2);
	msg += "D3DTSS_ALPHAARG2 =";
	switch(_D3DTSS_ALPHAARG2 & D3DTA_SELECTMASK) {
		case D3DTA_DIFFUSE:
			msg += " D3DTA_DIFFUSE";
			break;
		case D3DTA_CURRENT:
			msg += " D3DTA_CURRENT";
			break;
		case D3DTA_TEXTURE:
			msg += " D3DTA_TEXTURE";
			break;
		case D3DTA_TFACTOR:
			msg += " D3DTA_TFACTOR";
			break;
		case D3DTA_SPECULAR:
			msg += " D3DTA_SPECULAR";
			break;
		case D3DTA_TEMP:
			msg += " D3DTA_TEMP";
			break;
		case D3DTA_CONSTANT:
			msg += " D3DTA_CONSTANT";
			break;
		default:
			msg += "?";
			break;
	}
	if (_D3DTSS_ALPHAARG2 & D3DTA_ALPHAREPLICATE)
		msg += " D3DTA_ALPHAREPLICATE";				
	if (_D3DTSS_ALPHAARG2 & D3DTA_COMPLEMENT)
		msg += " D3DTA_COMPLEMENT";				
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_BUMPENVMAT00, &_D3DTSS_BUMPENVMAT00);
	logFloat(msg, "D3DTSS_BUMPENVMAT00", _D3DTSS_BUMPENVMAT00);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_BUMPENVMAT01, &_D3DTSS_BUMPENVMAT01);
	logFloat(msg, "D3DTSS_BUMPENVMAT01", _D3DTSS_BUMPENVMAT01);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_BUMPENVMAT10, &_D3DTSS_BUMPENVMAT10);
	logFloat(msg, "D3DTSS_BUMPENVMAT10", _D3DTSS_BUMPENVMAT10);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_BUMPENVMAT11, &_D3DTSS_BUMPENVMAT11);
	logFloat(msg, "D3DTSS_BUMPENVMAT11", _D3DTSS_BUMPENVMAT11);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_TEXCOORDINDEX, &_D3DTSS_TEXCOORDINDEX);
	logInt(msg, "D3DTSS_TEXCOORDINDEX", _D3DTSS_TEXCOORDINDEX);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_BUMPENVLSCALE, &_D3DTSS_BUMPENVLSCALE);
	logFloat(msg, "D3DTSS_BUMPENVLSCALE", _D3DTSS_BUMPENVLSCALE);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_BUMPENVLOFFSET, &_D3DTSS_BUMPENVLOFFSET);
	logFloat(msg, "D3DTSS_BUMPENVLOFFSET", _D3DTSS_BUMPENVLOFFSET);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_TEXTURETRANSFORMFLAGS, &_D3DTSS_TEXTURETRANSFORMFLAGS);
	msg += "D3DTSS_TEXTURETRANSFORMFLAGS = ";
	switch(_D3DTSS_TEXTURETRANSFORMFLAGS) {
		case D3DTTFF_DISABLE:
			msg += "D3DTTFF_DISABLE";
			break;
		case D3DTTFF_COUNT1:
			msg += "D3DTTFF_COUNT1";
			break;
		case D3DTTFF_COUNT2:
			msg += "D3DTTFF_COUNT2";
			break;
		case D3DTTFF_COUNT3:
			msg += "D3DTTFF_COUNT3";
			break;
		case D3DTTFF_COUNT4:
			msg += "D3DTTFF_COUNT4";
			break;
		case D3DTTFF_PROJECTED:
			msg += "D3DTTFF_PROJECTED";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_COLORARG0, &_D3DTSS_COLORARG0);
	msg += "D3DTSS_COLORARG0 =";
	switch(_D3DTSS_COLORARG0 & D3DTA_SELECTMASK) {
		case D3DTA_DIFFUSE:
			msg += " D3DTA_DIFFUSE";
			break;
		case D3DTA_CURRENT:
			msg += " D3DTA_CURRENT";
			break;
		case D3DTA_TEXTURE:
			msg += " D3DTA_TEXTURE";
			break;
		case D3DTA_TFACTOR:
			msg += " D3DTA_TFACTOR";
			break;
		case D3DTA_SPECULAR:
			msg += " D3DTA_SPECULAR";
			break;
		case D3DTA_TEMP:
			msg += " D3DTA_TEMP";
			break;
		case D3DTA_CONSTANT:
			msg += " D3DTA_CONSTANT";
			break;
		default:
			msg += "?";
			break;
	}
	if (_D3DTSS_COLORARG0 & D3DTA_ALPHAREPLICATE)
		msg += " D3DTA_ALPHAREPLICATE";				
	if (_D3DTSS_COLORARG0 & D3DTA_COMPLEMENT)
		msg += " D3DTA_COMPLEMENT";				
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_ALPHAARG0, &_D3DTSS_ALPHAARG0);
	msg += "D3DTSS_ALPHAARG0 =";
	switch(_D3DTSS_ALPHAARG0 & D3DTA_SELECTMASK) {
		case D3DTA_DIFFUSE:
			msg += " D3DTA_DIFFUSE";
			break;
		case D3DTA_CURRENT:
			msg += " D3DTA_CURRENT";
			break;
		case D3DTA_TEXTURE:
			msg += " D3DTA_TEXTURE";
			break;
		case D3DTA_TFACTOR:
			msg += " D3DTA_TFACTOR";
			break;
		case D3DTA_SPECULAR:
			msg += " D3DTA_SPECULAR";
			break;
		case D3DTA_TEMP:
			msg += " D3DTA_TEMP";
			break;
		case D3DTA_CONSTANT:
			msg += " D3DTA_CONSTANT";
			break;
		default:
			msg += "?";
			break;
	}
	if (_D3DTSS_ALPHAARG0 & D3DTA_ALPHAREPLICATE)
		msg += " D3DTA_ALPHAREPLICATE";				
	if (_D3DTSS_ALPHAARG0 & D3DTA_COMPLEMENT)
		msg += " D3DTA_COMPLEMENT";				
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetTextureStageState(TSSNum, D3DTSS_RESULTARG, &_D3DTSS_RESULTARG);
	msg += "D3DTSS_RESULTARG =";
	switch(_D3DTSS_RESULTARG & D3DTA_SELECTMASK) {
		case D3DTA_DIFFUSE:
			msg += " D3DTA_DIFFUSE";
			break;
		case D3DTA_CURRENT:
			msg += " D3DTA_CURRENT";
			break;
		case D3DTA_TEXTURE:
			msg += " D3DTA_TEXTURE";
			break;
		case D3DTA_TFACTOR:
			msg += " D3DTA_TFACTOR";
			break;
		case D3DTA_SPECULAR:
			msg += " D3DTA_SPECULAR";
			break;
		case D3DTA_TEMP:
			msg += " D3DTA_TEMP";
			break;
		case D3DTA_CONSTANT:
			msg += " D3DTA_CONSTANT";
			break;
		default:
			msg += "?";
			break;
	}
	if (_D3DTSS_RESULTARG & D3DTA_ALPHAREPLICATE)
		msg += " D3DTA_ALPHAREPLICATE";				
	if (_D3DTSS_RESULTARG & D3DTA_COMPLEMENT)
		msg += " D3DTA_COMPLEMENT";				
	logger(msg.c_str());

	TSSNum++;
	if ( (_D3DTSS_COLOROP == D3DTOP_DISABLE) && (_D3DTSS_ALPHAOP == D3DTOP_DISABLE) )
		wasDisabled = 1;
}
logger("================================ dxDumpTextureStageStates end ========================");
}


void dxDumpSamplerStates(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*)) {
string msg;
logger("================================ dxDumpSamplerStates start ========================");
logger("SamplerStates");
if (!initialized) logger ("dxDump wasn't initialized! unable to dump sampler states!");
	DWORD _D3DSAMP_ADDRESSU;
	DWORD _D3DSAMP_ADDRESSV;
	DWORD _D3DSAMP_ADDRESSW;
	DWORD _D3DSAMP_BORDERCOLOR;
	DWORD _D3DSAMP_MAGFILTER;
	DWORD _D3DSAMP_MINFILTER;
	DWORD _D3DSAMP_MIPFILTER;
	DWORD _D3DSAMP_MIPMAPLODBIAS;
	DWORD _D3DSAMP_MAXMIPLEVEL;
	DWORD _D3DSAMP_MAXANISOTROPY;
	DWORD _D3DSAMP_SRGBTEXTURE;
	DWORD _D3DSAMP_ELEMENTINDEX;
	DWORD _D3DSAMP_DMAPOFFSET;


DWORD samplerNum = 0;
while (samplerNum < MaxTSSNum) {
	char buf[1024];
	sprintf(buf, "	Sampler %d", samplerNum);
	logger(buf);

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_ADDRESSU, &_D3DSAMP_ADDRESSU);
	msg += "D3DSAMP_ADDRESSU = ";
	switch(_D3DSAMP_ADDRESSU) {
		case D3DTADDRESS_WRAP:
			msg += "D3DTADDRESS_WRAP";
			break;
		case D3DTADDRESS_MIRROR:
			msg += "D3DTADDRESS_MIRROR";
			break;
		case D3DTADDRESS_CLAMP:
			msg += "D3DTADDRESS_CLAMP";
			break;
		case D3DTADDRESS_BORDER:
			msg += "D3DTADDRESS_BORDER";
			break;
		case D3DTADDRESS_MIRRORONCE:
			msg += "D3DTADDRESS_MIRRORONCE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_ADDRESSV, &_D3DSAMP_ADDRESSV);
	msg += "D3DSAMP_ADDRESSV = ";
	switch(_D3DSAMP_ADDRESSV) {
		case D3DTADDRESS_WRAP:
			msg += "D3DTADDRESS_WRAP";
			break;
		case D3DTADDRESS_MIRROR:
			msg += "D3DTADDRESS_MIRROR";
			break;
		case D3DTADDRESS_CLAMP:
			msg += "D3DTADDRESS_CLAMP";
			break;
		case D3DTADDRESS_BORDER:
			msg += "D3DTADDRESS_BORDER";
			break;
		case D3DTADDRESS_MIRRORONCE:
			msg += "D3DTADDRESS_MIRRORONCE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_ADDRESSW, &_D3DSAMP_ADDRESSW);
	msg += "D3DSAMP_ADDRESSW = ";
	switch(_D3DSAMP_ADDRESSW) {
		case D3DTADDRESS_WRAP:
			msg += "D3DTADDRESS_WRAP";
			break;
		case D3DTADDRESS_MIRROR:
			msg += "D3DTADDRESS_MIRROR";
			break;
		case D3DTADDRESS_CLAMP:
			msg += "D3DTADDRESS_CLAMP";
			break;
		case D3DTADDRESS_BORDER:
			msg += "D3DTADDRESS_BORDER";
			break;
		case D3DTADDRESS_MIRRORONCE:
			msg += "D3DTADDRESS_MIRRORONCE";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_BORDERCOLOR, &_D3DSAMP_BORDERCOLOR);
	logDword(msg, "D3DSAMP_BORDERCOLOR", _D3DSAMP_BORDERCOLOR);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_MAGFILTER, &_D3DSAMP_MAGFILTER);
	msg += "D3DSAMP_MAGFILTER = ";
	switch(_D3DSAMP_MAGFILTER) {
		case D3DTEXF_NONE:
			msg += "D3DTEXF_NONE";
			break;
		case D3DTEXF_POINT:
			msg += "D3DTEXF_POINT";
			break;
		case D3DTEXF_LINEAR:
			msg += "D3DTEXF_LINEAR";
			break;
		case D3DTEXF_ANISOTROPIC:
			msg += "D3DTEXF_ANISOTROPIC";
			break;
		case D3DTEXF_PYRAMIDALQUAD:
			msg += "D3DTEXF_PYRAMIDALQUAD";
			break;
		case D3DTEXF_GAUSSIANQUAD:
			msg += "D3DTEXF_GAUSSIANQUAD";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_MINFILTER, &_D3DSAMP_MINFILTER);
	msg += "D3DSAMP_MINFILTER = ";
	switch(_D3DSAMP_MINFILTER) {
		case D3DTEXF_NONE:
			msg += "D3DTEXF_NONE";
			break;
		case D3DTEXF_POINT:
			msg += "D3DTEXF_POINT";
			break;
		case D3DTEXF_LINEAR:
			msg += "D3DTEXF_LINEAR";
			break;
		case D3DTEXF_ANISOTROPIC:
			msg += "D3DTEXF_ANISOTROPIC";
			break;
		case D3DTEXF_PYRAMIDALQUAD:
			msg += "D3DTEXF_PYRAMIDALQUAD";
			break;
		case D3DTEXF_GAUSSIANQUAD:
			msg += "D3DTEXF_GAUSSIANQUAD";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_MIPFILTER, &_D3DSAMP_MIPFILTER);
	msg += "D3DSAMP_MIPFILTER = ";
	switch(_D3DSAMP_MIPFILTER) {
		case D3DTEXF_NONE:
			msg += "D3DTEXF_NONE";
			break;
		case D3DTEXF_POINT:
			msg += "D3DTEXF_POINT";
			break;
		case D3DTEXF_LINEAR:
			msg += "D3DTEXF_LINEAR";
			break;
		case D3DTEXF_ANISOTROPIC:
			msg += "D3DTEXF_ANISOTROPIC";
			break;
		case D3DTEXF_PYRAMIDALQUAD:
			msg += "D3DTEXF_PYRAMIDALQUAD";
			break;
		case D3DTEXF_GAUSSIANQUAD:
			msg += "D3DTEXF_GAUSSIANQUAD";
			break;
		default:
			msg += "?";
			break;
	}
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_MIPMAPLODBIAS, &_D3DSAMP_MIPMAPLODBIAS);
	logInt(msg, "D3DSAMP_MIPMAPLODBIAS", _D3DSAMP_MIPMAPLODBIAS);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_MAXMIPLEVEL, &_D3DSAMP_MAXMIPLEVEL);
	logInt(msg, "D3DSAMP_MAXMIPLEVEL", _D3DSAMP_MAXMIPLEVEL);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_MAXANISOTROPY, &_D3DSAMP_MAXANISOTROPY);
	logInt(msg, "D3DSAMP_MAXANISOTROPY", _D3DSAMP_MAXANISOTROPY);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_SRGBTEXTURE, &_D3DSAMP_SRGBTEXTURE);
	logFloat(msg, "D3DSAMP_SRGBTEXTURE", _D3DSAMP_SRGBTEXTURE);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_ELEMENTINDEX, &_D3DSAMP_ELEMENTINDEX);
	logInt(msg, "D3DSAMP_ELEMENTINDEX", _D3DSAMP_ELEMENTINDEX);
	logger(msg.c_str());

	msg = "\t\t";
	p_pd3dDevice->GetSamplerState(samplerNum , D3DSAMP_DMAPOFFSET, &_D3DSAMP_DMAPOFFSET);
	logInt(msg, "D3DSAMP_DMAPOFFSET", _D3DSAMP_DMAPOFFSET);
	logger(msg.c_str());
samplerNum++;
}
logger("================================ dxDumpSamplerStates end ========================");
}


string dxError2Errname(HRESULT p_hr) {
	switch (p_hr) {
		case D3D_OK: return "D3D_OK"; break;
		case D3DERR_WRONGTEXTUREFORMAT: return "D3DERR_WRONGTEXTUREFORMAT"; break;
		case D3DERR_UNSUPPORTEDCOLOROPERATION: return "D3DERR_UNSUPPORTEDCOLOROPERATION"; break;
		case D3DERR_UNSUPPORTEDCOLORARG: return "D3DERR_UNSUPPORTEDCOLORARG"; break;
		case D3DERR_UNSUPPORTEDALPHAOPERATION: return "D3DERR_UNSUPPORTEDALPHAOPERATION"; break;
		case D3DERR_UNSUPPORTEDALPHAARG: return "D3DERR_UNSUPPORTEDALPHAARG"; break;
		case D3DERR_TOOMANYOPERATIONS: return "D3DERR_TOOMANYOPERATIONS"; break;
		case D3DERR_CONFLICTINGTEXTUREFILTER: return "D3DERR_CONFLICTINGTEXTUREFILTER"; break;
		case D3DERR_UNSUPPORTEDFACTORVALUE: return "D3DERR_UNSUPPORTEDFACTORVALUE"; break;
		case D3DERR_CONFLICTINGRENDERSTATE: return "D3DERR_CONFLICTINGRENDERSTATE"; break;
		case D3DERR_UNSUPPORTEDTEXTUREFILTER: return "D3DERR_UNSUPPORTEDTEXTUREFILTER"; break;
		case D3DERR_CONFLICTINGTEXTUREPALETTE: return "D3DERR_CONFLICTINGTEXTUREPALETTE"; break;
		case D3DERR_DRIVERINTERNALERROR: return "D3DERR_DRIVERINTERNALERROR"; break;
		case D3DERR_NOTFOUND: return "D3DERR_NOTFOUND"; break;
		case D3DERR_MOREDATA: return "D3DERR_MOREDATA"; break;
		case D3DERR_DEVICELOST: return "D3DERR_DEVICELOST"; break;
		case D3DERR_DEVICENOTRESET: return "D3DERR_DEVICENOTRESET"; break;
		case D3DERR_NOTAVAILABLE: return "D3DERR_NOTAVAILABLE"; break;
		case D3DERR_OUTOFVIDEOMEMORY: return "D3DERR_OUTOFVIDEOMEMORY"; break;
		case D3DERR_INVALIDDEVICE: return "D3DERR_INVALIDDEVICE"; break;
		case D3DERR_INVALIDCALL: return "D3DERR_INVALIDCALL"; break;
		case D3DERR_DRIVERINVALIDCALL: return "D3DERR_DRIVERINVALIDCALL"; break;
		case D3DERR_WASSTILLDRAWING: return "D3DERR_WASSTILLDRAWING"; break;
		case D3DOK_NOAUTOGEN: return "D3DOK_NOAUTOGEN"; break;
		case D3DXERR_CANNOTMODIFYINDEXBUFFER: return "D3DXERR_CANNOTMODIFYINDEXBUFFER"; break;
		case D3DXERR_INVALIDMESH: return "D3DXERR_INVALIDMESH"; break;
		case D3DXERR_CANNOTATTRSORT: return "D3DXERR_CANNOTATTRSORT"; break;
		case D3DXERR_SKINNINGNOTSUPPORTED: return "D3DXERR_SKINNINGNOTSUPPORTED"; break;
		case D3DXERR_TOOMANYINFLUENCES: return "D3DXERR_TOOMANYINFLUENCES"; break;
		case D3DXERR_INVALIDDATA: return "D3DXERR_INVALIDDATA"; break;
		case D3DXERR_LOADEDMESHASNODATA: return "D3DXERR_LOADEDMESHASNODATA"; break;
		case D3DXERR_DUPLICATENAMEDFRAGMENT: return "D3DXERR_DUPLICATENAMEDFRAGMENT"; break;
		case E_OUTOFMEMORY: return "E_OUTOFMEMORY"; break;
		default: return "?"; break;
	}
}
string dxError2Explanation(HRESULT p_hr) {
	switch (p_hr) {
		case D3D_OK: return "OK"; break;
		case D3DERR_WRONGTEXTUREFORMAT: return "The pixel format of the texture surface is not valid."; break;
		case D3DERR_UNSUPPORTEDCOLOROPERATION: return "The device does not support a specified texture-blending operation for color values."; break;
		case D3DERR_UNSUPPORTEDCOLORARG: return "The device does not support a specified texture-blending argument for color values."; break;
		case D3DERR_UNSUPPORTEDALPHAOPERATION: return "The device does not support a specified texture-blending operation for the alpha channel."; break;
		case D3DERR_UNSUPPORTEDALPHAARG: return "The device does not support a specified texture-blending argument for the alpha channel."; break;
		case D3DERR_TOOMANYOPERATIONS: return "The application is requesting more texture-filtering operations than the device supports."; break;
		case D3DERR_CONFLICTINGTEXTUREFILTER: return "The current texture filters cannot be used together."; break;
		case D3DERR_UNSUPPORTEDFACTORVALUE: return "The device does not support the specified texture factor value. Not used; provided only to support older drivers."; break;
		case D3DERR_CONFLICTINGRENDERSTATE: return "The currently set render states cannot be used together."; break;
		case D3DERR_UNSUPPORTEDTEXTUREFILTER: return "The device does not support the specified texture filter."; break;
		case D3DERR_CONFLICTINGTEXTUREPALETTE: return "The current textures cannot be used simultaneously."; break;
		case D3DERR_DRIVERINTERNALERROR: return "Internal driver error."; break;
		case D3DERR_NOTFOUND: return "The requested item was not found."; break;
		case D3DERR_MOREDATA: return "There is more data available than the specified buffer size can hold."; break;
		case D3DERR_DEVICELOST: return "The device has been lost but cannot be reset at this time. Therefore, rendering is not possible."; break;
		case D3DERR_DEVICENOTRESET: return "The device has been lost but can be reset at this time."; break;
		case D3DERR_NOTAVAILABLE: return "This device does not support the queried technique."; break;
		case D3DERR_OUTOFVIDEOMEMORY: return "Microsoft Direct3D does not have enough display memory to perform the operation."; break;
		case D3DERR_INVALIDDEVICE: return "The requested device type is not valid."; break;
		case D3DERR_INVALIDCALL: return "The method call is invalid. For example, a method's parameter may have an invalid value."; break;
		case D3DERR_DRIVERINVALIDCALL: return "D3DERR_DRIVERINVALIDCALL"; break;
		case D3DERR_WASSTILLDRAWING: return "D3DERR_WASSTILLDRAWING"; break;
		case D3DOK_NOAUTOGEN: return "This is a success code. However, the autogeneration of mipmaps is not supported for this format. This means that resource creation will succeed but the mipmap levels will not be automatically generated."; break;
		case D3DXERR_CANNOTMODIFYINDEXBUFFER: return "The index buffer cannot be modified."; break;
		case D3DXERR_INVALIDMESH: return "The mesh is invalid."; break;
		case D3DXERR_CANNOTATTRSORT: return "Attribute sort (D3DXMESHOPT_ATTRSORT) is not supported as an optimization technique."; break;
		case D3DXERR_SKINNINGNOTSUPPORTED: return "Skinning is not supported."; break;
		case D3DXERR_TOOMANYINFLUENCES: return "Too many influences specified."; break;
		case D3DXERR_INVALIDDATA: return "The data is invalid."; break;
		case D3DXERR_LOADEDMESHASNODATA: return "The mesh has no data."; break;
		case D3DXERR_DUPLICATENAMEDFRAGMENT: return "A fragment with that name already exists."; break;
		case E_OUTOFMEMORY: return "Direct3D could not allocate sufficient memory to complete the call."; break;
		default: return "?"; break;
	}
}

void dxDumpNextFrame(void) {
	frameNum++;
}

void dxDumpTexture(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*), int stageNum) {
	LPDIRECT3DBASETEXTURE9 pTexture = NULL;
	char buf[1024], buf2[1024];
	string frameDirName;
	HRESULT hr;
	hr = p_pd3dDevice->GetTexture(stageNum, &pTexture);
	if (hr == D3D_OK) {
		_mkdir(dirname);
		sprintf(buf, "%d", stageNum);
		sprintf(buf2, "%07d", frameNum);
		frameDirName = string(dirname) + "\\" + string(buf2);
		_mkdir(frameDirName.c_str());
		string filename = string(frameDirName) + "\\t" + string(buf) + ".bmp";
		hr = D3DXSaveTextureToFile(filename.c_str(), D3DXIFF_BMP, pTexture, NULL);
		if (hr == D3D_OK) {
			string msg = "Texture from stage " + string(buf) + " saved to " + string(filename);
			logger(msg.c_str());
		}
		else {
			logger("D3DXSaveTextureToFile failed in dxDumpTexture");
		}
	}
	else {
		logger("GetTexture failed in dxDumpTexture");
	}
}

void dxDumpAllTextures(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*)) {
	if (!initialized) logger ("dxDump wasn't initialized! unable to dump all textures!");
	for (DWORD i = 0; i < MaxTSSNum; i++) {
		dxDumpTexture(p_pd3dDevice, logger, i);
	}
}

void dxDumpRenderTarget(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*), int targetNum) {
	LPDIRECT3DSURFACE9 pRenderTarget = NULL, pInMemTarget = NULL;
	D3DSURFACE_DESC surfDesc;
	HRESULT hr;
	char buf[1024], buf2[1024];
	string frameDirName;

	hr = p_pd3dDevice->GetRenderTarget(targetNum, &pRenderTarget);
	if (hr != D3D_OK) {
		logger("GetRenderTarget failed in dxDumpRenderTarget");
		return;
	}

	hr = pRenderTarget->GetDesc(&surfDesc);
	if (hr != D3D_OK) {
		logger("GetDesc failed in dxDumpRenderTarget");
		return;
	}

	hr = p_pd3dDevice->CreateOffscreenPlainSurface(surfDesc.Width, surfDesc.Height, surfDesc.Format, D3DPOOL_SYSTEMMEM, &pInMemTarget, NULL);
	if (hr != D3D_OK) {
		logger("CreateOffscreenPlainSurface failed in dxDumpRenderTarget");
		return;
	}

	hr = p_pd3dDevice->GetRenderTargetData(pRenderTarget, pInMemTarget);
	if (hr != D3D_OK) {
		logger("GetRenderTargetData failed in dxDumpRenderTarget");
		return;
	}

	_mkdir(dirname);
	sprintf(buf, "%d", targetNum);
	sprintf(buf2, "%07d", frameNum);
	frameDirName = string(dirname) + "\\" + string(buf2);
	_mkdir(frameDirName.c_str());
	string filename = string(frameDirName) + "\\r" + string(buf) + ".bmp";

	hr = D3DXSaveSurfaceToFile(filename.c_str(), D3DXIFF_BMP, pInMemTarget, NULL, NULL);
		if (hr == D3D_OK) {
			string msg = "Render target " + string(buf) + " saved to " + string(filename);
			logger(msg.c_str());
		}
	else {
		logger("D3DXSaveSurfaceToFile failed in dxDumpRenderTarget");
		return;
	}	
}

void dxDumpAllRenderTargets(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*)) {
	if (!initialized) logger ("dxDump wasn't initialized! unable to dump all render targets!");
	for (DWORD i = 0; i < MaxRTNum; i++) {
		dxDumpRenderTarget(p_pd3dDevice, logger, i);
	}
}
