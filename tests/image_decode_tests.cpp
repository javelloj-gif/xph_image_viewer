#include "common/pch.h"
#include "loader/image_loader.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace kv {
namespace {
// Failure controls apply only to the first decoder attempt; all other WIC operations are real.
enum class DecodeFault { None, Open, Count, ZeroCount, FrameZero, AllFrames, LaterFrames };
// Owned by proxy factory/decoders; records WIC attempts and decoder lifetime overlap, never product state.
struct DecodeControl {
    DecodeFault fault = DecodeFault::None;
    int opens = 0;
    int active = 0;
    bool overlap = false;
};

// Test-only COM forwarding family: identity/lifetime stay on the wrapper and other methods delegate unchanged.
#define TEST_COM_IDENTITY(Interface) \
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override { \
        if (!out) return E_POINTER; *out = nullptr; \
        if (id != __uuidof(IUnknown) && id != __uuidof(Interface)) return E_NOINTERFACE; \
        *out = static_cast<Interface*>(this); AddRef(); return S_OK; } \
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; } \
    ULONG STDMETHODCALLTYPE Release() override { const ULONG left = --refs; if (!left) delete this; return left; }
#define TEST_WIC_FORWARD(Name, Signature, Arguments) \
    HRESULT STDMETHODCALLTYPE Name Signature override { return inner->Name Arguments; }

// Wraps a real decoder; controls count/frame failures without replacing conversion or GIF metadata.
class ControlledDecoder final : public IWICBitmapDecoder {
public:
    // Owns the real decoder and a snapshot of first-attempt failure behavior.
    ControlledDecoder(ComPtr<IWICBitmapDecoder> decoder, std::shared_ptr<DecodeControl> state, DecodeFault behavior)
        : inner(std::move(decoder)), control(std::move(state)), fault(behavior) { ++control->active; }
    // Releases the underlying decoder and marks this attempt finished before the next open.
    ~ControlledDecoder() { --control->active; }
    TEST_COM_IDENTITY(IWICBitmapDecoder)
    // Forwards the decoder's capability, metadata and ancillary resource methods without alterations.
    TEST_WIC_FORWARD(QueryCapability, (IStream* s, DWORD* c), (s,c))
    TEST_WIC_FORWARD(Initialize, (IStream* s, WICDecodeOptions o), (s,o))
    TEST_WIC_FORWARD(GetContainerFormat, (GUID* g), (g))
    TEST_WIC_FORWARD(GetDecoderInfo, (IWICBitmapDecoderInfo** p), (p))
    TEST_WIC_FORWARD(CopyPalette, (IWICPalette* p), (p))
    TEST_WIC_FORWARD(GetMetadataQueryReader, (IWICMetadataQueryReader** p), (p))
    TEST_WIC_FORWARD(GetPreview, (IWICBitmapSource** p), (p))
    TEST_WIC_FORWARD(GetColorContexts, (UINT c, IWICColorContext** p, UINT* n), (c,p,n))
    TEST_WIC_FORWARD(GetThumbnail, (IWICBitmapSource** p), (p))
    // Overrides only the first attempt's count when requested by a case.
    HRESULT STDMETHODCALLTYPE GetFrameCount(UINT* count) override {
        if (fault == DecodeFault::Count) return E_FAIL;
        if (fault == DecodeFault::ZeroCount) { *count = 0; return S_OK; }
        return inner->GetFrameCount(count);
    }
    // Fails selected frame retrieval; successful frames remain real WIC objects.
    HRESULT STDMETHODCALLTYPE GetFrame(UINT index, IWICBitmapFrameDecode** frame) override {
        if (fault == DecodeFault::AllFrames || (fault == DecodeFault::FrameZero && index == 0) ||
            (fault == DecodeFault::LaterFrames && index > 0)) { *frame = nullptr; return E_FAIL; }
        return inner->GetFrame(index, frame);
    }
private:
    std::atomic<ULONG> refs{1};
    ComPtr<IWICBitmapDecoder> inner;
    std::shared_ptr<DecodeControl> control;
    DecodeFault fault;
};

