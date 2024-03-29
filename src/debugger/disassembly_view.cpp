#include "disassembly_view.h"
#include <dear_imgui/imgui.h>
#include <capstone/capstone.h>
#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"

struct DisassemblyView {
    csh capstone;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DisassemblyView* DisassemblyView_create(csh capstone) {
    DisassemblyView* view = new DisassemblyView();
    view->capstone = capstone;
    return view;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DisassemblyView_destroy(DisassemblyView* self) {
    delete self;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void draw_disassembly(DisassemblyView* self) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    uae_u32 pc = M68K_GETPC;
    uae_u32 offset = 40;
    uae_u32 count_bytes = 80;

    uae_u8* pc_addr = memory_get_real_address(pc);
    uae_u32 start_disasm = pc - offset;

    cs_insn* insn = nullptr;
    size_t count = cs_disasm(self->capstone, pc_addr - offset, count_bytes, start_disasm, 0, &insn);

    int max_instruction_width = 20;
        
    ImGuiTableFlags flags = ImGuiTableFlags_Resizable
        | ImGuiTableFlags_Reorderable
        | ImGuiTableFlags_Hideable
        | ImGuiTableFlags_BordersOuter
        | ImGuiTableFlags_BordersV;
            
    char buffer[512];

    if (ImGui::BeginTable("disassembly", 3, flags)) {
        ImGui::TableSetupColumn("Adress");
        ImGui::TableSetupColumn("Instruction");
        ImGui::TableSetupColumn("Cycles");
        ImGui::TableHeadersRow();
        
        for (size_t j = 0; j < count; j++) {
            ImGui::TableNextColumn();
            ImGui::Text("0x%" PRIx64, insn[j].address);
            ImGui::TableNextColumn();

            int width = strlen(insn[j].mnemonic);
            memcpy(buffer, insn[j].mnemonic, width);

            for (int i = 0; i < max_instruction_width - width; i++) {
                buffer[width + i] = ' ';
            }

            strcpy(buffer + max_instruction_width, insn[j].op_str);

            ImGui::Text("%s", buffer);
            ImGui::TableNextColumn();
        }

        ImGui::EndTable();
    }


    //printf("%08x\n", M68K_GETPC);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DisassemblyView_update(DisassemblyView* self, const char* name) {
    //static bool show_demo_window = true;
    //ImGui::ShowDemoWindow(&show_demo_window);

    static bool open = true;

    float text_height = ImGui::GetTextLineHeight();

    if (ImGui::Begin("Disassembly", &open, ImGuiWindowFlags_NoScrollbar)) {
        //ImGui::BeginChild("##scrolling", ImVec2(00.0f, 0.0f), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
        //ImDrawList* draw_list = ImGui::GetWindowDrawList();
        //ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));


        draw_disassembly(self);
        
        //ImGui::PopStyleVar(2);

        //ImGui::EndChild();
    }

    ImGui::End();
}

