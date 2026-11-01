#pragma once
#include <cstdint>

typedef void MonoDomain;
typedef void MonoAssembly;
typedef void MonoImage;
typedef void MonoClass;
typedef void MonoObject;
typedef void MonoMethod;
typedef void MonoClassField;
typedef void MonoThread;
typedef void MonoString;
typedef void MonoType;
typedef void MonoArray;
typedef void (*MonoFunc)(void* data, void* user_data);

typedef MonoThread* (*mono_thread_attach_t)(MonoDomain* domain);
typedef MonoThread* (*mono_thread_current_t)();
typedef MonoDomain* (*mono_get_root_domain_t)();
typedef MonoDomain* (*mono_domain_get_t)();
typedef MonoImage* (*mono_assembly_get_image_t)(MonoAssembly* assembly);
typedef void (*mono_assembly_foreach_t)(MonoFunc func, void* user_data);
typedef const char* (*mono_image_get_name_t)(MonoImage* image);
typedef MonoClass* (*mono_class_from_name_t)(MonoImage* image, const char* name_space, const char* name);
typedef MonoClass* (*mono_class_from_name_case_t)(MonoImage* image, const char* name_space, const char* name);
typedef MonoClassField* (*mono_class_get_field_from_name_t)(MonoClass* klass, const char* name);
typedef void (*mono_field_get_value_t)(MonoObject* obj, MonoClassField* field, void* value);
typedef void (*mono_field_static_get_value_t)(void* vtable, MonoClassField* field, void* value);
typedef void* (*mono_class_vtable_t)(MonoDomain* domain, MonoClass* klass);
typedef void (*mono_runtime_class_init_t)(void* vtable);
typedef void* (*mono_vtable_get_static_field_data_t)(void* vtable);
typedef int (*mono_field_get_offset_t)(MonoClassField* field);
typedef MonoMethod* (*mono_class_get_method_from_name_t)(MonoClass* klass, const char* name, int param_count);
typedef MonoObject* (*mono_runtime_invoke_t)(MonoMethod* method, void* obj, void** params, MonoObject** exc);
typedef void* (*mono_object_unbox_t)(MonoObject* obj);
typedef char* (*mono_string_to_utf8_t)(MonoString* string_obj);
typedef MonoString* (*mono_string_new_t)(MonoDomain* domain, const char* text);
typedef void (*mono_free_t)(void* ptr);
typedef uint32_t (*mono_gchandle_new_t)(MonoObject* object, int pinned);
typedef void (*mono_gchandle_free_t)(uint32_t handle);

typedef MonoType* (*mono_class_get_type_t)(MonoClass* klass);
typedef MonoObject* (*mono_type_get_object_t)(MonoDomain* domain, MonoType* type);