// Forwarding WIC factory counts filename calls and wraps only decoders; no production test hooks.
class CountingFactory final : public IWICImagingFactory2 {
public:
    // Retains the real factory and shared test counters for this loader.
    CountingFactory(ComPtr<IWICImagingFactory2> factory, std::shared_ptr<DecodeControl> state)
        : inner(std::move(factory)), control(std::move(state)) {}
    // Supports the factory's base interface as well as wrapper identity/lifetime.
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (id != __uuidof(IUnknown) && id != __uuidof(IWICImagingFactory) && id != __uuidof(IWICImagingFactory2)) return E_NOINTERFACE;
        *out = static_cast<IWICImagingFactory2*>(this); AddRef(); return S_OK;
    }
    // COM lifetime family: the wrapper releases its owned factory on final Release.
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { const ULONG left = --refs; if (!left) delete this; return left; }
    // Counts actual WIC filename attempts, injects first-open failure or delegates and wraps the result.
    HRESULT STDMETHODCALLTYPE CreateDecoderFromFilename(LPCWSTR p, const GUID* v, DWORD a, WICDecodeOptions o, IWICBitmapDecoder** out) override {
        ++control->opens;
        control->overlap |= control->active != 0;
        *out = nullptr;
        const auto fault = control->opens == 1 ? control->fault : DecodeFault::None;
        if (fault == DecodeFault::Open) return E_FAIL;
        ComPtr<IWICBitmapDecoder> decoder;
        const HRESULT hr = inner->CreateDecoderFromFilename(p,v,a,o,&decoder);
        if (FAILED(hr)) return hr;
        *out = new ControlledDecoder(std::move(decoder), control, fault);
        return hr;
    }
    // Remaining factory methods forward exactly, including the real converters/scalers/encoders used by tests.
    TEST_WIC_FORWARD(CreateDecoderFromStream, (IStream* s,const GUID* v,WICDecodeOptions o,IWICBitmapDecoder** p), (s,v,o,p))
    TEST_WIC_FORWARD(CreateDecoderFromFileHandle, (ULONG_PTR f,const GUID* v,WICDecodeOptions o,IWICBitmapDecoder** p), (f,v,o,p))
    TEST_WIC_FORWARD(CreateComponentInfo, (REFCLSID c,IWICComponentInfo** p), (c,p))
    TEST_WIC_FORWARD(CreateDecoder, (REFGUID g,const GUID* v,IWICBitmapDecoder** p), (g,v,p))
    TEST_WIC_FORWARD(CreateEncoder, (REFGUID g,const GUID* v,IWICBitmapEncoder** p), (g,v,p))
    TEST_WIC_FORWARD(CreatePalette, (IWICPalette** p), (p))
    TEST_WIC_FORWARD(CreateFormatConverter, (IWICFormatConverter** p), (p))
    TEST_WIC_FORWARD(CreateBitmapScaler, (IWICBitmapScaler** p), (p))
    TEST_WIC_FORWARD(CreateBitmapClipper, (IWICBitmapClipper** p), (p))
    TEST_WIC_FORWARD(CreateBitmapFlipRotator, (IWICBitmapFlipRotator** p), (p))
    TEST_WIC_FORWARD(CreateStream, (IWICStream** p), (p))
    TEST_WIC_FORWARD(CreateColorContext, (IWICColorContext** p), (p))
    TEST_WIC_FORWARD(CreateColorTransformer, (IWICColorTransform** p), (p))
    TEST_WIC_FORWARD(CreateBitmap, (UINT w,UINT h,REFWICPixelFormatGUID g,WICBitmapCreateCacheOption o,IWICBitmap** p), (w,h,g,o,p))
    TEST_WIC_FORWARD(CreateBitmapFromSource, (IWICBitmapSource* s,WICBitmapCreateCacheOption o,IWICBitmap** p), (s,o,p))
    TEST_WIC_FORWARD(CreateBitmapFromSourceRect, (IWICBitmapSource* s,UINT x,UINT y,UINT w,UINT h,IWICBitmap** p), (s,x,y,w,h,p))
    TEST_WIC_FORWARD(CreateBitmapFromMemory, (UINT w,UINT h,REFWICPixelFormatGUID g,UINT stride,UINT n,BYTE* b,IWICBitmap** p), (w,h,g,stride,n,b,p))
    TEST_WIC_FORWARD(CreateBitmapFromHBITMAP, (HBITMAP b,HPALETTE p,WICBitmapAlphaChannelOption o,IWICBitmap** out), (b,p,o,out))
    TEST_WIC_FORWARD(CreateBitmapFromHICON, (HICON i,IWICBitmap** p), (i,p))
    TEST_WIC_FORWARD(CreateComponentEnumerator, (DWORD c,DWORD o,IEnumUnknown** p), (c,o,p))
    TEST_WIC_FORWARD(CreateFastMetadataEncoderFromDecoder, (IWICBitmapDecoder* d,IWICFastMetadataEncoder** p), (d,p))
    TEST_WIC_FORWARD(CreateFastMetadataEncoderFromFrameDecode, (IWICBitmapFrameDecode* f,IWICFastMetadataEncoder** p), (f,p))
    TEST_WIC_FORWARD(CreateQueryWriter, (REFGUID g,const GUID* v,IWICMetadataQueryWriter** p), (g,v,p))
    TEST_WIC_FORWARD(CreateQueryWriterFromReader, (IWICMetadataQueryReader* r,const GUID* v,IWICMetadataQueryWriter** p), (r,v,p))
    TEST_WIC_FORWARD(CreateImageEncoder, (ID2D1Device* d,IWICImageEncoder** p), (d,p))
