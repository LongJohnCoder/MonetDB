/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2019 MonetDB B.V.
 *
 * The tracer is the general logging system for the MonetDB stack.
 * It is modelled after well-known logging schems, eg. Python
 *
 * Internally, the logger uses a dual buffer to capture log messages
 * before they are written to a file. This way we avoid serial execution.
 *
 * The logger files come in two as well, where we switch them 
 * once the logger is full.
 * The logger file format is "tracer_YY-MM-DDTHH:MM:SS_number.log"
 * An option to consider is we need a rotating scheme over 2 files only,
 * Moreover, old log files might be sent in the background to long term storage as well.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "monetdb_config.h"
#include "gdk.h"
#include "gdk_tracer.h"

/* CHECK */
// Make CUR_FLUSH_LEVEL => ATOMIC_TYPE?

// 0 -> tracer
// 1 -> secondary_tracer
static gdk_tracer tracer = { .allocated_size = 0, .id = 0, .lock = MT_LOCK_INITIALIZER("GDKtracerL") };
static gdk_tracer secondary_tracer = { .allocated_size = 0, .id = 1, .lock = MT_LOCK_INITIALIZER("GDKtracerL2") };
static ATOMIC_TYPE SELECTED_tracer_ID = 0;

static bool GDK_TRACER_STOP = false;

static FILE *output_file;
static ATOMIC_TYPE CUR_ADAPTER = DEFAULT_ADAPTER;
static LOG_LEVEL CUR_FLUSH_LEVEL = DEFAULT_FLUSH_LEVEL;
LOG_LEVEL LOG_LEVELS_LIST[COMPONENTS_COUNT];


// Output error from snprintf of vsnprintf
static void 
_GDKtracer_log_output_error(int bytes_written)
{
    assert(bytes_written >= 0);
}


// Check if log file is open
static void 
_GDKtracer_file_is_open(FILE *file)
{
    assert(file);
}


static bool
_GDKtracer_adapter_exists(ADAPTER adapter)
{
    (void) adapter;
    return true;
}


static bool
_GDKtracer_level_exists(LOG_LEVEL level)
{
    (void) level;
    return true;
}


static bool
_GDKtracer_component_exists(COMPONENT comp)
{
    (void) comp;
    return true;   
}


static void
_GDKtracer_init_log_level_per_component(void)
{
    for(int i = 0; i < COMPONENTS_COUNT; i++)
    {
        LOG_LEVELS_LIST[i] = DEFAULT_LOG_LEVEL;
    }

    /* CHECK */
    // Remove it later :)
    // Put only MAL_RESOLVE in DEBUG mode for testing 
    LOG_LEVELS_LIST[MAL_RESOLVE] = M_DEBUG;
}


// Prepares a file in order to write the contents of the buffer 
// when necessary. The file name each time is merovingian_{int}.log
static void 
_GDKtracer_create_file(void)
{
    char id[INT_MAX_LEN]; 
    snprintf(id, INT_MAX_LEN, "%d", 1);

    char file_name[FILENAME_MAX];
    sprintf(file_name, "%s%c%s%c%s%c%s%s", GDKgetenv("gdk_dbpath"), DIR_SEP, FILE_NAME, NAME_SEP, GDKtracer_get_timestamp("%Y-%m-%dT%H:%M:%S"), NAME_SEP, id, ".log");

    output_file = fopen(file_name, "w");

    _GDKtracer_file_is_open(output_file);
}


// Candidate for 'gnu_printf' format attribute [-Werror=suggest-attribute=format]
static int 
_GDKtracer_fill_tracer(gdk_tracer *sel_tracer, const char *fmt, va_list va) __attribute__ ((format (printf, 2, 0)));

static int 
_GDKtracer_fill_tracer(gdk_tracer *sel_tracer, const char *fmt, va_list va)
{
    const char *msg;
    size_t fmt_len = strlen(fmt);

    // Check if the message contains at the end \n - if not add it
    if(fmt[fmt_len - 1] != NEW_LINE)
    {
        char *tmp = malloc(sizeof(char) * (fmt_len + 1));
        strcpy(tmp, fmt);
        tmp[fmt_len] = NEW_LINE;
        msg = tmp;
        free(tmp);
    }
    else
    {
        msg = fmt;
    }

    // vsnprintf(char *str, size_t count, ...) -> including null terminating character
    int bytes_written = vsnprintf(sel_tracer->buffer + sel_tracer->allocated_size, BUFFER_SIZE - sel_tracer->allocated_size, msg, va);
    _GDKtracer_log_output_error(bytes_written);

    // vsnprintf returned value -> does not include the null terminating character
    return bytes_written++;
}



/**
 * 
 * API CALLS
 * 
 */ 
char*
GDKtracer_get_timestamp(char* fmt)
{
    static char datetime[20];
    time_t now = time(NULL);
    struct tm *tmp = localtime(&now);
    strftime(datetime, sizeof(datetime), fmt, tmp);

    return datetime;
}


gdk_return
GDKtracer_init(void)
{
    _GDKtracer_init_log_level_per_component();
    _GDKtracer_create_file();
    return GDK_SUCCEED;
}


gdk_return
GDKtracer_stop(void)
{
    GDK_TRACER_STOP = true;
    return GDKtracer_flush_buffer();
}


gdk_return
GDKtracer_set_component_log_level(COMPONENT comp, LOG_LEVEL level)
{
    if(LOG_LEVELS_LIST[comp] == level)
        return GDK_SUCCEED;

    if(!_GDKtracer_component_exists(comp))
        return GDK_FAIL;

    if(!_GDKtracer_level_exists(level))
        return GDK_FAIL;
        
    LOG_LEVELS_LIST[comp] = level;
    
    return GDK_SUCCEED;
}


