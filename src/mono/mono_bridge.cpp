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
    if (name.find("Assembly-CSharp") != std::string::npos && !bridge->m_AppImage) {
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

    AttachCurrentThread();

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

void Bridge::AttachCurrentThread() {
    static thread_local bool s_bAttached = false;
    if (!s_bAttached && m_mono_thread_attach && m_RootDomain) {
        __try {
            m_mono_thread_attach(m_RootDomain);
            s_bAttached = true;
            LOG_OK("Attached thread (TID: %lu) to Mono runtime", GetCurrentThreadId());
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG_ERR("Failed to attach thread (TID: %lu) to Mono", GetCurrentThreadId());
        }
    }
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

    AttachCurrentThread();

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

    AttachCurrentThread();

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

MonoObject* Bridge::InvokeMethod(MonoMethod* method, void* obj, void** params, MonoObject** exc) {
    if (!method || !m_mono_runtime_invoke) return nullptr;

    AttachCurrentThread();

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

static char* SafeStringToUtf8Helper(mono_string_to_utf8_t fn, MonoString* mstr) {
    if (!fn || !mstr) return nullptr;
    __try {
        return fn(mstr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

std::string Bridge::ReadMonoString(MonoString* mstr) {
    char* utf8Str = SafeStringToUtf8Helper(m_mono_string_to_utf8, mstr);
    if (!utf8Str) return "";

    std::string result(utf8Str);
    if (m_mono_free) {
        m_mono_free(utf8Str);
    }
    return result;
}

MonoString* Bridge::CreateString(const char* text) {
    if (!m_mono_string_new || !text) return nullptr;
    AttachCurrentThread();
    MonoDomain* domain = m_mono_domain_get ? m_mono_domain_get() : m_RootDomain;
    if (!domain) domain = m_RootDomain;
    __try {
        return m_mono_string_new(domain, text);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

}
