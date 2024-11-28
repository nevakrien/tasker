#ifndef PLATFORM_H
#define PLATFORM_H



#define MAX_COMMAND 10*FILENAME_MAX

#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

#include "tinycsocket.h"




#endif // PLATFORM_H
