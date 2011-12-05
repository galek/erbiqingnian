import sys
import string
from xml.dom import ext
from xml.dom.ext.reader import PyExpat
from xml.dom.NodeFilter import NodeFilter
from xml.dom import Node  

#
# TODO
# dump zbuffer
# dump stencil buffer
# D3DTSS_TEXCOORDINDEX
# html output (ms-help://MS.VSCC.2003/MS.DirectX9.1033/DirectX9_c/directx/graphics/reference/d3d/enums/d3drenderstatetype.htm#D3DRS_VERTEXBLEND), texturakra
# IntelliDump
#

# visszaadja a kapott node nev nevu gyereket 
def belep(node, nev):
	children=node.childNodes
	for a in children:
		if a.nodeName==nev and a.nodeType==Node.ELEMENT_NODE:
			return a

# kiirja a kapott node gyermekeit
def printchildren(node):
	children=node.childNodes
	for a in children:
		if a.nodeType==Node.ELEMENT_NODE:
			print a.nodeName,' ', a.nodeType,' ', a.nodeValue

# megmondja, hogy a kapott node tipus tipusu-e
def szur(node, tipus): return node.nodeName==tipus

def renderstate(node): return szur(node, "RenderState")
def value(node): return szur(node, "Value")
def samplerstate(node): return szur(node, "SamplerState")
def texturestage(node): return szur(node, "TextureStageState")
def istype(node): return szur(node, "Type")
def errorcode(node): return szur(node, "ErrorCode")

xmlfile="dx9RenderStates.xml"
typefile="dx9Types.xml"
errorfile="dx9ErrorCodes.xml"

# beolvassuk az XML file-t
print "Reading XML files...",
reader = PyExpat.Reader()
doc = reader.fromUri(xmlfile)
doc2 = reader.fromUri(typefile)
doc3 = reader.fromUri(errorfile)
print "OK"

f=open("dxdump.cpp", 'w') 

# belepunk oda, ahol az erdekes node-ok vannak
ndx9Dump=belep(doc, "dx9Dump") 
nRenderStates=belep(ndx9Dump, "RenderStates") 
nSamplerStates=belep(ndx9Dump, "SamplerStates") 
nTextureStages=belep(ndx9Dump, "TextureStageStates") 


dxtypes = {}
nTypes=belep(doc2, "dx9Types")
nlTypes = filter(istype, nTypes.childNodes)
for i in nlTypes:
	dxtypes[i.attributes[(None,"name")].nodeValue] = i

f.write("#include \"dxdump.h\"\n")
f.write("#include <direct.h>\n")
f.write("#include <string>\n")
f.write("\n")
f.write("using namespace std;\n")
f.write("\n")
f.write("static int initialized = 0;\n")
f.write("static int frameNum = 0;\n")
f.write("static DWORD MaxTSSNum;\n")
f.write("static DWORD MaxRTNum;\n")
f.write("\n")
f.write("void initDxDump(IDirect3DDevice9* p_pd3dDevice) {\n")
f.write("	D3DCAPS9 caps;\n")
f.write("	p_pd3dDevice->GetDeviceCaps(&caps);\n")
f.write("	MaxTSSNum = caps.MaxTextureBlendStages;\n")
f.write("	MaxRTNum = caps.NumSimultaneousRTs;\n")
f.write("\n")
f.write("	initialized = 1;\n")
f.write("}\n")
f.write("\n")
f.write("void dxDumpAll(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*)) {\n")
f.write("	dxDumpRenderStates(p_pd3dDevice, logger);\n")
f.write("	dxDumpTextureStageStates(p_pd3dDevice, logger);\n")
f.write("	dxDumpSamplerStates(p_pd3dDevice, logger);\n")
f.write("	dxDumpAllTextures(p_pd3dDevice, logger);\n")
f.write("	dxDumpAllRenderTargets(p_pd3dDevice, logger);\n")
f.write("	frameNum++;\n")
f.write("}\n")
f.write("\n")
f.write("void logDword(string &msg, const char *name, DWORD value) {\n")
f.write("char buf[1024];\n")
f.write("\n")
f.write("	sprintf(buf, \"%s = 0x%08X\", name, value);\n")
f.write("	msg += buf;\n")
f.write("	\n")
f.write("}\n")
f.write("\n")
f.write("void logInt(string &msg, const char *name, DWORD value) {\n")
f.write("char buf[1024];\n")
f.write("\n")
f.write("	sprintf(buf, \"%s = %d\", name, value);\n")
f.write("	msg += buf;\n")
f.write("	\n")
f.write("}\n")
f.write("\n")
f.write("void logFloat(string &msg, const char *name, DWORD value) {\n")
f.write("char buf[1024];\n")
f.write("\n")
f.write("	sprintf(buf, \"%s = %f\", name, *((float*)&value));\n")
f.write("	msg += buf;\n")
f.write("	\n")
f.write("}\n")
f.write("void dxDumpRenderStates(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*)) {\n")
f.write("string msg;\n")
f.write("logger(\"================================ dxDumpRenderStates start ========================\");\n")
f.write("logger(\"RenderStates\");\n")

