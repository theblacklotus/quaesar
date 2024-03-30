#pragma once

struct RegisterView;

RegisterView* RegisterView_create();
void RegisterView_update(RegisterView* self);
void RegisterView_destroy(RegisterView* self);

