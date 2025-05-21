#include "mem_graph_wnd.h"
#include <EASTL/span.h>
#include <SDL.h>
#include <debugger/debugger.h>
#include <debugger/vm/vm.h>
#include <imgui_eastl.h>
#include <imgui_internal.h>
#include <quaesar.h>


namespace qd {
namespace window {
QDB_WINDOW_REGISTER(MemoryGraphWnd);

void MemoryGraphWnd::drawContent() {
    VM* vm = getDbg()->getVm();

    mNewTextureSize.y = (int)ImGui::GetWindowHeight() - 150;

    float curTime = (float)ImGui::GetTime();

    if (!mTextureId || mTextureSize != mNewTextureSize) {
        if (mTextureId) {
            SDL_DestroyTexture((SDL_Texture*)mTextureId);
            mTextureId = 0;
        }
        if (mNewTextureSize.y > 1 && mNewTextureSize.y > 0) {
            mTextureSize = mNewTextureSize;
            SDL_Texture* scrTexture = nullptr;
            scrTexture = SDL_CreateTexture(getDbg()->getRenderer(), SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING, mTextureSize.x, mTextureSize.y);
            if (scrTexture) {
                mTextureId = (ImTextureID)(scrTexture);
                SDL_SetTextureBlendMode(scrTexture, SDL_BLENDMODE_BLEND);
                SDL_SetTextureScaleMode(scrTexture, SDL_ScaleModeLinear);
                mLastTextureCreateTime = curTime;
            } else {
                SDL_Log("Can't not create texture: %s", SDL_GetError());
            }
        }
    }


    const MemBank* pCurBank = vm->mem->getBankByInd(mCurBank);
    if (!pCurBank) {
        mCurBank = MemBank::CHIP;
        return;
    }

    uint32_t addrPtr = mBankOffset + pCurBank->startAddr;
    uint32_t step = 1;
    if (ImGui::InputScalar("Address", ImGuiDataType_U32, &addrPtr, &step, nullptr, "%08Xh",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        mBankOffset = addrPtr;
    }

    eastl::inline_string<255, false> selBankName = "null";
    if (pCurBank) {
        selBankName.assign(pCurBank->name.begin(), pCurBank->name.end());
        selBankName.append_sprintf(" (%06Xh - %06Xh)", (uint32_t)pCurBank->startAddr,
                                   (uint32_t)pCurBank->startAddr + pCurBank->size);
    }
    if (ImGui::BeginCombo("Memory bank", selBankName.c_str(), ImGuiComboFlags_None)) {
        eastl::span<const qd::MemBank> banks = vm->mem->banks;
        for (int nBank = 0; nBank < banks.size(); ++nBank) {
            const qd::MemBank& curBank = banks[nBank];
            selBankName.assign(curBank.name.begin(), curBank.name.end());
            selBankName.append_sprintf(" (%06Xh-%06Xh)", (uint32_t)curBank.startAddr,
                                       (uint32_t)curBank.startAddr + curBank.size);
            if (ImGui::Selectable(selBankName.c_str(), nBank == mCurBank)) {
                mCurBank = nBank;
                mTextureMod = 0;
                pCurBank = vm->mem->getBankByInd(mCurBank);
            }
        }
        vm->custom->fetch();
        uint16_t ddfstrt = vm->custom->getRegVal(CustReg::DDFSTRT);
        uint16_t ddfstop = vm->custom->getRegVal(CustReg::DDFSTOP);

        for (int nb = 0; nb < 5; ++nb) {
            AddrRef bplPtr = vm->custom->getRegVal(CustReg::BPL1PTH + nb * 2) << 16 |
                             vm->custom->getRegVal(CustReg::BPL1PTH + nb * 2 + 1);
            selBankName.sprintf("BPL %i (%06Xh)###BPL%i", nb + 1, bplPtr, nb);
            if (ImGui::Selectable(selBankName.c_str())) {
                mCurBank = MemBank::CHIP;
                pCurBank = vm->mem->getBankByInd(mCurBank);
                mBankOffset = bplPtr - pCurBank->startAddr;
                CustReg modReg = (nb & 1) == 0 ? CustReg::BPL1MOD : CustReg::BPL2MOD;
                mTextureMod = (short)vm->custom->getRegVal(modReg);
                // FIXME: How to calculate real bitplane width from ddfstart/ddfstop
                // int res = 161 / 2 - ddfstrt;
                int bpWidth = ((ddfstop - ddfstrt) + 8) * 2;
                if (bpWidth > 320)
                    bpWidth -= ddfstrt;
                mNewTextureSize.x = bpWidth;
            }
        }
        ImGui::EndCombo();
    }

    if (mTextureId && (curTime - mLastTextureCreateTime) > 0.1f) {
        SDL_Texture* scrTexture = (SDL_Texture*)(mTextureId);
        void* pixels = nullptr;
        int pitch;
        if (SDL_LockTexture(scrTexture, nullptr, &pixels, &pitch) == 0) {
            uint8_t* memPtr = (uint8_t*)vm->mem->getRealAddr(mBankOffset + pCurBank->startAddr);

            // Change pixels
            uint32_t* dest = ((uint32_t*)pixels);
            const int rowBytes = mTextureSize.x / 8;
            for (int y = 0; y < mTextureSize.y; y++) {
                if (memPtr + rowBytes < pCurBank->realAddr + pCurBank->size) {
                    for (int x = 0; x < rowBytes; x++) {
                        uint8_t sb = *memPtr;
                        uint8_t m = 0x80;
                        for (int b = 0; b < 8; b++) {
                            Color c = (sb & m) != 0 ? Color::WHITE : Color::BLACK;
                            *dest = c;
                            dest++;
                            m >>= 1;
                        }
                        ++memPtr;
                    }
                    memPtr += mTextureMod;
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
        if (ImGui::VSliderInt("##AddrSlider", ImVec2(32.0f, (float)mNewTextureSize.y), &addrPtr, 0, pCurBank->size,
                              "")) {
            mBankOffset = pCurBank->size - addrPtr;
        }
    }
    ImGui::SameLine();

    ImVec2 scrollingChildSize = ImVec2(ImGui::GetContentRegionAvail().x - 00.f, mTextureSize.y + 32.f);
    ImGui::BeginChild("##scrolling", scrollingChildSize, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::Image(mTextureId, ImVec2((float)mTextureSize.x, (float)mTextureSize.y), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f),
                 ImVec4(1.f, 1.f, 1.f, 1.f), ImGui::GetStyleColorVec4(ImGuiCol_Border));
    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseClicked(0)) {
            mStartDragBankOffset = mBankOffset;
        }
        if (ImGui::IsMouseDragging(0)) {
            ImVec2 dragDelta = ImGui::GetMouseDragDelta();
            mBankOffset =
                mStartDragBankOffset - int(dragDelta.y / 8.f) * (mTextureSize.x + mTextureMod) - int(dragDelta.x / 8.f);
        }
    }

    ImGui::EndChild();


    // texture width
    {
        int& txSize = mNewTextureSize.x;
        if (ImGui::Button("<<"))
            txSize /= 2;
        ImGui::SameLine();
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        if (ImGui::ArrowButton("##minus", ImGuiDir_Left)) {
            txSize--;
        }
        // ImGui::PopItemFlag();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.5f);
        if (ImGui::DragInt("##Width", &txSize, 1.0f, 1, 320 * 8, "%d", ImGuiSliderFlags_None)) {
            mNewTextureSize.x = txSize;
        }
        ImGui::SameLine();
        // ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        if (ImGui::ArrowButton("##plus", ImGuiDir_Right)) {
            txSize++;
        }
        ImGui::SameLine();
        if (ImGui::Button(">>"))
            txSize *= 2;
        ImGui::PopItemFlag();  // button repeat

        ImGui::SameLine();
        ImGui::TextUnformatted("Width");
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputInt("mod", &mTextureMod, 2);

        txSize = qd::clamp(txSize, 1, 320 * 8);
    }
}

};  // namespace window
};  // namespace qd
