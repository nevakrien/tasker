#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>

//user optional definitions
#define CHECK_UNREACHABLE //puts an assert instead of ub 

#if defined(CHECK_UNREACHABLE)

#ifndef __cplusplus
#include <assert.h>
#else
#include <cassert>
#endif //__cplusplus

#define UNREACHABLE() assert(0 && "Unreachable code reached")

#define ASSERT(x) assert(x) 

#elif defined(__GNUC__) || defined(__clang__)
#define UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)

#define UNREACHABLE() __assume(0)
#define ASSERT(x) __assume(x)

#else
//null pointer dereference to signal unreachability
#define UNREACHABLE() (*(int*)0 = 0)
#endif

#ifndef ASSERT
#include <stdbool.h>
static inline void ASSERT(bool x){
    if(!x){UNREACHABLE();}
}
// #define ASSERT(x) if(!x){UNREACHABLE();}
#endif

static inline void* null_check(void* p){
	if(p==NULL){
		perror("went oom\n");
		exit(1);
	}
	return p;
}

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// Safely compare and return the minimum of an int and a size_t
static inline size_t min_int_size_t(int a, size_t b) {
    if (a < 0) {
        return b;
    }

    return (size_t)a < b ? (size_t)a : b;
}

// Safely compare and return the maximum of an int and a size_t
static inline size_t max_int_size_t(int a, size_t b) {
    if (a < 0) {
        return b;
    }

    return (size_t)a > b ? (size_t)a : b;
}


#endif// UTILS_H