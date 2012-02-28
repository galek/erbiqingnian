
#include "D3D9RenderMaterial.h"

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderMaterialDesc.h>
#include "D3D9RenderTexture2D.h"
#include <stdio.h>

_NAMESPACE_BEGIN

extern char gShadersDir[];

void D3D9RenderMaterial::setModelMatrix(const scalar *matrix)
{
	if ( m_instancedVertexConstants.table && m_instancedVertexConstants.modelMatrix )
		m_instancedVertexConstants.table->SetMatrix(m_renderer.getD3DDevice(),m_instancedVertexConstants.modelMatrix,(const D3DXMATRIX *)matrix);
}

D3D9RenderMaterial::ShaderConstants::ShaderConstants(void)
{
	memset(this, 0, sizeof(*this));
}

D3D9RenderMaterial::ShaderConstants::~ShaderConstants(void)
{
	if(table) table->Release();
}

static D3DXHANDLE getShaderConstantByName(ID3DXConstantTable &table, const char *name)
{
	D3DXHANDLE found = 0;
	D3DXCONSTANTTABLE_DESC desc;
	table.GetDesc(&desc);
	for(UINT i=0; i<desc.Constants; i++)
	{
		D3DXHANDLE constant = table.GetConstant(0, i);
		ph_assert2(constant, "Unable to find constant");
		if(constant)
		{
			D3DXCONSTANT_DESC cdesc;
			UINT              count=1;
			table.GetConstantDesc(constant, &cdesc, &count);
			ph_assert2(count==1, "Unable to obtain Constant Descriptor.");
			if(count==1 && !strcmp(cdesc.Name, name))
			{
				found = constant;
				break;
			}
		}
	}
	return found;
}

static D3DBLEND getD3DBlendFunc(RenderMaterial::BlendFunc func)
{
	D3DBLEND d3dfunc = D3DBLEND_FORCE_DWORD;
	switch(func)
	{
		case RenderMaterial::BLEND_ZERO:                d3dfunc = D3DBLEND_ZERO;         break;
		case RenderMaterial::BLEND_ONE:                 d3dfunc = D3DBLEND_ONE;          break;
		case RenderMaterial::BLEND_SRC_COLOR:           d3dfunc = D3DBLEND_SRCCOLOR;     break;
		case RenderMaterial::BLEND_ONE_MINUS_SRC_COLOR: d3dfunc = D3DBLEND_INVSRCCOLOR;  break;
		case RenderMaterial::BLEND_SRC_ALPHA:           d3dfunc = D3DBLEND_SRCALPHA;     break;
		case RenderMaterial::BLEND_ONE_MINUS_SRC_ALPHA: d3dfunc = D3DBLEND_INVSRCALPHA;  break;
		case RenderMaterial::BLEND_DST_ALPHA:           d3dfunc = D3DBLEND_DESTALPHA;    break;
		case RenderMaterial::BLEND_ONE_MINUS_DST_ALPHA: d3dfunc = D3DBLEND_INVDESTALPHA; break;
		case RenderMaterial::BLEND_DST_COLOR:           d3dfunc = D3DBLEND_DESTCOLOR;    break;
		case RenderMaterial::BLEND_ONE_MINUS_DST_COLOR: d3dfunc = D3DBLEND_INVDESTCOLOR; break;
		case RenderMaterial::BLEND_SRC_ALPHA_SATURATE:  d3dfunc = D3DBLEND_SRCALPHASAT;  break;
	}
	ph_assert2(d3dfunc!=D3DBLEND_FORCE_DWORD, "Failed to look up D3D Blend Func.");
	return d3dfunc;
}

