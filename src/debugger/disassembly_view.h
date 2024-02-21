#pragma once

struct DisassemblyView;

DisassemblyView* DisassemblyView_create();
void DisassemblyView_update(DisassemblyView* self, const char* name);
void DisassemblyView_destroy(DisassemblyView* self);
