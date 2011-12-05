#ifndef __DX10_DEBUG_UTILS__
#define __DX10_DEBUG_UTILS__
#include <d3d10.h>
#include "dxC_device.h"

inline void dx10_SaveRTVToDDS( ID3D10RenderTargetView* pRTV, LPCTSTR pFileName )
{
	ASSERT( pRTV && pFileName );
	ID3D10Texture2D* pRes = NULL;

	pRTV->GetResource( (ID3D10Resource**)&pRes );
	ASSERT( pRes );

	dxC_SaveTextureToFile( pFileName, D3DX10_IFF_DDS, pRes, false );
}

inline void dx10_PrintResourceDetails( ID3D10Resource* pRes, char* saveResource = NULL )
{
	ASSERT( pRes );
	D3D10_RESOURCE_DIMENSION viewDim;
	pRes->GetType( &viewDim );
	if( viewDim == D3D10_RESOURCE_DIMENSION_TEXTURE2D )
	{
		ID3D10Texture2D* pTexture = NULL;
		pRes->QueryInterface( __uuidof( ID3D10Texture2D ), (void**)&pTexture );
		ASSERT( pTexture );

		D3D10_TEXTURE2D_DESC texDesc;
		pTexture->GetDesc( &texDesc );
		LogMessage( "Texture array count %d %d x %d", texDesc.ArraySize, texDesc.Width, texDesc.Height );
		if( saveResource )
		{
			LogMessage( "Saving resource to %s...", saveResource );
			dxC_SaveTextureToFile( saveResource, D3DX10_IFF_DDS, pTexture );
		}
	}
}

inline void dx10_DebugBuffer( ID3D10Buffer* pBuffer, void** ppOutputBuff)
{
	D3D10_BUFFER_DESC bDesc;

	ASSERT( pBuffer );
	ASSERT( ppOutputBuff );

	pBuffer->GetDesc( &bDesc );
	bDesc.BindFlags = 0;
	bDesc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
	bDesc.Usage = D3D10_USAGE_STAGING;

	ID3D10Buffer* pTempBuff;
	dxC_GetDevice()->CreateBuffer( &bDesc, NULL, &pTempBuff );
	dxC_GetDevice()->CopyResource( pTempBuff, pBuffer );

	void* pData;
	*ppOutputBuff = malloc( bDesc.ByteWidth );
	pTempBuff->Map( D3D10_MAP_READ, 0, &pData );
	memcpy( *ppOutputBuff, pData, bDesc.ByteWidth );
	pTempBuff->Unmap();
	pTempBuff->Release();

}
#endif //__DX10_DEBUG_UTILS__