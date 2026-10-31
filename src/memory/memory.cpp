#include "memory.h"

namespace Memory {

bool SafeCopyMemory(void* destination, const void* source, size_t size) {
    if (!destination || !source || size == 0) {
        return false;
    }

    __try {
        const uint8_t* srcBytes = static_cast<const uint8_t*>(source);
        uint8_t* dstBytes = static_cast<uint8_t*>(destination);
        for (size_t i = 0; i < size; ++i) {
            dstBytes[i] = srcBytes[i];
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

std::string ReadString(uintptr_t address, size_t maxLen) {
    if (!IsValidAddress(address) || maxLen == 0) {
        return "";
    }

    std::string result;
    result.reserve(maxLen > 64 ? 64 : maxLen);

    for (size_t i = 0; i < maxLen; ++i) {
        char ch = 0;
        if (!Read<char>(address + i, ch) || ch == '\0') {
            break;
        }
        result.push_back(ch);
    }

    return result;
}

}