void D3D9RenderMaterial::ShaderConstants::loadConstants(void)
{
	if(table)
	{
		modelMatrix           = table->GetConstantByName(0, "g_" "modelMatrix");
		viewMatrix            = table->GetConstantByName(0, "g_" "viewMatrix");
		projMatrix            = table->GetConstantByName(0, "g_" "projMatrix");
		modelViewMatrix       = table->GetConstantByName(0, "g_" "modelViewMatrix");
		modelViewProjMatrix   = table->GetConstantByName(0, "g_" "modelViewProjMatrix");
		viewProjMatrix		  = table->GetConstantByName(0, "g_" "viewProjMatrix");
		
		boneMatrices          = getShaderConstantByName(*table, "g_" "boneMatrices");

		eyePosition           = table->GetConstantByName(0, "g_" "eyePosition");
		eyePositionObjSpace   = table->GetConstantByName(0, "g_" "eyePositionObjSpace");
		eyeDirection          = table->GetConstantByName(0, "g_" "eyeDirection");
		
		ambientColor          = table->GetConstantByName(0, "g_" "ambientColor");

		lightColor            = table->GetConstantByName(0, "g_" "lightColor");
		lightIntensity        = table->GetConstantByName(0, "g_" "lightIntensity");
		lightDirection        = table->GetConstantByName(0, "g_" "lightDirection");
		lightPosition         = table->GetConstantByName(0, "g_" "lightPosition");
		lightPositionObjSpace = table->GetConstantByName(0, "g_" "lightPositionObjSpace");
		lightInnerRadius      = table->GetConstantByName(0, "g_" "lightInnerRadius");
		lightOuterRadius      = table->GetConstantByName(0, "g_" "lightOuterRadius");
		lightInnerCone        = table->GetConstantByName(0, "g_" "lightInnerCone");
		lightOuterCone        = table->GetConstantByName(0, "g_" "lightOuterCone");
		
		lightShadowMap        = table->GetConstantByName(0, "g_" "lightShadowMap");
		lightShadowMatrix     = table->GetConstantByName(0, "g_" "lightShadowMatrix");
		
		vfaceScale            = table->GetConstantByName(0, "g_" "vfaceScale");
	}
}

