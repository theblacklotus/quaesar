// clang-format off
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// Order of these includes is important :(
#include <sys/timeb.h>
#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "inputdevice.h"
#include "autoconf.h"
#include "fsdb.h"
#include "uae/slirp.h"
#include "driveclick.h"
#include "pci_hw.h"
#include "blkdev.h"
#include "uae/time.h"
#include "fsusage.h"
#include "uae.h"
#include "ncr_scsi.h"
#include "ncr9x_scsi.h"
#include "gfxboard.h"
#include "avioutput.h"
#include "xwin.h"
#include "clipboard.h"
#include "keybuf.h"
#include "savestate.h"
#include "cdtv.h"
#include "gui.h"
#include "parallel.h"
#include "sampler.h"
#include "specialmonitors.h"
#include "bsdsocket.h"
#include "debug.h"
#include "ppc/ppcd.h"
#include "serial.h"
#include "x86.h"
#include "ethernet.h"
#include "dsp3210/dsp_glue.h"
#include "disk.h"
#include "cpuboard.h"
#include "draco.h"
#include "cd32_fmv.h"
#include "rommgr.h"
#include "newcpu.h"
#include "fpp.h"
#include "custom.h"
#include "drawing.h"
// clang-format on

#include <EASTL/fixed_string.h>
#include <SDL.h>
#include <debugger/debugger.h>
#include <log.h>
#include <src/generic/thread.h>
#include <src/quaesar.h>
#include <src/sounddep/sound.h>
#include <cstdarg>
#include <filesystem>

#ifdef TRACE
#undef TRACE
#endif
#define TRACE() SDL_Log("WARN: Using of unimplemented function: '%s()'", __func__)

void debug(const char* format, ...) {
    eastl::fixed_string<char, 1024, false> formatBuffer;
    formatBuffer.sprintf("[%s] %s", __func__, format);
    va_list args;
    va_start(args, format);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG, formatBuffer.c_str(), args);
    va_end(args);
}

int avioutput_enabled = 0;
bool beamracer_debug = false;
int volatile bsd_int_requested = 0;
int busywait = 0;
int key_swap_hack = 0;
int seriallog = 0;
int log_vsync, debug_vsync_min_delay, debug_vsync_forced_delay;
bool is_dsp_installed = false;
int tablet_log = 0;
int log_scsi = 0;
int uaelib_debug;
int flashscreen;
bool gfx_hdr = false;
uae_u32 redc[3 * 256], grec[3 * 256], bluc[3 * 256];

uae_u8* start_pc_p = nullptr;
uae_u32 start_pc = 0;
uae_u8* cubo_nvram = nullptr;


int dos_errno(void) {
    return errno;
}


void pausevideograb(int) {
    UNIMPLEMENTED();
}

void show_screen(int /*monid*/, int /*mode*/) {
    TRACE();
}

// from fs-uae
void vsync_clear() {
    TRACE();
}

int vsync_isdone(frame_time_t* /*dt*/) {
    TRACE();
    return 1;
}

bool target_osd_keyboard(int /*show*/) {
    UNIMPLEMENTED();
    return false;
}

bool specialmonitor_need_genlock() {
    return false;
}

void setmouseactive(int, int) {
    TRACE();
}

void screenshot(int /*monid*/, int, int) {
    UNIMPLEMENTED();
}


int same_aname(const TCHAR* an1, const TCHAR* an2) {
    //return CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, an1, -1, an2, -1) == CSTR_EQUAL;
    return SDL_strcasecmp(an1, an2) == 0;
}

int input_get_default_keyboard(int /*i*/) {
    TRACE();
    return 0;
}

uae_s64 getsetpositionvideograb(uae_s64 /*framepos*/) {
    UNIMPLEMENTED();
    return 0;
}

// Dummy initialization function
static int dummy_init(void) {
    TRACE();
    return 1;
}

// Dummy closing function
static void dummy_close(void) {
    TRACE();
}

// Dummy function to acquire an input device
static int dummy_acquire(int /*device_id*/, int /*exclusive*/) {
    TRACE();
    return 0;  // Return 0 for success, -1 for failure
}

// Dummy function to release/unacquire an input device
static void dummy_unacquire(int device_id) {
    debug("Unacquiring input device %d\n", device_id);
}

// Dummy function to read input from the device
static void dummy_read(void) {
    debug("Reading input from device\n");
}

// Dummy function to get the number of input devices
static int dummy_get_num(void) {
    TRACE();
    return 0;
}

// Dummy function to get the friendly name of an input device
static TCHAR* dummy_get_friendlyname(int /*device_id*/) {
    UNIMPLEMENTED();
    return nullptr;
}

// Dummy function to get the unique name of an input device
static TCHAR* dummy_get_uniquename(int /*device_id*/) {
    UNIMPLEMENTED();
    return nullptr;
}

// Dummy function to get the number of widgets (input elements) in an input device
static int dummy_get_widget_num(int /*device_id*/) {
    UNIMPLEMENTED();
    return 4;  // Return the number of widgets
}

// Dummy function to get the type and name of a widget
static int dummy_get_widget_type(int /*device_id*/, int /*widget_id*/, TCHAR* /*widget_name*/,
                                 uae_u32* /*widget_type*/) {
    UNIMPLEMENTED();
    return 0;  // Return 0 for success, -1 for failure
}

// Dummy function to get the first widget (input element) in an input device
static int dummy_get_widget_first(int /*device_id*/, int /*widget_type*/) {
    UNIMPLEMENTED();
    return 0;
}

// Dummy function to get the flags of an input device
int dummy_get_flags(int /*device_id*/) {
    return 0;  // Return flags (if any) for the input device
}

struct inputdevice_functions inputdevicefunc_mouse = {
    dummy_init,           dummy_close,           dummy_acquire,          dummy_unacquire,
    dummy_read,           dummy_get_num,         dummy_get_friendlyname, dummy_get_uniquename,
    dummy_get_widget_num, dummy_get_widget_type, dummy_get_widget_first, dummy_get_flags};

