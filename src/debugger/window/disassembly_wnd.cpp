#include "disassembly_wnd.h"
#include <debugger/debugger.h>
#include <debugger/vm.h>
#include <imgui_eastl.h>
// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
// clang-format on
#include <EASTL/fixed_string.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <capstone/capstone.h>

namespace qd::window {

QDB_WINDOW_REGISTER(DisassemblyView);

DisassemblyView::DisassemblyView(UiViewCreate* cp) : UiWindow(cp) {
    mTitle = "Disassembly";
}

void DisassemblyView::drawContent() {
    VM* vm = getDbg()->getVm();

    bool isDbgMode = getDbg()->isDebugActivated();
    if (ImGui::Checkbox("Debug", &isDbgMode)) {
        getDbg()->setDebugMode(isDbgMode ? DebuggerMode_Break : DebuggerMode_Live);
    }

    if (ImGui::InputText("##addr", &addrInputStr,
                         ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue |
                             ImGuiInputTextFlags_AutoSelectAll)) {
        size_t goto_addr;
        if (sscanf(addrInputStr.c_str(), "%" _PRISizeT "X", &goto_addr) == 1) {
            disasmAddr = goto_addr;
        } else
            disasmAddr.reset();
    }

    uae_u32 pc = vm->cpu->getPC();

    if (disasmAddr) {
        pc = *disasmAddr;
    }
    uae_u8* pc_addr = memory_get_real_address(pc);

    uae_u32 offset = 0;
    uae_u32 start_disasm = pc - offset;

    cs_insn* instructions = nullptr;
    uae_u32 count_bytes = 80;
    int instructionCount = cs_disasm(getDbg()->capstone, pc_addr - offset, count_bytes, start_disasm, 0, &instructions);

    int flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##disassembly", 3, flags, ImVec2(0, 300))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("address");
        ImGui::TableSetupColumn("bytes");
        ImGui::TableSetupColumn("instruction");
        ImGui::TableHeadersRow();

        eastl::string op_str;

        for (int i = 0; i < instructionCount; ++i) {
            const cs_insn& entry = instructions[i];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("0x%08X", entry.address);
            ImGui::TableNextColumn();
            op_str.clear();
            for (int b = 0; b < entry.size; ++b) {
                eastl::fixed_string<char, 4, false> bytes_str;
                bytes_str.sprintf("%02X", entry.bytes[b]);
                op_str.append(bytes_str.begin(), bytes_str.end());
            }
            ImGui::TextUnformatted(op_str.c_str());
            ImGui::TableNextColumn();

            op_str = entry.mnemonic;
            op_str += ' ';
            op_str += entry.op_str;
            ImGui::TextUnformatted(op_str.c_str());
        }

        ImGui::EndTable();
    }

    cs_free(instructions, instructionCount);
}

};  // namespace qd::window
