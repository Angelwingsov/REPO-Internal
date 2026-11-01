#pragma once

namespace Features {
class Removals {
public:
    static Removals& Get();
    void Initialize();
    bool IsLightingReady() const { return m_LightingReady; }
    bool IsPostProcessingReady() const { return m_PostProcessingReady; }
    bool RestoreBeforeUnload();
    void PollRestore(); // Called by the central game tick even if scene effect components disappeared.
private:
    bool m_LightingReady = false;
    bool m_PostProcessingReady = false;
};
}
