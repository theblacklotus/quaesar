// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "audio.h"
// clang-format on

#define SND_MAX_BUFFER 65536

uae_u16 paula_sndbuffer[SND_MAX_BUFFER];
uae_u16* paula_sndbufpt;

int paula_sndbufsize = 0;
int active_sound_stereo;

bool audio_finish_pull(void) {
    // UNIMPLEMENTED();
    return false;
}

void sound_volume(int) {
    UNIMPLEMENTED();
}

void set_volume(int, int) {
    UNIMPLEMENTED();
}

void master_sound_volume(int) {
    UNIMPLEMENTED();
}

void ahi_close_sound() {
    TRACE();
}

void finish_sound_buffer() {
    UNIMPLEMENTED();
}

int init_sound() {
    TRACE();
    return 0;
}

void pause_sound_buffer() {
    UNIMPLEMENTED();
}

void reset_sound() {
    TRACE();
}

void restart_sound_buffer() {
    TRACE();
}

int setup_sound() {
    TRACE();
    // UNIMPLEMENTED();
    return 0;
}

void sound_mute(int) {
    UNIMPLEMENTED();
}

void update_sound(float) {
    UNIMPLEMENTED();
}

void x86_update_sound(float) {
    UNIMPLEMENTED();
}

void close_sound() {
    UNIMPLEMENTED();
}

void pause_sound() {
    UNIMPLEMENTED();
}

void resume_sound() {
    TRACE();
    // UNIMPLEMENTED();
}

bool audio_is_pull_event() {
    // TRACE();
    return false;
}

int audio_pull_buffer() {
    UNIMPLEMENTED();
    return 0;
}

int audio_is_pull() {
    // TRACE();
    return 0;
}
