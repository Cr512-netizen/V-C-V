#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace nuc::world {

// CPU-side sparse voxel directed acyclic graph. Input is a cubic, power-of-two
// grid in x-fastest order. Identical subtrees are interned and shared.
class Svdag {
public:
    struct Node {
        std::array<std::uint32_t, 8> children{};
        std::uint32_t material{};
        bool leaf{true};
    };

    void build(std::size_t extent, const std::vector<std::uint32_t>& voxels);
    [[nodiscard]] std::uint32_t at(std::size_t x, std::size_t y, std::size_t z) const;
    [[nodiscard]] std::vector<std::uint32_t> decode() const;
    [[nodiscard]] std::size_t extent() const noexcept { return extent_; }
    [[nodiscard]] std::size_t node_count() const noexcept { return nodes_.size(); }
    [[nodiscard]] bool empty() const noexcept { return nodes_.empty(); }
    [[nodiscard]] std::uint32_t root_index() const noexcept { return root_; }
    [[nodiscard]] const std::vector<Node>& nodes() const noexcept { return nodes_; }

private:
    std::size_t extent_{};
    std::vector<Node> nodes_;
    std::uint32_t root_{};
};
} // namespace nuc::world