private:
    std::atomic<ULONG> refs{1};
    ComPtr<IWICImagingFactory2> inner;
    std::shared_ptr<DecodeControl> control;
};
#undef TEST_WIC_FORWARD
#undef TEST_COM_IDENTITY
} // namespace

// Exercises CPU decode equivalence with real WIC, owned tiny images and controlled decoder failures.
class ImageDecodeTests {
public:
    // Requires an isolated EXE/user_data staging directory prepared by the CTest wrapper.
    ImageDecodeTests() {
        wchar_t executable[32768] = {};
        GetModuleFileNameW(nullptr, executable, 32768);
        root = std::filesystem::path(executable).parent_path();
        Check(std::filesystem::is_directory(root / L"user_data") && std::filesystem::is_directory(root / L"fixtures"), "isolated decode staging");
        Check(loader.Initialize(0), "real WIC without workers");
        real = loader.m_wicFactory;
        loader.m_wicFactory.Attach(new CountingFactory(real, control));
        cases = root / L"casos Ω";
        std::filesystem::create_directories(cases);
    }

    // Assertions stay active in Release and provide a named failure instead of silently skipping a case.
    void Check(bool condition, const char* label) {
        if (!condition) throw std::runtime_error(label);
        ++checks;
    }

    // Resets one controlled run, requiring all decoders from the previous run to have released.
    void Reset(DecodeFault fault = DecodeFault::None) {
        Check(control->active == 0, "no decoder retained between loads");
        *control = {fault, 0, 0, false};
    }

    // Reproduces the old App decision using the unchanged public loader APIs.
    ViewerLoadResult Legacy(const std::wstring& path) {
        if (ImageLoader::IsAnimatedFormat(path)) {
            auto animation = loader.LoadAnimatedSync(path);
            if (animation && animation->IsValid()) return {nullptr, std::move(animation)};
        }
        return {loader.LoadSync(path), nullptr};
    }

