#ifndef epicsEnvUnset_h
#define epicsEnvUnset_h

#ifdef __cplusplus
extern "C" {
#endif

#include "shareLib.h"

epicsShareFunc void epicsShareAPI epicsEnvUnset (const char *name);

#ifdef __cplusplus
}
#endif

#endif
