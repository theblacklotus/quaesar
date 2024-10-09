#pragma once
#include <debugger/generic/types.h>

namespace qd {

class Color {
public:
    enum EColor : uint32_t {
        // AABBGGRR     AABBGGRR
        BLACK = 0xFF000000ui32,
        RED = 0xFF0000FFui32,
        GREEN = 0xFF00FF00ui32,
        BLUE = 0xFFFF0000ui32,
        WHITE = 0xFFFFFFFFui32,
        YELLOW = 0xFF00FFFFui32,
        CYAN = 0xFFFFFF00ui32,
        MAGENTA = 0xFFFF00FFui32,
    };

public:
    union {
        struct {
            uint8_t r, g, b, a;
        };
        uint32_t mColor;
        Color::EColor mEColor;
    };

    inline Color() : mColor((uint32_t)Color::WHITE) {
    }

    inline Color(uint32_t d) : mColor(d) {
    }

    inline Color(Color::EColor d) : mColor((uint32_t)d) {
    }

    inline Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255u) {
        set(_r, _g, _b, _a);
    }

    inline uint32_t getU32() const {
        return mColor;
    }

    inline void set(uint32_t Color) {
        mColor = Color;
    }
    inline void set(Color::EColor Clr) {
        mColor = (uint32_t)Clr;
    }

    inline constexpr void set(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255u) {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }

    inline void setF(float _r, float _g, float _b, float _a = 1.0f) {
        setRedF(_r);
        setGreenF(_g);
        setBlueF(_b);
        setAlphaF(_a);
    }

    uint32_t getColor32() const {
        return mColor;
    }
    void setColor32(uint32_t Color) {
        set(Color);
    }

    static inline Color makeFromF(float _r, float _g, float _b, float _a = 1.0f) {
        Color Color;
        Color.setF(_r, _g, _b, _a);
        return Color;
    }

    inline uint8_t getR() const {
        return r;
    }
    inline uint8_t getG() const {
        return g;
    }
    inline uint8_t getB() const {
        return b;
    }
    inline uint8_t getA() const {
        return a;
    }

    inline Color& setR(uint8_t _r) {
        r = _r;
        return *this;
    }
    inline Color& setG(uint8_t _g) {
        g = _g;
        return *this;
    }
    inline Color& setB(uint8_t _b) {
        b = _b;
        return *this;
    }
    inline Color& setA(uint8_t _a) {
        a = _a;
        return *this;
    }

    inline float getRF() const {
        return byte_to_float_01(r);
    }
    inline float getGF() const {
        return byte_to_float_01(g);
    }
    inline float getBF() const {
        return byte_to_float_01(b);
    }
    inline float getAlphaF() const {
        return byte_to_float_01(a);
    }

    inline Color& setRedF(float _r) {
        r = (uint8_t)(clamp(static_cast<uint32_t>(_r * 255.0f), 0u, 255u));
        return *this;
    }
    inline Color& setGreenF(float _g) {
        g = (uint8_t)(clamp(static_cast<uint32_t>(_g * 255.0f), 0u, 255u));
        return *this;
    }
    inline Color& setBlueF(float _b) {
        b = (uint8_t)(clamp(static_cast<uint32_t>(_b * 255.0f), 0u, 255u));
        return *this;
    }
    inline Color& setAlphaF(float _a) {
        a = (uint8_t)(clamp(static_cast<uint32_t>(_a * 255.0f), 0u, 255u));
        return *this;
    }

    inline void setWhite() {
        r = g = b = a = 255u;
    }
    inline void setBlack() {
        r = g = b = 0;
        a = 255u;
    }

    void setWhiteAlpha(uint8_t _a = 255) {
        r = g = b = 255;
        a = _a;
    }

    void setBlackAlpha(uint8_t _a = 255) {
        r = g = b = 0;
        a = _a;
    }

    static Color makeWhite(uint8_t _a = 255) {
        Color c;
        c.setWhiteAlpha(_a);
        return c;
    }
    static Color makeBlack(uint8_t _a = 255) {
        Color c;
        c.setBlackAlpha(_a);
        return c;
    }

    // Operators.
    template <typename TColorType>
    inline bool operator==(const TColorType& C) const {
        return mColor == (Color)C;
    }
    template <typename TColorType>
    inline bool operator!=(const TColorType& C) const {
        return mColor != (Color)C;
    }

    void add(const Color& C) {
        r = (uint8_t)qd::min((int)r + (int)C.r, 255);
        g = (uint8_t)qd::min((int)g + (int)C.g, 255);
        b = (uint8_t)qd::min((int)b + (int)C.b, 255);
        a = (uint8_t)qd::min((int)a + (int)C.a, 255);
    }

    void inline operator+=(const Color& c) {
        add(c);
    }

    void toColorF(float& _r, float& _g, float& _b, float& _a) const {
        _r = r * (1.f / 255.f);
        _g = g * (1.f / 255.f);
        _b = b * (1.f / 255.f);
        _a = a * (1.f / 255.f);
    }

    inline operator uint32_t() const {
        return getU32();
    }

};  // class Color
//////////////////////////////////////////////////////////////////////////

};  // namespace qd
