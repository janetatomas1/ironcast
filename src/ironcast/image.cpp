#include <fstream>
#include <iterator>
#include <utility>

#include "ironcast/assert.hpp"

// Static, so it can't clash with another copy of stb_image linked into the app.
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "ironcast/image.hpp"
#include "ironcast/log.hpp"

namespace ironcast {
    Image::Image(int width, int height, std::vector<std::byte> rgba)
    : pixels_(std::move(rgba)), width_(width), height_(height) {
        IRONCAST_DEBUG_ASSERT(width > 0 && height > 0, "Image with an empty size", width, height);
        IRONCAST_DEBUG_ASSERT(
            pixels_.size() == static_cast<size_t>(width) * static_cast<size_t>(height) * 4,
            "Image pixels don't match its size", pixels_.size(), width, height
        );
    }

    std::span<const std::byte> Image::pixels() const {
        return pixels_;
    }

    int Image::width() const {
        return width_;
    }

    int Image::height() const {
        return height_;
    }

    std::optional<Image> Image::from_file(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);

        if (!file) {
            logger().warn("can't open image {}", path.string());
            return std::nullopt;
        }
        std::vector<char> bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        return from_memory(std::as_bytes(std::span(bytes)));
    }

    std::optional<Image> Image::from_memory(std::span<const std::byte> data) {
        int width = 0;
        int height = 0;
        int channels = 0;
        // 4 = always convert to RGBA.
        stbi_uc* pixels = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(data.data()),
            static_cast<int>(data.size()),
            &width,
            &height,
            &channels,
            4
        );

        if (pixels == nullptr) {
            logger().warn("can't decode image: {}", stbi_failure_reason());
            return std::nullopt;
        }
        const auto view = std::as_bytes(std::span(pixels, static_cast<size_t>(width) * static_cast<size_t>(height) * 4));
        Image image(width, height, std::vector<std::byte>(view.begin(), view.end()));
        stbi_image_free(pixels);
        return image;
    }
}