nlRenderStates = filter(renderstate, nRenderStates.childNodes)
for i in nlRenderStates:
	rsname = i.attributes[(None,"name")].nodeValue
	_rsname = "_" + rsname
	addr_rsname = "&" + _rsname
	mytype = i.attributes[(None,"type")].nodeValue
	typenode = dxtypes[mytype]
	subtype = typenode.attributes[(None,"subtype")].nodeValue
	
	f.write("\n")
	f.write("	msg = \"\\t\";\n")
	f.write("	DWORD _" + str(rsname) + ";\n")
	f.write("	p_pd3dDevice->GetRenderState(" + str(rsname) + ", " + str(addr_rsname) + ");\n")

	# ----------------- ENUM ----------------------
	if subtype == "ENUM":
		f.write("	msg += \"" + str(rsname) + " = \";\n")
		f.write("	switch(" + str(_rsname) + ") {\n")
		nlValues = filter(value, typenode.childNodes)
		for j in nlValues:
			rsvalue = j.attributes[(None,"name")].nodeValue
			f.write("		case " + str(rsvalue) + ":\n")
			f.write("			msg += \"" + str(rsvalue) + "\";\n")
			f.write("			break;\n")
			
		f.write("		default:\n")
		f.write("			msg += \"?\";\n")
		f.write("			break;\n")
		f.write("	}\n")
		f.write("	logger(msg.c_str());\n")

	# ----------------- DWORD ----------------------
	if subtype == "DWORD":
		f.write("	logDword(msg, \"" + str(rsname) + "\", " + str(_rsname) + ");\n")
		f.write("	logger(msg.c_str());\n")

	# ----------------- int ----------------------
	if subtype == "INT":
		f.write("	logInt(msg, \"" + str(rsname) + "\", " + str(_rsname) + ");\n")
		f.write("	logger(msg.c_str());\n")

	# ----------------- float ----------------------
	if subtype == "FLOAT":
		f.write("	logFloat(msg, \"" + str(rsname) + "\", " + str(_rsname) + ");\n")
		f.write("	logger(msg.c_str());\n")

	# ----------------- BITFIELD ----------------------
	if subtype == "BITFIELD":
		f.write("	msg += \"" + str(rsname) + " =\";\n")
		nlValues = filter(value, typenode.childNodes)
		for j in nlValues:
			rsvalue = j.attributes[(None,"name")].nodeValue
			f.write("	if ( " + str(_rsname) + " & " + str(rsvalue) + ")\n")
			f.write("		msg += \" " + str(rsvalue) + "\";\n")
		f.write("	logger(msg.c_str());\n")
f.write("\n")
f.write("logger(\"================================ dxDumpRenderStates end ========================\");\n")
f.write("}\n")
f.write("\n")
f.write("\n")

#---------TEXTURESTAGESTATES------------------------------------------------------------------------------------------------------


f.write("void dxDumpTextureStageStates(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*)) {\n")
f.write("string msg;\n")
f.write("logger(\"================================ dxDumpTextureStageStates start ========================\");\n")
f.write("logger(\"TextureStageStates\");\n")
f.write("if (!initialized) logger (\"dxDump wasn't initialized! unable to dump texture stage states!\");\n")

nlTextureStages = filter(texturestage, nTextureStages.childNodes)

for i in nlTextureStages:
	tsname = i.attributes[(None,"name")].nodeValue
	f.write("	DWORD _" + str(tsname) + ";\n")

f.write("\n")
f.write("\n")

