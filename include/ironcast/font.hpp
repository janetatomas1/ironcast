#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

namespace ironcast {
    // TTF / OTF bytes, loaded without an ImGui context. Each window's font atlas borrows them
    // (FontDataOwnedByAtlas = false) and ImGui reads them for as long as the atlas lives, so a Font must
    // outlive every window using it: keep it in the app (Ironcast::add_font) or in static storage.
    class Font {
        // Empty for a Font that only views bytes it doesn't own.
        std::vector<std::byte> owned_;
        std::span<const std::byte> view_;

    public:
        // Views data without copying it; data must outlive the Font's use, e.g. static data.
        explicit Font(std::span<const std::byte> data);
        // Owns data.
        explicit Font(std::vector<std::byte> data);

        // The atlas borrows data(), so a copy would hand out a second Font pointing at the same bytes.
        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;
        Font(Font&&) noexcept = default;
        Font& operator=(Font&&) noexcept = default;

        [[nodiscard]] std::span<const std::byte> data() const;

        // std::nullopt if the file can't be read.
        [[nodiscard]] static std::optional<Font> from_file(const std::filesystem::path& path);

#ifdef IRONCAST_BUNDLE_FONTS
        // Fonts compiled into ironcast (IRONCAST_BUNDLE_FONTS), Regular weight. They view the bytes in the
        // binary, so they need no add_font() and are never copied.
        [[nodiscard]] static const Font& inter();
        [[nodiscard]] static const Font& jetbrains_mono();
        // Latin, Greek and Cyrillic only.
        [[nodiscard]] static const Font& noto_sans();
#endif
    };
}
