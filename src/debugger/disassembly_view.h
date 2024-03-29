#pragma once

#include <capstone/capstone.h>

struct DisassemblyView;

DisassemblyView* DisassemblyView_create(csh capstone);
void DisassemblyView_update(DisassemblyView* self, const char* name);
void DisassemblyView_destroy(DisassemblyView* self);
