
#ifndef RENDERER_H
#define RENDERER_H

#include "renderConfig.h"
#include "renderMaterial.h"

#include <vector>

_NAMESPACE_BEGIN

class ScreenQuad
{
public:
	ScreenQuad();

	uint32	mLeftUpColor;		//!< Color for left-up vertex
	uint32	mLeftDownColor;		//!< Color for left-down vertex
	uint32	mRightUpColor;		//!< Color for right-up vertex
	uint32	mRightDownColor;	//!< Color for right-down vertex
	scalar	mAlpha;				//!< Alpha value used if > 0.0f
	scalar	mX0, mY0;			//!< Up-left coordinates
	scalar	mX1, mY1;			//!< Bottom-right coordinates
};

class Render
{
	public:
		struct TextVertex
		{
			Vector3	p;
			scalar	rhw;
			uint32	color;
			scalar	u,v;
		};

		typedef enum DriverType
		{
			DRIVER_OPENGL   = 0, // Supports Windows, Linux, MacOSX and PS3.
			DRIVER_DIRECT3D9,    // Supports Windows, XBOX360.
			DRIVER_DIRECT3D10,   // Supports Windows Vista & 7.
			DRIVER_LIBGCM        // Supports PS3.
		};
		
	public:
		static Render *createRender(uint64 windowHandle);
		
		static const char *getDriverTypeName(DriverType type);
		
	protected:
		Render(DriverType driver);
		virtual ~Render(void);
		
	public:
		void release(void);
		
		// get the driver type for this renderer.
		DriverType getDriverType(void) const;
		
		// get the offset to the center of a pixel relative to the size of a pixel (so either 0 or 0.5).
		scalar getPixelCenterOffset(void) const;
		
		// get the name of the hardware device.
		const char *getDeviceName(void) const;

		// adds a mesh to the render queue.
		void queueMeshForRender(RenderElement &mesh);
		
		// adds a light to the render queue.
		void queueLightForRender(RenderLight &light);
		
		// renders the current scene to the offscreen buffers. empties the render queue when done.
		void render(const Matrix4 &eye, const Matrix4 &proj, RenderTarget *target=0, bool depthOnly=false);
		
		// sets the ambient lighting color.
		void setAmbientColor(const Colour &ambientColor);

        // sets the clear color.
		void setClearColor(const Colour &clearColor);
        Colour& getClearColor() { return m_clearColor; }
		
public:
		// clears the offscreen buffers.
		virtual void clearBuffers(void) = 0;
		
		// presents the current color buffer to the screen.
		// returns true on device reset and if buffers need to be rewritten.
		virtual bool swapBuffers(void) = 0;

		// get the device pointer (void * abstraction)
		virtual void *getDevice() = 0;

		virtual void getWindowSize(uint32 &width, uint32 &height) const = 0;

		virtual RenderVertexBuffer   *createVertexBuffer(  const RenderVertexBufferDesc   &desc) = 0;
		virtual RenderIndexBuffer    *createIndexBuffer(   const RenderIndexBufferDesc    &desc) = 0;
		virtual RenderInstanceBuffer *createInstanceBuffer(const RenderInstanceBufferDesc &desc) = 0;
		virtual RenderTexture2D      *createTexture2D(     const RenderTexture2DDesc      &desc) = 0;
		virtual RenderTarget         *createTarget(        const RenderTargetDesc         &desc) = 0;
		virtual RenderMaterial       *createMaterial(      const RenderMaterialDesc       &desc) = 0;
		virtual RenderMesh           *createMesh(          const RenderMeshDesc           &desc) = 0;
		virtual RenderLight          *createLight(         const RenderLightDesc          &desc) = 0;

		// Disable this performance optimization when CUDA/Graphics Interop is in use
		void setVertexBufferDeferredUnlocking( bool enabled );
		bool getVertexBufferDeferredUnlocking() const;
	
	private:
		void renderMeshes(std::vector<RenderElement*> & meshes, RenderMaterial::Pass pass);
		void renderDeferredLights(void);
	
	private:
		virtual bool beginRender(void) { return true;}
		virtual void endRender(void) {}
		virtual void bindViewProj(const Matrix4 &eye, const Matrix4 &proj)    = 0;
		virtual void bindAmbientState(const Colour &ambientColor)                 = 0;
		virtual void bindDeferredState(void)                                             = 0;
		virtual void bindMeshContext(const RenderElement &context)                 = 0;
		virtual void beginMultiPass(void)                                                = 0;
		virtual void endMultiPass(void)                                                  = 0;
		virtual void renderDeferredLight(const RenderLight &light)                     = 0;
		
		virtual bool isOk(void) const = 0;

	private:
		Render &operator=(const Render&) { return *this; }
	
	private:
		const DriverType					m_driver;
		
		std::vector<RenderElement*>	m_visibleLitMeshes;
		std::vector<RenderElement*>	m_visibleUnlitMeshes;
		std::vector<RenderElement*>	m_screenSpaceMeshes;
		std::vector<RenderLight*>			m_visibleLights;
		
		Colour								m_ambientColor;
		Colour								m_clearColor;

    protected:
		bool								m_deferredVBUnlock;
		scalar								m_pixelCenterOffset;
		char								m_deviceName[256];
};

_NAMESPACE_END

#endif