    // Compares all consumed CPU state and explicitly requires GPU objects to remain absent.
    void Equal(const ViewerLoadResult& a, const ViewerLoadResult& b) {
        Check(!(a.image && a.animation) && !(b.image && b.animation), "exclusive result alternatives");
        Check(static_cast<bool>(a.image) == static_cast<bool>(b.image) && static_cast<bool>(a.animation) == static_cast<bool>(b.animation), "same result kind/failure");
        if (a.image) Check(a.image->filePath == b.image->filePath && a.image->width == b.image->width && a.image->height == b.image->height &&
            a.image->hasAlpha == b.image->hasAlpha && a.image->pixels == b.image->pixels && !a.image->gpuBitmap && !b.image->gpuBitmap, "static pixels/alpha/path equal");
        if (a.animation) {
            Check(a.animation->filePath == b.animation->filePath && a.animation->loopCount == b.animation->loopCount &&
                a.animation->totalDurationMs == b.animation->totalDurationMs && a.animation->frames.size() == b.animation->frames.size(), "animation metadata equal");
            for (size_t i = 0; i < a.animation->frames.size(); ++i) {
                const auto& x = a.animation->frames[i]; const auto& y = b.animation->frames[i];
                Check(x.width == y.width && x.height == y.height && x.delayMs == y.delayMs && x.pixels == y.pixels &&
                    !x.gpuBitmap && !y.gpuBitmap, "frame bytes/dimensions/delay equal");
            }
        }
    }

    // Compares controlled old/new runs and enforces exact decoder attempts and release-before-fallback.
    ViewerLoadResult Compare(const std::filesystem::path& path, int oldOpens, int newOpens, DecodeFault fault = DecodeFault::None) {
        Reset(fault);
        const auto previous = Legacy(path.wstring());
        Check(control->opens == oldOpens && !control->overlap && control->active == 0, "legacy count/lifetime matches case");
        Reset(fault);
        auto candidate = loader.LoadForViewerSync(path.wstring());
        Check(control->opens == newOpens && !control->overlap && control->active == 0, "candidate count/lifetime matches case");
        Equal(previous, candidate);
        return candidate;
    }

