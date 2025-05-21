#pragma once
#include <EASTL/string.h>
#include <EASTL/vector_map.h>
#include <src/generic/base.h>
#include "imgui_eastl.h"

namespace qd {

template <class TBaseClass>
class ClassInfoRegistry_ {
    typedef ClassInfoRegistry_<TBaseClass> TThis;

public:
    struct MetaInfo {
        uint32_t classId;
        void* createCallback;
        const char* className = nullptr;
        const std::type_info* rtti = nullptr;

    public:
        void registerClass();
    };  // MetaInfo

    eastl::vector_map<uint32_t, TThis::MetaInfo> mClassInfoMap;
    typedef eastl::vector_map<uint32_t, TThis::MetaInfo> TClassInfoMap;

public:
    static TThis* get() {
        static TThis instance;
        return &instance;
    }

public:
    ClassInfoRegistry_() = default;
    ~ClassInfoRegistry_(void) = default;

    void registerClass(TThis::MetaInfo&& meta) {
        auto insIt = mClassInfoMap.insert(eastl::make_pair(meta.classId, eastl::move(meta)));
        if (insIt.second == false) {
            ASSERT_F(0, "Registered classId:%u already exists", meta.classId);
        }
    }

    const MetaInfo* findClassInfo(uint32_t class_id) const {
        typename TClassInfoMap::const_iterator it = mClassInfoMap.find(class_id);
        if (it == mClassInfoMap.end()) {
            ASSERT_F(0, "classId:%u not registered", class_id);
            return nullptr;
        }
        return &it->second;
    }

    template <typename... TArgs>
    inline TBaseClass* makeInstance(uint32_t class_id, TArgs... args) const {
        const TThis::MetaInfo* pClassInfo = findClassInfo(class_id);
        if (!pClassInfo)
            return nullptr;
        typedef TBaseClass* (*TCreateInstanceFunc)(const TThis::MetaInfo&, TArgs...);
        auto fn = reinterpret_cast<TCreateInstanceFunc>(pClassInfo->createCallback);
        TBaseClass* pInstance = (*fn)(*pClassInfo, args...);
        return pInstance;
    }

};  // class ClassInfoRegistry_
//////////////////////////////////////////////////////////////////////////


template <class TBaseClass>
inline void ClassInfoRegistry_<TBaseClass>::MetaInfo::registerClass() {
    TThis* pClassMgr = TThis::get();
    pClassMgr->registerClass(eastl::move(*this));
}

};  // namespace qd
