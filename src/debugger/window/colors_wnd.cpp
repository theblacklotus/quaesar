#include "colors_wnd.h"
#include <EASTL/fixed_vector.h>
#include <debugger/debugger.h>
#include <debugger/generic/color.h>
#include <debugger/vm.h>
#include <imgui_eastl.h>

namespace qd::window {

QDB_WINDOW_REGISTER(ColorsWnd);

ColorsWnd::ColorsWnd(UiViewCreate* cp) : UiWindow(cp) {
    mTitle = "Palette";
}

void ColorsWnd::drawContent() {
    VM* vm = getDbg()->getVm();

    eastl::fixed_vector<Color, 256> palette;

    for (int i = 0; i < 256; i++) {
        Color curCol;
        if (!vm->blitter->getColor(i, curCol))
            break;
        palette.push_back(curCol);
    }

    for (int n = 0; n < palette.size(); n++) {
        ImGui::PushID(n);
        if ((n % 8) != 0)
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);

        ImGuiColorEditFlags palette_button_flags =
            ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;

        Color& palCol = palette[n];
        ImVec4 imcolor;
        palCol.toColorF(imcolor.x, imcolor.y, imcolor.z, imcolor.w);
        if (ImGui::ColorButton("##palette", imcolor, palette_button_flags, ImVec2(20, 20))) {
            palCol = Color::makeFromF(imcolor.x, imcolor.y, imcolor.z, imcolor.w);
        }

        ImGui::PopID();
    }
}

};  // namespace qd::window
