#pragma once

namespace UI {

class Menu {
public:
    static Menu& Get();

    void Render();

private:
    Menu() = default;
};

}
