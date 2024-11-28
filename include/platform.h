#ifndef PLATFORM_H
#define PLATFORM_H


#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif



#endif // PLATFORM_H
