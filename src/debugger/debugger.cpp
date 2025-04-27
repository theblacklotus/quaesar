#include "debugger.h"
#include <EASTL/queue.h>
#include <EASTL/sort.h>
#include <SDL.h>
// clang-format off
#include <src/sysconfig.h>
#include <uae_src/include/sysdeps.h>
#include <uae_src/include/options.h>
#include <uae_src/include/memory.h>
#include <uae_src/include/newcpu.h>
#include <uae_src/include/debug.h>
// clang-format on
#include <capstone/capstone.h>
#include <dear_imgui/backends/imgui_impl_sdl2.h>
#include <dear_imgui/backends/imgui_impl_sdlrenderer2.h>
#include <dear_imgui/imgui.h>
#include <dear_imgui/imgui_internal.h>
#include <debugger/action_mgr.h>
#include <debugger/msg_list.h>
#include <debugger/vm/imp/vm_uae_imp.h>
#include <debugger/vm/vm.h>
#include <src/generic/thread.h>
#include <src/quaesar.h>
#include <src/shortcut/shortcut_mgr.h>
#include <src/ui/gui_manager.h>
#include <src/ui/ui_style.h>


namespace qd {

void Debugger::update() {
    qd::ShortcutsMgr::get()->update();

    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    gui->drawImGuiMainFrame();
}


//////////////////////////////////////////////////////////////////////////
namespace imp {
class ConsoleQueue {
public:
    eastl::queue<eastl::string> mConsoleCmdQueue;
    qd::thread::Event* mpEvent;
    qd::thread::Mutex* mpMutex;

public:
    ConsoleQueue() {
        mpEvent = new qd::thread::Event(true);
        mpMutex = new qd::thread::Mutex();
    }

    void addCmdToQueue(eastl::string cmd) {
        if (cmd.empty())
            return;
        mpMutex->lock();
        mConsoleCmdQueue.push(eastl::move(cmd));
        mpMutex->unlock();
        mpEvent->set();
    }

    bool waitConsoleCmd(eastl::string& out) {
        mpEvent->wait(100);
        qd::thread::MutexLock ml(*mpMutex);
        if (mConsoleCmdQueue.empty())
            return false;
        const eastl::string& cmd = mConsoleCmdQueue.front();
        out = eastl::move(cmd);
        mConsoleCmdQueue.pop();
        return true;
    }

    void destroy() {
        mConsoleCmdQueue = {};
        if (mpEvent) {
            mpEvent->set();
            SAFE_DELETE(mpEvent);
        }
        SAFE_DELETE(mpMutex);
    }

