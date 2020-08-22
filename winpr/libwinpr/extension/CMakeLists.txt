set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(${MODULE_PREFIX}_EXT_SRCS
    ConsoleShader.c
    ConsoleShader.h)

winpr_module_add(${${MODULE_PREFIX}_EXT_SRCS})
