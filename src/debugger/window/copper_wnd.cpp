#include <EASTL/fixed_string.h>
#include <EASTL/fixed_vector.h>
#include <debugger/debugger.h>
#include <debugger/msg_list.h>
#include <debugger/vm/memory.h>
#include <debugger/vm/vm.h>
#include <imgui_eastl.h>
#include <src/generic/color.h>
#include <src/shortcut/shortcut_mgr.h>
#include <src/ui/ui_style.h>
#include <src/ui/ui_view.h>


namespace qd {
namespace window {
class CopperDbgWnd : public UiWindow {
    QDB_CLASS_ID(WndId::CopperDbgWnd);

public:
    virtual void onCreate(UiViewCreate* cp) override {
        UiWindow::onCreate(cp);
        mTitle = "Copper debug";
    }

    virtual void drawContent() override;

} QDB_WINDOW_REGISTER(CopperDbgWnd);
//////////////////////////////////////////////////////////////////////////

};  // namespace window
//////////////////////////////////////////////////////////////////////////
//


struct DecodedCopperList {
    struct CopInst {
        AddrRef addr = 0;
        uint16_t w1 = -1;
        uint16_t w2 = -1;
    };

    struct Entry : public CopInst {
        int vpos = -1;
        int hpos = -1;
        eastl::fixed_string<char, 16, false> strInsn;
        eastl::fixed_string<char, 128, false> comment;
    };
    eastl::vector<DecodedCopperList::Entry> decoded;

public:
    void decodeInstr(Entry& ent) {
        uint32_t insn = ent.w1 << 16 | ent.w2;
        uint32_t insn_type;
        insn_type = insn & 0x00010001;

        ent.comment.clear();
        switch (insn_type) {
            case 0x00010000: /* WAIT insn */
                ent.strInsn = "WAIT";
                disassembleWait(ent, insn);
                if (insn == 0xfffffffe)
                    ent.comment = "End of Copperlist";
                break;

            case 0x00010001: /* SKIP insn */
                ent.strInsn = "SKIP";
                disassembleWait(ent, insn);
                break;

            case 0x00000000:
            case 0x00000001: /* MOVE insn */
            {
                ent.strInsn = "MOVE";
                AddrRef addr = ((insn >> 16) & 0x1fe) + 0xdff000;
                CustReg crg = CustReg::getRegByAddr(addr);
                if (crg.isValid())
                    ent.comment.sprintf("0x%04x -> %s", insn & 0xffff, crg.toStringC());
                else
                    ent.comment.sprintf("%04x := 0x%04x", addr, insn & 0xffff);
            } break;

            default:
                ent.comment = ("bad copper command");
                break;
        }
    }


    void disassembleWait(Entry& out, uint32_t insn) {
        int vp, hp, ve, he, bfd, v_mask, h_mask;
        int doout = 0;

        vp = (insn & 0xff000000) >> 24;
        hp = (insn & 0x00fe0000) >> 16;
        ve = (insn & 0x00007f00) >> 8;
        he = (insn & 0x000000fe);
        bfd = (insn & 0x00008000) >> 15;

        /* bit15 can never be masked out*/
        v_mask = vp & (ve | 0x80);
        h_mask = hp & he;
        if (v_mask > 0) {
            doout = 1;
            out.comment.append("vpos ");
            if (ve != 0x7f) {
                out.comment.append_sprintf("& 0x%02x ", ve);
            }
            out.comment.append_sprintf(">= 0x%02x", v_mask);
        }
        if (he > 0) {
            if (v_mask > 0) {
                out.comment.append(" and");
            }
            out.comment.append(" hpos ");
            if (he != 0xfe) {
                out.comment.append_sprintf("& 0x%02x ", he);
            }
            out.comment.append_sprintf(">= 0x%02x", h_mask);
        } else {
            if (doout)
                out.comment.append(", ");
            out.comment.append(", ignore horizontal");
        }

        out.comment.append_sprintf(", VP %02x, VE %02x; HP %02x, HE %02x; BFD %d", vp, ve, hp, he, bfd);
    }


