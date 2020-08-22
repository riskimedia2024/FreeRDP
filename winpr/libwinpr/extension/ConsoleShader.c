#include "ConsoleShader.h"
#include <string.h>

typedef enum
{
    /* CatchBlock tokens */
    MAYBE_TIMESTAMP_OR_LEVEL,
    MAYBE_MODULE_NAME,
    MAYBE_NUMBER,
    /* Final tokens */
    TIMESTAMP,
    LEVEL_TRACE,
    LEVEL_DEBUG,
    LEVEL_INFO,
    LEVEL_WARN,
    LEVEL_ERROR,
    LEVEL_FATAL,
    LEVEL_OFF,
    MODULE_NAME,
    NUMBER,
    STRING,
    SPACE,
    CONTENT
} Identifier;

typedef enum
{
    NONE        = 0,
    GREEN       = 0x001,
    RED         = 0x002,
    YELLOW      = 0x004,
    BLUE        = 0x008,
    PURPLE      = 0x010,
    DEEP_GREEN  = 0x020,
    WHITE       = 0x040,
    GRAY        = 0x080,
    HIGHLIGHT   = 0x100,
    DISABLE     = 0x200
} Colors;

typedef struct
{
    Colors color;
    char const *attribute;
} ColorAttributeMap;

typedef struct
{
    Identifier ident;
    Colors color;
} IdentColorMap;

typedef struct
{
    char const *begin;
    char const *end;
    Identifier ident;
} CatchBlock;

typedef struct
{
    Identifier level;
    char const *str;
} LevelStringMap;

static const ColorAttributeMap __cshader_camap[] = {
    { GREEN,        "\033[32m" },
    { RED,          "\033[31m" },
    { YELLOW,       "\033[33m" },
    { BLUE,         "\033[34m" },
    { PURPLE,       "\033[35m" },
    { DEEP_GREEN,   "\033[36m" },
    { WHITE,        "\033[37m" },
    { GRAY,         "\033[38m" },
    { HIGHLIGHT,    "\033[1m"  },
    { DISABLE,      "\033[0m"  },
    { NONE,         ""         }
};

static const IdentColorMap __cshader_icmap[] = {
    { TIMESTAMP,    PURPLE              },
    { LEVEL_TRACE,  GRAY | HIGHLIGHT    },
    { LEVEL_DEBUG,  PURPLE              },
    { LEVEL_INFO,   GREEN               },
    { LEVEL_WARN,   YELLOW              },
    { LEVEL_ERROR,  RED                 },
    { LEVEL_FATAL,  RED | HIGHLIGHT     },
    { LEVEL_OFF,    WHITE | HIGHLIGHT   },
    { MODULE_NAME,  WHITE | HIGHLIGHT   },
    { NUMBER,       DEEP_GREEN          },
    { STRING,       YELLOW              },
    { SPACE,        NONE                },
    { CONTENT,      NONE                }
};

static const LevelStringMap __cshader_lsmap[] = {
    { LEVEL_TRACE,  "trace" },
    { LEVEL_DEBUG,  "debug" },
    { LEVEL_INFO,   "info"  },
    { LEVEL_WARN,   "warn"  },
    { LEVEL_ERROR,  "error" },
    { LEVEL_FATAL,  "fatal" },
    { LEVEL_OFF,    "off"   }
};

_Bool __cshader_matches_catchblock(char const *str, CatchBlock *catchblock)
{
    _Bool expects = 0;
    _Bool expectExclude = 0;
    char expect;

    if (*str == '\0')
        return 0;

    while (*str != '\0')
    {
        if (expects)
        {
            if (expect != *str)
                goto next;
            catchblock->end = str;
            if (expectExclude)
                catchblock->end--;
            break;
        }

        catchblock->begin = str;
        if (*str == '[')
        {
            catchblock->ident = MAYBE_TIMESTAMP_OR_LEVEL;
            expects = 1;
            expect = ']';
        }
        else if (*str == '<')
        {
            catchblock->ident = MAYBE_MODULE_NAME;
            expects = 1;
            expect = '>';
        }
        else if (*str >= '0' && *str <= '9')
        {
            catchblock->ident = MAYBE_NUMBER;
            expects = 1;
            expectExclude = 1;
            expect = ' ';
        }
        else if (*str == '\"' || *str == '\'')
        {
            catchblock->ident = STRING;
            expects = 1;
            expect = *str;
        }
        else if (*str == ' ')
        {
            catchblock->begin = str;
            catchblock->end = str;
            catchblock->ident = SPACE;
            break;
        }
        else
        {
            catchblock->ident = CONTENT;
            expects = 1;
            expect = ' ';
            expectExclude = 1;
        }

        if (expects)
            catchblock->begin = str;

    next:
        str++;
    }

    if (expects && expect == ' ' && *str == '\0')
        catchblock->end = str - 1;
    return 1;
}

_Bool __cshader_is_number(char ch)
{ return (ch >= '0' && ch <= '9'); }
_Bool __cshader_is_upper(char ch)
{ return (ch >= 'A' && ch <= 'Z'); }
_Bool __cshader_is_lower(char ch)
{ return (ch >= 'a' && ch <= 'z'); }
char __cshader_to_lower(char ch)
{ return __cshader_is_lower(ch) ? ch : (__cshader_is_upper(ch) ? ('a' + (ch - 'A')) : ch); }

