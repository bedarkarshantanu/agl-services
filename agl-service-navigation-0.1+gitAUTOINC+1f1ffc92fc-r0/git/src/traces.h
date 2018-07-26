// Copyright 2017 AISIN AW CO.,LTD

#ifndef __TRACE_H__
#define __TRACE_H__

#include <stdio.h>

#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define PURPLE  "\033[35m"
#define DGREEN  "\033[6m"
#define WHITE   "\033[7m"
#define CYAN    "\x1b[36m"
#define NONE    "\033[0m"

#ifdef NDEBUG

#define TRACE_DEBUG_JSON(fmt, args...)
#define TRACE_DEBUG(fmt, args...)
#define TRACE_INFO(fmt, args...)
#define TRACE_WARN(fmt, args...)
#define TRACE_ERROR(fmt, args...)

#else

#define TRACE_DEBUG(fmt, args...) do { fprintf(stderr, "[%s:%d] " CYAN "DEBUG" NONE ": " fmt "\n", __func__, __LINE__, ##args); } while(0)
#define TRACE_INFO(fmt, args...)  do { fprintf(stderr, "[%s:%d] " GREEN "INFO" NONE ":  " fmt "\n", __func__, __LINE__, ##args); } while(0)
#define TRACE_WARN(fmt, args...)  do { fprintf(stderr, "[%s:%d] " YELLOW "WARN"  NONE":  " fmt "\n", __func__, __LINE__, ##args); } while(0)
#define TRACE_ERROR(fmt, args...) do { fprintf(stderr, "[%s:%d] " RED "ERROR" NONE ": " fmt "\n", __func__, __LINE__, ##args); } while(0)

#define TRACE_DEBUG_JSON(fmt, args...)

#endif

#endif // __TRACE_H__
