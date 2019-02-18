/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2019 MonetDB B.V.
 */

#ifndef _GDK_SYSTEM_H_
#define _GDK_SYSTEM_H_

#ifdef WIN32
#ifndef LIBGDK
#define gdk_export extern __declspec(dllimport)
#else
#define gdk_export extern __declspec(dllexport)
#endif
#else
#define gdk_export extern
#endif

/* if __has_attribute is not known to the preprocessor, we ignore
 * attributes completely; if it is known, use it to find out whether
 * specific attributes that we use are known */
#ifndef __has_attribute
#ifndef __GNUC__
#define __has_attribute(attr)	0
#ifndef __attribute__
#define __attribute__(attr)	/* empty */
#endif
#else
/* older GCC does have attributes, but not __has_attribute and not all
 * attributes that we use are known */
#define __has_attribute__alloc_size__ 1
#define __has_attribute__cold__ 1
#define __has_attribute__format__ 1
#define __has_attribute__malloc__ 1
#define __has_attribute__noreturn__ 1
#define __has_attribute__returns_nonnull__ 0
#define __has_attribute__visibility__ 1
#define __has_attribute__warn_unused_result__ 1
#define __has_attribute(attr)	__has_attribute##attr
#endif
#endif
#if !__has_attribute(__warn_unused_result__)
#define __warn_unused_result__
#endif
#if !__has_attribute(__malloc__)
#define __malloc__
#endif
#if !__has_attribute(__alloc_size__)
#define __alloc_size__(a)
#endif
#if !__has_attribute(__format__)
#define __format__(a,b,c)
#endif
#if !__has_attribute(__noreturn__)
#define __noreturn__
#endif
/* these are used in some *private.h files */
#if !__has_attribute(__visibility__)
#define __visibility__(a)
#elif defined(__CYGWIN__)
#define __visibility__(a)
#endif
#if !__has_attribute(__cold__)
#define __cold__
#endif

/*
 * @- pthreads Includes and Definitions
 */
#ifdef HAVE_PTHREAD_H
/* don't re-include config.h; on Windows, don't redefine pid_t in an
 * incompatible way */
#undef HAVE_CONFIG_H
#ifdef pid_t
#undef pid_t
#endif
#include <sched.h>
#include <pthread.h>
#endif

#ifdef HAVE_SEMAPHORE_H
# include <semaphore.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>	   /* prerequisite of sys/sysctl on OpenBSD */
#endif
#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif

/* new pthread interface, where the thread id changed to a struct */
#ifdef PTW32_VERSION
#define PTW32 1
#endif

/* debug and errno integers */
gdk_export int GDKdebug;
gdk_export void GDKsetdebug(int debug);
gdk_export int GDKverbose;
gdk_export void GDKsetverbose(int verbosity);

/* API */

/*
 * @- MT Thread Api
 */
typedef size_t MT_Id;		/* thread number. will not be zero */

enum MT_thr_detach { MT_THR_JOINABLE, MT_THR_DETACHED };

gdk_export bool MT_thread_init(void);
gdk_export int MT_create_thread(MT_Id *t, void (*function) (void *),
				void *arg, enum MT_thr_detach d,
				const char *threadname);
gdk_export const char *MT_thread_name(void);
gdk_export void MT_thread_setname(const char *name);
gdk_export void MT_exiting_thread(void);
gdk_export MT_Id MT_getpid(void);
gdk_export int MT_join_thread(MT_Id t);

#if SIZEOF_VOID_P == 4
/* "limited" stack size on 32-bit systems */
/* to avoid address space fragmentation   */
#define THREAD_STACK_SIZE	((size_t)1024*1024)
#else
/* "increased" stack size on 64-bit systems    */
/* since some compilers seem to require this   */
/* for burg-generated code in pathfinder       */
/* and address space fragmentation is no issue */
#define THREAD_STACK_SIZE	((size_t)2*1024*1024)
#endif


/*
 * @- MT Lock API
 */
#if !defined(HAVE_PTHREAD_H) && defined(WIN32)
typedef HANDLE pthread_mutex_t;
typedef void *pthread_mutexattr_t;
gdk_export void pthread_mutex_init(pthread_mutex_t *,
				   const pthread_mutexattr_t *);
gdk_export void pthread_mutex_destroy(pthread_mutex_t *);
gdk_export int pthread_mutex_lock(pthread_mutex_t *);
gdk_export int pthread_mutex_trylock(pthread_mutex_t *);
gdk_export int pthread_mutex_unlock(pthread_mutex_t *);
#endif

#include "gdk_atomic.h"

/* define this if you want to use pthread (or Windows) locks instead
 * of atomic instructions for locking (latching) */
/* #define USE_PTHREAD_LOCKS */

#ifdef USE_PTHREAD_LOCKS

typedef struct {
	pthread_mutex_t lock;
#ifndef NDEBUG
	const char *name;
#endif
} MT_Lock;

