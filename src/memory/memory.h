#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>
#include <string>
#include <type_traits>

namespace Memory {

bool SafeCopyMemory(void* destination, const void* source, size_t size);

inline bool IsValidAddress(uintptr_t address) {
#if defined(_M_X64) || defined(__x86_64__)
    return address >= 0x10000 && address < 0x00007FFFFFFEFFFFULL;
#else
    return address >= 0x10000 && address < 0xFFFE0000UL;
#endif
}

inline bool IsValidPtr(const void* ptr) {
    return IsValidAddress(reinterpret_cast<uintptr_t>(ptr));
}

template <typename T>
inline bool Read(uintptr_t address, T& outValue) {
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable for safe memory read");
    if (!IsValidAddress(address)) {
        outValue = T{};
        return false;
    }
    return SafeCopyMemory(&outValue, reinterpret_cast<const void*>(address), sizeof(T));
}

template <typename T>
inline T Read(uintptr_t address, const T& defaultValue = T{}) {
    T val = defaultValue;
    Read<T>(address, val);
    return val;
}

inline bool ReadPtr(uintptr_t address, uintptr_t& outValue) {
    return Read<uintptr_t>(address, outValue);
}

inline bool ReadInt(uintptr_t address, int32_t& outValue) {
    return Read<int32_t>(address, outValue);
}

inline bool ReadFloat(uintptr_t address, float& outValue) {
    return Read<float>(address, outValue);
}

inline bool ReadByte(uintptr_t address, uint8_t& outValue) {
    return Read<uint8_t>(address, outValue);
}

template <typename T>
inline bool Write(uintptr_t address, const T& value) {
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable for safe memory write");
    if (!IsValidAddress(address)) return false;
    return SafeCopyMemory(reinterpret_cast<void*>(address), &value, sizeof(T));
}

inline bool WriteFloat(uintptr_t address, float value) { return Write<float>(address, value); }
inline bool WriteInt(uintptr_t address, int32_t value) { return Write<int32_t>(address, value); }
inline bool WriteByte(uintptr_t address, uint8_t value) { return Write<uint8_t>(address, value); }
inline bool WriteBool(uintptr_t address, bool value) { return Write<bool>(address, value); }

std::string ReadString(uintptr_t address, size_t maxLen = 256);

template <typename T = uintptr_t>
bool ReadMonoList(uintptr_t listPtr, std::vector<T>& outList, size_t maxElements = 2000) {
    outList.clear();
    if (!IsValidAddress(listPtr)) return false;

    constexpr size_t ptrSize = sizeof(void*);
    const size_t itemsOffset = ptrSize * 2;
    const size_t countOffset = ptrSize * 2 + ptrSize;
    const size_t arrayElemOffset = ptrSize * 4;

    uintptr_t itemsArray = 0;
    int32_t count = 0;

    if (!ReadPtr(listPtr + itemsOffset, itemsArray)) return false;
    if (!ReadInt(listPtr + countOffset, count)) return false;

    if (!IsValidAddress(itemsArray) || count <= 0 || static_cast<size_t>(count) > maxElements) {
        return false;
    }

    uintptr_t arrayVTable = 0;
    if (!ReadPtr(itemsArray, arrayVTable) || !IsValidAddress(arrayVTable)) {
        return false;
    }

    outList.reserve(count);
    for (int32_t i = 0; i < count; ++i) {
        uintptr_t element = 0;
        uintptr_t elemAddress = itemsArray + arrayElemOffset + (i * sizeof(uintptr_t));
        if (ReadPtr(elemAddress, element) && IsValidAddress(element)) {
            if constexpr (std::is_pointer_v<T>) {
                outList.push_back(reinterpret_cast<T>(element));
            } else {
                outList.push_back(static_cast<T>(element));
            }
        }
    }

    return true;
}

}
