#ifndef _DEBUG_H
#define _DEBUG_H
#include <stdio.h>
#include <stdlib.h>

#define DE(format, args...) fprintf(stderr, "(\033[31;1mEE\033[m)  "format"\n", ##args)
#define DW(format, args...) fprintf(stderr, "(\033[33;1mWW\033[m)  "format"\n", ##args)
#define DI(format, args...) fprintf(stderr, "(\033[34;1mII\033[m)  "format"\n", ##args)

#ifndef RELEASE
#define D(format, args...) fprintf(stderr, "(--)  "format"\n", ##args)
#else
#define D(format, args...)
#endif

#define assert(ast, format, args...) if(!(ast)) {DE(format, ##args);exit(-1);}

#endif

