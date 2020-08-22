#ifndef __BUILD_TIME_H__
#define __BUILD_TIME_H__

#define USE_FILE_INTEGRATION_SYMBOL(sym) \
    __FRI_generated_##sym
#define DECLARE_FILE_INTEGRATION_STRING(sym) \
    extern char const *USE_FILE_INTEGRATION_SYMBOL(sym);
#define DEFINE_FILE_INTEGRATION_STRING(sym) \
    char const *USE_FILE_INTEGRATION_SYMBOL(sym)

DECLARE_FILE_INTEGRATION_STRING(OpenCLPrimitivesKernel)

#endif // __BUILD_TIME_H__
