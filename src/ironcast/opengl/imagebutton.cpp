#include <algorithm>

#include "ironcast/opengl/imagebutton.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast::opengl {
    ImageButton::ImageButton(const std::string& title, Widget* parent): RenderWidget(parent) {
        set_title(title);
    }

    ImageButton::ImageButton(const std::string& title, fu2::unique_function<void()> callback, Widget* parent)
    : RenderWidget(parent), callback_(std::move(callback)) {
        set_title(title);
    }

    void ImageButton::set_callback(fu2::unique_function<void()> callback) {
        callback_ = std::move(callback);
    }

    bool ImageButton::show() {
        if (texture() == nullptr) {
            return false;
        }

        render_texture();

        // ImGui adds FramePadding around the image; take it out so the button fills the box.
        const ImVec2 padding = ImGui::GetStyle().FramePadding;
        const ImVec2 imageSize{
            std::max(size().x - padding.x * 2.0f, 0.0f),
            std::max(size().y - padding.y * 2.0f, 0.0f)
        };

        if (ImGui::ImageButton(title_id().c_str(), static_cast<ImTextureID>(texture()->id()), imageSize, uv0(), uv1())) {
            if (callback_) {
                post([](auto& self) {
                    if (self.callback_) {
                        self.callback_();
                    }
                });
            }

            return true;
        }

        return false;
    }
}