struct inputdevice_functions inputdevicefunc_keyboard = {
    dummy_init,           dummy_close,           dummy_acquire,          dummy_unacquire,
    dummy_read,           dummy_get_num,         dummy_get_friendlyname, dummy_get_uniquename,
    dummy_get_widget_num, dummy_get_widget_type, dummy_get_widget_first, dummy_get_flags};

struct inputdevice_functions inputdevicefunc_joystick = {
    dummy_init,           dummy_close,           dummy_acquire,          dummy_unacquire,
    dummy_read,           dummy_get_num,         dummy_get_friendlyname, dummy_get_uniquename,
    dummy_get_widget_num, dummy_get_widget_type, dummy_get_widget_first, dummy_get_flags};

const TCHAR* my_getfilepart(const TCHAR* filename) {
    debug("filename: '%s'", filename);

    const TCHAR* p;
    p = strrchr(filename, '\\');
    if (p)
        return p + 1;
    p = strrchr(filename, '/');
    if (p)
        return p + 1;
    return filename;
}

void fetch_statefilepath(TCHAR* /*out*/, int /*size*/) {
    UNIMPLEMENTED();
}

uae_u32 cpuboard_ncr9x_scsi_get(uaecptr /*addr*/) {
    UNIMPLEMENTED();
    return 0;
}

void cpuboard_ncr9x_scsi_put(uaecptr /*addr*/, uae_u32 /*v*/) {
    UNIMPLEMENTED();
}

void getfilepart(TCHAR* /*out*/, int /*size*/, const TCHAR* /*path*/) {
    UNIMPLEMENTED();
}

void toggle_fullscreen(int /*monid*/, int) {
    UNIMPLEMENTED();
}

const TCHAR* target_get_display_name(int, bool) {
    TRACE();
    return "Amiga";
    // UNIMPLEMENTED();
    // return nullptr;
}

extern int target_get_display(const TCHAR*) {
    UNIMPLEMENTED();
    return 0;
}

int target_cfgfile_load(struct uae_prefs* p, const TCHAR* /*filename*/, int type, int /*isdefault*/) {
    if (type == 0 || type == 1) {
        discard_prefs(p, 0);
    }
    if (type == 0 || type == 3) {
        default_prefs(p, true, type);
    }
    write_log(_T("config reset\n"));
    return 1;
}

void target_addtorecent(const TCHAR* /*name*/, int /*t*/) {
    UNIMPLEMENTED();
}

int my_truncate(const TCHAR* /*name*/, uae_u64 /*len*/) {
    UNIMPLEMENTED();
    return 0;
}

bool my_issamepath(const TCHAR* /*path1*/, const TCHAR* /*path2*/) {
    UNIMPLEMENTED();
    return false;
}

int input_get_default_joystick(struct uae_input_device* /*uid*/, int /*i*/, int /*port*/, int /*af*/, int /*mode*/,
                               bool /*gp*/, bool /*joymouseswap*/) {
    UNIMPLEMENTED();
    return 0;
}

bool get_plugin_path(TCHAR* /*out*/, int /*len*/, const TCHAR* /*path*/) {
    TRACE();
    return false;
}

void getgfxoffset(int /*monid*/, float* /*dxp*/, float* /*dyp*/, float* /*mxp*/, float* /*myp*/) {
    UNIMPLEMENTED();
}

void fixtrailing(TCHAR* /*p*/) {
    UNIMPLEMENTED();
}

int uae_slirp_redir(int /*is_udp*/, int /*host_port*/, struct in_addr /*guest_addr*/, int /*guest_port*/) {
    UNIMPLEMENTED();
    return 0;
}

int translate_message(int /*msg*/, TCHAR* /*out*/) {
    UNIMPLEMENTED();
    return 0;
}

bool toggle_rtg(int /*monid*/, int /*mode*/) {
    UNIMPLEMENTED();
    return false;
}

struct netdriverdata** target_ethernet_enumerate() {
    UNIMPLEMENTED();
    return nullptr;
}

void refreshtitle() {
    UNIMPLEMENTED();
}

bool my_utime(const TCHAR* /*name*/, struct mytimeval* /*tv*/) {
    UNIMPLEMENTED();
    return false;
}

bool my_resolvesoftlink(char*, int, bool) {
    TRACE();
    return true;
}

int my_rename(char const*, char const*) {
    UNIMPLEMENTED();
    return 0;
}

void masoboshi_ncr9x_scsi_put(unsigned int, unsigned int, int) {
    UNIMPLEMENTED();
}

struct autoconfig_info;

bool isa_expansion_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

uae_u32 gfxboard_get_romtype(rtgboardconfig*) {
    TRACE();
    // UNIMPLEMENTED();
    return 0;
}

uae_u8* save_log(int, size_t*) {
    UNIMPLEMENTED();
    return nullptr;
}

uae_u32 cpuboard_ncr710_io_bget(unsigned int) {
    UNIMPLEMENTED();
    return 0;
}

void cpuboard_ncr710_io_bput(unsigned int, unsigned int) {
    UNIMPLEMENTED();
}

uae_u32 cpuboard_ncr720_io_bget(uaecptr /*addr*/) {
    UNIMPLEMENTED();
    return 0;
}

void cpuboard_ncr720_io_bput(unsigned int, unsigned int) {
    UNIMPLEMENTED();
}

void cpuboard_setboard(struct uae_prefs* /*p*/, int /*type*/, int /*subtype*/) {
    UNIMPLEMENTED();
}

int cpuboard_memorytype(struct uae_prefs* /*p*/) {
    TRACE();
    return 0;
}

bool cpuboard_fc_check(uaecptr /*addr*/, uae_u32* /*v*/, int /*size*/, bool /*write*/) {
    UNIMPLEMENTED();
    return false;
}

/*
int a1060_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return 0;
}
int a2088t_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return 0;
}
int a2088xt_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return 0;
}
int a2286_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return 0;
}
int a2386_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return 0;
}
*/

void a4000t_add_scsi_unit(int /*ch*/, struct uaedev_config_info* /*ci*/, struct romconfig* /*rc*/) {
    UNIMPLEMENTED();
}