void D3D9RenderMaterial::ShaderConstants::bindEnvironment(IDirect3DDevice9 &d3dDevice, const D3D9Render::ShaderEnvironment &shaderEnv) const
{
	if(table)
	{
		#define SET_MATRIX(_name)                         \
		    if(_name)                                     \
		    {                                             \
		        const D3DXMATRIX xm(shaderEnv._name);     \
		        table->SetMatrix(&d3dDevice, _name, &xm); \
		    }
		
		#define SET_FLOAT3(_name)                            \
		    if(_name)                                        \
		    {                                                \
		        const D3DXVECTOR4 xv(shaderEnv._name[0],     \
		                             shaderEnv._name[1],     \
		                             shaderEnv._name[2], 1); \
		        table->SetVector(&d3dDevice, _name, &xv);    \
		    }

		#define SET_FLOAT4(_name)                            	\
			if(_name)                                        	\
			{                                                	\
				const D3DXVECTOR4 xv(shaderEnv._name[0],     	\
				shaderEnv._name[1],								\
				shaderEnv._name[2], shaderEnv._name[3]);		\
				table->SetVector(&d3dDevice, _name, &xv);		\
			}

		#define SET_COLOR(_name)                                \
		    if(_name)                                           \
		    {                                                   \
				const D3DXCOLOR xc(shaderEnv._name);            \
		        const D3DXVECTOR4 xv(xc.r, xc.g, xc.b, xc.a);   \
		        table->SetVector(&d3dDevice, _name, &xv);       \
		    }
		
		#define SET_FLOAT(_name) \
		     if(_name) table->SetFloat(&d3dDevice, _name, shaderEnv._name);
		
		SET_MATRIX(modelMatrix)
		SET_MATRIX(viewMatrix)
		SET_MATRIX(projMatrix)
		SET_MATRIX(modelViewMatrix)
		SET_MATRIX(modelViewProjMatrix)
		SET_MATRIX(viewProjMatrix)

		if(boneMatrices && shaderEnv.numBones > 0)
		{
			table->SetMatrixArray(&d3dDevice, boneMatrices, shaderEnv.boneMatrices, shaderEnv.numBones);
		}
		
		SET_FLOAT3(eyePosition)
		SET_FLOAT4(eyePositionObjSpace)
		SET_FLOAT3(eyeDirection)
		
		SET_FLOAT3(ambientColor)
		
		SET_COLOR(lightColor)
		SET_FLOAT(lightIntensity)
		SET_FLOAT3(lightDirection)
		SET_FLOAT3(lightPosition)
		SET_FLOAT4(lightPositionObjSpace)
		SET_FLOAT(lightInnerRadius)
		SET_FLOAT(lightOuterRadius)
		SET_FLOAT(lightInnerCone)
		SET_FLOAT(lightOuterCone)
		if(lightShadowMap)
		{
			UINT index = table->GetSamplerIndex(lightShadowMap);
			d3dDevice.SetSamplerState((DWORD)index, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			d3dDevice.SetSamplerState((DWORD)index, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			d3dDevice.SetSamplerState((DWORD)index, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
			d3dDevice.SetSamplerState((DWORD)index, D3DSAMP_ADDRESSU,  D3DTADDRESS_CLAMP);
			d3dDevice.SetSamplerState((DWORD)index, D3DSAMP_ADDRESSV,  D3DTADDRESS_CLAMP);
			d3dDevice.SetTexture(index, shaderEnv.lightShadowMap);
		}
		SET_MATRIX(lightShadowMatrix)
		
		if(vfaceScale)
		{
			float vfs = 1.0f;
			DWORD cullmode = 0;
			d3dDevice.GetRenderState(D3DRS_CULLMODE, &cullmode);
			if(     cullmode == D3DCULL_CW)   vfs = -1.0f;
		#if defined(RENDERER_XBOX360)
			else if(cullmode == D3DCULL_NONE) vfs = -1.0f;
		#endif
			table->SetFloat(&d3dDevice, vfaceScale, vfs);
		}
		
		#undef SET_MATRIX
		#undef SET_FLOAT3
		#undef SET_COLOR
		#undef SET_FLOAT
	}
}

D3D9RenderMaterial::D3D9Variable::D3D9Variable(const char *name, VariableType type, uint32 offset) :
	Variable(name, type, offset)
{
	m_vertexHandle = 0;
	memset(m_fragmentHandles, 0, sizeof(m_fragmentHandles));
}

D3D9RenderMaterial::D3D9Variable::~D3D9Variable(void)
{
	
}

void D3D9RenderMaterial::D3D9Variable::addVertexHandle(ID3DXConstantTable &table, D3DXHANDLE handle)
{
	m_vertexHandle = handle;
	D3DXCONSTANT_DESC cdesc;
	UINT              count=1;
	table.GetConstantDesc(handle, &cdesc, &count);
	m_vertexRegister = cdesc.RegisterIndex;
}

void D3D9RenderMaterial::D3D9Variable::addFragmentHandle(ID3DXConstantTable &table, D3DXHANDLE handle, Pass pass)
{
	m_fragmentHandles[pass] = handle;
	D3DXCONSTANT_DESC cdesc;
	UINT              count=1;
	table.GetConstantDesc(handle, &cdesc, &count);
	m_fragmentRegisters[pass] = cdesc.RegisterIndex;
}

static void processCompileErrors(LPD3DXBUFFER errors)
{
#if defined(RENDERER_WINDOWS)
	if(errors)
	{
		const char *errorStr = (const char*)errors->GetBufferPointer();
		if(errorStr)
		{
			MessageBoxA(0, errorStr, "D3DXCompileShaderFromFile Error", MB_OK);
		}
	}
#endif
}

class D3D9ShaderIncluder : public ID3DXInclude
{
	private:
#if defined(RENDERER_XBOX360)
		STDMETHOD(Open)(THIS_ D3DXINCLUDE_TYPE includeType, LPCSTR fileName, LPCVOID parentData, LPCVOID *data, UINT *dataSize,LPSTR pFullPath, DWORD cbFullPath)
#else
		STDMETHOD(Open)(THIS_ D3DXINCLUDE_TYPE includeType, LPCSTR fileName, LPCVOID parentData, LPCVOID *data, UINT *dataSize)
#endif
		{
			HRESULT result = D3DERR_NOTFOUND;

			char fullpath[1024];
			strcpy_s(fullpath, 1024, gShadersDir);
			if(includeType == D3DXINC_SYSTEM)
			{
				strcat_s(fullpath, 1024, "include/");
			}
			strcat_s(fullpath, 1024, fileName);
			
			FILE *file = 0;
			fopen_s(&file, fullpath, "r");
			if(file)
			{
				fseek(file, 0, SEEK_END);
				size_t fileLen = ftell(file);
				if(fileLen > 1)
				{
					fseek(file, 0, SEEK_SET);
					char *fileData = new char[fileLen+1];
					fileLen = fread(fileData, 1, fileLen, file);
					fileData[fileLen] = 0;
					*data     = fileData;
					*dataSize = (UINT)fileLen;
				}
				fclose(file);
				result = D3D_OK;
			}
			ph_assert2(result == D3D_OK, "Failed to include shader header.");
			return result;
		}
		
		STDMETHOD(Close)(THIS_ LPCVOID data)
		{
			delete [] ((char*)data);
			return D3D_OK;
		}
};

D3D9RenderMaterial::D3D9RenderMaterial(D3D9Render &renderer, const RenderMaterialDesc &desc) :
	RenderMaterial(desc),
	m_renderer(renderer)
{
	m_d3dAlphaTestFunc      = D3DCMP_ALWAYS;
	m_vertexShader          = 0;
	m_instancedVertexShader = 0;
	memset(m_fragmentPrograms, 0, sizeof(m_fragmentPrograms));
	
	AlphaTestFunc alphaTestFunc = getAlphaTestFunc();
	switch(alphaTestFunc)
	{
		case ALPHA_TEST_ALWAYS:        m_d3dAlphaTestFunc = D3DCMP_ALWAYS;       break;
		case ALPHA_TEST_EQUAL:         m_d3dAlphaTestFunc = D3DCMP_EQUAL;        break;
		case ALPHA_TEST_NOT_EQUAL:     m_d3dAlphaTestFunc = D3DCMP_NOTEQUAL;     break;
		case ALPHA_TEST_LESS:          m_d3dAlphaTestFunc = D3DCMP_LESS;         break;
		case ALPHA_TEST_LESS_EQUAL:    m_d3dAlphaTestFunc = D3DCMP_LESSEQUAL;    break;
		case ALPHA_TEST_GREATER:       m_d3dAlphaTestFunc = D3DCMP_GREATER;      break;
		case ALPHA_TEST_GREATER_EQUAL: m_d3dAlphaTestFunc = D3DCMP_GREATEREQUAL; break;
		default:
			ph_assert2(0, "Unknown Alpha Test Func.");
	}
	
	m_d3dSrcBlendFunc = getD3DBlendFunc(getSrcBlendFunc());
	m_d3dDstBlendFunc = getD3DBlendFunc(getDstBlendFunc());
	
	D3D9Render::D3DXInterface &d3dx      = m_renderer.getD3DX();
	IDirect3DDevice9 *d3dDevice = m_renderer.getD3DDevice();
	if(d3dDevice)
	{
		D3D9ShaderIncluder shaderIncluder;
		
		const char *vertexEntry      = "vmain";
		const char *vertexProfile    = d3dx.GetVertexShaderProfile(d3dDevice);
		const char *vertexShaderPath = desc.vertexShaderPath;
		const DWORD vertexFlags      = D3DXSHADER_PACKMATRIX_COLUMNMAJOR;

		Array<ShaderDefines> defines;
		ShaderDefines def;
		def.define	= "RENDERER_VERTEX";
		def.value	= "1";
		defines.Append(def);
		defines.AppendArray(desc.extraDefines);
		D3DXMACRO* vertexDefines = new D3DXMACRO[defines.Size()+1];
		for (SizeT d = 0; d<defines.Size(); d++)
		{
			vertexDefines[d].Name = &defines[d].define[0];
			vertexDefines[d].Definition = &defines[d].value[0];
		}
		vertexDefines[defines.Size()].Name = 0;
		vertexDefines[defines.Size()].Definition = 0;

		LPD3DXBUFFER        vshader = 0;
		LPD3DXBUFFER        verrors = 0;
		HRESULT result = d3dx.CompileShaderFromFileA(vertexShaderPath, vertexDefines, &shaderIncluder, 
			vertexEntry, vertexProfile, vertexFlags, &vshader, &verrors, &m_vertexConstants.table);
		processCompileErrors(verrors);
		ph_assert2(result == D3D_OK && vshader, "Failed to compile Vertex Shader.");
		if(result == D3D_OK && vshader)
		{
			result = d3dDevice->CreateVertexShader((const DWORD*)vshader->GetBufferPointer(), &m_vertexShader);
			ph_assert2(result == D3D_OK && m_vertexShader, "Failed to load Vertex Shader.");
			if(result == D3D_OK && m_vertexShader)
			{
				m_vertexConstants.loadConstants();
				if(m_vertexConstants.table)
				{
					loadCustomConstants(*m_vertexConstants.table, NUM_PASSES);
				}
			}
		}
		if(vshader) 
		{
			vshader->Release();
		}
		if(verrors) 
		{
			verrors->Release();
		}

		Array<ShaderDefines> dins;
		dins.Append(ShaderDefines("RENDERER_VERTEX","1"));
		dins.Append(ShaderDefines("PX_WINDOWS","1"));
#if RENDERER_INSTANCING
		dins.Append(ShaderDefines("RENDERER_INSTANCED","1"));
#else
		dins.Append(ShaderDefines("RENDERER_INSTANCED","0"));
#endif
		dins.AppendArray(desc.extraDefines);

		D3DXMACRO* vertexDefinesInstanced = new D3DXMACRO[dins.Size()+1];
		for (SizeT d = 0; d<dins.Size(); d++)
		{
			vertexDefinesInstanced[d].Name = &dins[d].define[0];
			vertexDefinesInstanced[d].Definition = &dins[d].value[0];
		}
		vertexDefinesInstanced[dins.Size()].Name = 0;
		vertexDefinesInstanced[dins.Size()].Definition = 0;

		vshader = 0;
		verrors = 0;
		result = d3dx.CompileShaderFromFileA(vertexShaderPath, vertexDefinesInstanced,
			&shaderIncluder, vertexEntry, vertexProfile, vertexFlags, &vshader, &verrors, &m_instancedVertexConstants.table);
		processCompileErrors(verrors);
		ph_assert2(result == D3D_OK && vshader, "Failed to compile Vertex Shader.");
		if(result == D3D_OK && vshader)
		{
			result = d3dDevice->CreateVertexShader((const DWORD*)vshader->GetBufferPointer(), &m_instancedVertexShader);
			ph_assert2(result == D3D_OK && m_vertexShader, "Failed to load Vertex Shader.");
			if(result == D3D_OK && m_vertexShader)
			{
				m_instancedVertexConstants.loadConstants();
				if(m_instancedVertexConstants.table)
				{
					loadCustomConstants(*m_instancedVertexConstants.table, NUM_PASSES);
				}
			}
		}
		if(vshader)
		{
			vshader->Release();
		}
		if(verrors)
		{
			verrors->Release();
		}

		delete []vertexDefines;
		delete []vertexDefinesInstanced;
		
		const char *fragmentEntry      = "fmain";
		const char *fragmentProfile    = d3dx.GetPixelShaderProfile(d3dDevice);
		const char *fragmentShaderPath = desc.fragmentShaderPath;
		const DWORD fragmentFlags      = D3DXSHADER_PACKMATRIX_COLUMNMAJOR;
		for(uint32 i=0; i<NUM_PASSES; i++)
		{
			Array<ShaderDefines> defines;
			ShaderDefines def;
			def.define	= "RENDERER_VERTEX";
			def.value	= "1";
			defines.Append(ShaderDefines("RENDERER_FRAGMENT","1"));
			defines.Append(ShaderDefines(getPassName((Pass)i),"1"));
			defines.Append(ShaderDefines("ENABLE_VFACE","1"));
			defines.Append(ShaderDefines("ENABLE_VFACE_SCALE","1"));
			defines.Append(ShaderDefines("WIN32","1"));
			defines.Append(ShaderDefines("ENABLE_SHADOWS","1"));
			defines.AppendArray(desc.extraDefines);
			D3DXMACRO* fragmentDefines = new D3DXMACRO[defines.Size()+1];
			for (SizeT d = 0; d<defines.Size(); d++)
			{
				fragmentDefines[d].Name = &defines[d].define[0];
				fragmentDefines[d].Definition = &defines[d].value[0];
			}
			fragmentDefines[defines.Size()].Name = 0;
			fragmentDefines[defines.Size()].Definition = 0;

			LPD3DXBUFFER fshader = 0;
			LPD3DXBUFFER ferrors = 0;
			result = d3dx.CompileShaderFromFileA(fragmentShaderPath, fragmentDefines,
				&shaderIncluder, fragmentEntry, fragmentProfile, fragmentFlags, &fshader, &ferrors, &m_fragmentConstants[i].table);
			processCompileErrors(ferrors);
			ph_assert2(result == D3D_OK && fshader, "Failed to compile Fragment Shader.");
			if(result == D3D_OK && vshader)
			{
				result = d3dDevice->CreatePixelShader((const DWORD*)fshader->GetBufferPointer(), &m_fragmentPrograms[i]);
				ph_assert2(result == D3D_OK && m_fragmentPrograms[i], "Failed to load Fragment Shader.");
				if(result == D3D_OK && m_fragmentPrograms[i])
				{
					m_fragmentConstants[i].loadConstants();
					if(m_fragmentConstants[i].table)
					{
						loadCustomConstants(*m_fragmentConstants[i].table, (Pass)i);
					}
				}
			}
			if(fshader) 
			{
				fshader->Release();
			}
			if(ferrors) 
			{
				ferrors->Release();
			}
		}
	}
}

D3D9RenderMaterial::~D3D9RenderMaterial(void)
{
	if(m_vertexShader)          m_vertexShader->Release();
	if(m_instancedVertexShader) m_instancedVertexShader->Release();
	for(uint32 i=0; i<NUM_PASSES; i++)
	{
		IDirect3DPixelShader9 *fp = m_fragmentPrograms[i];
		if(fp) 
		{
			fp->Release();
		}
	}
}

void D3D9RenderMaterial::bind(RenderMaterial::Pass pass, RenderMaterialInstance *materialInstance, bool instanced) const
{
	IDirect3DDevice9 *d3dDevice = m_renderer.getD3DDevice();
	ph_assert2(pass < NUM_PASSES, "Invalid Material Pass.");
	if(d3dDevice && pass < NUM_PASSES)
	{
		const D3D9Render::ShaderEnvironment &shaderEnv = m_renderer.getShaderEnvironment();
		
		if(m_d3dAlphaTestFunc == D3DCMP_ALWAYS)
		{
			d3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
		}
		else
		{
			uint8 alphaTestRef = (uint8)(Math::Clamp(getAlphaTestRef(), 0.0f, 1.0f)*255.0f);
			d3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, 1);
			d3dDevice->SetRenderState(D3DRS_ALPHAFUNC,       (DWORD)m_d3dAlphaTestFunc);
			d3dDevice->SetRenderState(D3DRS_ALPHAREF ,       (DWORD)alphaTestRef);
		}
		
		if(getBlending())
		{
			d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
			d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE,     0);
			d3dDevice->SetRenderState(D3DRS_SRCBLEND,         (DWORD)m_d3dSrcBlendFunc);
			d3dDevice->SetRenderState(D3DRS_DESTBLEND,        (DWORD)m_d3dDstBlendFunc);
		}

		// µ¥Ë«Ãæ
		DWORD cullMode = D3DCULL_CCW;
		switch(m_cullMode)
		{
		case RenderMaterial::CLOCKWISE: 
			cullMode = D3DCULL_CCW;
			break;
		case RenderMaterial::COUNTER_CLOCKWISE: 
			cullMode = D3DCULL_CW;
			break;
		case RenderMaterial::NONE: 
			cullMode = D3DCULL_NONE;
			break;
		default:
			ph_assert2(0, "Invalid Cull Mode");
		}

		d3dDevice->SetRenderState(D3DRS_CULLMODE, cullMode);

		if(instanced)
		{
			d3dDevice->SetVertexShader(m_instancedVertexShader);
		}
		else
		{
			d3dDevice->SetVertexShader(m_vertexShader);
		}
		
		m_fragmentConstants[pass].bindEnvironment(*d3dDevice, shaderEnv);
		d3dDevice->SetPixelShader(m_fragmentPrograms[pass]);
		
		RenderMaterial::bind(pass, materialInstance, instanced);
	}
}

void D3D9RenderMaterial::bindMeshState(bool instanced) const
{
	IDirect3DDevice9 *d3dDevice = m_renderer.getD3DDevice();
	if(d3dDevice)
	{
		const D3D9Render::ShaderEnvironment &shaderEnv = m_renderer.getShaderEnvironment();
		
		if(instanced)
		{
			m_instancedVertexConstants.bindEnvironment(*d3dDevice, shaderEnv);
		}
		else
		{
			m_vertexConstants.bindEnvironment(*d3dDevice, shaderEnv);
		}
	}
}

void D3D9RenderMaterial::unbind(void) const
{
	IDirect3DDevice9 *d3dDevice = m_renderer.getD3DDevice();
	if(d3dDevice)
	{
		if(getBlending())
		{
			d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
			d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE,     1);
		}

		d3dDevice->SetVertexShader(0);
		d3dDevice->SetPixelShader(0);
	}
}

