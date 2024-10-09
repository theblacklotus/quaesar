#include "mem_graph_wnd.h"
#include <EASTL/span.h>
#include <SDL.h>
#include <debugger/debugger.h>
#include <debugger/vm.h>
#include <imgui_eastl.h>
#include <quaesar.h>

namespace qd::window {
QDB_WINDOW_REGISTER(MemoryGraphWnd);

MemoryGraphWnd::MemoryGraphWnd(UiViewCreate* cp) : UiWindow(cp) {
    mTitle = "Memory graph";
}

void MemoryGraphWnd::drawContent() {
    VM* vm = getDbg()->getVm();

    mTextureSize.y = ImGui::GetWindowHeight() - 150;

    float curTime = ImGui::GetTime();

    if (/*(curTime - mLastTextureCreateTime) > 0.1f &&*/ (!mTextureId || mTextureSize != mPrevTextureSize)) {
        if (mTextureId) {
            SDL_DestroyTexture((SDL_Texture*)mTextureId);
            mTextureId = nullptr;
        }
        if (mTextureSize.y > 1 && mTextureSize.y > 0) {
            SDL_Texture* scrTexture = nullptr;
            scrTexture = SDL_CreateTexture(getDbg()->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                           mTextureSize.x, mTextureSize.y);
            if (scrTexture) {
                mTextureId = scrTexture;
                SDL_SetTextureBlendMode(scrTexture, SDL_BLENDMODE_BLEND);
                SDL_SetTextureScaleMode(scrTexture, SDL_ScaleModeLinear);
                mPrevTextureSize = mTextureSize;
                mLastTextureCreateTime = curTime;
            } else {
                SDL_Log("Can't not create texture: %s", SDL_GetError());
            }
        }
    }

    ImGuiIO& io = ImGui::GetIO();

    const MemBank* pCurBank = vm->mem->getBankByInd(mCurBank);
    if (!pCurBank) {
        mCurBank = 0;
        return;
    }

    int addrPtr = mBankOffset + pCurBank->startAddr;
    if (ImGui::DragInt("Address", &addrPtr, 1.f, 0, 0xFFFFFF, "%08xh")) {
        mBankOffset = addrPtr;
    }

    if (ImGui::BeginCombo("Memory bank", pCurBank ? pCurBank->name.c_str() : "null", ImGuiComboFlags_None)) {
        eastl::span<const qd::MemBank> banks = vm->mem->banks;
        for (int nBank = 0; nBank < banks.size(); ++nBank) {
            const qd::MemBank& curBank = banks[nBank];
            if (ImGui::Selectable(curBank.name.c_str(), nBank == mCurBank)) {
                mCurBank = nBank;
                pCurBank = vm->mem->getBankByInd(mCurBank);
            }
        }
        ImGui::EndCombo();
    }

    if (mTextureId && (curTime - mLastTextureCreateTime) > 0.1f) {
        SDL_Texture* scrTexture = static_cast<SDL_Texture*>(mTextureId);
        void* pixels = nullptr;
        int pitch;
        if (SDL_LockTexture(scrTexture, nullptr, &pixels, &pitch) == 0) {
            uint8_t* sptr = (uint8_t*)vm->mem->getRealAddr(mBankOffset + pCurBank->startAddr);

            // Change pixels
            uint32_t* dest = ((uint32_t*)pixels) + (0 * mTextureSize.x);
            const int rowBytes = mTextureSize.x / 8;
            for (int y = 0; y < mTextureSize.y; y++) {
                if (sptr + rowBytes < pCurBank->realAddr + pCurBank->size) {
                    for (int x = 0; x < rowBytes; x++) {
                        uint8_t sb = *sptr;
                        uint8_t m = 0x80;
                        for (int b = 0; b < 8; b++) {
                            Color c = (sb & m) != 0 ? Color::WHITE : Color::BLACK;
                            *dest = c;
                            dest++;
                            m >>= 1;
                        }
                        ++sptr;
                    }
                } else {
                    for (int x = 0; x < rowBytes; x++)
                        (*dest++) = Color::BLACK;
                }
            }
            SDL_UnlockTexture(scrTexture);
        } else
            SDL_Log("Cant lock texture");
    }
    // address slider
    {
        int addrPtr = pCurBank->size - mBankOffset;
        if (ImGui::VSliderInt("##AddrSlider", ImVec2(32.0f, (float)mTextureSize.y), &addrPtr, 0, pCurBank->size, "")) {
            mBankOffset = pCurBank->size - addrPtr;
        }
    }
    ImGui::SameLine();

    ImVec2 scrollingChildSize = ImVec2(ImGui::GetWindowWidth() - 100, mTextureSize.y + 32);
    ImGui::BeginChild("##scrolling", scrollingChildSize, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::Image(mTextureId, ImVec2(mTextureSize.x, mTextureSize.y), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                 ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImGui::GetStyleColorVec4(ImGuiCol_Border));
    ImGui::EndChild();

    // texture width
    {
        int& txSize = mTextureSize.x;
        if (ImGui::Button("<<"))
            txSize /= 2;
        ImGui::SameLine();
        if (ImGui::Button("--"))
            txSize -= 1;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.5f);
        if (ImGui::DragInt("##Width", &txSize, 1.0f, 1, 320 * 8, "%d", ImGuiSliderFlags_None)) {
            mTextureSize.x = txSize;
        }
        ImGui::SameLine();
        if (ImGui::Button("++"))
            txSize += 1;
        ImGui::SameLine();
        if (ImGui::Button(">>"))
            txSize *= 2;

        txSize = qd::clamp(txSize, 1, 320 * 8);
    }
}

};  // namespace qd::window
