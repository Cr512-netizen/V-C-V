#include "render/texture_gen.h"
#include "render/texture_canvas.h"
#include "render/texture_noise.h"

namespace nuc::render {
namespace {
namespace cv = detail::canvas;
using detail::Rgb;
using detail::rgb;
using cv::Canvas;

constexpr int n = cv::extent;

RgbaTile riveted_plate(std::uint64_t seed) {
    Canvas c;
    const Rgb base = rgb(96, 102, 110);
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x) {
            const float blotch = detail::noise(static_cast<float>(x) / 8.0F, static_cast<float>(y) / 8.0F, 4, seed, 1);
            c.at(x, y) = detail::add(base, (blotch - 0.5F) * 0.10F);
        }
    cv::plate_seams(c, 16, rgb(42, 46, 52), rgb(150, 158, 168), rgb(54, 58, 64));
    const Rgb head = rgb(186, 192, 200);
    const Rgb groove = rgb(38, 40, 45);
    cv::rivet(c, 4, 4, head, groove);
    cv::rivet(c, n - 5, 4, head, groove);
    cv::rivet(c, 4, n - 5, head, groove);
    cv::rivet(c, n - 5, n - 5, head, groove);
    cv::scratches(c, 4, rgb(176, 182, 190), seed);
    cv::grain(c, 0.020F, seed, 2);
    return cv::to_tile(c);
}

RgbaTile rusted_sheet(std::uint64_t seed) {
    Canvas c;
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            const float corr = detail::noise(static_cast<float>(x) / 6.0F, static_cast<float>(y) / 6.0F, 5, seed, 3);
            const float fine = detail::noise(static_cast<float>(x) / 2.0F, static_cast<float>(y) / 2.0F, 16, seed, 4);
            Rgb col = detail::blend(rgb(104, 62, 36), rgb(158, 88, 40), corr * 0.75F);
            col = detail::blend(col, rgb(58, 33, 20), (1.0F - corr) * 0.45F);
            c.at(x, y) = detail::add(col, (fine - 0.5F) * 0.07F);
        }
    }
    cv::vertical_runs(c, rgb(48, 26, 14), seed);
    cv::blobs(c, 3, rgb(112, 114, 110), rgb(70, 72, 70), 2.6F, seed, 31);
    cv::grain(c, 0.025F, seed, 5);
    return cv::to_tile(c);
}

RgbaTile painted_grate(std::uint64_t seed) {
    Canvas c;
    const Rgb paint = rgb(52, 92, 58);
    const Rgb metal = rgb(112, 112, 106);
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            const bool raised = (x + y) % 8 < 3;
            const float wear = detail::noise(static_cast<float>(x) / 5.0F, static_cast<float>(y) / 5.0F, 7, seed, 6);
            Rgb col = raised ? detail::scale(paint, 0.80F) : paint;
            col = detail::blend(col, metal, wear > 0.68F ? (wear - 0.68F) * 2.6F : 0.0F);
            c.at(x, y) = col;
        }
    }
    cv::scratches(c, 3, rgb(126, 128, 122), seed);
    cv::grain(c, 0.018F, seed, 7);
    return cv::to_tile(c);
}

RgbaTile coolant(std::uint64_t seed) {
    Canvas c;
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            const float swirl = detail::noise(static_cast<float>(x) / 7.0F, static_cast<float>(y) / 9.0F, 4, seed, 8);
            const float caus = detail::noise(static_cast<float>(x) / 3.0F, static_cast<float>(y) / 3.0F, 11, seed, 9);
            Rgb col = detail::blend(rgb(16, 58, 74), rgb(30, 96, 112), swirl * 0.6F);
            col = detail::blend(col, rgb(96, 196, 208), caus * caus * caus * 0.55F);
            c.at(x, y) = col;
            c.alpha[static_cast<std::size_t>(y * n + x)] = 170;
        }
    }
    for (int k = 0; k < 6; ++k) {
        const int x = static_cast<int>(detail::unit_at(seed, k, 10, 41) * static_cast<float>(n));
        const int y = static_cast<int>(detail::unit_at(seed, k, 11, 42) * static_cast<float>(n));
        c.at(x, y) = detail::blend(c.at(x, y), rgb(150, 220, 230), 0.5F);
    }
    return cv::to_tile(c);
}

RgbaTile ore_in_concrete(std::uint64_t seed) {
    Canvas c;
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            const float speck = detail::unit_at(seed, x, y, 51);
            Rgb col = detail::add(rgb(126, 124, 118), (speck - 0.5F) * 0.12F);
            if (speck > 0.93F) col = detail::add(col, 0.10F);
            if (speck < 0.05F) col = detail::add(col, -0.12F);
            c.at(x, y) = col;
        }
    }
    cv::blobs(c, 4, rgb(168, 96, 48), rgb(84, 136, 108), 2.8F, seed, 61);
    cv::blobs(c, 2, rgb(198, 126, 62), rgb(120, 78, 40), 1.8F, seed, 62);
    cv::grain(c, 0.020F, seed, 52);
    return cv::to_tile(c);
}

RgbaTile dark_rock(std::uint64_t seed) {
    Canvas c;
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            const float strata = detail::noise(static_cast<float>(x) / 10.0F, static_cast<float>(y) / 2.5F, 4, seed, 71);
            Rgb col = detail::blend(rgb(56, 58, 64), rgb(34, 35, 40), strata * 0.6F);
            c.at(x, y) = detail::blend(col, rgb(78, 80, 88), (1.0F - strata) * 0.35F);
        }
    }
    for (int k = 0; k < 14; ++k) {
        const int x = static_cast<int>(detail::unit_at(seed, k, 12, 72) * static_cast<float>(n));
        const int y = static_cast<int>(detail::unit_at(seed, k, 13, 73) * static_cast<float>(n));
        c.at(x, y) = detail::blend(c.at(x, y), rgb(150, 158, 168), 0.55F);
    }
    cv::grain(c, 0.022F, seed, 74);
    return cv::to_tile(c);
}
} // namespace

TilePattern pattern_for(std::uint32_t material) noexcept {
    switch (material) {
    case 1: return TilePattern::riveted_plate;
    case 2: return TilePattern::rusted_sheet;
    case 3: return TilePattern::painted_grate;
    case 4: return TilePattern::coolant;
    case 10: return TilePattern::ore_in_concrete;
    case 11: return TilePattern::dark_rock;
    default: return TilePattern::dark_rock;
    }
}

RgbaTile generate_tile(TilePattern pattern, std::uint32_t material) {
    const std::uint64_t seed = detail::mix(0x5EED0000ULL + material);
    switch (pattern) {
    case TilePattern::riveted_plate: return riveted_plate(seed);
    case TilePattern::rusted_sheet: return rusted_sheet(seed);
    case TilePattern::painted_grate: return painted_grate(seed);
    case TilePattern::coolant: return coolant(seed);
    case TilePattern::ore_in_concrete: return ore_in_concrete(seed);
    case TilePattern::dark_rock: return dark_rock(seed);
    }
    return dark_rock(seed);
}
} // namespace nuc::render
