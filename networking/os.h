#ifndef __OS_H__
#define __OS_H__

#if defined(_WIN32) || defined(_WIN64)
#define OS_WINDOWS
#elif defined(__linux__)
#define OS_LINUX
#endif

#endif // __OS_H__