    // Encodes a small alpha PNG through the real WIC encoder, only in the owned staging tree.
    void Png(const std::filesystem::path& path) {
        ComPtr<IWICBitmapEncoder> encoder;
        ComPtr<IWICStream> stream;
        Check(SUCCEEDED(real->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) &&
            SUCCEEDED(real->CreateStream(&stream)) && SUCCEEDED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE)), "PNG encoder/stream");
        Check(SUCCEEDED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)), "PNG encoder init");
        ComPtr<IWICBitmapFrameEncode> frame;
        ComPtr<IPropertyBag2> properties;
        Check(SUCCEEDED(encoder->CreateNewFrame(&frame, &properties)) && SUCCEEDED(frame->Initialize(properties.Get())) &&
            SUCCEEDED(frame->SetSize(64, 32)), "PNG frame init");
        GUID format = GUID_WICPixelFormat32bppBGRA;
        Check(SUCCEEDED(frame->SetPixelFormat(&format)) && format == GUID_WICPixelFormat32bppBGRA, "PNG BGRA support");
        std::vector<uint32_t> pixels(64 * 32);
        for (size_t i = 0; i < pixels.size(); ++i) pixels[i] = i % 3 == 0 ? 0x80402010u : i % 3 == 1 ? 0xff123456u : 0;
        Check(SUCCEEDED(frame->WritePixels(32, 64 * 4, static_cast<UINT>(pixels.size() * 4), reinterpret_cast<BYTE*>(pixels.data()))) &&
            SUCCEEDED(frame->Commit()) && SUCCEEDED(encoder->Commit()), "PNG encoded");
    }

    // Writes tiny GIF89a frames with independent clear codes, transparent palette entry, offsets/disposal and delays.
    void Gif(const std::filesystem::path& path, bool animated) {
        std::vector<uint8_t> data{'G','I','F','8','9','a',4,0,4,0,0x81,0,0,
            0,0,0,255,0,0,0,255,0,0,0,255};
        const std::vector<uint8_t> loops{0x21,0xff,11,'N','E','T','S','C','A','P','E','2','.','0',3,1,0,0,0};
        data.insert(data.end(), loops.begin(), loops.end());
        // Emits a local rectangle with three-bit clear/literal codes, avoiding encoder-specific optimization.
        auto frame = [&](uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t disposal, uint8_t delay, std::vector<uint8_t> colors) {
            const std::vector<uint8_t> prefix{0x21,0xf9,4,static_cast<uint8_t>((disposal << 2) | 1),delay,0,0,0,
                0x2c,x,0,y,0,w,0,h,0,0,2};
            data.insert(data.end(), prefix.begin(), prefix.end());
            std::vector<uint8_t> packed;
            uint32_t bits = 0; int used = 0;
            auto code = [&](uint8_t value) { bits |= static_cast<uint32_t>(value) << used; used += 3;
                while (used >= 8) { packed.push_back(static_cast<uint8_t>(bits)); bits >>= 8; used -= 8; } };
            for (const auto color : colors) { code(4); code(color); }
            code(5);
            if (used) packed.push_back(static_cast<uint8_t>(bits));
            data.push_back(static_cast<uint8_t>(packed.size()));
            data.insert(data.end(), packed.begin(), packed.end()); data.push_back(0);
        };
        frame(0,0,4,4,1,5,std::vector<uint8_t>(16,1));
        if (animated) {
            frame(1,1,2,2,2,1,{2,0,0,2});
            frame(0,0,1,1,3,7,{3});
            frame(3,3,1,1,1,8,{2});
            frame(2,2,1,1,1,9,{3});
        }
        data.push_back(0x3b);
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        Check(file.good(), "GIF fixture written");
    }

    // Independent expectations cover premultiplication, disposal 2/3, transparent pixels and legacy minimum delay.
    void GifExpectations(const std::shared_ptr<AnimatedImageData>& animation) {
        Check(animation && animation->frames.size() == 5 && animation->loopCount == 0 && animation->totalDurationMs == 390, "GIF frame count/loops/duration");
        auto pixel = [&](size_t f, size_t x, size_t y) { uint32_t value; const auto& frame = animation->frames[f];
            Check(frame.width == 4 && frame.height == 4, "GIF logical canvas");
            std::memcpy(&value, frame.pixels.data() + (y * 4 + x) * 4, 4); return value; };
        Check(pixel(0,0,0) == 0xffff0000 && pixel(1,1,1) == 0xff00ff00 && pixel(1,2,1) == 0xffff0000, "GIF initial frame/offset/transparency");
        Check(pixel(2,1,1) == 0 && pixel(2,0,0) == 0xff0000ff, "GIF disposal 2 clears previous rectangle");
        Check(pixel(3,0,0) == 0xffff0000 && pixel(3,3,3) == 0xff00ff00 && pixel(4,2,2) == 0xff0000ff, "GIF disposal 3 restores previous canvas");
        Check(animation->frames[0].delayMs == 50 && animation->frames[1].delayMs == 100 && animation->frames[2].delayMs == 70,
            "GIF delays preserved including legacy minimum");
    }

    // Writes an embedded 4x4 two-frame lossless WebP generated once from red/blue RGBA test images; no Python dependency.
    void TinyWebp(const std::filesystem::path& path) {
        const uint8_t bytes[]{
            0x52,0x49,0x46,0x46,0x84,0,0,0,0x57,0x45,0x42,0x50,0x56,0x50,0x38,0x58,0x0a,0,0,0,
            0x12,0,0,0,3,0,0,3,0,0,0x41,0x4e,0x49,0x4d,6,0,0,0,0,0,0,0,0,0,
            0x41,0x4e,0x4d,0x46,0x28,0,0,0,0,0,0,0,0,0,3,0,0,3,0,0,0x32,0,0,2,
            0x56,0x50,0x38,0x4c,0x0f,0,0,0,0x2f,3,0xc0,0,0,7,0x10,0xfd,0x8f,0xfe,7,0x22,0xa2,0xff,1,0,
            0x41,0x4e,0x4d,0x46,0x28,0,0,0,0,0,0,0,0,0,3,0,0,3,0,0,0x5a,0,0,2,
            0x56,0x50,0x38,0x4c,0x0f,0,0,0,0x2f,3,0xc0,0,0x10,7,0x10,0xd1,0xff,2,6,0x22,0xa2,0xff,1,0};
        static_assert(sizeof(bytes) == 140);
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
        Check(file.good(), "tiny WebP fixture written");
    }

    // Checks optional installed codecs only within a stored-frame estimate, avoiding unbounded fixture decoding.
    void Optional(const std::filesystem::path& path) {
        ComPtr<IWICBitmapDecoder> decoder;
        const bool available = SUCCEEDED(real->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder));
        UINT count = 0, width = 0, height = 0;
        bool counted = false;
        if (available) {
            counted = SUCCEEDED(decoder->GetFrameCount(&count));
            ComPtr<IWICBitmapFrameDecode> frame;
            if (SUCCEEDED(decoder->GetFrame(0, &frame))) frame->GetSize(&width, &height);
            ComPtr<IWICMetadataQueryReader> meta;
            if (SUCCEEDED(decoder->GetMetadataQueryReader(&meta))) {
                PROPVARIANT w, h; PropVariantInit(&w); PropVariantInit(&h);
                if (SUCCEEDED(meta->GetMetadataByName(L"/logscrdesc/Width", &w)) && w.vt == VT_UI2) width = w.uiVal;
                if (SUCCEEDED(meta->GetMetadataByName(L"/logscrdesc/Height", &h)) && h.vt == VT_UI2) height = h.uiVal;
                PropVariantClear(&w); PropVariantClear(&h);
            }
        }
        decoder.Reset();
        const uint64_t estimated = static_cast<uint64_t>(std::max(1u, count)) * width * height * 4;
        std::cout << "Optional " << path.filename().string() << ": codec=" << available << " frames=" << count << " estimate=" << estimated;
        if (estimated > 32 * 1024 * 1024) { std::cout << " SKIP (stored-frame budget)\n"; return; }
        Reset(); const auto old = Legacy(path.wstring()); const int oldCount = control->opens;
        Reset(); const auto current = loader.LoadForViewerSync(path.wstring());
        Equal(old, current);
        const int expected = current.animation ? 1 : !ImageLoader::IsAnimatedFormat(path.wstring()) ? 1 :
            available && counted && count <= 1 && current.image && current.image->IsValid() ? 1 : 2;
        Check(control->opens == expected && control->active == 0 && !control->overlap, "optional decoder counts/lifetime");
        std::cout << " old/new=" << oldCount << '/' << control->opens << " kind=" << (current.animation ? "animated" : current.image ? "static" : "failed") << '\n';
    }

    // Runs an alternating-order microbenchmark of CPU loading only; counts separately prove removed decoder calls.
    void Benchmark(const std::filesystem::path& path) {
        std::vector<double> oldTimes, newTimes;
        for (int sample = 0; sample < 6; ++sample) {
            auto measure = [&](bool optimized) {
                Reset(); const auto begin = std::chrono::steady_clock::now();
                size_t bytes = 0;
                for (int i = 0; i < 20; ++i) { const auto value = optimized ? loader.LoadForViewerSync(path.wstring()) : Legacy(path.wstring()); bytes += value.image->pixels.size(); }
                const auto end = std::chrono::steady_clock::now();
                Check(bytes > 0, "benchmark consumes CPU bytes");
                return std::chrono::duration<double, std::micro>(end - begin).count() / 20;
            };
            double old, current;
            if (sample % 2 == 0) { old = measure(false); current = measure(true); }
            else { current = measure(true); old = measure(false); }
            if (sample) { oldTimes.push_back(old); newTimes.push_back(current); }
        }
        std::sort(oldTimes.begin(), oldTimes.end()); std::sort(newTimes.begin(), newTimes.end());
        std::cout << "PNG64x32 CPU median us old/new=" << oldTimes[2] << '/' << newTimes[2] << '\n';
    }

    // Covers normal paths and controlled recovery, then bounded optional codecs and an informational benchmark.
    int Run() {
        const auto png = cases / L"imagen Ω.PNG";
        const auto gif = cases / L"animated.gif";
        Png(png); Gif(gif, true); Gif(cases / L"single.gif", false);
        Check(Compare(png,2,1).image->IsValid(), "native alpha PNG required");
        const auto scaled = loader.LoadSync(png.wstring(), 32, 16);
        Check(scaled && scaled->width == 32 && scaled->height == 16 && scaled->pixels.size() == 32 * 16 * 4 && !scaled->gpuBitmap,
            "legacy scaled static API remains usable");
        Check(Compare(root / L"fixtures/noname.jpg",1,1).image->IsValid(), "native JPEG required");
        Check(Compare(cases / L"single.gif",2,1).image->IsValid(), "one-frame GIF static fallback required");
        GifExpectations(Compare(gif,1,1).animation);
        for (const auto fault : {DecodeFault::Open,DecodeFault::Count,DecodeFault::FrameZero})
            Check(Compare(png,2,2,fault).image->IsValid(), "fresh fallback recovers PNG");
        Check(Compare(png,2,1,DecodeFault::ZeroCount).image->IsValid(), "zero-frame report retains frame-zero fallback");
        Check(Compare(gif,2,2,DecodeFault::AllFrames).image->IsValid(), "empty animation falls back to fresh static");
        Check(Compare(gif,1,1,DecodeFault::LaterFrames).animation->frames.size() == 1, "partially decoded animation retains legacy acceptance");
        std::filesystem::copy_file(root / L"fixtures/noname.jpg", cases / L"misnamed.png");
        Check(Compare(cases / L"misnamed.png",2,1).image->IsValid(), "extension-based policy preserved for misnamed image");
        std::filesystem::copy_file(gif, cases / L"animation.jpg");
        Check(Compare(cases / L"animation.jpg",1,1).image->IsValid(), "noncandidate does not become animated based on container");
        { std::ofstream bad(cases / L"corrupt.png", std::ios::binary); bad << "not a PNG"; }
        const auto corrupt = Compare(cases / L"corrupt.png",2,2);
        const auto absent = Compare(cases / L"absent.png",2,2);
        Check(!corrupt.image && !corrupt.animation && !absent.image && !absent.animation, "corrupt/missing return no usable result");
        for (const auto* name : {L"apng.apng", L"apng_png.png", L"static.webp", L"animated.webp", L"rubik.gif"}) Optional(root / L"fixtures" / name);
        TinyWebp(cases / L"tiny.webp"); Optional(cases / L"tiny.webp");
        Benchmark(png);
        std::cout << "Passed " << checks << " viewer decoder checks\n";
        return 0;
    }
private:
    ImageLoader loader;
    ComPtr<IWICImagingFactory2> real;
    std::shared_ptr<DecodeControl> control = std::make_shared<DecodeControl>();
    std::filesystem::path root, cases;
    int checks = 0;
};
} // namespace kv

// Dispatches a dedicated isolated CPU-only decoder suite, without constructing App or initializing its services.
int RunImageDecodeTests() {
    try {
        kv::ScopedComInit com;
        kv::detail::g_enableFileLog = false;
        kv::ImageDecodeTests tests;
        return tests.Run();
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