bool a4000t_scsi_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}
void a4091_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}
void activate_console() {
    TRACE();
}

void alf3_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void amiga_clipboard_die(TrapContext*) {
    UNIMPLEMENTED();
}

void amiga_clipboard_got_data(TrapContext*, unsigned int, unsigned int, unsigned int) {
    UNIMPLEMENTED();
}

void amiga_clipboard_init(TrapContext*) {
    UNIMPLEMENTED();
}

uaecptr amiga_clipboard_proc_start(TrapContext*) {
    UNIMPLEMENTED();
    return 0;
}

void amiga_clipboard_task_start(TrapContext*, unsigned int) {
    UNIMPLEMENTED();
}

int amiga_clipboard_want_data(TrapContext*) {
    UNIMPLEMENTED();
    return 0;
}

bool ariadne2_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

void AVIOutput_Restart(bool) {
    TRACE();
    // UNIMPLEMENTED();
}

void AVIOutput_Toggle(int, bool) {
    UNIMPLEMENTED();
}

void blizzardppc_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void bsdlib_install() {
    UNIMPLEMENTED();
}

uaecptr bsdlib_startup(TrapContext*, unsigned int) {
    UNIMPLEMENTED();
    return 0;
}

void bsdsock_fake_int_handler() {
    UNIMPLEMENTED();
}

void casablanca_map_overlay() {
    UNIMPLEMENTED();
}

addrbank* cd32_fmv_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return nullptr;
}

void cd32_fmv_set_sync(float, float) {
    TRACE();
    // return 0;
}

void cdtv_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

uae_u8 cdtv_battram_read(int) {
    UNIMPLEMENTED();
    return 0;
}

void cdtv_battram_write(int, int) {
    UNIMPLEMENTED();
}

bool cdtv_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool cdtvscsi_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool cdtvsram_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

int check_for_cache_miss() {
    UNIMPLEMENTED();
    return 0;
}

static int s_has_changed_setting = 1;

int check_prefs_changed_gfx() {
    // TODO: Fix
    int has_changed = s_has_changed_setting;
    s_has_changed_setting = 0;
    TRACE();
    return has_changed;
}

void clipboard_unsafeperiod() {
    UNIMPLEMENTED();
}

void clipboard_vsync() {
    TRACE();
}

void close_console() {
    SDL_Log("uae: close_console()");
}

int compemu_reset() {
    UNIMPLEMENTED();
    return 0;
}

struct cpu_history;

int compile_block(cpu_history*, int, int) {
    UNIMPLEMENTED();
    return 0;
}

int compiler_init() {
    UNIMPLEMENTED();
    return 0;
}

void console_flush() {
    fflush(stdout);
}

int console_get(char* out, int maxlen) {
    qd::Debugger* dbg = app->getDbg();
    if (!dbg)
        return -1;
    for (;;) {
        int len = dbg->waitConsoleCmd(out, maxlen);
        if (len > 0)
            return len;
    }
    return -1;
}

bool console_isch() {
    UNIMPLEMENTED();
    return false;
}

bool cpuboard_32bit(uae_prefs*) {
    UNIMPLEMENTED();
    return false;
}

bool cpuboard_autoconfig_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

void cpuboard_cleanup() {
    TRACE();
}

void cpuboard_clear() {
    TRACE();
}

void cpuboard_dkb_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

bool cpuboard_forced_hardreset() {
    UNIMPLEMENTED();
    return false;
}

uaecptr cpuboard_get_reset_pc(unsigned int*) {
    UNIMPLEMENTED();
    return 0;
}

void cpuboard_init() {
    TRACE();
}

bool cpuboard_maprom() {
    UNIMPLEMENTED();
    return false;
}

void cpuboard_overlay_override() {
    TRACE();
}

void cpuboard_rethink() {
    TRACE();
}

a_inode* custom_fsdb_lookup_aino_aname(a_inode_struct*, char const*) {
    UNIMPLEMENTED();
    return nullptr;
}

a_inode* custom_fsdb_lookup_aino_nname(a_inode_struct*, char const*) {
    UNIMPLEMENTED();
    return nullptr;
}

int custom_fsdb_used_as_nname(a_inode_struct*, char const*) {
    UNIMPLEMENTED();
    return 0;
}

int debuggable() {
    UNIMPLEMENTED();
    return 0;
}

void debugger_change(int) {
    UNIMPLEMENTED();
}

void desktop_coords(int, int*, int*, int*, int*, int*, int*) {
    UNIMPLEMENTED();
}

void doflashscreen() {
    UNIMPLEMENTED();
}

void doprinter(uae_u8) {
    UNIMPLEMENTED();
}

bool draco_mouse(int, int, int, int, int) {
    TRACE();
    return false;
}

void driveclick_fdrawcmd_detect() {
    TRACE();
    // UNIMPLEMENTED();
}

void driveclick_fdrawcmd_motor(int, int) {
    TRACE();
}

int driveclick_fdrawcmd_open(int) {
    TRACE();
    return 0;
}

void driveclick_fdrawcmd_seek(int, int) {
    TRACE();
}

void driveclick_fdrawcmd_vsync() {
    TRACE();
    // UNIMPLEMENTED();
}

int driveclick_loadresource(drvsample*, int) {
    TRACE();
    return 0;
}

bool dsp_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

uae_u8 dsp_read() {
    UNIMPLEMENTED();
    return 0;
}

void dsp_write(unsigned char) {
    UNIMPLEMENTED();
}

void ematrix_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

bool emulate_genlock(vidbuffer*, vidbuffer*, bool) {
    UNIMPLEMENTED();
    return false;
}

bool emulate_grayscale(vidbuffer*, vidbuffer*) {
    UNIMPLEMENTED();
    return false;
}

bool emulate_specialmonitors(vidbuffer*, vidbuffer*) {
    UNIMPLEMENTED();
    return false;
}

uae_u32 emulib_target_getcpurate(unsigned int, unsigned int*) {
    UNIMPLEMENTED();
    return 0;
}

void ethernet_reset() {
    TRACE();
    // UNIMPLEMENTED();
}