#ifdef NDEBUG
#define MT_lock_init(l, n)	pthread_mutex_init(&(l)->lock, 0)
#define MT_lock_set(l)		pthread_mutex_lock(&(l)->lock)
#define MT_lock_unset(l)	pthread_mutex_unlock(&(l)->lock)
#ifdef PTHREAD_MUTEX_INITIALIZER
#define MT_LOCK_INITIALIZER(name)	= { PTHREAD_MUTEX_INITIALIZER }
#endif
#else
#define MT_lock_init(l, n)				\
	do {						\
		(l)->name = (n);			\
		pthread_mutex_init(&(l)->lock, 0);	\
	} while (0)
#define MT_lock_set(l)							\
	do {								\
		TEMDEBUG fprintf(stderr, "#%s: locking %s...\n",	\
				 __func__, (l)->name);			\
		pthread_mutex_lock(&(l)->lock);				\
		TEMDEBUG fprintf(stderr, "#%s: locking %s complete\n",	\
				 __func__, (l)->name);			\
	} while (0)
#define MT_lock_unset(l)						\
	do {								\
		TEMDEBUG fprintf(stderr, "#%s: unlocking %s\n",		\
				 __func__, (l)->name);			\
		pthread_mutex_unlock(&(l)->lock);			\
	} while (0)
#ifdef PTHREAD_MUTEX_INITIALIZER
#define MT_LOCK_INITIALIZER(name)	= { PTHREAD_MUTEX_INITIALIZER, name }
#endif
#endif
#define MT_lock_destroy(l)	pthread_mutex_destroy(&(l)->lock)

#ifndef PTHREAD_MUTEX_INITIALIZER
/* no static initialization possible, so we need dynamic initialization */
#define MT_LOCK_INITIALIZER(name)
#define NEED_MT_LOCK_INIT
#endif

#else

/* if NDEBUG is not set, i.e., if assertions are enabled, we maintain
 * a bunch of counters and maintain a linked list of active locks */
typedef struct MT_Lock {
	ATOMIC_FLAG volatile lock;
#ifndef NDEBUG
	size_t count;
	size_t contention;
	size_t sleep;
	struct MT_Lock * volatile next;
	const char *name;
	const char *locker;
	const char *thread;
#endif
} MT_Lock;

#ifndef NDEBUG

#define MT_LOCK_INITIALIZER(name)	= {0, 0, 0, 0, (struct MT_Lock *) -1, name, NULL}

gdk_export void GDKlockstatistics(int);
gdk_export MT_Lock * volatile GDKlocklist;
gdk_export ATOMIC_FLAG volatile GDKlocklistlock;
gdk_export ATOMIC_TYPE volatile GDKlockcnt;
gdk_export ATOMIC_TYPE volatile GDKlockcontentioncnt;
gdk_export ATOMIC_TYPE volatile GDKlocksleepcnt;
#define _DBG_LOCK_COUNT_0(l, n)		(void) ATOMIC_INC(GDKlockcnt, dummy)
#define _DBG_LOCK_LOCKER(l, n)		((l)->locker = (n), (l)->thread = MT_thread_name())
#define _DBG_LOCK_CONTENTION(l, n)					\
	do {								\
		TEMDEBUG fprintf(stderr, "#lock %s contention in %s\n", \
				 (l)->name, n);				\
		(void) ATOMIC_INC(GDKlockcontentioncnt, dummy);		\
		(l)->contention++;					\
	} while (0)
#define _DBG_LOCK_SLEEP(l, n)						\
	do {								\
		if (_spincnt == 1024)					\
			(void) ATOMIC_INC(GDKlocksleepcnt, dummy);	\
		(l)->sleep++;						\
	} while (0)
#define _DBG_LOCK_COUNT_2(l)						\
	do {								\
		(l)->count++;						\
		if ((l)->next == (struct MT_Lock *) -1) {		\
			while (ATOMIC_TAS(GDKlocklistlock, dummy) != 0) \
				;					\
			(l)->next = GDKlocklist;			\
			GDKlocklist = (l);				\
			ATOMIC_CLEAR(GDKlocklistlock, dummy);		\
		}							\
	} while (0)
#define _DBG_LOCK_INIT(l, n)						\
	do {								\
		(l)->count = (l)->contention = (l)->sleep = 0;		\
		(l)->name = (n);					\
		_DBG_LOCK_LOCKER(l, NULL);				\
		/* if name starts with "sa_" don't link in GDKlocklist */ \
		/* since the lock is in memory that is governed by the */ \
		/* SQL storage allocator, and hence we have no control */ \
		/* over when the lock is destroyed and the memory freed */ \
		if (strncmp((n), "sa_", 3) != 0) {			\
			MT_Lock * volatile _p;				\
			while (ATOMIC_TAS(GDKlocklistlock, dummy) != 0) \
				;					\
			for (_p = GDKlocklist; _p; _p = _p->next)	\
				assert(_p != (l));			\
			(l)->next = GDKlocklist;			\
			GDKlocklist = (l);				\
			ATOMIC_CLEAR(GDKlocklistlock, dummy);		\
		} else {						\
			(l)->next = NULL;				\
		}							\
	} while (0)