static void bindSampler2DVariable(IDirect3DDevice9 &d3dDevice, ID3DXConstantTable &table, D3DXHANDLE handle, RenderTexture2D &texture)
{
	if(handle)
	{
		UINT samplerIndex = table.GetSamplerIndex(handle);
		static_cast<D3D9RenderTexture2D*>(&texture)->bind(samplerIndex);
	}
}

void D3D9RenderMaterial::bindVariable(Pass pass, const Variable &variable, const void *data) const
{
	D3D9Variable &var = *(D3D9Variable*)&variable;
	IDirect3DDevice9 *d3dDevice = m_renderer.getD3DDevice();
	if(d3dDevice)
	{
		switch(var.getType())
		{
			case VARIABLE_FLOAT:
			{
				const float fdata[4] = {*(const float*)data, 0,0,0};
				if(var.m_vertexHandle)               d3dDevice->SetVertexShaderConstantF(var.m_vertexRegister,          fdata, 1);
				else if(var.m_fragmentHandles[pass]) d3dDevice->SetPixelShaderConstantF( var.m_fragmentRegisters[pass], fdata, 1);
				break;
			}
			case VARIABLE_FLOAT2:
			{
				const float fdata[4] = {((const float*)data)[0], ((const float*)data)[1],0,0};
				if(var.m_vertexHandle)               d3dDevice->SetVertexShaderConstantF(var.m_vertexRegister,          fdata, 1);
				else if(var.m_fragmentHandles[pass]) d3dDevice->SetPixelShaderConstantF( var.m_fragmentRegisters[pass], fdata, 1);
				break;
			}
			case VARIABLE_FLOAT3:
			{
				const float fdata[4] = {((const float*)data)[0], ((const float*)data)[1],((const float*)data)[2],0};
				if(var.m_vertexHandle)               d3dDevice->SetVertexShaderConstantF(var.m_vertexRegister,          fdata, 1);
				else if(var.m_fragmentHandles[pass]) d3dDevice->SetPixelShaderConstantF( var.m_fragmentRegisters[pass], fdata, 1);
				break;
			}
			case VARIABLE_FLOAT4:
			{
				const float fdata[4] = {((const float*)data)[0], ((const float*)data)[1],((const float*)data)[2],((const float*)data)[3]};
				if(var.m_vertexHandle)               d3dDevice->SetVertexShaderConstantF(var.m_vertexRegister,          fdata, 1);
				else if(var.m_fragmentHandles[pass]) d3dDevice->SetPixelShaderConstantF( var.m_fragmentRegisters[pass], fdata, 1);
				break;
			}
			case VARIABLE_SAMPLER2D:
				data = *(void**)data;
				ph_assert2(data, "NULL Sampler.");
				if(data)
				{
					bindSampler2DVariable(*m_renderer.getD3DDevice(), *m_vertexConstants.table,         var.m_vertexHandle,          *(RenderTexture2D*)data);
					bindSampler2DVariable(*m_renderer.getD3DDevice(), *m_fragmentConstants[pass].table, var.m_fragmentHandles[pass], *(RenderTexture2D*)data);
				}
				break;
		}
	}
}