void fastlane_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void fetch_inputfilepath(char* out, int) {
    *out = 0;
    UNIMPLEMENTED();
}

void fetch_ripperpath(char* out, int) {
    *out = 0;
    UNIMPLEMENTED();
}

void fetch_rompath(char* out, int) {
    *out = 0;
    UNIMPLEMENTED();
}

void fetch_saveimagepath(char* out, int, int) {
    *out = 0;
    TRACE();
}

void fetch_videopath(char* out, int) {
    *out = 0;
    UNIMPLEMENTED();
}

#ifndef _WIN32
// TODO: Investigate why this is present in filesys.o
void filesys_addexternals() {
    // UNIMPLEMENTED();
}
#endif

void flush_log() {
    UNIMPLEMENTED();
}

void fpux_restore(int*) {
    TRACE();
}

bool frame_drawn(int) {
    // UNIMPLEMENTED();
    TRACE();
    return true;
}

void free_ahi_v2() {
    TRACE();
}

int getcapslockstate() {
    UNIMPLEMENTED();
    return 0;
}

int get_guid_target(unsigned char*) {
    UNIMPLEMENTED();
    return 0;
}

void gfxboard_free() {
    TRACE();
}

bool gfxboard_init_memory(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

void gfxboard_refresh(int) {
    UNIMPLEMENTED();
}

uae_u32 golemfast_ncr9x_scsi_get(unsigned int, int) {
    UNIMPLEMENTED();
    return 0;
}

void golemfast_ncr9x_scsi_put(unsigned int, unsigned int, int) {
    UNIMPLEMENTED();
}


int graphics_init(bool) {
    const int amiga_width = 754;
    const int amiga_height = 576;
    const int depth = 32;

    //int monitor_id = 0;
    struct vidbuf_description* avidinfo = &adisplays[0].gfxvidinfo;

    avidinfo->drawbuffer.inwidth = avidinfo->drawbuffer.outwidth = amiga_width;
    avidinfo->drawbuffer.inheight = avidinfo->drawbuffer.outheight = amiga_height;

    int pitch = amiga_width * depth >> 3;

    avidinfo->drawbuffer.pixbytes = depth >> 3;
    avidinfo->drawbuffer.bufmem = NULL;
    avidinfo->drawbuffer.linemem = NULL;
    avidinfo->drawbuffer.rowbytes = pitch;

    struct vidbuffer* buf = &avidinfo->drawbuffer;

    int width = amiga_width;
    int height = amiga_height;

    buf->monitor_id = 0;
    buf->pixbytes = (depth + 7) / 8;
    buf->width_allocated = (width + 7) & ~7;  // =760
    buf->height_allocated = height;           // 576

    int w = buf->width_allocated;
    int h = buf->height_allocated;
    int size = (w * 2) * (h * 2) * buf->pixbytes;
    buf->rowbytes = w * 2 * buf->pixbytes;
    buf->realbufmem = xcalloc(uae_u8, size);
    buf->bufmem_allocated = buf->bufmem = buf->realbufmem + (h / 2) * buf->rowbytes + (w / 2) * buf->pixbytes;
    buf->bufmemend = buf->realbufmem + size - buf->rowbytes;
    buf->bufmem_lockable = true;

    int bits = 8;
    int red_shift = 16;
    int green_shift = 8;
    int blue_shift = 0;

    alloc_colors64k(0, bits, bits, bits, red_shift, green_shift, blue_shift, bits, 24, 0, 0, false);

    qd::onUaeInitialized->set();
    return 1;
}

bool render_screen(int /*monid*/, int, bool) {
    return true;
}


void unlockscr(struct vidbuffer* vb_in, int /*y_start*/, int /*y_end*/) {
    // copy UAE screen to texture buf
    struct vidbuf_description* avidinfo = &adisplays[vb_in->monitor_id].gfxvidinfo;
    struct vidbuffer* vb = avidinfo->outbuffer;

    if (!vb || !vb->bufmem) {
        return;
    }

    uint8_t* sptr = vb->bufmem;
    int amiga_width = vb->outwidth;
    int amiga_height = vb->outheight;

    if (uint32_t* pixels = app->lockUaeScreenTexBuf(amiga_width, amiga_height)) {
        for (int y = 0; y < amiga_height; y++) {
            uint8_t* dest = (uint8_t*)&pixels[y * amiga_width];
            memcpy(dest, sptr, amiga_width * 4);
            sptr += vb->rowbytes;
        }

        app->unlockUaeScreenTexBuf();
    }
}


void graphics_leave() {
    SDL_Log("quaesar: %s()", __func__);

    int monitor_id = 0;
    struct vidbuf_description* avidinfo = &adisplays[monitor_id].gfxvidinfo;

    reset_sound();
    freevidbuffer(monitor_id, &avidinfo->drawbuffer);
    freevidbuffer(monitor_id, &avidinfo->tempbuffer);
}

void graphics_reset(bool) {
    SDL_Log("quaesar: %s()", __func__);
}

int graphics_setup() {
    TRACE();
    // UNIMPLEMENTED();
    return 1;
}

bool gui_ask_disk(int, char*) {
    UNIMPLEMENTED();
    return false;
}

bool handle_events() {
    TRACE();
    return false;
}

bool hydra_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

void init_fpucw_x87_80() {
    TRACE();
    // UNIMPLEMENTED();
}

void initparallel() {
    // UNIMPLEMENTED();
}

bool isguiactive() {
    return false;
}

bool is_mainthread() {
    TRACE();
    return true;
}

bool ismouseactive() {
    return false;
}

int is_tablet() {
    UNIMPLEMENTED();
    return 0;
}

int is_touch_lightpen() {
    UNIMPLEMENTED();
    return 0;
}

bool lanrover_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

void logging_init() {
    TRACE();
    // UNIMPLEMENTED();
    return;
}

void machdep_free() {
    SDL_Log("quaesar: %s()", __func__);
}

int machdep_init() {
    TRACE();
    // UNIMPLEMENTED();
    return 1;
}

void magnum40_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void mtecmastercard_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void multievolution_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}


