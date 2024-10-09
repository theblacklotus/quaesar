#include "action_mgr.h"
#include <debugger/ui/gui_manager.h>

namespace qd {

extern uint32_t qdbActionAutoClassId = 0;

Debugger* Action::getDbg() const {
    return gui->getDbg();
}

};  // namespace qd
