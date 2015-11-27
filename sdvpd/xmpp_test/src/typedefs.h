#ifndef _TYPEDEFS_H
#define _TYPEDEFS_H

#include <pthread.h>

typedef unsigned char b8;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef float f32;
typedef double f64;

typedef struct {
  pthread_t threadId;
  u8 created:1;
  u8 destroyed:1;
} thread_info_t;


#endif
