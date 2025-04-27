#include "disassembly_wnd.h"
#include <EASTL/fixed_string.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <capstone/capstone.h>
#include <debugger/debugger.h>
#include <debugger/msg_list.h>
#include <debugger/vm/vm.h>
#include <imgui_eastl.h>
#include <src/ui/ui_style.h>

namespace qd {
namespace window {

QDB_WINDOW_REGISTER(DisassemblyView);


void DisassemblyView::drawContent() {
    Debugger* dbg = getDbg();
    VM* vm = dbg->getVm();

    if (ImGui::InputText("##addr", &addrInputStr,
                         ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue |
                             ImGuiInputTextFlags_AutoSelectAll)) {
        size_t goto_addr;
        if (sscanf(addrInputStr.c_str(), "%" _PRISizeT "X", &goto_addr) == 1) {
            mDisasmAddr = static_cast<uint32_t>(goto_addr);
        } else
            mDisasmAddr.reset();
    }

    AddrRef pcAddr = vm->cpu->getPC();
    uint8_t* pcAddrDat = vm->mem->getRealAddr(pcAddr);

    const BreakpointsSortedList& bpList = dbg->getBreakpointsSorted();

    uint32_t offset = 0;
    uint32_t start_disasm = pcAddr - offset;

    cs_insn* instructions = nullptr;
    uint32_t count_bytes = 80;
    cs_option(*dbg->capstone, CS_OPT_SKIPDATA, CS_OPT_ON);
    size_t instructionCount =
        cs_disasm(*dbg->capstone, pcAddrDat - offset, count_bytes, start_disasm, 100, &instructions);
    static float row_min_height = 0.0f;  // for auto height

    int flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##disassembly", 4, flags, ImVec2(0, ImGui::GetWindowHeight() - 32))) {
        // ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn(
            "##breakpoint",
            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder, 8.f);
        ImGui::TableSetupColumn("##address");
        ImGui::TableSetupColumn("##bytes");
        ImGui::TableSetupColumn("##OpCodes");
        ImGui::TableHeadersRow();

        eastl::fixed_string<char, 255, false> strAddr, strTmp;

        for (size_t i = 0; i < instructionCount; ++i) {
            const cs_insn& entry = instructions[i];
            ImGui::TableNextRow(ImGuiTableRowFlags_None, row_min_height);
            AddrRef curAddr = (uint32_t)entry.address;
            ImGui::PushID(curAddr);
            ImGui::TableSetColumnIndex(0);

            if (curAddr == pcAddr)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, uiGetColorU(UiStyle::DisasmWnd_PcCursor));

            // col:breakpoint
            strTmp = " ";
            const qd::Breakpoint* curBp;
            curBp = bpList.getBpByAddr(curAddr, EReg::PC);
            if (curBp) {
                strTmp = curBp->enabled ? "0" : "O";
            }
            if (ImGui::Selectable(strTmp.c_str(), false, 0, ImVec2(0, row_min_height))) {
                action::msg::DisasmToggleBreakpoint p;
                p.address = curAddr;
                p.reg = EReg::PC;
                dbg->applyActionMsg(&p);
            }
            ImGui::TableNextColumn();

            // col:addr
            bool isRowSelected = false;
            strAddr.sprintf("%08X", (uint32_t)curAddr);
            ImGuiSelectableFlags selectableFlags =
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;
            ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::DisasmWnd_Addr));
            if (ImGui::Selectable(strAddr.c_str(), isRowSelected, selectableFlags, ImVec2(0, row_min_height))) {
            }
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();

            // col:code bytes
            strTmp.clear();
            for (int b = 0; b < entry.size; ++b) {
                strTmp.append_sprintf("%02X", entry.bytes[b]);
            }
            ImGui::TextColored(uiGetColorF(UiStyle::DisasmWnd_OpCodeBytes), "%s", strTmp.c_str());
            ImGui::TableNextColumn();
            // col:instr
            strTmp = entry.mnemonic;
            do {
                strTmp += ' ';
            } while (strTmp.size() < 8);
            strTmp += entry.op_str;
            ImGui::TextUnformatted(strTmp.c_str());
            // ImGui::TableNextColumn();

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    cs_free(instructions, instructionCount);
}

};  // namespace window
};  // namespace qd
