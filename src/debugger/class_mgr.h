#pragma once
#include <EASTL/string.h>
#include <EASTL/vector_map.h>
#include "imgui_eastl.h"

namespace qd {

template <class TClassInt>
class ClassInfoMgr_ {
    typedef ClassInfoMgr_<TClassInt> TThis;

public:
    eastl::vector_map<uint32_t, void* /*Callback*/> m_classInfoMap;
    typedef eastl::vector_map<uint32_t, void* /*Callback*/> TModuleInfoMap;

public:
    static TThis* get() {
        static TThis instance;
        return &instance;
    }

public:
    ClassInfoMgr_() = default;
    ~ClassInfoMgr_(void) = default;

    inline void registerClass(uint32_t classId, TClassInt* (*func)()) {
        return _registerClass(classId, (void*)func);
    }

    template <typename P1>
    inline void registerClass(uint32_t classId, TClassInt* (*func)(P1)) {
        return _registerClass(classId, (void*)func);
    }

    void* findModuleInfo(uint32_t classId) const {
        typename TModuleInfoMap::const_iterator It = m_classInfoMap.find(classId);
        if (It == m_classInfoMap.end()) {
            EASTL_ASSERT_MSG(0,
                             eastl::string(eastl::string::CtorSprintf(), "classId:%u not registered", classId).c_str());
        }
        return It->second;
    }

    template <typename... TArgs>
    inline TClassInt* makeInstance(uint32_t classId, TArgs... args) const {
        void* createFunc = findModuleInfo(classId);
        typedef TClassInt* (*TCreateFunc)(TArgs...);
        auto fn = reinterpret_cast<TCreateFunc>(createFunc);
        TClassInt* pInstance = (*fn)(args...);
        return pInstance;
    }

private:
    void _registerClass(uint32_t classId, void* createFunc) {
        auto insIt = m_classInfoMap.insert(eastl::make_pair(classId, createFunc));
        if (insIt.second == false)
            EASTL_ASSERT_MSG(
                0,
                eastl::string(eastl::string::CtorSprintf(), "Registered classId:%u already exists", classId).c_str());
    }

};  // class ClassInfoMgr_
//////////////////////////////////////////////////////////////////////////

};  // namespace qd
