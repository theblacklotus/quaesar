#pragma once
#include <EASTL/internal/config.h>
#include <debugger/generic/memory.h>
#include <stdint.h>

#if !defined(NOMINMAX)
#define NOMINMAX 1
#endif  // !defined(NOMINMAX)
#undef min
#undef max

namespace qd {

template <typename T1, typename T2>
[[nodiscard]] inline T1 min(const T1 __a, const T2 __b) {
    return (T1)__a < (T1)__b ? (T1)__a : (T1)__b;
}

template <typename T1, typename T2>
[[nodiscard]] inline T1 max(const T1 __a, const T2 __b) {
    return (T1)__a > (T1)__b ? (T1)__a : (T1)__b;
}

template <typename T, typename T1, typename T2>
[[nodiscard]] inline constexpr T clamp(const T Value, const T1 tMin, const T2 tMax) {
    EASTL_ASSERT(tMax >= tMin);
    if (Value < tMin)
        return tMin;
    if (Value > tMax)
        return tMax;
    return Value;
}

// returns byte color to float [0.0f - 1.0f]
inline static constexpr float byte_to_float_01(uint8_t x) {
    union {
        float f;
        uint32_t i;
    } u = {u.f = 32768.0f};
    u.i |= x;
    return (u.f - 32768.0f) * (256.0f / 255.0f);
}

template <typename TInt>
class TPoint {
    typedef TPoint<TInt> TThis;

public:
    TInt x, y;

    template <typename TInt2>
    inline TPoint(const TPoint<TInt2>& p) : x((TInt)p.x), y((TInt)p.y) {
    }

    template <typename T0, typename T1>
    inline TPoint(T0 _x, T1 _y) : x((TInt)_x), y((TInt)_y) {
    }

    template <typename TVal>
    inline TThis& operator=(const TPoint<TVal>& p) {
        x = (TInt)p.x;
        y = (TInt)p.y;
        return *this;
    }

    inline bool operator==(const TPoint& p) const {
        return x == p.x && y == p.y;
    }

    inline bool operator!=(const TPoint& p) const {
        return x != p.x || y != p.y;
    }

    inline TPoint& operator+=(const TPoint& p) {
        x += p.x;
        y += p.y;
        return *this;
    }

    inline TPoint& operator*=(const TPoint& p) {
        x *= p.x;
        y *= p.y;
        return *this;
    }

    inline TPoint& operator/=(const TPoint& p) {
        x /= p.x;
        y /= p.y;
        return *this;
    }

    inline TPoint& operator-=(const TPoint& p) {
        x -= p.x;
        y -= p.y;
        return *this;
    }

    template <typename T0, typename T1>
    inline void set(const T0& _x, const T1& _y) {
        x = (TInt)_x;
        y = (TInt)_y;
    }

    template <typename TVal>
    inline void set(const TPoint<TVal>& p) {
        x = (TInt)p.x;
        y = (TInt)p.y;
    }

};  // class TPoint

using IVec2 = TPoint<int>;
using Vec2 = TPoint<float>;

};  // namespace qd