bool ncr710_a4091_autoconfig_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool ncr710_draco_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool ncr710_magnum40_autoconfig_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool ncr710_warpengine_autoconfig_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool ncr710_zeus040_autoconfig_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool ncr_alf3_autoconfig_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool ncr_dkb_autoconfig_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool ncr_ematrix_autoconfig_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool ncr_fastlane_autoconfig_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

void ncr_golemfast_autoconfig_init(romconfig*, unsigned int) {
    UNIMPLEMENTED();
}

void ncr_masoboshi_autoconfig_init(romconfig*, unsigned int) {
    UNIMPLEMENTED();
}

bool ncr_mtecmastercard_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool ncr_multievolution_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool ncr_oktagon_autoconfig_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool ncr_rapidfire_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

bool ncr_scram5394_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

void ncr_squirrel_init(romconfig*, unsigned int) {
    UNIMPLEMENTED();
}

void ncr_trifecta_autoconfig_init(romconfig*, unsigned int) {
    UNIMPLEMENTED();
}

void notify_user_parms(int, char const*, ...) {
    UNIMPLEMENTED();
}

void oktagon_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

int parallel_direct_read_data(unsigned char*) {
    UNIMPLEMENTED();
    return 0;
}

int parallel_direct_read_status(unsigned char*) {
    UNIMPLEMENTED();
    return 0;
}

int parallel_direct_write_data(unsigned char, unsigned char) {
    UNIMPLEMENTED();
    return 0;
}

int parallel_direct_write_status(unsigned char, unsigned char) {
    UNIMPLEMENTED();
    return 0;
}

void pci_read_dma(pci_board_state*, unsigned int, unsigned char*, int) {
    UNIMPLEMENTED();
}

void PPCDisasm(PPCD_CB*) {
    UNIMPLEMENTED();
}

void quikpak_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void rapidfire_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void release_keys() {
    UNIMPLEMENTED();
}

void restore_cdtv_final() {
    UNIMPLEMENTED();
}

void restore_cdtv_finish() {
    UNIMPLEMENTED();
}

bool samepath(char const*, char const*) {
    UNIMPLEMENTED();
    return false;
}

void sampler_free() {
    SDL_Log("quaesar: %s()", __func__);
}

uae_u8 sampler_getsample(int) {
    UNIMPLEMENTED();
    return 0;
}

int sampler_init() {
    TRACE();
    return 0;
}

void sampler_vsync() {
    TRACE();
}

uae_u8* save_screenshot(int, size_t*) {
    UNIMPLEMENTED();
    return nullptr;
}

void scram5394_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void serial_uartbreak(int) {
    // UNIMPLEMENTED();
}

void serial_hsynchandler() {
    // UNIMPLEMENTED();
}

void serial_rbf_clear() {
    SDL_Log("quaesar: %s()", __func__);
}

uae_u8 serial_readstatus(uae_u8 v, uae_u8) {
    // UNIMPLEMENTED();
    return v;
}

void serial_rethink() {
    serial_flush_buffer();
}

void setup_brkhandler() {
    TRACE();
}

bool show_screen_maybe(int, bool) {
    UNIMPLEMENTED();
    return false;
}

int sleep_millis_main(int) {
    UNIMPLEMENTED();
    return 0;
}

void sndboard_free_capture() {
    SDL_Log("quaesar: %s()", __func__);
}

int sndboard_get_buffer(int*) {
    UNIMPLEMENTED();
    return 0;
}

/*
bool sndboard_init_capture(int) {
    UNIMPLEMENTED();
    return false;
}
*/

int sndboard_release_buffer(unsigned char*, int) {
    UNIMPLEMENTED();
    return 0;
}

bool specialmonitor_autoconfig_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

void specialmonitor_reset() {
    TRACE();
}

void specialmonitor_store_fmode(int, int, unsigned short) {
    UNIMPLEMENTED();
}

bool specialmonitor_uses_control_lines() {
    UNIMPLEMENTED();
    return 0;
}

void squirrel_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void statusline_render(int, unsigned char*, int, int, int, int, unsigned int*, unsigned int*, unsigned int*,
                       unsigned int*) {
    TRACE();
}

void statusline_updated(int) {
    TRACE();
}

void sub_to_deinterleaved(unsigned char const*, unsigned char*) {
    UNIMPLEMENTED();
}

void sub_to_interleaved(unsigned char const*, unsigned char*) {
    UNIMPLEMENTED();
}

float target_adjust_vblank_hz(int, float hz) {
    TRACE();
    return hz;
}

bool target_can_autoswitchdevice() {
    UNIMPLEMENTED();
    return false;
}

int target_checkcapslock(int, int*) {
    UNIMPLEMENTED();
    return 0;
}

void target_fixup_options(uae_prefs*) {
    TRACE();
    // UNIMPLEMENTED();
}

float target_getcurrentvblankrate(int) {
    UNIMPLEMENTED();
    return 50.0f;
}

void target_getdate(int*, int*, int*) {
    TRACE();
}

#ifndef _WIN32
// TODO: Investigate why this is present in filesys.o already on win32
int target_get_volume_name(uaedev_mount_info*, uaedev_config_info*, bool, bool, int) {
    UNIMPLEMENTED();
    return 0;
}
#endif

void target_inputdevice_acquire() {
    // UNIMPLEMENTED();
}

void target_inputdevice_unacquire() {
    // UNIMPLEMENTED();
}

bool target_isrelativemode() {
    UNIMPLEMENTED();
    return false;
}

uae_u8* target_load_keyfile(uae_prefs*, char const*, int*, char*) {
    UNIMPLEMENTED();
    return 0;
}

void target_multipath_modified(uae_prefs*) {
    UNIMPLEMENTED();
}

void target_osk_control(int, int, int, int) {
    UNIMPLEMENTED();
}

void target_paste_to_keyboard() {
    UNIMPLEMENTED();
}

void target_quit() {
    SDL_Log("quaesar: %s()", __func__);
}

void target_reset() {
    SDL_Log("quaesar: %s()", __func__);
}

