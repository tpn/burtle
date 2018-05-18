/*
-----------------------------------------------------------------------
Experiment with Windows threading and I/O
This contains a threadsafe printf equivalent, tprint().
-----------------------------------------------------------------------
 */

#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include <memory.h>
#include <windows.h>

#define CORES 8

/* context given to a new thread */
typedef struct threadargs
{
  int    id;       /* id for this thread (in 0..CORES-1) */
} threadargs;

/* a common structure shared by all threads */
typedef struct common
{
  struct threadargs t[CORES];
  HANDLE tHandle[CORES];
} common;


/*
 * A threadsafe printf statement
 */
static CRITICAL_SECTION tprint_crit;
void tprint(const char *fmt, ...)
{
  va_list args;
  EnterCriticalSection(&tprint_crit);
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  LeaveCriticalSection(&tprint_crit);
}

/* thread driver */
DWORD WINAPI tdriver(LPVOID arg)
{
  int i;
  threadargs *targs = (threadargs *)arg;
  for (i=targs->id; i<20; i+=CORES) {
    tprint("counter %d\n", i);
  }
  return (DWORD)0;
}

int main()
{
  int i;
  common *c = malloc(sizeof(common));
  InitializeCriticalSection(&tprint_crit);
  for (i=0; i<CORES; ++i) {
    c->t[i].id = i;
    tprint("spawning thread %d\n", c->t[i].id);
    c->tHandle[i] = CreateThread(NULL, 0, tdriver, &c->t[i], 0, 0);
  }
  WaitForMultipleObjects(CORES, c->tHandle, TRUE, INFINITE);
  free(c);
  tprint("finished\n");
}
