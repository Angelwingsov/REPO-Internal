#include "mono_bridge.h"

namespace Mono {

Bridge& Bridge::Get() {
    static Bridge instance;
    return instance;
}

void Bridge::AssemblyCallback(void* item, void* userData) {
    auto* bridge = static_cast<Bridge*>(userData);
    if (!bridge || !item || !bridge->m_mono_assembly_get_image || !bridge->m_mono_image_get_name) {
        return;
    }

    auto* asmItem = static_cast<MonoAssembly*>(item);
    MonoImage* img = bridge->m_mono_assembly_get_image(asmItem);
    if (!img) return;

    const char* rawName = bridge->m_mono_image_get_name(img);
    if (!rawName) return;

    std::string name(rawName);
    if ((name == "Assembly-CSharp" || name == "Assembly-CSharp.dll") && !bridge->m_AppImage) {
        bridge->m_AppImage = img;
        LOG_OK("Found Assembly-CSharp Image: %p (%s)", img, rawName);
    }
    if ((name.find("UnityEngine.CoreModule") != std::string::npos || name.find("UnityEngine.dll") != std::string::npos) && !bridge->m_CoreImage) {
        bridge->m_CoreImage = img;
        LOG_OK("Found UnityEngine Image: %p (%s)", img, rawName);
    }
    if ((name.find("UnityEngine.PhysicsModule") != std::string::npos || name.find("UnityEngine.Physics.dll") != std::string::npos) && !bridge->m_PhysicsImage) {
        bridge->m_PhysicsImage = img;
        LOG_OK("Found UnityEngine.PhysicsModule Image: %p (%s)", img, rawName);
    }

    if ((name == "PhotonUnityNetworking" || name == "PhotonUnityNetworking.dll") && !bridge->m_PhotonImage) {
        bridge->m_PhotonImage = img;
        LOG_OK("Found PhotonUnityNetworking Image: %p (%s)", img, rawName);
    }
    if (name.find("Unity.TextMeshPro") != std::string::npos && !bridge->m_TextMeshProImage) {
        bridge->m_TextMeshProImage = img;
        LOG_OK("Found Unity.TextMeshPro Image: %p (%s)", img, rawName);
    }
}

bool Bridge::Initialize() {
    if (m_Initialized && m_AppImage && m_CoreImage) {
        return true;
    }

    if (!m_hMono) {
        m_hMono = GetModuleHandleA("mono-2.0-bdwgc.dll");
        if (!m_hMono) {
            m_hMono = GetModuleHandleA("mono.dll");
        }
        if (!m_hMono) {
            return false;
        }
        LOG_OK("Mono runtime base: %p", m_hMono);
    }

    #define RESOLVE_EXPORT(fn) \
        m_##fn = reinterpret_cast<fn##_t>(GetProcAddress(m_hMono, #fn)); \
        if (!m_##fn) LOG_WARN("Mono export missing: %s", #fn);

    RESOLVE_EXPORT(mono_class_get_type);
    RESOLVE_EXPORT(mono_type_get_object);
    RESOLVE_EXPORT(mono_thread_attach);
    RESOLVE_EXPORT(mono_thread_current);
    RESOLVE_EXPORT(mono_get_root_domain);
    RESOLVE_EXPORT(mono_domain_get);
    RESOLVE_EXPORT(mono_assembly_get_image);
    RESOLVE_EXPORT(mono_assembly_foreach);
    RESOLVE_EXPORT(mono_image_get_name);
    RESOLVE_EXPORT(mono_class_from_name);
    RESOLVE_EXPORT(mono_class_from_name_case);
    RESOLVE_EXPORT(mono_class_get_field_from_name);
    RESOLVE_EXPORT(mono_field_get_value);
    RESOLVE_EXPORT(mono_field_static_get_value);
    RESOLVE_EXPORT(mono_class_vtable);
    RESOLVE_EXPORT(mono_runtime_class_init);
    RESOLVE_EXPORT(mono_vtable_get_static_field_data);
    RESOLVE_EXPORT(mono_field_get_offset);
    RESOLVE_EXPORT(mono_class_get_method_from_name);
    RESOLVE_EXPORT(mono_runtime_invoke);
    RESOLVE_EXPORT(mono_object_unbox);
    RESOLVE_EXPORT(mono_string_to_utf8);
    RESOLVE_EXPORT(mono_string_new);
    RESOLVE_EXPORT(mono_free);
    RESOLVE_EXPORT(mono_gchandle_new);
    RESOLVE_EXPORT(mono_gchandle_free);
    #undef RESOLVE_EXPORT

    if (!m_mono_get_root_domain) {
        LOG_ERR("mono_get_root_domain is null!");
        return false;
    }

    m_RootDomain = m_mono_get_root_domain();
    if (!m_RootDomain) {
        LOG_ERR("mono_get_root_domain returned NULL!");
        return false;
    }

    if (!AttachCurrentThread()) return false;

    if (m_mono_assembly_foreach) {
        m_mono_assembly_foreach(reinterpret_cast<MonoFunc>(AssemblyCallback), this);
    }

    if (!m_AppImage || !m_CoreImage) {
        return false;
    }

    m_Initialized = true;
    LOG_OK("Mono Bridge successfully initialized and attached!");
    return true;
}

bool Bridge::AttachCurrentThread() {
    static thread_local bool s_bAttached = false;
    if (!s_bAttached && m_mono_thread_attach && m_RootDomain) {
        __try {

            if (m_mono_thread_attach(m_RootDomain)) {
                s_bAttached = true;
                LOG_OK("Attached thread (TID: %lu) to Mono runtime", GetCurrentThreadId());
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG_ERR("Failed to attach thread (TID: %lu) to Mono", GetCurrentThreadId());
        }
    }
    return s_bAttached;
}

MonoClass* Bridge::FindClass(MonoImage* image, const char* nameSpace, const char* name) {
    if (!image || !name || !m_mono_class_from_name) return nullptr;
    return m_mono_class_from_name(image, nameSpace ? nameSpace : "", name);
}

MonoMethod* Bridge::FindMethod(MonoClass* klass, const char* name, int paramCount) {
    if (!klass || !name || !m_mono_class_get_method_from_name) return nullptr;
    return m_mono_class_get_method_from_name(klass, name, paramCount);
}

MonoClassField* Bridge::FindField(MonoClass* klass, const char* name) {
    if (!klass || !name || !m_mono_class_get_field_from_name) return nullptr;
    return m_mono_class_get_field_from_name(klass, name);
}

int Bridge::GetFieldOffset(MonoClass* klass, const char* name, int fallback) {
    if (!klass || !name || !m_mono_class_get_field_from_name || !m_mono_field_get_offset) {
        return fallback;
    }
    MonoClassField* field = m_mono_class_get_field_from_name(klass, name);
    if (!field) return fallback;
    int offset = m_mono_field_get_offset(field);
    return (offset > 0) ? offset : fallback;
}

void* Bridge::GetStaticFieldValue(MonoClass* klass, const char* name) {
    if (!klass || !name || !m_RootDomain || !m_mono_class_vtable || !m_mono_field_static_get_value) {
        return nullptr;
    }

    if (!AttachCurrentThread()) return nullptr;

    __try {
        void* vtable = m_mono_class_vtable(m_RootDomain, klass);
        if (!vtable) return nullptr;

        MonoClassField* field = FindField(klass, name);
        if (!field) return nullptr;

        void* val = nullptr;
        m_mono_field_static_get_value(vtable, field, &val);
        return val;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool Bridge::GetStaticBool(MonoClass* klass, const char* name, bool defaultVal) {
    if (!klass || !name || !m_RootDomain || !m_mono_class_vtable || !m_mono_field_static_get_value) {
        return defaultVal;
    }

    if (!AttachCurrentThread()) return defaultVal;

    __try {
        void* vtable = m_mono_class_vtable(m_RootDomain, klass);
        if (!vtable) return defaultVal;

        MonoClassField* field = FindField(klass, name);
        if (!field) return defaultVal;

        bool val = defaultVal;
        m_mono_field_static_get_value(vtable, field, &val);
        return val;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return defaultVal;
    }
}

bool Bridge::TryInvokeMethod(MonoMethod* method, void* obj, void** params) {
    if (!method || !m_mono_runtime_invoke) return false;
    if (!AttachCurrentThread()) return false;
    __try {
        MonoObject* exception = nullptr;
        m_mono_runtime_invoke(method, obj, params, &exception);
        return exception == nullptr;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        LOG_ERR("Managed invocation failed with native exception 0x%08X", GetExceptionCode());
        return false;
    }
}

MonoObject* Bridge::InvokeMethod(MonoMethod* method, void* obj, void** params, MonoObject** exc) {
    if (!method || !m_mono_runtime_invoke) return nullptr;

    if (!AttachCurrentThread()) return nullptr;

    __try {
        return m_mono_runtime_invoke(method, obj, params, exc);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

void* Bridge::SafeUnboxRaw(MonoObject* obj) {
    if (!obj || !m_mono_object_unbox) return nullptr;
    __try {
        return m_mono_object_unbox(obj);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

std::string Bridge::ReadMonoString(MonoString* mstr) {

    uintptr_t address = reinterpret_cast<uintptr_t>(mstr);
    int32_t length = 0;
    constexpr size_t lengthOffset = sizeof(void*) * 2;
    if (!mstr || !Memory::ReadInt(address + lengthOffset, length) || length <= 0 || length > 16384) return "";
    std::wstring characters(static_cast<size_t>(length), L'\0');
    if (!Memory::SafeCopyMemory(characters.data(), reinterpret_cast<void*>(address + lengthOffset + sizeof(int32_t)),
        characters.size() * sizeof(wchar_t))) return "";
    int required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, characters.data(), length, nullptr, 0, nullptr, nullptr);
    if (required <= 0) return "";
    std::string result(static_cast<size_t>(required), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, characters.data(), length, result.data(), required, nullptr, nullptr) != required) return "";
    return result;
}

MonoObject* Bridge::GetReflectionType(MonoClass* klass) {
    if (!klass || !m_mono_class_get_type || !m_mono_type_get_object || !AttachCurrentThread()) return nullptr;
    MonoDomain* domain = m_mono_domain_get ? m_mono_domain_get() : m_RootDomain;
    if (!domain) domain = m_RootDomain;
    __try {
        MonoType* type = m_mono_class_get_type(klass);
        return type ? m_mono_type_get_object(domain, type) : nullptr;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}

MonoString* Bridge::CreateString(const char* text) {
    if (!m_mono_string_new || !text) return nullptr;
    if (!AttachCurrentThread()) return nullptr;
    MonoDomain* domain = m_mono_domain_get ? m_mono_domain_get() : m_RootDomain;
    if (!domain) domain = m_RootDomain;
    __try {
        return m_mono_string_new(domain, text);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

uint32_t Bridge::PinObject(MonoObject* object) {
    if (!object || !m_mono_gchandle_new || !m_mono_gchandle_free) return 0;
    __try { return m_mono_gchandle_new(object, 1); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

void Bridge::FreePin(uint32_t handle) {
    if (!handle || !m_mono_gchandle_free) return;
    __try { m_mono_gchandle_free(handle); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

}