#define _DBG_LOCK_DESTROY(l)						\
	do {								\
		/* if name starts with "sa_" don't link in GDKlocklist */ \
		/* since the lock is in memory that is governed by the */ \
		/* SQL storage allocator, and hence we have no control */ \
		/* over when the lock is destroyed and the memory freed */ \
		if (strncmp((l)->name, "sa_", 3) != 0) {		\
			MT_Lock * volatile *_p;				\
			while (ATOMIC_TAS(GDKlocklistlock, dummy) != 0) \
				;					\
			for (_p = &GDKlocklist; *_p; _p = &(*_p)->next)	\
				if ((l) == *_p) {			\
					*_p = (l)->next;		\
					break;				\
				}					\
			ATOMIC_CLEAR(GDKlocklistlock, dummy);		\
		}							\
	} while (0)

#else

#define MT_LOCK_INITIALIZER(name)	= ATOMIC_FLAG_INIT

#define _DBG_LOCK_COUNT_0(l, n)		((void) (n))
#define _DBG_LOCK_CONTENTION(l, n)	((void) (n))
#define _DBG_LOCK_SLEEP(l, n)		((void) (n))
#define _DBG_LOCK_COUNT_2(l)		((void) 0)
#define _DBG_LOCK_INIT(l, n)		((void) 0)
#define _DBG_LOCK_DESTROY(l)		((void) 0)
#define _DBG_LOCK_LOCKER(l, n)		((void) (n))

#endif

#define MT_lock_set(l)							\
	do {								\
		_DBG_LOCK_COUNT_0(l, __func__);				\
		if (ATOMIC_TAS((l)->lock, dummy) != 0) {		\
			/* we didn't get the lock */			\
			int _spincnt = GDKnr_threads > 1 ? 0 : 1023;	\
			_DBG_LOCK_CONTENTION(l, __func__);		\
			do {						\
				if (++_spincnt >= 1024) {		\
					_DBG_LOCK_SLEEP(l, __func__);	\
					MT_sleep_ms(1);			\
				}					\
			} while (ATOMIC_TAS((l)->lock, dummy) != 0);	\
		}							\
		_DBG_LOCK_LOCKER(l, __func__);				\
		_DBG_LOCK_COUNT_2(l);					\
	} while (0)
#define MT_lock_init(l, n)			\
	do {					\
		ATOMIC_CLEAR((l)->lock, dummy);	\
		_DBG_LOCK_INIT(l, n);		\
	} while (0)
#define MT_lock_unset(l)				\
		do {					\
			_DBG_LOCK_LOCKER(l, __func__);	\
			ATOMIC_CLEAR((l)->lock, dummy);	\
		} while (0)
#define MT_lock_destroy(l)	_DBG_LOCK_DESTROY(l)

#endif

/*
 * @- MT Semaphore API
 */
#if !defined(HAVE_PTHREAD_H) && defined(WIN32)

typedef HANDLE pthread_sema_t;
gdk_export void pthread_sema_init(pthread_sema_t *s, int flag, int nresources);
gdk_export void pthread_sema_destroy(pthread_sema_t *s);
gdk_export void pthread_sema_up(pthread_sema_t *s);
gdk_export void pthread_sema_down(pthread_sema_t *s);

#elif defined(_AIX) || defined(__MACH__)

typedef struct {
	int cnt;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} pthread_sema_t;

gdk_export void pthread_sema_init(pthread_sema_t *s, int flag, int nresources);
gdk_export void pthread_sema_destroy(pthread_sema_t *s);
gdk_export void pthread_sema_up(pthread_sema_t *s);
gdk_export void pthread_sema_down(pthread_sema_t *s);

#else

#define pthread_sema_t		sem_t
#define pthread_sema_init	sem_init
#define pthread_sema_destroy	sem_destroy
#define pthread_sema_up		sem_post
#define pthread_sema_down(x)	while(sem_wait(x))

#endif

typedef struct {
	pthread_sema_t sema;
	const char *name;
} MT_Sema;

#define MT_sema_init(s, nr, n)				\
	do {						\
		(s)->name = (n);			\
		pthread_sema_init(&(s)->sema, 0, nr);	\
	} while (0)
#define MT_sema_destroy(s)	pthread_sema_destroy(&(s)->sema)
#define MT_sema_up(s)						\
	do {							\
		TEMDEBUG fprintf(stderr, "#%s: sema %s up\n",	\
				 __func__, (s)->name);		\
		pthread_sema_up(&(s)->sema);			\
	} while (0)
#define MT_sema_down(s)							\
	do {								\
		TEMDEBUG fprintf(stderr, "#%s: sema %s down...\n",	\
				 __func__, (s)->name);			\
		pthread_sema_down(&(s)->sema);				\
		TEMDEBUG fprintf(stderr, "#%s: sema %s down complete\n", \
				 __func__, (s)->name);			\
	} while (0)

gdk_export int MT_check_nr_cores(void);

#endif /*_GDK_SYSTEM_H_*/
