
#include "common.h"
#include "gearsComandLine.h"
#include "testApplication.h"

using namespace Philo;

bool mainImpl()
{
	GearCommandLine cl(GetCommandLineA());

	try
	{
		TestApplication app(cl);
		app.open(800, 600, "Philo Render Test");

		while (app.isOpen())
		{
			app.update();
		}
		app.close();
	}
	catch(std::exception& e)
	{
		ph_error("%s",e.what());
	}

	return true;
}

int main()
{
	bool ok = mainImpl();
	return ok ? 0 : 1;
}