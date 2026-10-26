#pragma once
#include <oleacc.h>
#include <atomic>
#include <functional>
#include <memory>

namespace kv {
// UI-apartment callbacks borrow App only while its shared lifetime state remains alive.
struct DockAccessibleState {
    HWND hwnd = nullptr;
    std::function<bool()> visible;
    std::function<bool(int)> disabled;
    std::function<RECT(int)> rect;
    std::function<void(int)> invoke;
};

// MSAA client object exposes the custom drawn dock's named actions without owning App or GPU resources.
// COM calls are delivered in the UI apartment; an expired owner returns disconnected rather than dereferencing App.
class DockAccessibility final : public IAccessible {
public:
    // Retains only a weak UI-owner lifetime; external accessibility clients may outlive the application.
    explicit DockAccessibility(const std::shared_ptr<DockAccessibleState>& state) : m_state(state) {}
    // IUnknown lifetime and interface discovery for the native accessibility bridge.
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** value) override {
        if (!value) return E_POINTER;
        *value = nullptr;
        if (iid != IID_IUnknown && iid != IID_IDispatch && iid != IID_IAccessible) return E_NOINTERFACE;
        *value = static_cast<IAccessible*>(this); AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refs; }
    ULONG STDMETHODCALLTYPE Release() override { const auto refs = --m_refs; if (!refs) delete this; return refs; }
    // The bridge uses IAccessible directly; no dynamic type library or scripting dispatch is exposed.
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* n) override { if (!n) return E_POINTER; *n=0; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT, LCID, ITypeInfo**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID,LPOLESTR*,UINT,LCID,DISPID*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Invoke(DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*) override { return E_NOTIMPL; }
    // Returns six simple child IDs; geometry and enablement are queried from current UI state.
    HRESULT STDMETHODCALLTYPE get_accParent(IDispatch** value) override { if (!value) return E_POINTER; *value=nullptr; return S_FALSE; }
    HRESULT STDMETHODCALLTYPE get_accChildCount(long* count) override { if (!count) return E_POINTER; *count=6; return S_OK; }
    HRESULT STDMETHODCALLTYPE get_accChild(VARIANT,IDispatch** value) override { if (!value) return E_POINTER; *value=nullptr; return S_FALSE; }
    HRESULT STDMETHODCALLTYPE get_accName(VARIANT child,BSTR* value) override {
        if (!value) return E_POINTER; *value=nullptr;
        if (!Valid(child)) return E_INVALIDARG;
        const wchar_t* names[] = {L"Viewer actions",L"Waterfall",L"Board",L"Rotate 90 degrees",L"Tags",L"Sort & Filter (coming later)",L"Settings"};
        *value=SysAllocString(names[child.lVal]); return *value ? S_OK : E_OUTOFMEMORY;
    }
    HRESULT STDMETHODCALLTYPE get_accRole(VARIANT child,VARIANT* value) override {
        if (!value) return E_POINTER; if (!Valid(child)) return E_INVALIDARG;
        VariantInit(value); value->vt=VT_I4; value->lVal=child.lVal ? ROLE_SYSTEM_PUSHBUTTON : ROLE_SYSTEM_TOOLBAR; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE get_accState(VARIANT child,VARIANT* value) override {
        if (!value) return E_POINTER; if (!Valid(child)) return E_INVALIDARG;
        const auto state=m_state.lock(); if (!state) return CO_E_OBJNOTCONNECTED;
        VariantInit(value); value->vt=VT_I4; value->lVal=state->visible() ? 0 : STATE_SYSTEM_INVISIBLE;
        if (child.lVal && state->disabled(child.lVal-1)) value->lVal |= STATE_SYSTEM_UNAVAILABLE;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE get_accDefaultAction(VARIANT child,BSTR* value) override {
        if (!value) return E_POINTER; *value=nullptr; if (!Valid(child) || !child.lVal) return S_FALSE;
        *value=SysAllocString(L"Press"); return *value ? S_OK : E_OUTOFMEMORY;
    }
    HRESULT STDMETHODCALLTYPE accLocation(long* x,long* y,long* w,long* h,VARIANT child) override {
        if (!x || !y || !w || !h) return E_POINTER; if (!Valid(child)) return E_INVALIDARG;
        const auto state=m_state.lock(); if (!state) return CO_E_OBJNOTCONNECTED;
        auto r=state->rect(child.lVal-1); POINT p{r.left,r.top}; ClientToScreen(state->hwnd,&p);
        *x=p.x; *y=p.y; *w=r.right-r.left; *h=r.bottom-r.top; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE accDoDefaultAction(VARIANT child) override {
        if (!Valid(child) || !child.lVal) return E_INVALIDARG;
        const auto state=m_state.lock(); if (!state) return CO_E_OBJNOTCONNECTED;
        if (!state->visible() || state->disabled(child.lVal-1)) return S_FALSE;
        state->invoke(child.lVal-1); return S_OK;
    }
    HRESULT STDMETHODCALLTYPE accHitTest(long x,long y,VARIANT* value) override {
        if (!value) return E_POINTER; VariantInit(value);
        const auto state=m_state.lock(); if (!state) return CO_E_OBJNOTCONNECTED;
        if (!state->visible()) return S_FALSE;
        POINT p{x,y}; ScreenToClient(state->hwnd,&p);
        for (int i=0;i<6;++i) { auto r=state->rect(i); if (PtInRect(&r,p)) { value->vt=VT_I4; value->lVal=i+1; return S_OK; } }
        return S_FALSE;
    }
    // Unsupported optional metadata/navigation have explicit empty results; names and default actions remain available.
    HRESULT STDMETHODCALLTYPE get_accValue(VARIANT,BSTR* v) override { return EmptyString(v); }
    HRESULT STDMETHODCALLTYPE get_accDescription(VARIANT,BSTR* v) override { return EmptyString(v); }
    HRESULT STDMETHODCALLTYPE get_accHelp(VARIANT,BSTR* v) override { return EmptyString(v); }
    HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut(VARIANT,BSTR* v) override { return EmptyString(v); }
    HRESULT STDMETHODCALLTYPE get_accHelpTopic(BSTR* v,VARIANT,long* topic) override { if (topic) *topic=0; return EmptyString(v); }
    HRESULT STDMETHODCALLTYPE get_accFocus(VARIANT* v) override { if (!v) return E_POINTER; VariantInit(v); return S_FALSE; }
    HRESULT STDMETHODCALLTYPE get_accSelection(VARIANT* v) override { return get_accFocus(v); }
    HRESULT STDMETHODCALLTYPE accSelect(long,VARIANT) override { return S_FALSE; }
    HRESULT STDMETHODCALLTYPE accNavigate(long,VARIANT,VARIANT* v) override { return get_accFocus(v); }
    HRESULT STDMETHODCALLTYPE put_accName(VARIANT,BSTR) override { return E_ACCESSDENIED; }
    HRESULT STDMETHODCALLTYPE put_accValue(VARIANT,BSTR) override { return E_ACCESSDENIED; }
private:
    // Validates MSAA simple child identifiers, including the toolbar itself (zero).
    static bool Valid(VARIANT child) { return child.vt==VT_I4 && child.lVal>=0 && child.lVal<=6; }
    // Initializes optional string output without allocating or publishing stale storage.
    static HRESULT EmptyString(BSTR* value) { if (!value) return E_POINTER; *value=nullptr; return S_FALSE; }
    std::weak_ptr<DockAccessibleState> m_state;
    std::atomic<ULONG> m_refs{1};
};
} // namespace kv
