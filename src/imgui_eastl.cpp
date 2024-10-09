#include "imgui_eastl.h"

struct InputTextCallback_UserData {
    eastl::string* str = nullptr;
};

static int _inputTextCallback(ImGuiInputTextCallbackData* data) {
    auto user_data = static_cast<InputTextCallback_UserData*>(data->UserData);
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        eastl::string* str = user_data->str;
        IM_ASSERT(data->Buf == str->c_str());
        str->resize(data->BufTextLen);
        data->Buf = (char*)str->c_str();
    }
    return 0;
}

bool ImGui::InputText(const char* label, eastl::string* p_str, ImGuiInputTextFlags flags) {
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;
    InputTextCallback_UserData cb_user_data = {p_str};
    return ImGui::InputText(label, p_str->data(), p_str->capacity() + 1, flags, _inputTextCallback, &cb_user_data);
}

bool ImGui::InputTextMultiline(const char* label, eastl::string* p_str, const ImVec2& size, ImGuiInputTextFlags flags) {
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;
    InputTextCallback_UserData cb_user_data = {p_str};
    return ImGui::InputTextMultiline(label, p_str->data(), p_str->capacity() + 1, size, flags, _inputTextCallback,
                                     &cb_user_data);
}

bool ImGui::InputTextWithHint(const char* label, const char* hint, eastl::string* p_str, ImGuiInputTextFlags flags) {
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;
    InputTextCallback_UserData cb_user_data = {p_str};
    return ImGui::InputTextWithHint(label, hint, p_str->data(), p_str->capacity() + 1, flags, _inputTextCallback,
                                    &cb_user_data);
}