void target_restart() {
    SDL_Log("quaesar: %s()", __func__);
}

void target_run() {
    SDL_Log("quaesar: %s()", __func__);
}

void target_save_options(zfile*, uae_prefs*) {
    SDL_Log("quaesar: %s()", __func__);
}

void tekmagic_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void toggle_mousegrab() {
    UNIMPLEMENTED();
}

void to_upper(char* s, int len) {
    for (int i = 0; i < len; i++) {
        s[i] = toupper(s[i]);
    }
}

uae_u32 trifecta_ncr9x_scsi_get(unsigned int, int) {
    UNIMPLEMENTED();
    return 0;
}

void typhoon2scsi_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

bool typhoon2scsi_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

const TCHAR** uaenative_get_library_dirs() {
    UNIMPLEMENTED();
    return nullptr;
}

void* uaenative_get_uaevar() {
    UNIMPLEMENTED();
    return nullptr;
}

void uaeser_clearbuffers(void*) {
    SDL_Log("quaesar: %s()", __func__);
}

int uaeser_getdatalength() {
    UNIMPLEMENTED();
    return 0;
}

int uaeser_open(void*, void*, int) {
    UNIMPLEMENTED();
    return 0;
}

int uaeser_query(void*, unsigned short*, unsigned int*) {
    UNIMPLEMENTED();
    return 0;
}

int uaeser_read(void*, unsigned char*, unsigned int) {
    UNIMPLEMENTED();
    return 0;
}

int uaeser_setparams(void*, int, int, int, int, int, int, unsigned int) {
    UNIMPLEMENTED();
    return 0;
}

void uaeser_trigger(void*) {
    UNIMPLEMENTED();
}

void uae_slirp_cleanup() {
    SDL_Log("quaesar: %s()", __func__);
}

void uae_slirp_end() {
    UNIMPLEMENTED();
}

int uae_slirp_init() {
    UNIMPLEMENTED();
    return 0;
}

void uae_slirp_input(unsigned char const*, int) {
    UNIMPLEMENTED();
}

bool uae_slirp_start() {
    UNIMPLEMENTED();
    return false;
}

void update_debug_info() {
    // UNIMPLEMENTED();
}

void updatedisplayarea(int) {
    TRACE();
}

bool vsync_switchmode(int, int) {
    UNIMPLEMENTED();
    return false;
}

void warpengine_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void x86_bridge_sync_change() {
    TRACE();
    // return 0;
}

void x86_doirq(unsigned char) {
    UNIMPLEMENTED();
}

bool x86_mouse(int, int, int, int, int) {
    TRACE();
    return false;
}

int x86_rt1000_add_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
    return 0;
}

int x86_rt1000_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return 0;
}

void x86_xt_ide_bios(zfile*, romconfig*) {
    UNIMPLEMENTED();
}

bool xsurf_init(autoconfig_info*) {
    UNIMPLEMENTED();
    return false;
}

void zeus040_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void bsdlib_reset() {
    UNIMPLEMENTED();
}

int build_comp() {
    UNIMPLEMENTED();
    return 0;
}

TCHAR console_getch() {
    UNIMPLEMENTED();
    return 0;
}

bool cpuboard_io_special(int, unsigned int*, int, bool) {
    UNIMPLEMENTED();
    return false;
}

void cpuboard_map() {
    TRACE();
}

int cpuboard_maxmemory(uae_prefs*) {
    UNIMPLEMENTED();
    return 0;
}

void cpuboard_reset(int) {
    TRACE();
}

void cyberstorm_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void draco_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void draco_ext_interrupt(bool) {
    UNIMPLEMENTED();
}

void driveclick_fdrawcmd_close(int) {
    TRACE();
    // UNIMPLEMENTED();
}

void dsp_pause(int) {
    UNIMPLEMENTED();
}

void ethernet_pause(int) {
    UNIMPLEMENTED();
}

bool fp_init_native_80() {
    UNIMPLEMENTED();
    return false;
}

uae_u32 getlocaltime() {
    TRACE();
    // UNIMPLEMENTED();
    return 0;
}

/*
float getvsyncrate(int, float, int*) {
    UNIMPLEMENTED();
    return 50.0f;
}
*/

const TCHAR* gfxboard_get_configname(int) {
    UNIMPLEMENTED();
    return nullptr;
}

int gfxboard_get_configtype(rtgboardconfig*) {
    TRACE();
    // UNIMPLEMENTED();
    return 0;
}

bool gfxboard_set(int, bool) {
    UNIMPLEMENTED();
    return false;
}

void golemfast_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

int handle_msgpump(bool) {
    TRACE();
    return 0;
}

int input_get_default_joystick_analog(uae_input_device*, int, int, int, bool, bool) {
    UNIMPLEMENTED();
    return 0;
}

int input_get_default_lightpen(uae_input_device*, int, int, int, bool, bool, int) {
    UNIMPLEMENTED();
    return 0;
}

int input_get_default_mouse(uae_input_device*, int, int, int, bool, bool, bool) {
    UNIMPLEMENTED();
    return 0;
}

int isfullscreen() {
    return 0;
}

void masoboshi_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

uae_u32 masoboshi_ncr9x_scsi_get(unsigned int, int) {
    UNIMPLEMENTED();
    return 0;
}

uae_u8* restore_cdtv_dmac(unsigned char*) {
    UNIMPLEMENTED();
    return nullptr;
}

uae_u8* restore_cdtv(unsigned char*) {
    UNIMPLEMENTED();
    return nullptr;
}

uae_u8* save_cdtv_dmac(size_t*, uae_u8*) {
    UNIMPLEMENTED();
    return nullptr;
}

uae_u8* save_cdtv(size_t*, uae_u8*) {
    UNIMPLEMENTED();
    return nullptr;
}

int set_cache_state(int) {
    UNIMPLEMENTED();
    return 0;
}

void setcapslockstate(int) {
    UNIMPLEMENTED();
}

void setmouseactivexy(int, int, int, int) {
    // UNIMPLEMENTED();
}

uae_u32 squirrel_ncr9x_scsi_get(unsigned int, int) {
    UNIMPLEMENTED();
    return 0;
}

