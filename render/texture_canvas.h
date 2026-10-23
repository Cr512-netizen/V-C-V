#pragma once
#include "render/texture_gen.h"
#include "render/texture_noise.h"
#include <cmath>

namespace nuc::render::detail {
// Painting surface shared by every tile generator: a float colour buffer plus
// an alpha plane, converted to RGBA8 in one place. Helpers are inline so the
// pattern file stays short and each tile recipe reads as a list of strokes.
namespace canvas {
constexpr int extent = tile_extent;
constexpr int cells = extent * extent;

struct Canvas {
    Canvas() { alpha.fill(255); } // opaque unless a recipe says otherwise
    std::array<Rgb, cells> color{};
    std::array<std::uint8_t, cells> alpha{};
    Rgb& at(int x, int y) { return color[static_cast<std::size_t>(y * extent + x)]; }
    [[nodiscard]] const Rgb& at(int x, int y) const { return color[static_cast<std::size_t>(y * extent + x)]; }
};

inline int wrap(int v) { return ((v % extent) + extent) % extent; }

inline RgbaTile to_tile(const Canvas& c) {
    RgbaTile tile;
    for (int y = 0; y < extent; ++y) {
        for (int x = 0; x < extent; ++x) {
            const auto i = static_cast<std::size_t>(y * extent + x);
            const auto& col = c.color[i];
            tile.pixels[i * 4 + 0] = to_byte(col[0]);
            tile.pixels[i * 4 + 1] = to_byte(col[1]);
            tile.pixels[i * 4 + 2] = to_byte(col[2]);
            tile.pixels[i * 4 + 3] = c.alpha[i];
        }
    }
    return tile;
}

inline void fill(Canvas& c, Rgb base) {
    c.color.fill(base);
    c.alpha.fill(255);
}

inline void grain(Canvas& c, float amount, std::uint64_t seed, int salt) {
    for (int y = 0; y < extent; ++y)
        for (int x = 0; x < extent; ++x)
            c.at(x, y) = add(c.at(x, y), (unit_at(seed, x, y, salt) * 2.0F - 1.0F) * amount);
}

// Dark grooves on the plate boundaries plus a lit top-left and shaded
// bottom-right bevel, so a flat tile still reads as a raised slab.
inline void plate_seams(Canvas& c, int step, Rgb groove, Rgb light, Rgb shade) {
    for (int y = 0; y < extent; ++y) {
        for (int x = 0; x < extent; ++x) {
            if (x % step == 0 || y % step == 0) { c.at(x, y) = blend(c.at(x, y), groove, 0.55F); continue; }
            if (x % step == 1 || y % step == 1) c.at(x, y) = blend(c.at(x, y), light, 0.35F);
            if (x % step == step - 1 || y % step == step - 1) c.at(x, y) = blend(c.at(x, y), shade, 0.30F);
        }
    }
}

inline void rivet(Canvas& c, int cx, int cy, Rgb metal, Rgb groove) {
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            const float dist = static_cast<float>(dx * dx + dy * dy);
            if (dist > 5.0F) continue;
            const int x = wrap(cx + dx);
            const int y = wrap(cy + dy);
            if (dist > 3.5F) { c.at(x, y) = blend(c.at(x, y), groove, 0.6F); continue; }
            const float lift = 1.0F - dist / 6.0F - static_cast<float>(dx + dy) * 0.04F;
            c.at(x, y) = blend(c.at(x, y), metal, 0.35F + lift * 0.5F);
        }
    }
}

inline void scratches(Canvas& c, int count, Rgb tint, std::uint64_t seed) {
    for (int k = 0; k < count; ++k) {
        float x = unit_at(seed, k, 1, 11) * static_cast<float>(extent);
        float y = unit_at(seed, k, 2, 12) * static_cast<float>(extent);
        const int length = 6 + static_cast<int>(unit_at(seed, k, 3, 13) * 10.0F);
        for (int s = 0; s < length; ++s) {
            c.at(wrap(static_cast<int>(x)), wrap(static_cast<int>(y))) =
                blend(c.at(wrap(static_cast<int>(x)), wrap(static_cast<int>(y))), tint, 0.22F);
            x += 0.9F;
            y += 0.55F;
        }
    }
}

inline void vertical_runs(Canvas& c, Rgb dark, std::uint64_t seed) {
    for (int k = 0; k < 5; ++k) {
        const int x = static_cast<int>(unit_at(seed, k, 4, 21) * static_cast<float>(extent));
        const int start = static_cast<int>(unit_at(seed, k, 5, 22) * static_cast<float>(extent));
        const int length = 8 + static_cast<int>(unit_at(seed, k, 6, 23) * 14.0F);
        for (int s = 0; s < length; ++s) {
            const int y = (start + s) % extent;
            c.at(wrap(x), y) = blend(c.at(x, y), dark, 0.45F * (1.0F - static_cast<float>(s) / static_cast<float>(length)));
        }
    }
}

inline void blobs(Canvas& c, int count, Rgb core, Rgb rim, float radius, std::uint64_t seed, int salt) {
    for (int k = 0; k < count; ++k) {
        const float cx = unit_at(seed, k, 7, salt) * static_cast<float>(extent);
        const float cy = unit_at(seed, k, 8, salt + 1) * static_cast<float>(extent);
        const float r = radius * (0.6F + unit_at(seed, k, 9, salt + 2) * 0.8F);
        for (int dy = -4; dy <= 4; ++dy) {
            for (int dx = -4; dx <= 4; ++dx) {
                const float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                if (dist > r) continue;
                const int x = wrap(static_cast<int>(cx) + dx);
                const int y = wrap(static_cast<int>(cy) + dy);
                const float t = 1.0F - dist / r;
                c.at(x, y) = blend(c.at(x, y), dist > r - 0.9F ? rim : core, t * 0.85F);
            }
        }
    }
}
} // namespace canvas
} // namespace nuc::render::detail