    ~ConsoleQueue() {
        destroy();
    }

};  // class ConsoleQueue
ConsoleQueue console_queue;


static bool uae_bp_reg_convert(int uae_reg, EReg& out) {
    if (uae_reg >= qd::breakpoint_reg_end)
        return false;
    out = (EReg::Type)uae_reg;
    return true;
}

};  // namespace imp
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

void Debugger::init() {
    mbInit = true;
    createRenderWindow();
    initImGui();
    vm = VM::setVmInst(new qd::vm::imp::UaeEmuVmImp());
    vm->init();
    gui = new GuiManager(this);
    mActions = action::ActionManager::get();
    mActions->create(gui, this);
    capstone = new csh();

    // TODO: Pick correct CPU depending on starting CPU
    cs_err err = cs_open(CS_ARCH_M68K, (cs_mode)(CS_MODE_BIG_ENDIAN | CS_MODE_M68K_000), capstone);
    if (err) {
        printf("Failed on cs_open() with error returned: %u\n", err);
        abort();
    }
    cs_option(*capstone, CS_OPT_DETAIL, CS_OPT_ON);
}


void Debugger::createRenderWindow() {
    uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN;
    SDL_Window* window =
        SDL_CreateWindow("Quaesar: Debugger", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    if (!window) {
        fprintf(stderr, "Error creating window.\n");
        return;
    }

    mRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!mRenderer) {
        SDL_DestroyWindow(window);
        SDL_Log("Error creating SDL_Renderer!");
        return;
    }
    mWindow = window;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}


void Debugger::initImGui() {
    // Setup Dear ImGui style
    UiStyle::get()->applyImGuiDarkStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(mWindow, mRenderer);
    ImGui_ImplSDLRenderer2_Init(mRenderer);
}


Debugger::~Debugger() {
    assert(!mbInit);
}


bool Debugger::isDebugActivated() {
    return ::debugging > 0 && (::debugger_active > 0);
}

bool Debugger::isDebugActivatedFull() {
    return ::debugging > 0 && (::debugger_active > 0 && ::regs.spcflags & SPCFLAG_BRK);
}


void Debugger::setDebugMode(DebuggerMode debug_mode) {
    if (debug_mode == DebuggerMode_Break) {
        while (!isDebugActivatedFull()) {
            ::debugger_active = 0;
            ::debugging = 0;
            activate_debugger_new();
        }
    } else if (debug_mode == DebuggerMode_Live) {
        action::msg::DoDebugTraceContinue m;
        mActions->applyActionMsg(&m);
        ::debugger_active = 0;
    }
}

qd::EFlow Debugger::applyActionMsg(qd::action::msg::Base* p_msg) const {
    return mActions->applyActionMsg(p_msg);
}


int Debugger::waitConsoleCmd(char* out, int maxlen) {
    eastl::string cmd;
    if (!qd::imp::console_queue.waitConsoleCmd(cmd))
        return -1;

    const int len = (int)cmd.size();
    if (len < maxlen)
        strcpy(out, cmd.data());
    else
        EASTL_ASSERT(0);
    return len;
}


void qd::Debugger::execConsoleCmd(eastl::string&& cmd) {
    imp::console_queue.addCmdToQueue(eastl::move(cmd));
}


void Debugger::applyImmediateConsoleCmd(eastl::string&& cmd) {
    qd::uae::do_console_cmd_immediate(cmd.c_str());
}

void Debugger::destroy() {
    if (gui)
        gui->destroy();
    imp::console_queue.destroy();
    if (mActions)
        mActions->destroy();
    mActions = nullptr;

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    delete gui;
    delete capstone;
    capstone = nullptr;
    gui = nullptr;
    vm = nullptr;
    VM::destrotVmInst();

    SDL_DestroyRenderer(mRenderer);
    mRenderer = nullptr;
    SDL_DestroyWindow(mWindow);
    mWindow = nullptr;

    mbInit = false;
}


void Debugger::render() {
    Debugger* debugger = this;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Render();
    SDL_RenderSetScale(debugger->mRenderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(debugger->mRenderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255),
                           (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
    SDL_RenderClear(debugger->mRenderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), debugger->mRenderer);
    SDL_RenderPresent(debugger->mRenderer);
}


bool Debugger::isVisible() const {
    uint32_t window_flags = SDL_GetWindowFlags(mWindow);
    if (window_flags & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED)) {
        return false;
    } else {
        return true;
    }
}


void Debugger::toggleWndVisible(DebuggerMode mode) {
    if (!isVisible()) {
        SDL_ShowWindow(mWindow);
    } else {
        SDL_HideWindow(mWindow);
    }
}


void Debugger::sdlEventProc(SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
}


void BreakpointsSortedList::create() {
    static_assert(qd::BREAKPOINTS_MAX == BREAKPOINT_TOTAL);

    breakpoints.clear();
    for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
        const ::breakpoint_node& bpNode = bpnodes[i];
        if (bpNode.value1 == 0 || bpNode.enabled <= 0)
            continue;
        qd::Breakpoint& curBp = breakpoints.emplace_back();
        curBp.addr1 = bpNode.value1;
        curBp.addr2 = bpNode.value2;
        curBp.enabled = bpNode.enabled;
        imp::uae_bp_reg_convert(bpNode.type, curBp.reg);

        if (bpNode.oper == BREAKPOINT_CMP_EQUAL && bpNode.enabled > 0) {
            OneAddrBp bp;
            bp.addr = curBp.addr1;
            bp.bpIdx = (int)breakpoints.size() - 1;
            oneAddrBps.insert(bp);
        }
    }
}


const qd::Breakpoint* BreakpointsSortedList::getBpByAddr(AddrRef addr, EReg reg) const {
    OneAddrBp lh;
    lh.addr = addr;
    auto it = oneAddrBps.find(lh);
    if (it == oneAddrBps.end())
        return nullptr;
    return &breakpoints[it->bpIdx];
}


};  // namespace qd
