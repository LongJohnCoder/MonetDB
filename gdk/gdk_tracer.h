/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2019 MonetDB B.V.
 */

#ifndef _GDK_TRACER_H_
#define _GDK_TRACER_H_

#define INT_MAX_LEN ((__CHAR_BIT__ * sizeof(int) - 1) / 3  + 2)
#define BUFFER_SIZE 64000

#define DEFAULT_COMPONENT_SEL ALL
#define DEFAULT_LOG_LEVEL M_CRITICAL
#define DEFAULT_FLUSH_LEVEL M_ERROR

#define FILE_NAME "trace"
#define NAME_SEP '_'
#define NEW_LINE '\n'
#define MAX_FILE_SIZE 1073741824

// TODO -> Sort it per layer
// COMPONENTS 
typedef enum { 
               // ALL
               ALL,

               // SQL
               SQL_ALL,
               SQL_ATOM_TR,
               SQL_STATISTICS,
               SQL_ORDERIDX,
               SQL_N,
               SQL_WLR,
               SQL_SCENARIO,
               SQL_CACHE_TR,
               SQL_PARSER,
               SQL_READER,
               SQL_SYMBOL,

               // MAL
               MAL_ALL,

               // GDK
               GDK_ALL

              } COMPONENT;
              
// LOG LEVELS
typedef enum { 

               M_CRITICAL = 1,
               M_ERROR = 100, 
               M_WARNING = 150,
               M_INFO = 200,
               M_DEBUG = 255

              } LOG_LEVEL;


extern ATOMIC_TYPE CUR_LOG_LEVEL;

/**
 * 
 * Macros for logging
 * Function name is detected automatically
 * 
 */
#define GDK_TRACER_LOG(LOG_LEVEL, MSG, ...)                             \
    if((int) ATOMIC_GET(&CUR_LOG_LEVEL) >= (int) LOG_LEVEL)             \
    {                                                                   \
        GDKtracer_log(LOG_LEVEL,                                        \
                      "[%s] %s (%s:%d) # "MSG,                          \
                      GDKtracer_get_timestamp("%Y-%m-%d %H:%M:%S"),     \
                      __FILE__,                                         \
                      __FUNCTION__,                                     \
                      __LINE__,                                         \
                      ## __VA_ARGS__);                                  \
    }                                                                   \

#define CRITICAL(COMP, MSG, ...)                                        \
    GDK_TRACER_LOG(M_CRITICAL, MSG, ## __VA_ARGS__)                     \

#define ERROR(COMP, MSG, ...)                                           \
    GDK_TRACER_LOG(M_ERROR, MSG, ## __VA_ARGS__)                        \

#define WARNING(COMP, MSG, ...)                                         \
    GDK_TRACER_LOG(M_WARNING, MSG, ## __VA_ARGS__)                      \

#define INFO(COMP, MSG, ...)                                            \
    GDK_TRACER_LOG(M_INFO, MSG, ## __VA_ARGS__)                         \

#define DEBUG(COMP, MSG, ...)                                           \
    GDK_TRACER_LOG(M_DEBUG, MSG, ## __VA_ARGS__)                        \



/**
 *  
 * Macros for logging 
 * They take as argument the logical function name 
 *  
 */
#define GDK_TRACER_LOG_LN(LOG_LEVEL, FUNC_LN, MSG, ...)                 \
    if((int) ATOMIC_GET(&CUR_LOG_LEVEL) >= (int) LOG_LEVEL)             \
    {                                                                   \
        GDKtracer_log(LOG_LEVEL,                                        \
                      "[%s] %s (%s:%d) # "MSG,                          \
                      GDKtracer_get_timestamp("%Y-%m-%d %H:%M:%S"),     \
                      __FILE__,                                         \
                      FUNC_LN,                                          \
                      __LINE__,                                         \
                      ## __VA_ARGS__);                                  \
    }                                                                   \

#define CRITICAL_LN(COMP, FUNC_LN, MSG, ...)                            \
    GDK_TRACER_LOG_LN(M_CRITICAL, FUNC_LN, MSG, ## __VA_ARGS__)         \

#define ERROR_LN(COMP, FUNC_LN, MSG, ...)                               \
    GDK_TRACER_LOG_LN(M_ERROR, FUNC_LN, MSG, ## __VA_ARGS__)            \

#define WARNING_LN(COMP, FUNC_LN, MSG, ...)                             \
    GDK_TRACER_LOG_LN(M_WARNING, FUNC_LN, MSG, ## __VA_ARGS__)          \

#define INFO_LN(COMP, FUNC_LN, MSG, ...)                                \
    GDK_TRACER_LOG_LN(M_INFO, FUNC_LN, MSG, ## __VA_ARGS__)             \

#define DEBUG_LN(COMP, FUNC_LN, MSG, ...)                               \
    GDK_TRACER_LOG_LN(M_DEBUG, FUNC_LN, MSG, ## __VA_ARGS__)            \


// GDKtracer Buffer
typedef struct GDKtracer
{
    int id;
    char buffer[BUFFER_SIZE];
    int allocated_size;
    MT_Lock lock;
}
gdk_tracer;



/**
 *  GDKtracer API
 */
// Returns the timestamp in the form of datetime
char* GDKtracer_get_timestamp(char* fmt);


// Initialize tracer - basically creates the file
gdk_return GDKtracer_init(void);


// Flushes the contents of the buffer and closes the log file
gdk_return GDKtracer_stop(void);


// Sets the log level to one of the enum LOG_LEVELS above. If the current log level 
// is not NONE and GDK_tracer_set_log_level sets it to NONE we flush the buffer first 
// in order to "discard" the messages that are there from the previous log levels
gdk_return GDKtracer_set_log_level(int *level);


// Resets the log level to the default one - NONE. If the current log level is not NONE 
// and GDK_tracer_reset_log_level() is called we need to flush the buffer first in order to 
// "discard" messages kept from other log levels
gdk_return GDKtracer_reset_log_level(void);


// Sets the minimum flush level that an event will trigger the logger to flush the buffer
gdk_return GDKtracer_set_flush_level(int *level);


// Resets the flush level to the default (ERROR)
gdk_return GDKtracer_reset_flush_level(void);


// TODO -> Write comments
// Candidate for 'gnu_printf' format attribute [-Werror=suggest-attribute=format] 
gdk_return GDKtracer_log(LOG_LEVEL level, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));


// Flush the buffer to the file. If after flushing the buffer, the file is greater 
// or equal to 1 GB, then we close the current one and we swap to a new one increasing the ID
gdk_return GDKtracer_flush_buffer(void);

#endif
