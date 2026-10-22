#include "world/svdag.h"
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace nuc::world {
namespace {
struct Key {
    bool leaf{};
    std::uint32_t material{};
    std::array<std::uint32_t, 8> children{};
    bool operator==(const Key&) const = default;
};
struct KeyHash {
    std::size_t operator()(const Key& k) const noexcept {
        std::size_t h = k.leaf ? 0x9e3779b9u : 0x85ebca6bu;
        auto mix = [&h](std::uint32_t v) { h ^= static_cast<std::size_t>(v) + 0x9e3779b97f4a7c15ULL + (h << 6u) + (h >> 2u); };
        mix(k.material);
        for (auto v : k.children) mix(v);
        return h;
    }
};
}
void Svdag::build(std::size_t extent, const std::vector<std::uint32_t>& voxels) {
    if (extent == 0 || (extent & (extent - 1)) != 0)
        throw std::invalid_argument("SVDAG extent must be a non-zero power of two");
    if (extent > 1024 || extent > std::numeric_limits<std::size_t>::max() / extent ||
        extent * extent > std::numeric_limits<std::size_t>::max() / extent)
        throw std::invalid_argument("SVDAG extent is too large");
    if (voxels.size() != extent * extent * extent)
        throw std::invalid_argument("SVDAG voxel buffer size does not match extent");

    std::vector<Node> nodes;
    std::unordered_map<Key, std::uint32_t, KeyHash> interned;
    auto intern = [&](const Key& key) -> std::uint32_t {
        if (auto it = interned.find(key); it != interned.end()) return it->second;
        if (nodes.size() >= std::numeric_limits<std::uint32_t>::max())
            throw std::overflow_error("SVDAG node index overflow");
        const auto index = static_cast<std::uint32_t>(nodes.size());
        nodes.push_back(Node{key.children, key.material, key.leaf});
        interned.emplace(key, index);
        return index;
    };

    const auto build_region = [&](auto&& self, std::size_t x, std::size_t y, std::size_t z,
                                  std::size_t size) -> std::uint32_t {
        const auto first = voxels[x + extent * (y + extent * z)];
        bool uniform = true;
        for (std::size_t dz = 0; dz < size && uniform; ++dz)
            for (std::size_t dy = 0; dy < size && uniform; ++dy)
                for (std::size_t dx = 0; dx < size; ++dx)
                    if (voxels[(x + dx) + extent * ((y + dy) + extent * (z + dz))] != first) {
                        uniform = false; break;
                    }
        if (uniform) {
            Key key{}; key.leaf = true; key.material = first;
            return intern(key);
        }
        const std::size_t half = size / 2;
        Key key{}; key.leaf = false;
        for (std::size_t i = 0; i < 8; ++i) {
            const auto dx = (i & 1u) ? half : 0;
            const auto dy = (i & 2u) ? half : 0;
            const auto dz = (i & 4u) ? half : 0;
            key.children[i] = self(self, x + dx, y + dy, z + dz, half);
        }
        return intern(key);
    };

    const auto root = build_region(build_region, 0, 0, 0, extent);
    extent_ = extent;
    nodes_ = std::move(nodes);
    root_ = root;
}

std::uint32_t Svdag::at(std::size_t x, std::size_t y, std::size_t z) const {
    if (empty()) throw std::logic_error("SVDAG has not been built");
    if (x >= extent_ || y >= extent_ || z >= extent_) throw std::out_of_range("SVDAG voxel coordinate out of bounds");
    std::uint32_t index = root_;
    std::size_t size = extent_, ox = 0, oy = 0, oz = 0;
    while (!nodes_[index].leaf) {
        const auto half = size / 2;
        std::size_t child = 0;
        if (x >= ox + half) { child |= 1u; ox += half; }
        if (y >= oy + half) { child |= 2u; oy += half; }
        if (z >= oz + half) { child |= 4u; oz += half; }
        index = nodes_[index].children[child];
        size = half;
    }
    return nodes_[index].material;
}

std::vector<std::uint32_t> Svdag::decode() const {
    if (empty()) return {};
    std::vector<std::uint32_t> result(extent_ * extent_ * extent_);
    for (std::size_t z = 0; z < extent_; ++z)
        for (std::size_t y = 0; y < extent_; ++y)
            for (std::size_t x = 0; x < extent_; ++x)
                result[x + extent_ * (y + extent_ * z)] = at(x, y, z);
    return result;
}
} // namespace nuc::world
