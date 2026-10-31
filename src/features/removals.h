#pragma once

namespace Features {
class Removals {
public:
    static Removals& Get();
    void Initialize();
    bool IsLightingReady() const { return m_LightingReady; }
    bool IsPostProcessingReady() const { return m_PostProcessingReady; }
    bool RestoreBeforeUnload();
private:
    bool m_LightingReady = false;
    bool m_PostProcessingReady = false;
};
}