gdk_return
GDKtracer_reset_component_log_level(COMPONENT comp)
{  
    if(LOG_LEVELS_LIST[comp] == DEFAULT_LOG_LEVEL)
        return GDK_SUCCEED;
   
    if(!_GDKtracer_component_exists(comp))
        return GDK_FAIL;

    LOG_LEVELS_LIST[comp] = DEFAULT_LOG_LEVEL; 

    return GDK_SUCCEED;
}


gdk_return
GDKtracer_set_flush_level(LOG_LEVEL level)
{
    if(CUR_FLUSH_LEVEL == level)
        return GDK_SUCCEED;

    if(!_GDKtracer_level_exists(level))
        return GDK_FAIL;
        
    CUR_FLUSH_LEVEL = level;

    return GDK_SUCCEED;
}


gdk_return
GDKtracer_reset_flush_level(void)
{
    if(CUR_FLUSH_LEVEL == DEFAULT_FLUSH_LEVEL)
        return GDK_SUCCEED;

    CUR_FLUSH_LEVEL = DEFAULT_FLUSH_LEVEL;

    return GDK_SUCCEED;
}


gdk_return
GDKtracer_set_adapter(ADAPTER adapter)
{
    if(ATOMIC_GET(&CUR_ADAPTER) == adapter)
        return GDK_SUCCEED;

    // Here when switching between adapters we can open/close the file
    // But it is not so important to keep it open in case the adapter switches
    // From BASIC to other => close the file
    // From other to BASIC => open the file
    
    if(!_GDKtracer_adapter_exists(adapter))
        return GDK_FAIL;

    ATOMIC_SET(&CUR_ADAPTER, adapter);

    return GDK_SUCCEED;
}


gdk_return
GDKtracer_reset_adapter(void)
{
    if(ATOMIC_GET(&CUR_ADAPTER) == DEFAULT_ADAPTER)
        return GDK_SUCCEED;

    ATOMIC_SET(&CUR_ADAPTER, DEFAULT_ADAPTER);

    return GDK_SUCCEED;
}


gdk_return
GDKtracer_log(LOG_LEVEL level, const char *fmt, ...)
{   
    // Select a tracer
    gdk_tracer *fill_tracer;
    int GDK_result;
    bool SWITCH_tracer = true;
    int bytes_written = 0;        

    if((int) ATOMIC_GET(&SELECTED_tracer_ID) == tracer.id)
        fill_tracer = &tracer;
    else
        fill_tracer = &secondary_tracer;

    MT_lock_set(&fill_tracer->lock);
    {
        va_list va;
        va_start(va, fmt);
        bytes_written = _GDKtracer_fill_tracer(fill_tracer, fmt, va);
        va_end(va);

        // The message fits the buffer OR the buffer is empty (we don't care if it fits - just cut it off)
        if(bytes_written < (BUFFER_SIZE - fill_tracer->allocated_size) || 
            fill_tracer->allocated_size == 0)
        {
            fill_tracer->allocated_size += bytes_written;
            SWITCH_tracer = false;
        }
    }
    MT_lock_unset(&fill_tracer->lock);

    if(SWITCH_tracer)
    {       
        // Switch tracer
        if((int) ATOMIC_GET(&SELECTED_tracer_ID) == tracer.id)
            fill_tracer = &secondary_tracer;
        else
            fill_tracer = &tracer;
            
        MT_lock_set(&fill_tracer->lock);
        {
            // Flush current tracer
            MT_Id tid;
            
            if(MT_create_thread(&tid, (void(*) (void*)) GDKtracer_flush_buffer, NULL, MT_THR_JOINABLE, "GDKtracerFlush") < 0)
                return GDK_FAIL;
            
            va_list va;
            va_start(va, fmt);
            bytes_written = _GDKtracer_fill_tracer(fill_tracer, fmt, va);
            va_end(va);

            // The second buffer will always be empty at start
            // So if the message does not fit we cut it off
            // message might be > BUFFER_SIZE
            fill_tracer->allocated_size += bytes_written;

            GDK_result = MT_join_thread(tid);
            if(GDK_result == GDK_FAIL)
                return GDK_FAIL;

            // Set the new selected tracer 
            ATOMIC_SET(&SELECTED_tracer_ID, fill_tracer->id);
        }
        MT_lock_unset(&fill_tracer->lock);
    }
        
    // Flush the current buffer in case the event is 
    // important depending on the flush-level
    if(level >= CUR_FLUSH_LEVEL)
    {
        GDK_result = GDKtracer_flush_buffer();
        if(GDK_result == GDK_FAIL)
            return GDK_FAIL;
    }

    return GDK_SUCCEED;
}


gdk_return
GDKtracer_flush_buffer(void)
{
    // Select a tracer
    gdk_tracer *fl_tracer;
    if((int) ATOMIC_GET(&SELECTED_tracer_ID) == tracer.id)
        fl_tracer = &tracer;
    else
        fl_tracer = &secondary_tracer;
        
    // No reason to flush a buffer with no content 
    if(fl_tracer->allocated_size == 0)
        return GDK_SUCCEED;

    if(ATOMIC_GET(&CUR_ADAPTER) == BASIC)
    {
        // Check if file is open
        _GDKtracer_file_is_open(output_file);
        
        MT_lock_set(&fl_tracer->lock);
        {
            fwrite(&fl_tracer->buffer, fl_tracer->allocated_size, 1, output_file);
            fflush(output_file);
            
            // Reset buffer
            memset(fl_tracer->buffer, 0, BUFFER_SIZE);
            fl_tracer->allocated_size = 0;
        }
        MT_lock_unset(&fl_tracer->lock);
    }

    if(GDK_TRACER_STOP)
    {
        fclose(output_file);
    }
    
    return GDK_SUCCEED;
}
