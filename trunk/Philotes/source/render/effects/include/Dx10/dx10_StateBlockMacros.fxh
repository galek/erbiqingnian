#ifndef __DX10_STATE_BLOCK_MACROS__
#define __DX10_STATE_BLOCK_MACROS__

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------
#define DEFINE_RASTERIZER_STATE_NAME( fillmode, cullmode, frontcounterclockwise, depthbias, depthbiasclamp, depthclipenable, scissorenable, multisampleenable, antialiasedlineenable ) RS_##fillmode##cullmode##frontcounterclockwise##depthbias##depthbiasclamp##depthclipenable##scissorenable##multisampleenable##antialiasedlineenable
#define DEFINE_BLEND_STATE_NAME( writemask, blendenable, srcblend, destblend, blendop, srcblendalpha, destblendalpha, blendopalpha, alphatocoverage ) BS_##writemask##blendenable##srcblend##destblend##blendop##srcblendalpha##destblendalpha##blendopalpha##alphatocoverage
#define DEFINE_DEPTHSTENCIL_STATE_NAME( depthenable, depthwritemask, depthfunc, stencilenable, stencilreadmask, stencilwritemask, frontfacestencilfail, frontfacestencildepthfail, frontfacestencilpass, frontfacestencilfunc, backfacestencilfail, backfacestencildepthfail, backfacestencilpass, backfacestencilfunc ) DSS_##depthenable##depthwritemask##depthfunc##stencilenable##stencilreadmask##stencilwritemask##frontfacestencilfail##frontfacestencildepthfail##frontfacestencilpass##frontfacestencilfunc##backfacestencilfail##backfacestencildepthfail##backfacestencilpass##backfacestencilfunc \

#define DEFINE_RASTERIZER_STATE_EX( name, fillmode, cullmode, frontcounterclockwise, depthbias, depthbiasclamp, depthclipenable, scissorenable, multisampleenable, antialiasedlineenable ) \
	RasterizerState name \
	{\
	FillMode					=	fillmode; \
	CullMode					=	cullmode; \
	FrontCounterClockWise		=	frontcounterclockwise; \
	DepthBias					=	depthbias; \
	DepthBiasClamp				=	depthbiasclamp; \
	DepthClipEnable				=	depthclipenable; \
	ScissorEnable				=	scissorenable; \
	MultisampleEnable			=	multisampleenable; \
	AntialiasedLineEnable		=	antialiasedlineenable; \
	};
	
/*
    RenderTargetWriteMask[1]	=	( ( writemask >> 1 ) & 0x0000000f ); \
    RenderTargetWriteMask[2]	=	( ( writemask >> 2 ) & 0x0000000f ); \
    RenderTargetWriteMask[3]	=	( ( writemask >> 3 ) & 0x0000000f ); \
    RenderTargetWriteMask[4]	=	( ( writemask >> 4 ) & 0x0000000f ); \
    RenderTargetWriteMask[5]	=	( ( writemask >> 5 ) & 0x0000000f ); \
    RenderTargetWriteMask[6]	=	( ( writemask >> 6 ) & 0x0000000f ); \
    RenderTargetWriteMask[7]	=	( ( writemask >> 7 ) & 0x0000000f ); \
*/

/*
	BlendEnable[1]				=	( ( blendenable >> 1 )& 0x0000000f ); \
	BlendEnable[2]				=	( ( blendenable >> 2 )& 0x0000000f ); \
	BlendEnable[3]				=	( ( blendenable >> 3 )& 0x0000000f ); \
	BlendEnable[4]				=	( ( blendenable >> 4 )& 0x0000000f ); \
	BlendEnable[5]				=	( ( blendenable >> 5 )& 0x0000000f ); \
	BlendEnable[6]				=	( ( blendenable >> 6 )& 0x0000000f ); \
	BlendEnable[7]				=	( ( blendenable >> 7 )& 0x0000000f ); \
*/

