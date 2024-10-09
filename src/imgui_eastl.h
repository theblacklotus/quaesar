#pragma once
#include <EASTL/fixed_vector.h>
#include <EASTL/utility.h>
#include <dear_imgui/imgui.h>
#include "eastl.h"

#ifdef _MSC_VER
#define _PRISizeT "I"
#define ImSnprintf _snprintf
#else
#define _PRISizeT "z"
#define ImSnprintf snprintf
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)  // warning C4996: 'sprintf': This function or variable may be unsafe.
#endif

namespace ImGui {

// ImGui::InputText() with eastl::string
// Because text input needs dynamic resizing, we need to setup a callback to grow the capacity
bool InputText(const char* label, eastl::string* str, ImGuiInputTextFlags flags = 0);

bool InputTextMultiline(const char* label, eastl::string* str, const ImVec2& size = ImVec2(0, 0),
                        ImGuiInputTextFlags flags = 0);

bool InputTextWithHint(const char* label, const char* hint, eastl::string* str, ImGuiInputTextFlags flags = 0);

struct FixedStringInputTextCallback {
    void* str;
};

template <class TAllocator>
static int _inputFixedStringCallback(ImGuiInputTextCallbackData* data) {
    using TString = eastl::basic_string<char, TAllocator>;
    auto user_data = static_cast<FixedStringInputTextCallback*>(data->UserData);
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        TString* str = static_cast<TString*>(user_data->str);
        IM_ASSERT(data->Buf == str->c_str());
        if (str->capacity() >= data->BufTextLen) {
            str->resize(data->BufTextLen);
        } else {
            data->BufTextLen = str->capacity();
            data->BufDirty = true;
        }
        data->Buf = (char*)str->c_str();
    }
    return 0;
}

template <int S, bool OV, typename A>
bool InputText(const char* label, eastl::fixed_string<char, S, OV, A>* str, ImGuiInputTextFlags flags = 0) {
    flags |= ImGuiInputTextFlags_CallbackResize;
    using TString = eastl::basic_string<char, eastl::fixed_string<char, S, OV, A>::fixed_allocator_type>;
    FixedStringInputTextCallback data = {static_cast<TString*>(str)};
    return ImGui::InputText(label, str->data(), str->capacity() + 1, flags,
                            _inputFixedStringCallback<TString::allocator_type>, &data);
}

}  // namespace ImGui
//////////////////////////////////////////////////////////////////////////

class QImPushFloatLock {
    eastl::fixed_vector<eastl::pair<float*, float>, 8, false> stack;

public:
    void pushFloat(float* p_val, float new_val) {
        float oldVal = *p_val;
        *p_val = new_val;
        stack.emplace_back(p_val, oldVal);
    }

    ~QImPushFloatLock() {
        while (!stack.empty()) {
            auto& it = stack.back();
            float* valPtr = it.first;
            float prevVal = it.second;
            *valPtr = prevVal;
            stack.pop_back();
        }
    }
};  // class QImPushFloatLock
