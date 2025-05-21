#pragma once
#include <stdint.h>


#define SAFE_DELETE(p) \
    {                  \
        delete (p);    \
        (p) = nullptr; \
    }
#define SAFE_FREE(p)   \
    {                  \
        free(p);       \
        (p) = nullptr; \
    }
#define SAFE_DESTROY(p)     \
    {                       \
        if (p) {            \
            (p)->destroy(); \
            (p) = nullptr;  \
        }                   \
    }
#define SAFE_DESTROY_AND_DELETE(p) \
    {                              \
        if (p) {                   \
            (p)->destroy();        \
            delete (p);            \
            (p) = nullptr;         \
        }                          \
    }


#define ASSERT_F(expr, format, ...) \
    EASTL_ASSERT_MSG(0, eastl::string(eastl::string::CtorSprintf(), format, __VA_ARGS__).c_str());


//////////////////////////////////////////////////////////////////////////
// forward for classes
#define FORWARD_DECLARATION_1(c) class c;
#define FORWARD_DECLARATION_2(n1, c) \
    namespace n1 {                   \
    FORWARD_DECLARATION_1(c);        \
    }
#define FORWARD_DECLARATION_3(n1, n2, c) \
    namespace n1 {                       \
    FORWARD_DECLARATION_2(n2, c);        \
    }
#define FORWARD_DECLARATION_4(n1, n2, n3, c) \
    namespace n1 {                           \
    FORWARD_DECLARATION_3(n2, n3, c);        \
    }

// forward for structs
#define FORWARD_DECLARATION_1S(c) struct c;
#define FORWARD_DECLARATION_2S(n1, c) \
    namespace n1 {                    \
    FORWARD_DECLARATION_1S(c)         \
    }
#define FORWARD_DECLARATION_3S(n1, n2, c) \
    namespace n1 {                        \
    FORWARD_DECLARATION_2S(n2, c);        \
    }
#define FORWARD_DECLARATION_4S(n1, n2, n3, c) \
    namespace n1 {                            \
    FORWARD_DECLARATION_3S(n2, n3, c);        \
    }
//////////////////////////////////////////////////////////////////////////
