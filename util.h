#ifndef _UTIL_H_
#define _UTIL_H_

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#if !defined(MAX)
/// Returns the maximum of two numbers
#define MAX(a, b)	((a)>(b)?(a):(b))
#endif
#if !defined(MIN)
/// Returns the minimum of two numbers
#define MIN(a, b)	((a)<(b)?(a):(b))
#endif

/// Prints to the error output stream, using printf formatting.
#define eprintf(...)	do{fprintf(stderr, __VA_ARGS__);} while(0)

/// Prints to the error output stream using printf formatting, and exits
/// the program with specified code.
#define ErrExit(code, ...)	do{fprintf(stderr, __VA_ARGS__); exit(code);} \
   							while(0)

/// Prints a msg using perror, and returns from the caller function.
#define HandleError(msg, ret) \
	do {perror(msg); return(ret);} while(0)
/// The same as above, for functions that do not directly set errno vairable.
#define HandleErrorEn(en, msg, ret) \
	do {errno = en; perror(msg); return(ret);} while(0)

#endif //_UTIL_H_

