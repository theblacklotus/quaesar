#include "uae_actions.h"
#include <debugger/action_mgr.h>
#include <debugger/debugger.h>
#include <imgui_eastl.h>
#include <uae_src/include/debug.h>

namespace qd::action {

class DebugDmaOption : public Action {
public:
    DebugDmaOption(QdbActionCreate* cp) : Action(cp) {
        supportMtd.set(UiDrawEvent::MainMenu_Debug);
    }

    virtual void onMainMenuItem(UiDrawEvent::Idx event, void*) {
        switch (event) {
            case UiDrawEvent::MainMenu_Debug: {
                int n = ::debug_dma;
                const char* options =
                    "off\0"
                    "mode 1\0"
                    "mode 2\0"
                    "mode 3\0"
                    "mode 4\0"
                    "\0";
                if (ImGui::Combo("Debug DMA", &n, options)) {
                    char buf[64];
                    ImSnprintf(buf, sizeof(buf), "v -%d", n);
                    getDbg()->applyConsoleCmd(buf);
                }
            } break;
            default:
                break;
        }
    }
};  // class DebugDmaOption
QDB_ACTION_REGISTER(DebugDmaOption);

};  // namespace qd::action
