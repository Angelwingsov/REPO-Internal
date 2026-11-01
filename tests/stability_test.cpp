#include "core/keybind.h"
#include "core/config.h"
#include "core/hook_activity.h"
#include "core/snapshot.h"
#include "memory/memory.h"
#include "mono/mono_bridge.h"
#include "unity/unity_engine.h"
#include "game/state.h"
#include <atomic>
#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace Core { void Logger::Log(LogLevel, const char*, ...) {} }
namespace Game { GameState& GameState::Get() { static GameState state; return state; } }
namespace {
unsigned pins = 0, frees = 0;
unsigned unmanagedInvocations = 0;
MonoObject* UnexpectedInvoke(MonoMethod*, void*, void**, MonoObject**) { ++unmanagedInvocations; return nullptr; }
uint32_t NewPin(MonoObject* object, int pinned) { assert(object && pinned == 1); return ++pins; }
void FreePin(uint32_t id) { assert(id > 0 && id <= pins); ++frees; }
}

int main() {
    SYSTEM_INFO system{};
    GetSystemInfo(&system);
    size_t pageSize = system.dwPageSize;
    auto* pages = static_cast<unsigned char*>(VirtualAlloc(nullptr, pageSize * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    assert(pages);
    DWORD previous = 0;
    assert(VirtualProtect(pages + pageSize, pageSize, PAGE_NOACCESS, &previous));
    uint64_t value = 123;
    assert(!Memory::Read(reinterpret_cast<uintptr_t>(pages + pageSize - 4), value));
    assert(value == 0);
    assert(!Memory::IsValidRange(UINTPTR_MAX - 2, 8));
    assert(!Memory::IsValidRange(0, 1));

    auto* header = reinterpret_cast<uintptr_t*>(pages + pageSize - sizeof(uintptr_t) * 2);
    header[0] = reinterpret_cast<uintptr_t>(pages);
    header[1] = 0;
    assert(Unity::Engine::IsValidMonoObject(header));
    assert(!Unity::Engine::IsValidUnityObject(header));
    uintptr_t object[] = {reinterpret_cast<uintptr_t>(pages), 0, reinterpret_cast<uintptr_t>(pages)};
    assert(Unity::Engine::IsValidUnityObject(object));
    object[2] = 0;
    assert(!Unity::Engine::IsValidUnityObject(object));
    VirtualFree(pages, 0, MEM_RELEASE);

    auto& bridge = Mono::Bridge::Get();

    bridge.m_mono_runtime_invoke = UnexpectedInvoke;
    assert(!bridge.AttachCurrentThread());
    assert(!bridge.TryInvokeMethod(reinterpret_cast<MonoMethod*>(1), nullptr));
    assert(bridge.InvokeMethod(reinterpret_cast<MonoMethod*>(1), nullptr) == nullptr);
    assert(unmanagedInvocations == 0);
    struct ManagedString { uintptr_t header[2]{}; int32_t length = 0; wchar_t text[64]{}; } string;
    static_assert(offsetof(ManagedString, text) == sizeof(void*) * 2 + 4);
    const wchar_t* text = L"\u041F\u0440\u0438\u0432\u0435\u0442 \xD83D\xDE00";
    string.length = static_cast<int32_t>(wcslen(text));
    std::memcpy(string.text, text, static_cast<size_t>(string.length) * sizeof(wchar_t));
    assert(bridge.ReadMonoString(&string) == u8"Привет 😀");
    string.length = -1; assert(bridge.ReadMonoString(&string).empty());
    string.length = INT32_MAX; assert(bridge.ReadMonoString(&string).empty());
    string.length = 1; string.text[0] = 0xD800; assert(bridge.ReadMonoString(&string).empty());
    assert(bridge.ReadMonoString(nullptr).empty());
    assert(bridge.ReadMonoString(reinterpret_cast<MonoString*>(1)).empty());

    struct Array { uintptr_t header[2]{0x10000, 0}; uintptr_t bounds = 0, length = 3; uintptr_t items[3]{}; } array;
    struct List { uintptr_t header[2]{}; uintptr_t items = 0; int32_t count = 0, version = 0; } list;
    list.items = reinterpret_cast<uintptr_t>(&array);
    list.count = 3;
    array.items[0] = reinterpret_cast<uintptr_t>(&object);
    array.items[1] = 0;
    array.items[2] = reinterpret_cast<uintptr_t>(&string);
    std::vector<uintptr_t> references;
    assert(Memory::ReadMonoList(reinterpret_cast<uintptr_t>(&list), references));
    assert(references.size() == 2 && references[1] == array.items[2]);
    list.count = 4;
    assert(!Memory::ReadMonoList(reinterpret_cast<uintptr_t>(&list), references) && references.empty());
    list.count = -1; assert(!Memory::ReadMonoList(reinterpret_cast<uintptr_t>(&list), references));
    list.count = 0; list.items = 0;
    assert(Memory::ReadMonoList(reinterpret_cast<uintptr_t>(&list), references) && references.empty());

    bridge.m_mono_gchandle_new = NewPin;
    bridge.m_mono_gchandle_free = FreePin;
    {
        Mono::PinnedObject first(&object);
        assert(first);
        Mono::PinnedObject second(std::move(first));
        assert(!first && second);
        first = Mono::PinnedObject(&string);
        second = std::move(first);
        assert(frees == 1 && !first && second);
    }
    assert(pins == 2 && frees == 2);

    bool toggle = false;
    assert(!Core::EvaluateKeybind(0, false, false, toggle));
    assert(Core::EvaluateKeybind(0, true, false, toggle));
    assert(Core::EvaluateKeybind(1, true, false, toggle));
    assert(Core::EvaluateKeybind(1, true, true, toggle));
    assert(Core::EvaluateKeybind(1, false, true, toggle));
    assert(!Core::EvaluateKeybind(1, true, false, toggle));
    assert(Core::EvaluateKeybind(2, false, false, toggle));
    assert(Core::EvaluateKeybind(2, true, false, toggle));
    assert(Core::EvaluateKeybind(3, false, true, toggle));
    assert(!Core::EvaluateKeybind(3, true, false, toggle));

    auto& global = Core::Config::Global();
    global.fullbright.brightness = 0.7f;
    auto isolated = Core::Config::Copy();
    isolated.fullbright.brightness = 2.0f;
    try {
        Core::Config::Scope scope(isolated);
        assert(Core::Config::Get().fullbright.brightness == 2.0f);
        std::thread reader([] { assert(Core::Config::Copy().fullbright.brightness == 0.7f); });
        reader.join();
        throw std::runtime_error("unwind scoped configuration");
    } catch (const std::runtime_error&) {}
    assert(Core::Config::Get().fullbright.brightness == 0.7f);

    Core::Snapshot<std::vector<int>> snapshot;
    std::atomic<bool> done{false};
    std::thread writer([&] {
        for (int iteration = 1; iteration < 10000; ++iteration)
            snapshot.Publish(std::vector<int>(32, iteration));
        done = true;
    });
    do {
        auto frame = snapshot.Read();
        for (int element : frame) assert(element == frame.front());
    } while (!done);
    writer.join();

    std::atomic<bool> entered{false};
    std::thread callback([&] {
        Core::HookActivity activity;
        entered = true;
        Sleep(30);
    });
    while (!entered) std::this_thread::yield();
    assert(Core::HookActivity::WaitForIdle());
    callback.join();
    std::cout << "PASS: inaccessible/partial reads, destroyed Unity objects, UTF-16 names, array bounds, GC handle ownership, concurrent snapshots, scoped config, hook draining\n";
}
