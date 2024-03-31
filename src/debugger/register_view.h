#pragma once

struct RegisterView;
struct SelectedRegisters;

RegisterView* RegisterView_create();
void RegisterView_update(RegisterView* self, const SelectedRegisters* selected_registers);
void RegisterView_destroy(RegisterView* self);

