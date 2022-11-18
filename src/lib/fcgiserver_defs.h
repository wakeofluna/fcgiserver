#ifndef FCGISERVER_DEFS_H
#define FCGISERVER_DEFS_H

#ifdef fcgiserver_EXPORTS
#define DLL_PUBLIC __attribute__ ((visibility("default")))
#define DLL_PRIVATE __attribute__ ((visibility("hidden")))
#else
#define DLL_PUBLIC
#define DLL_PRIVATE
#endif

#endif // FCGISERVER_DEFS_H