_Bool __cshader_case_insensitive_compare_equal(const char *begin, const char *end, char const *str)
{
    if (begin > end)
        return 0;
    if (end - begin + 1 != strlen(str))
        return 0;
    
    char const *p0 = begin, *p1 = str;
    while (p0 <= end && *p1 != '\0')
    {
        char c0 = __cshader_to_lower(*p0);
        char c1 = __cshader_to_lower(*p1);
        if (c0 != c1)
            return 0;
        p0++;
        p1++;
    }
    return 1;
}

_Bool __cshader_is_integer(char const *begin, char const *end)
{
    if (begin > end)
        return 0;
    while (begin <= end)
    {
        if (!__cshader_is_number(*begin))
            return 0;
        begin++;
    }
    return 1;
}

_Bool __cshader_is_float(char const *begin, char const *end)
{
    if (begin > end)
        return 0;
    
    _Bool point = 0;
    while (begin <= end)
    {
        if (*begin == '.')
        {
            if (point)
                return 0;
            else if (begin == end)
                /* Point shouldn't appears at the end */
                return 0;
            point = 1;
        }
        else if (!__cshader_is_number(*begin))
            return 0;
        begin++;
    }
    return 1;
}

_Bool __cshader_check_number(CatchBlock *cb)
{
    if (__cshader_is_integer(cb->begin, cb->end)
        || __cshader_is_float(cb->begin, cb->end))
    {
        cb->ident = NUMBER;
        return 1;
    }
    return 0;
}

/* There's no regex engine, so we must handle it manually... */
_Bool __cshader_check_timestamp(CatchBlock *cb)
{
    const char *ptr = cb->begin + 1;
    while (*ptr++ == ' ')
        ;
    
    if (__cshader_is_float(ptr, cb->end - 1))
    {
        cb->ident = TIMESTAMP;
        return 1;
    }
    return 0;
}

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof(a[0]))
_Bool __cshader_check_level(CatchBlock *cb)
{
    for (int i = 0; i < ARRAY_SIZE(__cshader_lsmap); i++)
    {
        if (__cshader_case_insensitive_compare_equal(cb->begin + 1, cb->end - 1,
            __cshader_lsmap[i].str))
        {
            cb->ident = __cshader_lsmap[i].level;
            return 1;
        }
    }
    return 0;
}

_Bool __cshader_is_llegal_identifier(char const *begin, char const *end)
{
    if (begin > end)
        return 0;

    _Bool allowNumber = 0;
    while (begin <= end)
    {
        if (__cshader_is_number(*begin) && !allowNumber)
            return 0;
        else if (!__cshader_is_lower(*begin) && !__cshader_is_upper(*begin)
            && !__cshader_is_number(*begin) && *begin != '_')
            return 0;
        allowNumber = 1;
        begin++;
    }
    return 1;
}

_Bool __cshader_check_modulename(CatchBlock *cb)
{
    /* A module name is like: org.sora.xxx */
    enum
    {
        Point,
        Identifier
    } expect;

    char const *p = cb->begin;
    char const *pIdBegin = cb->begin + 1, *pIdEnd;
    while (p <= cb->end)
    {
        if (*p == '.' || p == cb->end)
        {
            pIdEnd = p - 1;
            if (!__cshader_is_llegal_identifier(pIdBegin, pIdEnd))
                return 0;
            pIdBegin = p + 1;
        }
        p++;
    }
    cb->ident = MODULE_NAME;
    return 1;
}

void __cshader_catchblock_tochecked(CatchBlock *cb)
{
    switch (cb->ident)
    {
    case MAYBE_TIMESTAMP_OR_LEVEL:
        if (!__cshader_check_timestamp(cb) && !__cshader_check_level(cb))
            cb->ident = CONTENT;
        break;
    case MAYBE_NUMBER:
        if (!__cshader_check_number(cb))
            cb->ident = CONTENT;
        break;
    case MAYBE_MODULE_NAME:
        if (!__cshader_check_modulename(cb))
            cb->ident = CONTENT;
        break;
    default:
        break;
    }
}

void __cshader_apply(FILE *fp, CatchBlock *cb)
{
    Colors color = NONE;
    for (int i = 0; i < ARRAY_SIZE(__cshader_icmap); i++)
    {
        if (__cshader_icmap[i].ident == cb->ident)
        {
            color = __cshader_icmap[i].color;
            break;
        }
    }

    char const *disable_attribute;
    for (int i = 0; i < ARRAY_SIZE(__cshader_camap); i++)
    {
        if (color & __cshader_camap[i].color)
            fprintf(fp, "%s", __cshader_camap[i].attribute);
        if (__cshader_camap[i].color == DISABLE)
            disable_attribute = __cshader_camap[i].attribute;
    }

    size_t siz = cb->end - cb->begin + 1;
    char buffer[siz + 1];
    memcpy(buffer, cb->begin, siz);
    buffer[siz] = '\0';

    fprintf(fp, "%s%s", buffer, disable_attribute);
}

void __cshader_transport(FILE *fp, char const *str)
{
    CatchBlock catchblock;
    char const *ptr;
    while (__cshader_matches_catchblock(str, &catchblock))
    {
        __cshader_catchblock_tochecked(&catchblock);

        ptr = catchblock.begin;
        __cshader_apply(fp, &catchblock);

        str = catchblock.end + 1;
    }
}

void CsShaderTransport(FILE *fp, char const *prefix, char const *content)
{
    if (prefix)
        __cshader_transport(fp, prefix);
    if (content)
        __cshader_transport(fp, content);
    fprintf(fp, "\n");
}

/*
int main(int argc, char const *argv[])
{
    CsShaderTransport(stdout, "[2197.3242] ", "\"String\" rrss 2.2");
    return 0;
}
*/
