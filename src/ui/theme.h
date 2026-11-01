#pragma once
struct ID3D11Device;
struct ImFont;
struct ID3D11ShaderResourceView;

namespace UI {

class Theme {
public:
    static void Apply();
    static void InitializeAssets(ID3D11Device* device);
    static void ShutdownAssets();
    static ImFont* TextFont();
    static ImFont* IconFont();
    static ImFont* BoldFont();
    static ImFont* SmallFont();
    static ImFont* LegitIconFont();
    static ID3D11ShaderResourceView* Background();
};

}
