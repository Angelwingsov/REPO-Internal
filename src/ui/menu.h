#pragma once
#include <Windows.h>
struct ID3D11Device;

namespace UI {

class Menu {
public:
    static Menu& Get();

    void Initialize(ID3D11Device* device, HWND window);
    void Shutdown();
    void Render();

private:
    Menu() = default;
};

}
