#pragma once

#include <capstone/capstone.h>

struct SelectedRegisters {
    uint32_t read_registers_colors[64];
    uint32_t write_register_colors[64];
    uint8_t read_registers[64];
    uint8_t write_registers[64];
    uint8_t read_register_count;
    uint8_t write_register_count;
};

struct DisassemblyView;

DisassemblyView* DisassemblyView_create(csh capstone);
SelectedRegisters* DisassemblyView_get_selected_registers(DisassemblyView* self);
void DisassemblyView_update(DisassemblyView* self, const char* name);
void DisassemblyView_destroy(DisassemblyView* self);
