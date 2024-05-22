#include <iostream>
#include "Application/Application.h"

/*
* Нормаль - вектор, который перпендикулярен к поверхности объекта в данной точке.
* 
*/


FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }

int main(int /*argc*/, char** /*argv*/)
{
	Application app;
	app.MainLoop();
	return 0;
}