    void decodeLines(VM* vm, AddrRef startAddr, int num_lines) {
        decoded.clear();
        AddrRef addr = startAddr;
        decoded.reserve(num_lines);
        for (int i = 0; i < num_lines; ++i) {
            Entry& curEnt = decoded.push_back();
            curEnt.addr = addr;
            if (vm->mem->getU16(addr, &curEnt.w1) && vm->mem->getU16(addr + 2, &curEnt.w2)) {
                decodeInstr(curEnt);
            } else {
                break;
            }
            addr += 4;
        }
    }
};  // struct DecodedCopperList


//
//////////////////////////////////////////////////////////////////////////
namespace window {


void CopperDbgWnd::drawContent() {
    Debugger* dbg = getDbg();
    VM* vm = dbg->vm;

    qd::VM::CustomRegs* custRegs = vm->custom;
    custRegs->fetch();

    QImPushFloatLock st;
    st.pushFloat(&ImGui::GetStyle().CellPadding.y, 2);
    static float row_min_height = 0.0f;  // for auto height

    eastl::fixed_string<char, 30, false> strAddr;
    eastl::fixed_string<char, 255, false> strTmp;

    uint16_t rDmaCon = custRegs->getRegVal(CustReg::DMACONR);

    bool bCopEn = rDmaCon & DMAC::COPEN;
    ImGui::Checkbox("COPEN", &bCopEn);
    ImGui::SameLine();

    if (ImGui::Button("Trace")) {
        ShortcutsMgr::get()->triggerShortcut(shortcut::EId::CopperToggleBreakpoint);
    }

    AddrRef pcAddr = vm->copper->getCopperAddr(qd::CopperAddr_ip);
    AddrRef lc1 = vm->copper->getCopperAddr(qd::CopperAddr_cop1lc);
    AddrRef lc2 = vm->copper->getCopperAddr(qd::CopperAddr_cop2lc);
    AddrRef startAddr = (pcAddr - lc1) < (pcAddr - lc2) ? lc1 : lc2;
    DecodedCopperList copDec;
    copDec.decodeLines(vm, startAddr, 1024);

    ImVec2 rgn = ImGui::GetContentRegionAvail();
    int flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##copperInst", 5, flags, ImVec2(rgn.x, rgn.y))) {
        // ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn(
            "##breakpoint",
            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder, 8.f);
        ImGui::TableSetupColumn("##address");
        ImGui::TableSetupColumn("##bytes");
        ImGui::TableSetupColumn("##copCmd");
        ImGui::TableSetupColumn("##data");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < copDec.decoded.size(); ++i) {
            const DecodedCopperList::Entry& curEntry = copDec.decoded[i];
            AddrRef curAddr = curEntry.addr;
            ImGui::TableNextRow(ImGuiTableRowFlags_None, row_min_height);
            ImGui::PushID(curAddr);
            ImGui::TableSetColumnIndex(0);

            if (curAddr == pcAddr)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, uiGetColorU(UiStyle::DisasmWnd_PcCursor));

            // col:breakpoint
            strTmp = " ";
            if (ImGui::Selectable(strTmp.c_str(), false, 0, ImVec2(0, row_min_height))) {
            }
            ImGui::TableNextColumn();

            // col:addr
            ImGui::TextColored(uiGetColorF(UiStyle::DisasmWnd_Addr),"%06X", curAddr);
            ImGui::TableNextColumn();

            // col:code bytes
            ImGui::TextColored(uiGetColorF(UiStyle::DisasmWnd_OpCodeBytes),"%04X %04X", curEntry.w1, curEntry.w2);
            ImGui::TableNextColumn();

            // col:instr
            ImGui::TextUnformatted(curEntry.strInsn.c_str());
            ImGui::TableNextColumn();

            // col: comment
            ImGui::TextUnformatted(curEntry.comment.c_str());
            // ImGui::TableNextColumn();

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

};  // namespace window
};  // namespace qd
