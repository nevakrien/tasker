#ifndef PLATFORM_H
#define PLATFORM_H



#define MAX_COMMAND 10*FILENAME_MAX

#ifdef _WIN32
#define POPEN _popen
#define POCLOSE _pclose
#else
#define POPEN popen
#define POCLOSE pclose
#endif

#include "tinycsocket.h"




#endif // PLATFORM_H