#define DEFINE_BLEND_STATE_EX( name, writemask, blendenable, srcblend, destblend, blendop, srcblendalpha, destblendalpha, blendopalpha, alphatocoverage ) \
	BlendState name \
	{ \
    RenderTargetWriteMask[0]	=	writemask; \
    BlendEnable[0]				= blendenable; \
	SrcBlend					= srcblend; \
	DestBlend					= destblend; \
	BlendOp						= blendop; \
	SrcBlendAlpha				= srcblendalpha; \
	DestBlendAlpha				= destblendalpha; \
	BlendOpAlpha				= blendopalpha; \
	AlphaToCoverageEnable		= alphatocoverage; \
	};
	
#define DEFINE_DEPTHSTENCIL_STATE_EX( name, depthenable, depthwritemask, depthfunc, stencilenable, stencilreadmask, stencilwritemask, frontfacestencilfail, frontfacestencildepthfail, frontfacestencilpass, frontfacestencilfunc, backfacestencilfail, backfacestencildepthfail, backfacestencilpass, backfacestencilfunc ) \
	DepthStencilState name \
	{ \
	DepthEnable					=	depthenable; \
	DepthWriteMask				=	depthwritemask; \
	DepthFunc					=	depthfunc; \
    StencilEnable				=	stencilenable; \
    StencilReadMask				=	stencilreadmask; \
    StencilWriteMask			=	stencilwritemask; \
    FrontFaceStencilFail		=	frontfacestencilfail; \
    FrontFaceStencilDepthFail	=	frontfacestencildepthfail; \
    FrontFaceStencilPass		=	frontfacestencilpass; \
    FrontFaceStencilFunc		=	frontfacestencilfunc; \
    BackFaceStencilFail			=	backfacestencilfail; \
    BackFaceStencilDepthFail	=	backfacestencildepthfail; \
    BackFaceStencilPass			=	backfacestencilpass; \
    BackFaceStencilFunc			=	backfacestencilfunc; \
	};

#define DEFINE_RASTERIZER_STATE( name, fillmode, cullmode, multisampleenable ) DEFINE_RASTERIZER_STATE_EX( name, fillmode, cullmode, false, 0, 0, true, false, multisampleenable, false )

#define DEFINE_BLEND_STATE( name, writemask, blendenable, srcblend, destblend, blendop, srcblendalpha, destblendalpha, blendopalpha ) DEFINE_BLEND_STATE_EX( name, writemask, blendenable, srcblend, destblend, blendop, srcblendalpha, destblendalpha, blendopalpha, false )

#define DEFINE_BLEND_STATE_NO_BLEND( name, writemask ) DEFINE_BLEND_STATE( name, writemask, 0x00000000, 0, 0, 0, 0, 0, 0 ) 

#define DEFINE_BLEND_STATE_RGBA(	name, blendenable, srcblend, destblend, blendop ) DEFINE_BLEND_STATE( name,	0x0000000f, blendenable, srcblend, destblend, blendop, srcblend, destblend, blendop ) 
#define DEFINE_BLEND_STATE_RGB(		name, blendenable, srcblend, destblend, blendop ) DEFINE_BLEND_STATE( name,	0x00000007, blendenable, srcblend, destblend, blendop, srcblend, destblend, blendop )
#define DEFINE_BLEND_STATE_A(		name, blendenable, srcblend, destblend, blendop ) DEFINE_BLEND_STATE( name,	0x00000008, blendenable, srcblend, destblend, blendop, srcblend, destblend, blendop )  

#define DEFINE_DEPTHSTENCIL_STATE( name, depthenable, depthwritemask ) DEFINE_DEPTHSTENCIL_STATE_EX( name, depthenable, depthwritemask, Less_Equal, false, 0x00000000, 0x00000000, Keep, Keep, Keep, Always, Keep, Keep, Keep, Always )

#endif //__DX10_STATE_BLOCK_MACROS__