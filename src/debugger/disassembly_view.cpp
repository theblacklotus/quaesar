#include "disassembly_view.h"
#include <dear_imgui/imgui.h>

struct DisassemblyView {
    int foo;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DisassemblyView* DisassemblyView_create() {
    DisassemblyView* view = new DisassemblyView();
    return view;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DisassemblyView_destroy(DisassemblyView* self) {
    delete self;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DisassemblyView_update(DisassemblyView* self, const char* name) {
    //static bool show_demo_window = true;
    //ImGui::ShowDemoWindow(&show_demo_window);

    float text_height = ImGui::GetTextLineHeight();

    if (ImGui::Begin(title, &open, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::BeginChild("##scrolling", ImVec2(00.0f, 0.0f), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    
        ImGui::PopStyleVar(2);
    }
}

