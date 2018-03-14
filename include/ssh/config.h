#pragma once

#if defined(WIN32)
#define SSH_OS_WIN32 1
#elif defined(__linux__)
#define SSH_OS_LINUX 1
#define SSH_OS_UNIX 1
#elif defined(__FreeBSD__)
#define SSH_OS_FREEBSD 1
#define SSH_OS_UNIX 1
#else
#error Unsupported OS
#endif

#ifndef SSH_OS_WIN32
#define SSH_OS_WIN32 0
#endif

#ifndef SSH_OS_LINUX
#define SSH_OS_LINUX 0
#endif

#ifndef SSH_OS_FREEBSD
#define SSH_OS_FREEBSD 0
#endif

#ifndef SSH_OS_UNIX
#define SSH_OS_UNIX 0
#endif
