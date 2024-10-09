#include "screen_wnd.h"
#include <SDL.h>
#include <debugger/debugger.h>
#include <debugger/vm.h>
#include <imgui_eastl.h>
#include <quaesar.h>

namespace qd::window {
QDB_WINDOW_REGISTER(ScreenWnd);

ScreenWnd::ScreenWnd(UiViewCreate* cp) : UiWindow(cp) {
    mTitle = "Screen";
}

void ScreenWnd::drawContent() {
    VM* vm = getDbg()->getVm();
    ImGuiIO& io = ImGui::GetIO();

    int amiga_width = vm->getScreenSizeX();
    int amiga_height = vm->getScreenSizeY();

    if (!mTextureId) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
            return;

        SDL_Texture* scrTexture = SDL_CreateTexture(getDbg()->renderer, SDL_PIXELFORMAT_ARGB8888,
                                                    SDL_TEXTUREACCESS_STREAMING, amiga_width, amiga_height);
        if (scrTexture) {
            mTextureId = scrTexture;
            SDL_SetTextureBlendMode(scrTexture, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(scrTexture, SDL_ScaleModeLinear);
        } else {
            SDL_Log("Could not create texture: %s", SDL_GetError());
        }
    }

    if (mTextureId) {
        SDL_Texture* scrTexture = static_cast<SDL_Texture*>(mTextureId);
        void* pixels = nullptr;
        int pitch;
        if (SDL_LockTexture(scrTexture, nullptr, &pixels, &pitch) == 0) {
            int vbSizeX = 0;
            int vbSizeY = 0;
            int vbPitch = 0;
            void* scrBuf = vm->blitter->getScreenPixBuf(0, &vbSizeX, &vbSizeY, &vbPitch);

            if (scrBuf) {
                for (int y = 0; y < amiga_height; y++) {
                    uint8_t* sptr = (uint8_t*)scrBuf + (y * vbPitch);
                    uint32_t* dest = ((uint32_t*)pixels) + (y * amiga_width);
                    // memcpy(dest, sptr, amiga_width * 4);
                    for (int x = 0; x < amiga_height; ++x) {
                        Color c = *(uint32_t*)(sptr);
                        c.a = 255;
                        *dest = c;
                        ++dest;
                        sptr += 4;
                    }
                }
            }
            SDL_UnlockTexture(scrTexture);
        }
    }

    ImVec2 scrollingChildSize = ImVec2(ImGui::GetWindowWidth() - 10, ImGui::GetWindowHeight() - 20);
    ImGui::BeginChild("##scrolling", scrollingChildSize, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::Image(mTextureId, ImVec2(amiga_width, amiga_height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                 ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImGui::GetStyleColorVec4(ImGuiCol_Border));
    ImGui::EndChild();
}

};  // namespace qd::window
