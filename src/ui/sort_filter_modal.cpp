#include "common/pch.h"
#include "sort_filter_modal.h"
#include "core/app.h"

namespace kv {
void SortFilterModal::Show() {
    m_mode=m_app->m_settings.sort_mode; m_mask=m_app->m_settings.image_filter_mask;
    m_focus=0; m_hover=-1; m_visible=true; m_app->m_window.RequestRedraw();
}
void SortFilterModal::Hide() { m_visible=false; m_app->m_window.RequestRedraw(); }
RectF SortFilterModal::Bounds() const {
    const auto size=m_app->m_renderer.GetTargetSize().ToFloat();
    const float scale=std::max(0.1f,std::min({m_app->m_window.GetDpiScale(),(size.width-16)/560,(size.height-16)/470}));
    return RectF::FromXYWH((size.width-560*scale)/2,(size.height-470*scale)/2,560*scale,470*scale);
}
RectF SortFilterModal::Control(int index) const {
    const auto box=Bounds(); const float s=box.Width()/560;
    float x=24,y=0,w=512;
    if (index<3) { x+=index*172; y=82; w=164; }
    else if (index<5) { x+=(index-3)*260; y=128; w=252; }
    else if (index==5) y=218;
    else if (index<15) { x+=((index-6)%2)*260; y=254+((index-6)/2)*30.0f; w=252; }
    else { x+=(index-15)*260; y=422; w=252; }
    return RectF::FromXYWH(box.left+x*s,box.top+y*s,w*s,(index>=5 && index<15 ? 27 : 34)*s);
}
void SortFilterModal::Activate(int index) {
    if (index<3) m_mode=index*2+m_mode%2;
    else if (index<5) m_mode=m_mode/2*2+(index-3);
    else if (index==5) m_mask=m_mask==511 ? 0 : 511;
    else if (index<15) m_mask^=1<<(index-6);
    else if (index==15) { m_app->ApplyFileView(m_mode,m_mask); Hide(); }
    else Hide();
    m_app->m_window.RequestRedraw();
}
bool SortFilterModal::OnMouseMove(float x,float y) {
    if (!m_visible) return false;
    int hovered=-1;
    for (int i=0;i<17;++i) if (Control(i).Contains(x,y)) hovered=i;
    if (hovered!=m_hover) { m_hover=hovered; m_app->m_window.RequestRedraw(); }
    return true;
}
bool SortFilterModal::OnMouseDown(MouseButton button,float x,float y) {
    if (!m_visible) return false;
    if (button==MouseButton::Left) for (int i=0;i<17;++i) if (Control(i).Contains(x,y)) { m_focus=i; Activate(i); break; }
    return true;
}
bool SortFilterModal::OnKeyDown(uint32_t key,KeyModifiers mods) {
    if (!m_visible) return false;
    if (key==VK_ESCAPE) Hide();
    else if (key==VK_TAB || key==VK_RIGHT || key==VK_DOWN) m_focus=(m_focus+(mods.shift?16:1))%17;
    else if (key==VK_LEFT || key==VK_UP) m_focus=(m_focus+16)%17;
    else if (key==VK_RETURN || key==VK_SPACE) Activate(m_focus);
    m_app->m_window.RequestRedraw(); return true;
}
void SortFilterModal::Draw(Renderer& renderer) {
    if (!m_visible) return;
    const auto box=Bounds(); const float s=box.Width()/560;
    const auto size=renderer.GetTargetSize().ToFloat();
    renderer.FillRect(RectF::FromXYWH(0,0,size.width,size.height),Color::Black(0.6f));
    renderer.FillRoundedRect(box,8*s,8*s,Color::Black(m_app->m_settings.settings_window_opacity));
    renderer.DrawRoundedRect(box,8*s,8*s,Color::White(0.15f),1.5f*s);
    renderer.DrawText(_(L"Sort & Filter"),RectF::FromXYWH(box.left+24*s,box.top+18*s,512*s,40*s),Color::White(),24*s);
    renderer.DrawText(_(L"Sort by"),RectF::FromXYWH(box.left+24*s,box.top+58*s,512*s,24*s),Color::White(),13*s);
    renderer.DrawText(_(L"Image types"),RectF::FromXYWH(box.left+24*s,box.top+184*s,512*s,28*s),Color::White(),16*s);
    const wchar_t* names[]={L"Name",L"Modified date",L"File size",L"Ascending",L"Descending",L"All",L"JPEG / JFIF",L"PNG / APNG",L"GIF",L"BMP",L"TIFF",L"WebP",L"AVIF",L"HEIF / HEIC",L"Others",L"Apply",L"Cancel"};
    for (int i=0;i<17;++i) {
        const auto r=Control(i);
        const bool checked=i<3 ? m_mode/2==i : i<5 ? m_mode%2==i-3 : i==5 ? m_mask==511 : i<15 ? (m_mask&(1<<(i-6)))!=0 : false;
        renderer.FillRoundedRect(r,4*s,4*s,Color::White((m_hover==i || m_focus==i)?0.16f:0.06f));
        if (m_focus==i) renderer.DrawRoundedRect(r,4*s,4*s,Color::FromHex(0x53deee),s);
        auto text=r;
        if (i<15) {
            auto mark=RectF::FromXYWH(r.left+8*s,r.Center().y-6*s,12*s,12*s);
            renderer.DrawRect(mark,Color::White(0.6f),s);
            if (checked) renderer.FillRect(mark.Inset(2*s),Color::FromHex(0x53deee));
            text.left+=28*s;
        }
        renderer.DrawText(_(names[i]),text,Color::White(),13*s,i<15?TextAlign::Left:TextAlign::Center,TextVAlign::Center);
    }
}
}
