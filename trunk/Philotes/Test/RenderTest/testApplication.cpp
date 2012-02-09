
#include "testApplication.h"
#include "render.h"
#include "renderProjection.h"

_NAMESPACE_BEGIN

TestApplication::TestApplication( const GearCommandLine& cmdline )
	: GearApplication(cmdline)
{

}

TestApplication::~TestApplication()
{

}

void TestApplication::onInit( void )
{

}

void TestApplication::onShutdown( void )
{

}

void TestApplication::onTickPreRender( float dtime )
{

}

void TestApplication::onRender( void )
{
	Renderer* renderer = getRenderer();

	if (renderer)
	{
		uint32 windowWidth  = 0;
		uint32 windowHeight = 0;
		getSize(windowWidth, windowHeight);
		if (windowWidth > 0 && windowHeight > 0)
		{
			renderer->clearBuffers();

			RendererProjection::makeProjectionMatrix(Radian(Degree(45)),windowWidth / (float)windowHeight, 0.1f, 10000.0f,m_projection);

			renderer->render(getEyeTransform(), m_projection);

			m_rewriteBuffers = renderer->swapBuffers();
		}
	}
}

void TestApplication::onTickPostRender( float dtime )
{

}

int TestApplication::getDisplayWidth( void )
{
	return 800;
}

int TestApplication::getDisplayHeight( void )
{
	return 600;
}

_NAMESPACE_END
