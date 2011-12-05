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