void squirrel_ncr9x_scsi_put(unsigned int, unsigned int, int) {
    UNIMPLEMENTED();
}

void target_cpu_speed() {
    // UNIMPLEMENTED();
}

void target_default_options(uae_prefs* p, int type) {
    TRACE();
    if (type == 1 || type == 0 || type == 3) {
        //p->win32_uaescsimode = UAESCSI_CDEMU;
        p->win32_midioutdev = -2;
        p->win32_midiindev = 0;
        p->win32_midirouter = false;
        p->win32_automount_removable = 0;
        p->win32_automount_drives = 0;
        p->win32_automount_removabledrives = 0;
        p->win32_automount_cddrives = 0;
        p->win32_automount_netdrives = 0;
        p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5PC | RGBFF_B8G8R8A8;
        p->win32_filesystem_mangle_reserved_names = true;
    }
}

static int old_w = -1;
static int old_h = -1;

bool target_graphics_buffer_update(int monid, bool /*force*/) {
    struct vidbuf_description* avidinfo = &adisplays[monid].gfxvidinfo;
    struct vidbuffer* vb = avidinfo->drawbuffer.tempbufferinuse ? &avidinfo->tempbuffer : &avidinfo->drawbuffer;

    if ((vb->outwidth != old_w) || (old_h != vb->outheight)) {
        old_w = vb->outwidth;
        old_h = vb->outheight;
        return true;
    }

    TRACE();

    return false;
}

int target_parse_option(uae_prefs*, char const*, char const*, int) {
    UNIMPLEMENTED();
    return 0;
}

int target_sleep_nanos(int) {
    UNIMPLEMENTED();
    return 0;
}

void trifecta_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void trifecta_ncr9x_scsi_put(unsigned int, unsigned int, int) {
    UNIMPLEMENTED();
}

int uaeser_break(void*, int) {
    UNIMPLEMENTED();
    return 0;
}

void uaeser_close(void*) {
    UNIMPLEMENTED();
}

int uaeser_write(void*, unsigned char*, unsigned int) {
    UNIMPLEMENTED();
    return 0;
}

bool cdtv_front_panel(int) {
    UNIMPLEMENTED();
    return false;
}

/*
int check_prefs_changed_comp(bool) {
    UNIMPLEMENTED();
    return 0;
}
*/

void cpuboard_ncr9x_add_scsi_unit(int, uaedev_config_info*, romconfig*) {
    UNIMPLEMENTED();
}

void f_out(void*, char const*, ...) {
    UNIMPLEMENTED();
}

static int dummy_open_bus_func(int flags) {
    debug("Dummy open_bus_func called with flags: %d\n", flags);
    return 0;
}

static void dummy_close_bus_func(void) {
    debug("Dummy close_bus_func called\n");
}

static int dummy_open_device_func(int deviceID, const TCHAR* deviceName, int flags) {
    debug("Dummy open_device_func called with deviceID: %d, deviceName: %s, flags: %d\n", deviceID, deviceName, flags);
    return 0;
}

static void dummy_close_device_func(int deviceID) {
    debug("Dummy close_device_func called with deviceID: %d\n", deviceID);
}

static struct device_info* dummy_info_device_func(int deviceID, struct device_info* /*info*/, int size, int flags) {
    debug("Dummy info_device_func called with deviceID: %d, size: %d, flags: %d\n", deviceID, size, flags);
    return NULL;
}

static uae_u8* dummy_execscsicmd_out_func(int deviceID, uae_u8* /*cmd*/, int size) {
    debug("Dummy execscsicmd_out_func called with deviceID: %d, size: %d\n", deviceID, size);
    return NULL;
}

static uae_u8* dummy_execscsicmd_in_func(int deviceID, uae_u8* /*cmd*/, int size, int* result) {
    debug("Dummy execscsicmd_in_func called with deviceID: %d, size: %d\n", deviceID, size);
    *result = 0;
    return NULL;
}

static int dummy_execscsicmd_direct_func(int deviceID, struct amigascsi* /*cmd*/) {
    debug("Dummy execscsicmd_direct_func called with deviceID: %d\n", deviceID);
    return 0;
}

// static void dummy_play_subchannel_callback(uae_u8* data, int size) {
//     debug("Dummy play_subchannel_callback called with size: %d\n", size);
// }
// static int dummy_play_status_callback(int status, int subcode) {
//     debug("Dummy play_status_callback called with status: %d, subcode: %d\n", status, subcode);
//     return 0;
// }

static int dummy_pause_func(int deviceID, int flags) {
    debug("Dummy pause_func called with deviceID: %d, flags: %d\n", deviceID, flags);
    return 0;
}

static int dummy_stop_func(int deviceID) {
    debug("Dummy stop_func called with deviceID: %d\n", deviceID);
    return 0;
}

static int dummy_play_func(int deviceID, int track, int index, int flags, play_status_callback /*status_callback*/,
                           play_subchannel_callback /*subchannel_callback*/) {
    debug("Dummy play_func called with deviceID: %d, track: %d, index: %d, flags: %d\n", deviceID, track, index, flags);
    return 0;
}

static uae_u32 dummy_volume_func(int deviceID, uae_u16 left, uae_u16 right) {
    debug("Dummy volume_func called with deviceID: %d, left: %u, right: %u\n", deviceID, left, right);
    return 0;
}

static int dummy_qcode_func(int deviceID, uae_u8* /*qcode*/, int size, bool msf) {
    debug("Dummy qcode_func called with deviceID: %d, size: %d, msf: %d\n", deviceID, size, msf);
    return 0;
}

static int dummy_toc_func(int deviceID, struct cd_toc_head* /*toc*/) {
    debug("Dummy toc_func called with deviceID: %d\n", deviceID);
    return 0;
}

static int dummy_read_func(int deviceID, uae_u8* /*buffer*/, int size, int flags) {
    debug("Dummy read_func called with deviceID: %d, size: %d, flags: %d\n", deviceID, size, flags);
    return 0;
}

