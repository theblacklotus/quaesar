#pragma once
#include <EASTL/fixed_string.h>
#include <EASTL/string.h>

namespace eastl {

template <size_t S, bool bEnableOverflow = true>
using inline_string = eastl::fixed_string<char, S + 1, bEnableOverflow, eastl::allocator>;

}  // namespace eastl
