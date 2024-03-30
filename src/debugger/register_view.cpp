#include <stdint.h>
#include <dear_imgui/imgui.h>
#include "register_view.h"
#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"

struct RegisterView {
    uint32_t dummy;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

RegisterView* RegisterView_create() {
    return new RegisterView();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RegisterView_update(RegisterView* self) {
    static bool open = true;

    ImGuiTableFlags flags = ImGuiTableFlags_Resizable
        | ImGuiTableFlags_Reorderable
        | ImGuiTableFlags_Hideable
        | ImGuiTableFlags_BordersOuter
        | ImGuiTableFlags_BordersV;

    if (ImGui::Begin("Registers", &open, ImGuiWindowFlags_NoScrollbar)) {
        if (ImGui::BeginTable("disassembly", 2, flags)) {
            ImGui::TableSetupColumn("Register");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            for (int i = 0; i < 8; i++) {
                ImGui::TableNextColumn();
                ImGui::Text("a%d", i);
                ImGui::TableNextColumn();
                ImGui::Text("%08X", m68k_areg(regs, i)); 
            }

            for (int i = 0; i < 8; i++) {
                ImGui::TableNextColumn();
                ImGui::Text("d%d", i);
                ImGui::TableNextColumn();
                ImGui::Text("%08X", m68k_dreg(regs, i)); 
            }
        }
        
        ImGui::EndTable();
    }
    
    ImGui::End();
}