static int dummy_rawread_func(int deviceID, uae_u8* /*buffer*/, int size, int subcode, int flags, uae_u32 offset) {
    debug("Dummy rawread_func called with deviceID: %d, size: %d, subcode: %d, flags: %d, offset: %u\n", deviceID, size,
          subcode, flags, offset);
    return 0;
}

static int dummy_write_func(int deviceID, uae_u8* /*buffer*/, int size, int flags) {
    debug("Dummy write_func called with deviceID: %d, size: %d, flags: %d\n", deviceID, size, flags);
    return 0;
}

int dummy_isatapi_func(int deviceID) {
    debug("Dummy isatapi_func called with deviceID: %d\n", deviceID);
    return 0;
}

int dummy_ismedia_func(int deviceID, int flags) {
    debug("Dummy ismedia_func called with deviceID: %d, flags: %d\n", deviceID, flags);
    return 0;
}

int dummy_scsiemu_func(int deviceID, uae_u8* /*data*/) {
    debug("Dummy scsiemu_func called with deviceID: %d\n", deviceID);
    return 0;
}

struct device_functions devicefunc_cdimage = {_T("DummyDevice"),
                                              dummy_open_bus_func,
                                              dummy_close_bus_func,
                                              dummy_open_device_func,
                                              dummy_close_device_func,
                                              dummy_info_device_func,
                                              dummy_execscsicmd_out_func,
                                              dummy_execscsicmd_in_func,
                                              dummy_execscsicmd_direct_func,
                                              dummy_pause_func,
                                              dummy_stop_func,
                                              dummy_play_func,
                                              dummy_volume_func,
                                              dummy_qcode_func,
                                              dummy_toc_func,
                                              dummy_read_func,
                                              dummy_rawread_func,
                                              dummy_write_func,
                                              dummy_isatapi_func,
                                              dummy_ismedia_func,
                                              dummy_scsiemu_func};

struct device_functions devicefunc_scsi_ioctl = {_T("IOCTL"),
                                                 dummy_open_bus_func,
                                                 dummy_close_bus_func,
                                                 dummy_open_device_func,
                                                 dummy_close_device_func,
                                                 dummy_info_device_func,
                                                 dummy_execscsicmd_out_func,
                                                 dummy_execscsicmd_in_func,
                                                 dummy_execscsicmd_direct_func,
                                                 dummy_pause_func,
                                                 dummy_stop_func,
                                                 dummy_play_func,
                                                 dummy_volume_func,
                                                 dummy_qcode_func,
                                                 dummy_toc_func,
                                                 dummy_read_func,
                                                 dummy_rawread_func,
                                                 dummy_write_func,
                                                 dummy_isatapi_func,
                                                 dummy_ismedia_func,
                                                 dummy_scsiemu_func};

struct device_functions devicefunc_scsi_spti = {_T("IOCTL"),
                                                dummy_open_bus_func,
                                                dummy_close_bus_func,
                                                dummy_open_device_func,
                                                dummy_close_device_func,
                                                dummy_info_device_func,
                                                dummy_execscsicmd_out_func,
                                                dummy_execscsicmd_in_func,
                                                dummy_execscsicmd_direct_func,
                                                dummy_pause_func,
                                                dummy_stop_func,
                                                dummy_play_func,
                                                dummy_volume_func,
                                                dummy_qcode_func,
                                                dummy_toc_func,
                                                dummy_read_func,
                                                dummy_rawread_func,
                                                dummy_write_func,
                                                dummy_isatapi_func,
                                                dummy_ismedia_func,
                                                dummy_scsiemu_func};

const TCHAR* specialmonitorconfignames[] = {_T("none"), NULL};

TCHAR avioutput_filename_gui[MAX_DPATH];
void* pushall_call_handler = nullptr;

#ifdef _WIN32
void gettimeofday(struct timeval* tv, void* /*blah*/) {
#if 1
    struct timeb time;

    ftime(&time);

    tv->tv_sec = (long)time.time;
    tv->tv_usec = time.millitm * 1000;
#else
    SYSTEMTIME st;
    FILETIME ft;
    uae_u64 v, sec;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);
    v = (ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    v /= 10;
    sec = v / 1000000;
    tv->tv_usec = (unsigned long)(v - (sec * 1000000));
    tv->tv_sec = (unsigned long)(sec - 11644463600);
#endif
}
#endif

void update_disassembly(uae_u32) {
    UNIMPLEMENTED();
}

void update_memdump(uae_u32) {
    UNIMPLEMENTED();
}

// dummy write_log
void write_log(const char* format, ...) {
    va_list vargs;

    va_start(vargs, format);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, format, vargs);
    qd::logConsole().logV(qd::LogEntry::E_INFO, format, vargs);
    va_end(vargs);
}

// dummy write_log
void write_dlog(const char* format, ...) {
    va_list parms;

    va_start(parms, format);
    // vprintf(format, parms);
    qd::logConsole().logV(qd::LogEntry::E_INFO, format, parms);
    va_end(parms);
}

void console_out_f(const TCHAR* format, ...) {
    va_list parms;

    va_start(parms, format);
    qd::logConsole().logV(qd::LogEntry::E_INFO, format, parms);
    va_end(parms);
}

void console_out(const TCHAR* txt) {
    console_out_f("%s", txt);
}

TCHAR* buf_out(TCHAR* buffer, int* bufsize, const TCHAR* format, ...) {
    va_list parms;
    va_start(parms, format);

    if (buffer == NULL)
        return 0;
    _vsntprintf(buffer, (*bufsize) - 1, format, parms);
    va_end(parms);
    *bufsize -= uaetcslen(buffer);
    return buffer + uaetcslen(buffer);
}

static TCHAR* console_buffer;
static int console_buffer_size;

TCHAR* setconsolemode(TCHAR* buffer, int maxlen) {
    TCHAR* ret = NULL;
    if (buffer) {
        console_buffer = buffer;
        console_buffer_size = maxlen;
    } else {
        ret = console_buffer;
        console_buffer = NULL;
    }
    return ret;
}

// dummy win support for blkdev.cpp
int GetDriveType(TCHAR* /*vol*/) {
    return 0;
}
