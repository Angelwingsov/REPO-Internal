#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "mono_types.h"
#include "core/logger.h"
#include "memory/memory.h"

namespace Mono {

class Bridge {
public:
    static Bridge& Get();

    bool Initialize();
    bool IsReady() const { return m_Initialized && m_AppImage != nullptr && m_CoreImage != nullptr; }
    void AttachCurrentThread();

    MonoImage* GetAppImage() const { return m_AppImage; }
    MonoImage* GetCoreImage() const { return m_CoreImage; }
    MonoImage* GetPhysicsImage() const { return m_PhysicsImage; }

    MonoClass* FindClass(MonoImage* image, const char* nameSpace, const char* name);
    MonoMethod* FindMethod(MonoClass* klass, const char* name, int paramCount = 0);
    MonoClassField* FindField(MonoClass* klass, const char* name);
    int GetFieldOffset(MonoClass* klass, const char* name, int fallback = -1);

    void* GetStaticFieldValue(MonoClass* klass, const char* name);
    bool GetStaticBool(MonoClass* klass, const char* name, bool defaultVal = false);

    MonoObject* InvokeMethod(MonoMethod* method, void* obj, void** params = nullptr, MonoObject** exc = nullptr);
    void* SafeUnboxRaw(MonoObject* obj);

    template <typename T>
    bool Unbox(MonoObject* obj, T& outVal) {
        void* raw = SafeUnboxRaw(obj);
        if (!raw) return false;
        return Memory::Read<T>(reinterpret_cast<uintptr_t>(raw), outVal);
    }

    std::string ReadMonoString(MonoString* mstr);
    MonoString* CreateString(const char* text);

    mono_thread_attach_t m_mono_thread_attach = nullptr;
    mono_thread_current_t m_mono_thread_current = nullptr;
    mono_get_root_domain_t m_mono_get_root_domain = nullptr;
    mono_domain_get_t m_mono_domain_get = nullptr;
    mono_assembly_get_image_t m_mono_assembly_get_image = nullptr;
    mono_assembly_foreach_t m_mono_assembly_foreach = nullptr;
    mono_image_get_name_t m_mono_image_get_name = nullptr;
    mono_class_from_name_t m_mono_class_from_name = nullptr;
    mono_class_from_name_case_t m_mono_class_from_name_case = nullptr;
    mono_class_get_field_from_name_t m_mono_class_get_field_from_name = nullptr;
    mono_field_get_value_t m_mono_field_get_value = nullptr;
    mono_field_static_get_value_t m_mono_field_static_get_value = nullptr;
    mono_class_vtable_t m_mono_class_vtable = nullptr;
    mono_runtime_class_init_t m_mono_runtime_class_init = nullptr;
    mono_vtable_get_static_field_data_t m_mono_vtable_get_static_field_data = nullptr;
    mono_field_get_offset_t m_mono_field_get_offset = nullptr;
    mono_class_get_method_from_name_t m_mono_class_get_method_from_name = nullptr;
    mono_runtime_invoke_t m_mono_runtime_invoke = nullptr;
    mono_object_unbox_t m_mono_object_unbox = nullptr;
    mono_string_to_utf8_t m_mono_string_to_utf8 = nullptr;
    mono_string_new_t m_mono_string_new = nullptr;
    mono_free_t m_mono_free = nullptr;

private:
    Bridge() = default;
    static void AssemblyCallback(void* item, void* userData);

    bool m_Initialized = false;
    HMODULE m_hMono = nullptr;
    MonoDomain* m_RootDomain = nullptr;
    MonoImage* m_AppImage = nullptr;
    MonoImage* m_CoreImage = nullptr;
    MonoImage* m_PhysicsImage = nullptr;
};

}
