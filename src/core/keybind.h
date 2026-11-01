#pragma once

namespace Core {

inline bool EvaluateKeybind(int mode, bool down, bool previousDown, bool& toggled) {
    if (mode == 0) return down;
    if (mode == 2) return true;
    if (mode == 3) return !down;
    if (down && !previousDown) toggled = !toggled;
    return toggled;
}
}