f.write("DWORD TSSNum = 0;\n")
f.write("int wasDisabled = 0;\n")
f.write("while (TSSNum < MaxTSSNum && !wasDisabled) {\n")
f.write("	char buf[1024];\n")
f.write("	sprintf(buf, \"	TextureStage %d\", TSSNum);\n")
f.write("	logger(buf);\n")
for i in nlTextureStages:
	tsname = i.attributes[(None,"name")].nodeValue
	_tsname = "_" + tsname
	addr_tsname = "&" + _tsname
	mytype = i.attributes[(None,"type")].nodeValue
	typenode = dxtypes[i.attributes[(None,"type")].nodeValue]
	subtype = typenode.attributes[(None,"subtype")].nodeValue

	f.write("\n")
	f.write("	msg = \"\\t\\t\";\n")
	f.write("	p_pd3dDevice->GetTextureStageState(TSSNum, " + str(tsname) + ", " + str(addr_tsname) + ");\n")

	# ----------------- ENUM ----------------------
	if subtype == "ENUM":
		f.write("	msg += \"" + str(tsname) + " = \";\n")
		f.write("	switch(" + str(_tsname) + ") {\n")
		nlValues = filter(value, typenode.childNodes)
		for j in nlValues:
			tsvalue = j.attributes[(None,"name")].nodeValue
			f.write("		case " + str(tsvalue) + ":\n")
			f.write("			msg += \"" + str(tsvalue) + "\";\n")
			f.write("			break;\n")
			
		f.write("		default:\n")
		f.write("			msg += \"?\";\n")
		f.write("			break;\n")
		f.write("	}\n")
		f.write("	logger(msg.c_str());\n")

	# ----------------- D3DTA ----------------------
	if subtype == "D3DTA":
		f.write("	msg += \"" + str(tsname) + " =\";\n")
		f.write("	switch(" + str(_tsname) + " & D3DTA_SELECTMASK) {\n")
		nlValues = filter(value, typenode.childNodes)
		for j in nlValues:
			tsvalue = j.attributes[(None,"name")].nodeValue
			f.write("		case " + str(tsvalue) + ":\n")
			f.write("			msg += \" " + str(tsvalue) + "\";\n")
			f.write("			break;\n")
			
		f.write("		default:\n")
		f.write("			msg += \"?\";\n")
		f.write("			break;\n")
		f.write("	}\n")
		typenode = dxtypes["D3DTA_supp"]
		nlValues = filter(value, typenode.childNodes)
		for j in nlValues:
			tsvalue = j.attributes[(None,"name")].nodeValue
			f.write("	if (" + str(_tsname) + " & " + str(tsvalue) + ")\n")
			f.write("		msg += \" " + str(tsvalue) + "\";				\n")
		f.write("	logger(msg.c_str());\n")


	# ----------------- DWORD ----------------------
	if subtype == "DWORD":
		f.write("	logDword(msg, \"" + str(tsname) + "\", " + str(_tsname) + ");\n")
		f.write("	logger(msg.c_str());\n")

	# ----------------- int ----------------------
	if subtype == "INT":
		f.write("	logInt(msg, \"" + str(tsname) + "\", " + str(_tsname) + ");\n")
		f.write("	logger(msg.c_str());\n")

	# ----------------- float ----------------------
	if subtype == "FLOAT":
		f.write("	logFloat(msg, \"" + str(tsname) + "\", " + str(_tsname) + ");\n")
		f.write("	logger(msg.c_str());\n")

	# ----------------- BITFIELD ----------------------
	if subtype == "BITFIELD":
		f.write("	msg += \"" + str(tsname) + " =\";\n")
		nlValues = filter(value, i.childNodes)
		for j in nlValues:
			tsvalue = typenode.attributes[(None,"name")].nodeValue
			f.write("	if ( " + str(_tsname) + " & " + str(tsvalue) + ")\n")
			f.write("		msg += \" " + str(tsvalue) + "\";\n")
		f.write("	logger(msg.c_str());\n")
f.write("\n")
f.write("	TSSNum++;\n")
f.write("	if ( (_D3DTSS_COLOROP == D3DTOP_DISABLE) && (_D3DTSS_ALPHAOP == D3DTOP_DISABLE) )\n")
f.write("		wasDisabled = 1;\n")
f.write("}\n")
f.write("logger(\"================================ dxDumpTextureStageStates end ========================\");\n")
f.write("}\n")
f.write("\n")
f.write("\n")

#---------SAMPLERSTATES------------------------------------------------------------------------------------------------------

f.write("void dxDumpSamplerStates(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*)) {\n")
f.write("string msg;\n")
f.write("logger(\"================================ dxDumpSamplerStates start ========================\");\n")
f.write("logger(\"SamplerStates\");\n")
f.write("if (!initialized) logger (\"dxDump wasn't initialized! unable to dump sampler states!\");\n")

nlSamplerStates = filter(samplerstate, nSamplerStates.childNodes)

for i in nlSamplerStates:
	ssname = i.attributes[(None,"name")].nodeValue
	f.write("	DWORD _" + str(ssname) + ";\n")

f.write("\n")
f.write("\n")

