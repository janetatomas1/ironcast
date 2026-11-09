#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

namespace ironcast {
    // Decoded RGBA8 pixels, loaded without a GL or ImGui context. Each window that draws it uploads its own
    // texture from them (a texture belongs to one GL context). Keep it in the app (Ironcast::add_image).
    class Image {
        std::vector<std::byte> pixels_;
        int width_ = 0;
        int height_ = 0;

    public:
        // rgba holds width * height * 4 bytes, rows top to bottom.
        Image(int width, int height, std::vector<std::byte> rgba);

        // Windows refer to it by address; a copy would be a second image nobody draws.
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;
        Image(Image&&) noexcept = default;
        Image& operator=(Image&&) noexcept = default;

        [[nodiscard]] std::span<const std::byte> pixels() const;
        [[nodiscard]] int width() const;
        [[nodiscard]] int height() const;

        // PNG, JPEG, BMP, TGA, GIF (first frame), PSD, HDR, PIC, PNM via stb_image.
        // std::nullopt if the data can't be read or decoded.
        [[nodiscard]] static std::optional<Image> from_file(const std::filesystem::path& path);
        [[nodiscard]] static std::optional<Image> from_memory(std::span<const std::byte> data);
    };
}
