#pragma once
#include <mutex>
#include <utility>

namespace Core {
// The render thread receives values only; managed objects never cross this boundary.
template<typename T> class Snapshot {
public:
    T Read() const { std::lock_guard<std::mutex> lock(m_Mutex); return m_Value; }
    void Publish(T value) { std::lock_guard<std::mutex> lock(m_Mutex); m_Value = std::move(value); }
private:
    mutable std::mutex m_Mutex;
    T m_Value{};
};
}
