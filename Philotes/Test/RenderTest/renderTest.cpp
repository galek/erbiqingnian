
#include "common.h"
#include "gearsComandLine.h"
#include "testApplication.h"

using namespace Philo;

bool mainImpl()
{
	GearCommandLine cl(GetCommandLineA());

	int width, height;
	TestApplication app(cl);
	width = app.getDisplayWidth();
	height = app.getDisplayHeight();

	app.open(width, height, "Philo Render Test");
	bool ok = true;
	while (app.isOpen())
	{
		app.update();
	}
	app.close();
	return ok;

	return true;
}

int main()
{
	bool ok = mainImpl();
	return ok ? 0 : 1;
}