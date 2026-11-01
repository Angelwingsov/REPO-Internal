#pragma once

namespace Features {

class AnimBreaker {
public:
    static AnimBreaker& Get();

    void Update(float dt);
    void Reset();
    float GetHeadAngle() const { return m_HeadAngle; }

private:
    AnimBreaker() = default;

    float m_PoseTimer = 0.0f;
    int m_PoseIndex = 0;
    float m_HeadAngle = 0.0f;
};

}
