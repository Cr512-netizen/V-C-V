#pragma once
#include <filesystem>
#include <string_view>

namespace nuc::test {
// Tests never write outside the build tree. A sandboxed child process may be
// confined to the workspace, so %TEMP% is not reliably writable, and a CI
// runner should not care about the host's temp policies either.
[[nodiscard]] inline std::filesystem::path scratch_dir(std::string_view name) {
    std::error_code ec;
    std::filesystem::path dir = std::filesystem::path(NUC_TEST_TMP_DIR) / name;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir, ec);
    return dir;
}

[[nodiscard]] inline std::filesystem::path scratch_file(std::string_view name) {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(NUC_TEST_TMP_DIR), ec);
    return std::filesystem::path(NUC_TEST_TMP_DIR) / name;
}
} // namespace nuc::test
