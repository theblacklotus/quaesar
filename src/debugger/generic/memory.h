#pragma once
#include <EASTL/string.h>

namespace qd {

typedef uint32_t AddrRef;

class MemBank {
public:
    int id = -1;
    AddrRef startAddr = 0;
    uint32_t size = 0;
    uint32_t mask = 0;
    eastl::string name;
    eastl::string label;
    uint8_t* realAddr;

public:
};  // class MemBank

};  // namespace qd
