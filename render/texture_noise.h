#pragma once
#include <array>
#include <cstdint>

namespace nuc::render::detail {
// Deterministic helpers shared by the tile generators. Everything is integer
// hashing and plain float math so a tile renders byte for byte identically on
// every platform, with no state and no global tables.

using Rgb = std::array<float, 3>;

inline std::uint64_t mix(std::uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

// Uniform value in [0, 1) from a 64-bit hash.
inline float unit(std::uint64_t h) {
    return static_cast<float>(h >> 40) / 16777216.0F;
}

inline float unit_at(std::uint64_t seed, int x, int y, int salt) {
    const std::uint64_t h = mix(seed ^ (static_cast<std::uint64_t>(salt) << 40) ^
                                (static_cast<std::uint64_t>(x) << 20) ^
                                static_cast<std::uint64_t>(y));
    return unit(h);
}

inline float smoothstep01(float t) { return t * t * (3.0F - 2.0F * t); }

// Value noise on a wrapping lattice, so sampling across the tile edge stays
// continuous and a repeated tile shows no seam.
inline float noise(float x, float y, int period, std::uint64_t seed, int salt) {
    const auto ix = static_cast<int>(x);
    const auto iy = static_cast<int>(y);
    const float fx = x - static_cast<float>(ix);
    const float fy = y - static_cast<float>(iy);
    const int x0 = ((ix % period) + period) % period;
    const int y0 = ((iy % period) + period) % period;
    const int x1 = (x0 + 1) % period;
    const int y1 = (y0 + 1) % period;
    const float a = unit_at(seed, x0, y0, salt);
    const float b = unit_at(seed, x1, y0, salt);
    const float c = unit_at(seed, x0, y1, salt);
    const float d = unit_at(seed, x1, y1, salt);
    const float sx = smoothstep01(fx);
    const float sy = smoothstep01(fy);
    const float top = a + (b - a) * sx;
    const float bottom = c + (d - c) * sx;
    return top + (bottom - top) * sy;
}

inline Rgb rgb(int r, int g, int b) {
    return {static_cast<float>(r) / 255.0F, static_cast<float>(g) / 255.0F,
            static_cast<float>(b) / 255.0F};
}

inline Rgb blend(Rgb a, Rgb b, float t) {
    const float k = t < 0.0F ? 0.0F : (t > 1.0F ? 1.0F : t);
    return {a[0] + (b[0] - a[0]) * k, a[1] + (b[1] - a[1]) * k, a[2] + (b[2] - a[2]) * k};
}

inline Rgb scale(Rgb c, float k) {
    return {c[0] * k, c[1] * k, c[2] * k};
}

inline Rgb add(Rgb c, float d) { return {c[0] + d, c[1] + d, c[2] + d}; }

inline std::uint8_t to_byte(float v) {
    const float scaled = v * 255.0F + 0.5F;
    if (scaled <= 0.0F) return 0;
    if (scaled >= 255.0F) return 255;
    return static_cast<std::uint8_t>(scaled);
}
} // namespace nuc::render::detail
