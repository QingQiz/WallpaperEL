#ifndef _DEBUG_H
#define _DEBUG_H
#include <stdio.h>
#include <stdlib.h>

#define DE(format, args...) fprintf(stderr, "(\033[31;1mEE\033[m)  "format"\n", ##args);exit(-1)
#define DW(format, args...) fprintf(stderr, "(\033[33;1mWW\033[m)  "format"\n", ##args)
#define DI(format, args...) fprintf(stderr, "(\033[34;1mII\033[m)  "format"\n", ##args)

#define P(sem) sem_wait(&sem)
#define V(sem) sem_post(&sem)

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) < (b) ? (b) : (a))

#define fsleep(s) usleep((int)((s) * 1000000))


#ifndef RELEASE
#define D(format, args...) fprintf(stderr, "(--)  "format"\n", ##args)
#else
#define D(format, args...)
#endif

#undef assert
#define assert(ast, format, args...) if(!(ast)) {DE(format, ##args);exit(-1);}

#endif

