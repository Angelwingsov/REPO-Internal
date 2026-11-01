#pragma once
#include "unity/unity_math.h"
#include <algorithm>
#include <limits>

namespace Game {
class PlayerBoundsAccumulator {
public:
    void Add(const Unity::Bounds& part) {
        if (!part.IsValid()) return;
        auto low=part.center-part.extents, high=part.center+part.extents;
        if (!m_HasBounds) {m_Min=low;m_Max=high;m_HasBounds=true;return;}
        m_Min={(std::min)(m_Min.x,low.x),(std::min)(m_Min.y,low.y),(std::min)(m_Min.z,low.z)};
        m_Max={(std::max)(m_Max.x,high.x),(std::max)(m_Max.y,high.y),(std::max)(m_Max.z,high.z)};
    }
    bool Get(Unity::Bounds& result) const {
        if (!m_HasBounds) return false;
        Unity::Bounds value{(m_Min+m_Max)*.5f,(m_Max-m_Min)*.5f};
        if (!value.IsValid()) return false;
        result=value;return true;
    }
private:
    Unity::Vector3 m_Min,m_Max;
    bool m_HasBounds=false;
};

inline bool FitPlayerBoundsToHead(const Unity::Bounds& collider,const Unity::Vector3& head,Unity::Bounds& result) {
    if (!collider.IsValid() || !head.IsValid() || collider.center.Distance(head)>5) return false;
    auto low=collider.center-collider.extents, high=collider.center+collider.extents;

    high.y=head.y+.03f;
    low.y=(std::min)(low.y,head.y-.25f);
    low.x=(std::min)(low.x,head.x-.15f);high.x=(std::max)(high.x,head.x+.15f);
    low.z=(std::min)(low.z,head.z-.15f);high.z=(std::max)(high.z,head.z+.15f);
    Unity::Bounds value{(low+high)*.5f,(high-low)*.5f};
    if (!value.IsValid()) return false;
    result=value;return true;
}

struct ScreenBounds {
    Unity::Vector3 min, max;
};

inline Unity::Bounds PlayerFallbackBounds(const Unity::Vector3& feet) {

    return {feet + Unity::Vector3(0, .9f, 0), Unity::Vector3(.35f, .9f, .35f)};
}

template<class Project>
bool ProjectPlayerBounds(const Unity::Bounds& bounds, Project project, ScreenBounds& out) {
    if (!bounds.IsValid()) return false;
    ScreenBounds result;
    result.min = {(std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), 0};
    result.max = {-(std::numeric_limits<float>::max)(), -(std::numeric_limits<float>::max)(), 0};
    for (int i = 0; i < 8; ++i) {
        Unity::Vector3 corner = bounds.center + Unity::Vector3(
            (i & 1) ? bounds.extents.x : -bounds.extents.x,
            (i & 2) ? bounds.extents.y : -bounds.extents.y,
            (i & 4) ? bounds.extents.z : -bounds.extents.z);
        Unity::Vector3 screen;
        if (!project(corner, screen) || !std::isfinite(screen.x) || !std::isfinite(screen.y)) return false;
        result.min.x = (std::min)(result.min.x, screen.x);
        result.min.y = (std::min)(result.min.y, screen.y);
        result.max.x = (std::max)(result.max.x, screen.x);
        result.max.y = (std::max)(result.max.y, screen.y);
    }
    if (result.max.x <= result.min.x || result.max.y <= result.min.y) return false;
    out = result;
    return true;
}
}