f.write("DWORD samplerNum = 0;\n")
f.write("while (samplerNum < MaxTSSNum) {\n")
f.write("	char buf[1024];\n")
f.write("	sprintf(buf, \"	Sampler %d\", samplerNum);\n")
f.write("	logger(buf);\n")
for i in nlSamplerStates:
	ssname = i.attributes[(None,"name")].nodeValue
	_ssname = "_" + ssname
	addr_ssname = "&" + _ssname
	mytype = i.attributes[(None,"type")].nodeValue
	typenode = dxtypes[mytype]
	subtype = typenode.attributes[(None,"subtype")].nodeValue

	f.write("\n")
	f.write("	msg = \"\\t\\t\";\n")
	f.write("	p_pd3dDevice->GetSamplerState(samplerNum , " + str(ssname) + ", " + str(addr_ssname) + ");\n")

	# ----------------- ENUM ----------------------
	if subtype == "ENUM":
		f.write("	msg += \"" + str(ssname) + " = \";\n")
		f.write("	switch(" + str(_ssname) + ") {\n")
		nlValues = filter(value, typenode.childNodes)
		for j in nlValues:
			ssvalue = j.attributes[(None,"name")].nodeValue
			f.write("		case " + str(ssvalue) + ":\n")
			f.write("			msg += \"" + str(ssvalue) + "\";\n")
			f.write("			break;\n")
			
		f.write("		default:\n")
		f.write("			msg += \"?\";\n")
		f.write("			break;\n")
		f.write("	}\n")
		f.write("	logger(msg.c_str());\n")

	# ----------------- DWORD ----------------------
	if subtype == "DWORD":
		f.write("	logDword(msg, \"" + str(ssname) + "\", " + str(_ssname) + ");\n")
		f.write("	logger(msg.c_str());\n")

	# ----------------- int ----------------------
	if subtype == "INT":
		f.write("	logInt(msg, \"" + str(ssname) + "\", " + str(_ssname) + ");\n")
		f.write("	logger(msg.c_str());\n")

	# ----------------- float ----------------------
	if subtype == "FLOAT":
		f.write("	logFloat(msg, \"" + str(ssname) + "\", " + str(_ssname) + ");\n")
		f.write("	logger(msg.c_str());\n")
f.write("samplerNum++;\n")
f.write("}\n")
f.write("logger(\"================================ dxDumpSamplerStates end ========================\");\n")
f.write("}\n")
f.write("\n")
f.write("\n")

ndx9ErrorCodes=belep(doc3, "dx9ErrorCodes") 
nlErrorCodes = filter(errorcode, ndx9ErrorCodes.childNodes)
f.write("string dxError2Errname(HRESULT p_hr) {\n")
f.write("	switch (p_hr) {\n")
for i in nlErrorCodes:
	errname = i.attributes[(None,"name")].nodeValue
	f.write("		case " + str(errname) + ": return \"" + str(errname) + "\"; break;\n")
f.write("		default: return \"?\"; break;\n")
f.write("	}\n")
f.write("}\n")
f.write("string dxError2Explanation(HRESULT p_hr) {\n")
f.write("	switch (p_hr) {\n")
for i in nlErrorCodes:
	errname = i.attributes[(None,"name")].nodeValue
	errexplanation = i.attributes[(None,"explanation")].nodeValue
	f.write("		case " + str(errname) + ": return \"" + str(errexplanation) + "\"; break;\n")