static RenderMaterial::VariableType getVariableType(const D3DXCONSTANT_DESC &desc)
{
	RenderMaterial::VariableType vt = RenderMaterial::NUM_VARIABLE_TYPES;
	switch(desc.Type)
	{
		case D3DXPT_FLOAT:
			if(     desc.Rows == 4 && desc.Columns == 4) vt = RenderMaterial::VARIABLE_FLOAT4x4;
			else if(desc.Rows == 1 && desc.Columns == 1) vt = RenderMaterial::VARIABLE_FLOAT;
			else if(desc.Rows == 1 && desc.Columns == 2) vt = RenderMaterial::VARIABLE_FLOAT2;
			else if(desc.Rows == 1 && desc.Columns == 3) vt = RenderMaterial::VARIABLE_FLOAT3;
			else if(desc.Rows == 1 && desc.Columns == 4) vt = RenderMaterial::VARIABLE_FLOAT4;
			break;
		case D3DXPT_SAMPLER2D:
			vt = RenderMaterial::VARIABLE_SAMPLER2D;
			break;
	}
	ph_assert2(vt < RenderMaterial::NUM_VARIABLE_TYPES, "Unable to convert shader variable type.");
	return vt;
}

void D3D9RenderMaterial::loadCustomConstants(ID3DXConstantTable &table, Pass pass)
{
	D3DXCONSTANTTABLE_DESC desc;
	table.GetDesc(&desc);
	for(UINT i=0; i<desc.Constants; i++)
	{
		D3DXHANDLE constant = table.GetConstant(0, i);
		ph_assert2(constant, "Unable to find constant");
		if(constant)
		{
			D3DXCONSTANT_DESC cdesc;
			UINT              count = 1;
			table.GetConstantDesc(constant, &cdesc, &count);
			ph_assert(count == 1);
			if(count == 1 && strncmp(cdesc.Name, "g_", 2))
			{
				VariableType type = getVariableType(cdesc);
				if(type < NUM_VARIABLE_TYPES)
				{
					D3D9Variable *var = 0;
					// search to see if the variable already exists...
					uint32 numVariables = (uint32)m_variables.Size();
					for(uint32 i=0; i<numVariables; i++)
					{
						if (m_variables[i]->getName() == cdesc.Name)
						{
							var = static_cast<D3D9Variable*>(m_variables[i]);
							break;
						}
					}
					// check to see if the variable is of the same type.
					if(var)
					{
						ph_assert2(var->getType() == type, "Variable changes type!");
					}
					// if we couldn't find the variable... create a new variable...
					if(!var)
					{
						var = new D3D9Variable(cdesc.Name, type, m_variableBufferSize);
						m_variables.Append(var);
						m_variableBufferSize += var->getDataSize();
					}
					// add the handle to the variable...
					if(pass < NUM_PASSES)
					{
						var->addFragmentHandle(table, constant, pass);
					}
					else
					{
						var->addVertexHandle(table, constant);
					}
				}
			}
		}
	}
}

_NAMESPACE_END

#endif
