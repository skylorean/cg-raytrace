#include <iostream>
#include "Application/Application.h"


FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }

int main(int /*argc*/, char** /*argv*/)
{
	Application app;
	app.MainLoop();
	return 0;
}