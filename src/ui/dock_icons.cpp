#include "common/pch.h"
#include "dock_icons.h"
#include <cmath>

namespace kv {
namespace {
// UI-local geometry writer; the caller owns its sink and closes each path before publishing cached artwork.
class GlyphWriter {
public:
    // Borrows an open sink for a single outline or fill geometry; does not retain it beyond construction.
    explicit GlyphWriter(ID2D1GeometrySink* sink) : m_sink(sink) { sink->SetFillMode(D2D1_FILL_MODE_WINDING); }
    // Starts/extends/ends independent subpaths; round stroke caps and joins are applied at draw time.
    void Begin(float x, float y) { m_sink->BeginFigure(D2D1::Point2F(x,y), D2D1_FIGURE_BEGIN_FILLED); }
    void Line(float x, float y) { m_sink->AddLine(D2D1::Point2F(x,y)); }
    void Curve(float x1,float y1,float x2,float y2,float x,float y) { m_sink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(x1,y1),D2D1::Point2F(x2,y2),D2D1::Point2F(x,y))); }
    void End(bool closed = false) { m_sink->EndFigure(closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN); }
    void Segment(float x,float y,float u,float v) { Begin(x,y); Line(u,v); End(); }
    // Emits a gently rounded frame in reference coordinates, including cubic corners rather than square joins.
    void Box(float x,float y,float w,float h,float r = 0.8f) {
        constexpr float k = 0.55228475f;
        Begin(x+r,y); Line(x+w-r,y); Curve(x+w-r+k*r,y,x+w,y+r-k*r,x+w,y+r);
        Line(x+w,y+h-r); Curve(x+w,y+h-r+k*r,x+w-r+k*r,y+h,x+w-r,y+h);
        Line(x+r,y+h); Curve(x+r-k*r,y+h,x,y+h-r+k*r,x,y+h-r);
        Line(x,y+r); Curve(x,y+r-k*r,x+r-k*r,y,x+r,y); End(true);
    }
    // Emits an original photo sun/hole contour or a filled triangular mountain in the same coordinate space.
    void Circle(float x,float y,float r) {
        constexpr float k = 0.55228475f;
        Begin(x+r,y); Curve(x+r,y+k*r,x+k*r,y+r,x,y+r); Curve(x-k*r,y+r,x-r,y+k*r,x-r,y);
        Curve(x-r,y-k*r,x-k*r,y-r,x,y-r); Curve(x+k*r,y-r,x+r,y-k*r,x+r,y); End(true);
    }
    void Triangle(float x,float y,float w,float h) { Begin(x,y+h); Line(x+w/2,y); Line(x+w,y+h); End(true); }
private:
    ID2D1GeometrySink* m_sink;
};

