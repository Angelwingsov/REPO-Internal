#pragma once
#include "platform.h"
#include "framework.h"
#include <memory>
#include <map>

#define get_a(col) (((col)&0xff000000)>>24)
#define get_r(col) (((col)&0x00ff0000)>>16)
#define get_g(col) (((col)&0x0000ff00)>>8)
#define get_b(col) ((col)&0x000000ff)

enum circle_type { FULL, HALF, QUARTER };
enum text_alignment { LEFT, CENTER, RIGHT };

namespace Render
{
    using IdaLovesMe::Vec2;

    namespace Fonts
    {
        extern LPD3DXFONT TabIcons;
        extern LPD3DXFONT LegitTabIcons;
        extern LPD3DXFONT Verdana;
        extern LPD3DXFONT Tahombd;
        extern LPD3DXFONT SmallFont;
    }

    class CDraw
    {
    public:
        struct sScreen
        {
            float Width;
            float Height;
            float x_center;
            float y_center;
        } Screen;

        void Sprite(ID3D11ShaderResourceView* Texture, Vec2 Pos, Vec2 Size, D3DCOLOR Color);

        void Line(Vec2 Pos, Vec2 Pos2, D3DCOLOR Color);

        void Rect(Vec2 Pos, Vec2 Size, float linewidth, D3DCOLOR Color, bool Antialias = false);
        void FilledRect(Vec2 Pos, Vec2 Size, D3DCOLOR color, bool Antialias = false);
        void BorderedRect(Vec2 Pos, Vec2 Size, float BorderWidth, D3DCOLOR Color, D3DCOLOR BorderColor);
        void Gradient(Vec2 Pos, Vec2 Size, D3DCOLOR LColor, D3DCOLOR ROtherColor, bool Vertical = false, bool Antialias = false);
        void Triangle(Vec2 Top, Vec2 Left, Vec2 Right, D3DCOLOR Color, bool antialias = false);
        void Text(const char* Text, float X, float Y, int Orientation, LPD3DXFONT Font, bool Bordered, D3DCOLOR Color, Vec2 MaxSize = Vec2(0, 0), bool ncl = false);

        void Init(ID3D11Device* D3dDevice, HWND window);
        void CreateObjects();
        void ReleaseObjects();
        void Reset();

        ID3D11ShaderResourceView* GetBgTexture();
        Vec2   GetTextSize(ImFont* Font, const char* Text);
        ID3D11Device*   GetD3dDevice();
        HWND GetWindow();

    private:
        ID3D11Device*       m_Device;
        HWND m_Window;

        ID3D11ShaderResourceView* m_BgTexture;

        HANDLE                  m_TabFont;
        HANDLE                  m_LegitTabFont;

        bool Initialized = false;
    };

    extern std::unique_ptr<CDraw> Draw;
}