f.write("		default: return \"?\"; break;\n")
f.write("	}\n")
f.write("}\n")
f.write("\n")
f.write("void dxDumpNextFrame(void) {\n")
f.write("	frameNum++;\n")
f.write("}\n")
f.write("\n")
f.write("void dxDumpTexture(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*), int stageNum) {\n")
f.write("	LPDIRECT3DBASETEXTURE9 pTexture = NULL;\n")
f.write("	char buf[1024], buf2[1024];\n")
f.write("	string frameDirName;\n")
f.write("	HRESULT hr;\n")
f.write("	hr = p_pd3dDevice->GetTexture(stageNum, &pTexture);\n")
f.write("	if (hr == D3D_OK) {\n")
f.write("		_mkdir(dirname);\n")
f.write("		sprintf(buf, \"%d\", stageNum);\n")
f.write("		sprintf(buf2, \"%07d\", frameNum);\n")
f.write("		frameDirName = string(dirname) + \"\\\\\" + string(buf2);\n")
f.write("		_mkdir(frameDirName.c_str());\n")
f.write("		string filename = string(frameDirName) + \"\\\\t\" + string(buf) + \".bmp\";\n")
f.write("		hr = D3DXSaveTextureToFile(filename.c_str(), D3DXIFF_BMP, pTexture, NULL);\n")
f.write("		if (hr == D3D_OK) {\n")
f.write("			string msg = \"Texture from stage \" + string(buf) + \" saved to \" + string(filename);\n")
f.write("			logger(msg.c_str());\n")
f.write("		}\n")
f.write("		else {\n")
f.write("			logger(\"D3DXSaveTextureToFile failed in dxDumpTexture\");\n")
f.write("		}\n")
f.write("	}\n")
f.write("	else {\n")
f.write("		logger(\"GetTexture failed in dxDumpTexture\");\n")
f.write("	}\n")
f.write("}\n")
f.write("\n")
f.write("void dxDumpAllTextures(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*)) {\n")
f.write("	if (!initialized) logger (\"dxDump wasn't initialized! unable to dump all textures!\");\n")
f.write("	for (DWORD i = 0; i < MaxTSSNum; i++) {\n")
f.write("		dxDumpTexture(p_pd3dDevice, logger, i);\n")
f.write("	}\n")
f.write("}\n")
f.write("\n")
f.write("void dxDumpRenderTarget(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*), int targetNum) {\n")
f.write("	LPDIRECT3DSURFACE9 pRenderTarget = NULL, pInMemTarget = NULL;\n")
f.write("	D3DSURFACE_DESC surfDesc;\n")
f.write("	HRESULT hr;\n")
f.write("	char buf[1024], buf2[1024];\n")
f.write("	string frameDirName;\n")
f.write("\n")
f.write("	hr = p_pd3dDevice->GetRenderTarget(targetNum, &pRenderTarget);\n")
f.write("	if (hr != D3D_OK) {\n")
f.write("		logger(\"GetRenderTarget failed in dxDumpRenderTarget\");\n")
f.write("		return;\n")
f.write("	}\n")
f.write("\n")
f.write("	hr = pRenderTarget->GetDesc(&surfDesc);\n")
f.write("	if (hr != D3D_OK) {\n")
f.write("		logger(\"GetDesc failed in dxDumpRenderTarget\");\n")
f.write("		return;\n")
f.write("	}\n")
f.write("\n")
f.write("	hr = p_pd3dDevice->CreateOffscreenPlainSurface(surfDesc.Width, surfDesc.Height, surfDesc.Format, D3DPOOL_SYSTEMMEM, &pInMemTarget, NULL);\n")
f.write("	if (hr != D3D_OK) {\n")
f.write("		logger(\"CreateOffscreenPlainSurface failed in dxDumpRenderTarget\");\n")
f.write("		return;\n")
f.write("	}\n")
f.write("\n")
f.write("	hr = p_pd3dDevice->GetRenderTargetData(pRenderTarget, pInMemTarget);\n")
f.write("	if (hr != D3D_OK) {\n")
f.write("		logger(\"GetRenderTargetData failed in dxDumpRenderTarget\");\n")
f.write("		return;\n")
f.write("	}\n")
f.write("\n")
f.write("	_mkdir(dirname);\n")
f.write("	sprintf(buf, \"%d\", targetNum);\n")
f.write("	sprintf(buf2, \"%07d\", frameNum);\n")
f.write("	frameDirName = string(dirname) + \"\\\\\" + string(buf2);\n")
f.write("	_mkdir(frameDirName.c_str());\n")
f.write("	string filename = string(frameDirName) + \"\\\\r\" + string(buf) + \".bmp\";\n")
f.write("\n")
f.write("	hr = D3DXSaveSurfaceToFile(filename.c_str(), D3DXIFF_BMP, pInMemTarget, NULL, NULL);\n")
f.write("		if (hr == D3D_OK) {\n")
f.write("			string msg = \"Render target \" + string(buf) + \" saved to \" + string(filename);\n")
f.write("			logger(msg.c_str());\n")
f.write("		}\n")
f.write("	else {\n")
f.write("		logger(\"D3DXSaveSurfaceToFile failed in dxDumpRenderTarget\");\n")
f.write("		return;\n")
f.write("	}	\n")
f.write("}\n")
f.write("\n")
f.write("void dxDumpAllRenderTargets(IDirect3DDevice9* p_pd3dDevice, void (*logger)(const char*)) {\n")
f.write("	if (!initialized) logger (\"dxDump wasn't initialized! unable to dump all render targets!\");\n")
f.write("	for (DWORD i = 0; i < MaxRTNum; i++) {\n")
f.write("		dxDumpRenderTarget(p_pd3dDevice, logger, i);\n")
f.write("	}\n")
f.write("}\n")
f.close() 

print "Cleaning up...",
reader.releaseNode(doc)
reader.releaseNode(doc2)
reader.releaseNode(doc3)
print "OK"