// Builds one reference glyph's strokes and solid photo details; no raster image or third-party icon is embedded.
void WriteGlyph(int index, GlyphWriter& outline, GlyphWriter& fill) {
    auto& o = outline; auto& f = fill;
    if (index == 0) {
        o.Box(2,10,8,20); o.Box(12,3,8,22); o.Box(22,11,8,19);
        o.Segment(2,20,10,20); o.Segment(12,14,20,14); o.Segment(22,21,30,21);
        f.Triangle(3.4f,15,4.5f,3.5f); f.Circle(7.5f,14.2f,0.7f);
        f.Triangle(13.4f,8,4.5f,3.5f); f.Circle(17.5f,7.2f,0.7f);
        f.Triangle(13.4f,20,4.5f,3.5f);
        f.Triangle(23.4f,16,4.5f,3.5f); f.Circle(27.5f,15.2f,0.7f);
    } else if (index == 1) {
        o.Box(2,4,18,13,1); o.Box(23,11,8,10,0.8f); o.Box(7,22,14,9,0.8f);
        f.Triangle(5,9,6,5); f.Circle(14,8.5f,1.1f);
        f.Triangle(24.2f,16,4,3); f.Circle(28,14.5f,0.7f);
        f.Triangle(9,25,5,3.5f); f.Circle(17,24.8f,0.8f);
    } else if (index == 2) {
        o.Begin(5,12); o.Curve(8,4,15,2,21,6); o.Curve(25,8,26,10,27,15); o.End();
        o.Begin(23,11); o.Line(27,15); o.Line(30,10); o.End();
        o.Box(2.5f,18,11.5f,11.5f,1.2f);
    } else if (index == 3) {
        o.Begin(2,16); o.Line(13,5); o.Curve(13.5f,4.5f,14,4.5f,15,4.5f);
        o.Line(20,4.5f); o.Curve(21,4.5f,21.5f,5,21.5f,6);
        o.Line(21.5f,13); o.Curve(21.5f,14,21,14.5f,20.5f,15);
        o.Line(10,26); o.Curve(9,27,8,27,7,26); o.Line(2,21); o.Curve(0.7f,19.7f,0.7f,17.3f,2,16); o.End(true);
        o.Circle(16.7f,9.4f,1.7f);
        o.Begin(24,11); o.Line(25.5f,11); o.Curve(26.2f,11,26.5f,11.5f,27,12);
        o.Line(30.5f,16.5f); o.Curve(31.3f,17.5f,31.3f,18.5f,30.5f,19.3f);
        o.Line(21,29); o.Curve(20,30,19,30,18,29); o.Line(14,25); o.End();
        o.Circle(25.6f,17.1f,1.6f);
    } else if (index == 4) {
        o.Segment(2,6,23,6); o.Segment(2,12,18,12); o.Segment(2,18,12,18);
        f.Begin(3.5f,24); f.Line(8.5f,24); f.Line(6,27); f.End(true);
        o.Begin(21,17); o.Line(31,17); o.Line(27,23); o.Line(27,29); o.Line(24,27.5f); o.Line(24,23); o.End(true);
    } else {
        // Six connected teeth, matching the reference silhouette rather than detached radial spokes.
        for (int tooth=0; tooth<6; ++tooth) {
            for (int corner=0; corner<4; ++corner) {
                const float degrees = -90.0f + tooth*60.0f + (corner==0 ? -23.0f : corner==1 ? -8.0f : corner==2 ? 8.0f : 23.0f);
                const float radius = corner==1 || corner==2 ? 14.0f : 10.0f;
                const float angle=degrees*3.14159265358979323846f/180.0f;
                const float x=16+radius*std::cos(angle), y=16+radius*std::sin(angle);
                if (tooth==0 && corner==0) o.Begin(x,y); else o.Line(x,y);
            }
        }
        o.End(true); o.Circle(16,16,4.4f);
    }
}
} // namespace

bool DockIcons::EnsureResources(ID2D1Factory* factory) {
    if (m_factory.Get() == factory && m_stroke) return true;
    m_factory = factory; m_stroke.Reset(); m_outlines = {}; m_fills = {};
    if (!factory) return false;
    const auto properties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND,D2D1_CAP_STYLE_ROUND,D2D1_CAP_STYLE_ROUND,D2D1_LINE_JOIN_ROUND);
    if (FAILED(factory->CreateStrokeStyle(properties,nullptr,0,&m_stroke))) return false;
    for (int index=0;index<6;++index) {
        ComPtr<ID2D1GeometrySink> outlineSink, fillSink;
        if (FAILED(factory->CreatePathGeometry(&m_outlines[index])) || FAILED(factory->CreatePathGeometry(&m_fills[index])) ||
            FAILED(m_outlines[index]->Open(&outlineSink)) || FAILED(m_fills[index]->Open(&fillSink))) { m_stroke.Reset(); return false; }
        GlyphWriter outline(outlineSink.Get()), fill(fillSink.Get());
        WriteGlyph(index,outline,fill);
        if (FAILED(outlineSink->Close()) || FAILED(fillSink->Close())) { m_stroke.Reset(); return false; }
    }
    return true;
}

bool DockIcons::Draw(Renderer& renderer, int index, const RectF& bounds, const Color& color) {
    auto* context = renderer.GetD2DContext();
    if (!context || index < 0 || index >= 6 || bounds.Width() <= 0 || bounds.Height() <= 0) return false;
    ComPtr<ID2D1Factory> factory; context->GetFactory(&factory);
    if (!EnsureResources(factory.Get())) return false;
    auto* brush = renderer.GetBrush(color);
    if (!brush) return false;
    D2D1_MATRIX_3X2_F previous; context->GetTransform(&previous);
    const auto transform = D2D1::Matrix3x2F::Scale(bounds.Width()/32,bounds.Height()/32) *
        D2D1::Matrix3x2F::Translation(bounds.left,bounds.top) * previous;
    context->SetTransform(transform);
    context->DrawGeometry(m_outlines[index].Get(),brush,1.65f,m_stroke.Get());
    context->FillGeometry(m_fills[index].Get(),brush);
    context->SetTransform(previous);
    return true;
}
} // namespace kv
