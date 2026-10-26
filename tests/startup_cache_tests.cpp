#include "common/pch.h"
#include "core/app.h"
#include "loader/thumbnail_cache.h"
#include <winsqlite/winsqlite3.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <random>

namespace kv {
// Isolated real-WIC/WARP tests: never launches a global IPC receiver or writes source fixtures.
class StartupCacheTests {
public:
    // Owns a fresh run directory; source fixtures are copied before mutation tests.
    StartupCacheTests() {
        wchar_t executable[32768]{}; GetModuleFileNameW(nullptr,executable,32768);
        Check(std::filesystem::path(executable).filename()==L"startup_cache_tests.exe" &&
            std::filesystem::is_directory(std::filesystem::path(executable).parent_path()/L"user_data"),"isolated test executable/user_data required");
        root=std::filesystem::current_path()/std::format(L"startup-cache-{}-{}",GetCurrentProcessId(),GetTickCount64());
        std::filesystem::create_directories(root/L"user_data");
        const auto fixtures=std::filesystem::path(__FILE__).parent_path()/L"fixtures";
        a=root/L"a.jpg"; b=root/L"imagen \u03a9.jpg";
        std::filesystem::copy_file(fixtures/L"noname.jpg",a);
        std::filesystem::copy_file(fixtures/L"noname.jpg",b);
    }
    // Assertions remain active in Release and identify behavioral failures.
    void Check(bool ok,const char* label) { if (!ok) throw std::runtime_error(label); ++checks; }
    // Bounded polling is used only for worker completion; no wall-clock performance thresholds in CTest.
    template<class F> void Until(F predicate,const char* label) {
        const auto end=GetTickCount64()+10000;
        while (!predicate() && GetTickCount64()<end) Sleep(2);
        Check(predicate(),label);
    }
    // Ensures source validation, cross-process-compatible storage, exact variants and corruption misses.
    void Cache() {
        Check(ThumbnailCache::Budget(10ull*1024*1024*1024)==(10ull*1024*1024*1024)/100,"adaptive one-percent budget");
        Check(ThumbnailCache::Budget(1000ull*1024*1024*1024)==1024ull*1024*1024,"one-GiB hard budget ceiling");
        ImageLoader loader; Check(loader.Initialize(0),"WIC initialized");
        auto image=loader.LoadSync(a.wstring(),100,100);
        auto large=loader.LoadSync(a.wstring(),200,200);
        Check(image && large,"real source thumbnails decoded");
        const auto signature=ThumbnailCache::Signature(a.wstring());
        ThumbnailCache cache; cache.Initialize(root/L"cache");
        cache.Store(signature,100,*image);
        std::shared_ptr<ImageData> restored;
        Until([&] { restored=cache.Read(a.wstring(),signature,100); return restored!=nullptr; },"async cache write readable");
        Check(restored->pixels==image->pixels && restored->width==image->width && restored->height==image->height && restored->hasAlpha==image->hasAlpha,"exact BGRA round trip");
        Check(!cache.Read(a.wstring(),signature,200),"100 variant does not satisfy 200");
        cache.Store(signature,200,*large);
        Until([&] { return cache.Read(a.wstring(),signature,200)!=nullptr; },"200 stored independently");
        // A second connection sees committed data; it may miss transient lock contention but never partial bytes.
        ThumbnailCache peer; peer.Initialize(root/L"cache");
        Until([&] { auto hit=peer.Read(a.wstring(),signature,100); return hit && hit->pixels==image->pixels; },"concurrent cache reader exact");
        peer.Shutdown(); cache.Shutdown(); cache.Initialize(root/L"cache");
        restored=cache.Read(a.wstring(),signature,100);
        Check(restored && restored->pixels==image->pixels,"persistent reopening exact");
        wchar_t executable[32768]{}; GetModuleFileNameW(nullptr,executable,32768);
        auto command=std::format(L"\"{}\" --startup-cache-peer \"{}\" \"{}\"",executable,(root/L"cache").wstring(),a.wstring());
        STARTUPINFOW startup{}; startup.cb=sizeof(startup); PROCESS_INFORMATION child{};
        Check(CreateProcessW(executable,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child)!=FALSE,"own cache peer process starts");
        const auto wait=WaitForSingleObject(child.hProcess,10000);
        if(wait!=WAIT_OBJECT_0) { TerminateProcess(child.hProcess,99); WaitForSingleObject(child.hProcess,1000); }
        DWORD code=99; GetExitCodeProcess(child.hProcess,&code); CloseHandle(child.hThread); CloseHandle(child.hProcess);
        Check(wait==WAIT_OBJECT_0 && code==0,"another process reuses exact cached pixels");
        sqlite3* blockingReader=nullptr;
        Check(sqlite3_open16((root/L"cache"/L"thumbnails.sqlite").c_str(),&blockingReader)==SQLITE_OK,"independent locking connection");
        Check(sqlite3_exec(blockingReader,"BEGIN; SELECT pixels FROM thumbs LIMIT 1",nullptr,nullptr,nullptr)==SQLITE_OK,"external read transaction holds shared lock");
        const auto lockedKey=signature+L"|locked-write";
        std::thread blockedWriter([&] { cache.Commit({lockedKey,100,image->width,image->height,image->hasAlpha,image->pixels}); });
        blockedWriter.join();
        { std::lock_guard lock(cache.m_dbMutex);
            Check(sqlite3_get_autocommit(cache.m_db)!=0,"failed cache commit releases its transaction"); }
        Check(!cache.Read(a.wstring(),lockedKey,100),"blocked commit never publishes uncommitted cache bytes");
        sqlite3_exec(blockingReader,"ROLLBACK",nullptr,nullptr,nullptr); sqlite3_close(blockingReader);
        cache.Store(lockedKey,100,*image);
        Until([&] { return cache.Read(a.wstring(),lockedKey,100)!=nullptr; },"cache recovers after external reader releases lock");
        auto previousTime=std::filesystem::last_write_time(a);
        std::filesystem::last_write_time(a,previousTime+std::chrono::seconds(2));
        const auto changed=ThumbnailCache::Signature(a.wstring());
        Check(signature!=changed && !cache.Read(a.wstring(),changed,100),"mtime change invalidates");
        std::filesystem::remove(a); std::filesystem::copy_file(b,a); std::filesystem::last_write_time(a,previousTime);
        Check(signature!=ThumbnailCache::Signature(a.wstring()),"replacement identity invalidates even restored mtime/size");
        const auto clone=root/L"old-a.jpg"; std::filesystem::rename(a,clone);
        Check(ThumbnailCache::Signature(a.wstring()).empty(),"deleted source cannot hit");
        std::filesystem::rename(clone,a);
        cache.Shutdown();
        sqlite3* db=nullptr; Check(sqlite3_open16((root/L"cache"/L"thumbnails.sqlite").c_str(),&db)==SQLITE_OK,"inspect cache");
        Check(sqlite3_exec(db,"UPDATE thumbs SET pixels=X'00'",nullptr,nullptr,nullptr)==SQLITE_OK,"corrupt disposable payload only");
        sqlite3_close(db); cache.Initialize(root/L"cache");
        Check(!cache.Read(a.wstring(),signature,100),"corrupt payload is miss");
        cache.Shutdown();
        std::filesystem::create_directories(root/L"not-a-directory");
        std::ofstream(root/L"not-a-directory"/L"file")<<"blocked";
        cache.Initialize(root/L"not-a-directory"/L"file"/L"child");
        Check(!cache.Read(a.wstring(),signature,100),"unwritable/invalid root degrades to miss");
        for(int i=0;i<100;++i) cache.Store(signature,100,*image);
        cache.Shutdown(); // Bounded queue can be discarded without a flush obligation.
        ThumbnailCache bounded; bounded.Initialize(root/L"bounded",4*1024*1024);
        ImageData noise; noise.width=200; noise.height=200; noise.pixels.resize(200*200*4);
        std::mt19937 random(003);
        for(size_t i=0;i<noise.pixels.size();++i) noise.pixels[i]=static_cast<uint8_t>(random());
        for(int i=0;i<24;++i) {
            const auto key=L"bounded-"+std::to_wstring(i);
            bounded.Store(key,200,noise);
            Until([&] { return bounded.Read(L"test",key,200)!=nullptr; },"bounded insert completes");
        }
        bounded.Shutdown(); bounded.Initialize(root/L"bounded",4*1024*1024);
        Check(!bounded.Read(L"test",L"bounded-0",200),"LRU removes oldest entry under byte budget");
        Check(bounded.Read(L"test",L"bounded-23",200)!=nullptr,"LRU preserves recent entry");
        Check(std::filesystem::file_size(root/L"bounded"/L"thumbnails.sqlite")<=4*1024*1024,"allocated SQLite bytes stay within budget");
        bounded.Shutdown();
    }
    // Exercises worker snapshots, all sort modes and cancellation without any UI access.
    void Scanning() {
        for(int mode=0;mode<6;++mode) {
            const auto result=FolderScanService::Scan(root.wstring(),mode);
            Check(result.complete && result.files.size()==2,"scan filters non-images and completes sort");
            FolderDataManager legacy; legacy.Initialize(nullptr); legacy.AddFolder(root.wstring(),true,false);
            legacy.SortFolder(0,static_cast<FolderDataManager::SortMode>(mode));
            Check(result.files==legacy.m_folders[0].filePaths,"worker snapshot preserves original sort and tie behavior");
        }
        auto lease=std::make_shared<const bool>(true); ImageCancellation cancel(lease); lease.reset();
        Check(!FolderScanService::Scan(root.wstring(),0,cancel).complete,"canceled scan never publishes partial result");
        FolderScanService scanner;
        bool delivered=false;
        scanner.Request(root.wstring(),0,true,{},[&](FolderSnapshot result) { delivered=result.complete; });
        Until([&] { scanner.ProcessPending(); return delivered; },"snapshot delivered only by owner drain");
        scanner.Shutdown();

        FolderDataManager startup; startup.Initialize(nullptr); startup.EnableAsync();
        const auto restored = root / L"pending-restoration";
        const auto session = startup.GetSessionFilePath();
        const auto utf8Root = std::filesystem::path(root).u8string();
        const auto utf8Restored = restored.u8string();
        std::ofstream sessionFile(session, std::ios::binary | std::ios::trunc);
        sessionFile.write(reinterpret_cast<const char*>(utf8Restored.data()), utf8Restored.size()) << "\n\n\n\n";
        sessionFile.write(reinterpret_cast<const char*>(utf8Root.data()), utf8Root.size()) << "\n\n\n\n";
        sessionFile.close();
        Check(startup.LoadSession(false, root.wstring(), false), "startup queues foreground and restoration");
        Check(startup.m_pendingFolders.size()==2 && !startup.m_pendingFolders[0].restoring
            && !startup.m_pendingFolders[0].loadImage, "duplicate session target retains single foreground lease");
        Check(startup.m_sessionOrder.size()==2 && startup.m_sessionOrder[0]==restored.wstring()
            && startup.m_sessionOrder[1]==root.wstring(), "foreground scheduling preserves persisted record order");
        Check(startup.m_folders.empty(), "worker cannot publish models before UI drain");
        Until([&] { startup.ProcessPending(); return !startup.m_folders.empty(); }, "startup foreground publishes on owner");
        Check(startup.m_folders.size()==1 && startup.m_folders[0].filePaths.size()==2,
            "duplicate startup target publishes once without losing files");
        startup.StopScanning();
    }
    // Real worker gating proves foreground capacity and stable priority without relying on decode speed.
    void Scheduling() {
        ImageLoader loader; Check(loader.Initialize(0),"controlled four-worker admission");
        std::atomic<int> active=0,peak=0;
        std::atomic<bool> release=false,foreground=false;
        // Always unblock and join gated workers before the test's atomic state is destroyed, including failure.
        struct FinishWorkers { ImageLoader& loader; std::atomic<bool>& release;
            ~FinishWorkers() { release=true; loader.Shutdown(); } } cleanup{loader,release};
        for(int i=0;i<8;++i) {
            loader.LoadAsync(a.wstring(),{},400,400); // Same admission used by Board, retaining its cache policy.
            loader.m_taskQueue.back().work=[&] {
            const int count=++active; int observed=peak.load();
            while(count>observed && !peak.compare_exchange_weak(observed,count)) { }
            while(!release.load()) Sleep(1);
            --active;
            };
        }
        for(int i=0;i<4;++i) loader.m_workers.emplace_back(&ImageLoader::WorkerLoop,&loader);
        Until([&] { return active==3; },"exactly three thumbnail slots become occupied");
        loader.EnqueueTask([&] { foreground=true; },{},0,false);
        Until([&] { return foreground.load(); },"reserved worker serves foreground while thumbnails blocked");
        Check(peak<=3,"thumbnail concurrency stays bounded");
        release=true; loader.Shutdown();
        ImageLoader ordered; Check(ordered.Initialize(0),"controlled scheduler admission");
        std::mutex mutex; std::vector<int> sequence;
        auto record=[&](int value) { std::lock_guard lock(mutex); sequence.push_back(value); };
        ordered.EnqueueTask([&] { record(2); },{},2,true);
        ordered.EnqueueTask([&] { record(1); },{},1,true);
        ordered.EnqueueTask([&] { record(0); },{},0,false);
        ordered.EnqueueTask([&] { record(3); },{},2,true);
        ordered.m_workers.emplace_back(&ImageLoader::WorkerLoop,&ordered);
        Until([&] { std::lock_guard lock(mutex); return sequence.size()==4; },"priority tasks complete");
        ordered.Shutdown();
        Check(sequence==std::vector<int>({0,1,2,3}),"foreground/destination/visible priority is stable");
    }
    // Constructs D2D resources on the test/UI thread; no swapchain or visible window is needed.
    void RendererContext(App& app) {
        app.m_window.m_hwnd=CreateWindowExW(0,L"STATIC",L"startup-cache-tests",0,0,0,0,0,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);
        Check(app.m_window.m_hwnd!=nullptr,"isolated message-only redraw target");
        ComPtr<ID3D11Device> d3d;
        Check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&d3d,nullptr,nullptr)),"WARP");
        ComPtr<IDXGIDevice> dxgi; Check(SUCCEEDED(d3d.As(&dxgi)),"DXGI");
        ComPtr<ID2D1Factory1> factory; Check(SUCCEEDED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,IID_PPV_ARGS(&factory))),"D2D factory");
        ComPtr<ID2D1Device> device; Check(SUCCEEDED(factory->CreateDevice(dxgi.Get(),&device)),"D2D device");
        ComPtr<ID2D1DeviceContext> context; Check(SUCCEEDED(device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,&context)),"D2D context");
        Check(SUCCEEDED(context.As(&app.m_renderer.m_d2dContext)),"context2");
    }
    // A completed old CPU job cannot upload/replace a newer request; folder snapshots cannot undo it.
    void Generations() {
        App app; RendererContext(app);
        Check(app.m_imageLoader.Initialize(0),"controlled loader");
        app.m_folderData.Initialize(&app); app.m_folderData.EnableAsync();
        app.m_thumbnailStrip.Initialize(&app); app.m_waterfallPage.Initialize(&app);
        app.m_viewerPage.Initialize(&app); app.m_asyncLoading=true;
        app.m_window.m_fullscreen=true;
        app.LoadImage(a.wstring());
        auto old=std::move(app.m_imageLoader.m_foregroundQueue.front()); app.m_imageLoader.m_foregroundQueue.pop(); old();
        Check(!app.m_currentImage,"CPU decode does not mutate UI");
        app.LoadImage(b.wstring());
        auto current=std::move(app.m_imageLoader.m_foregroundQueue.front()); app.m_imageLoader.m_foregroundQueue.pop(); current();
        app.m_imageLoader.ProcessPendingTextures(app.m_renderer);
        Check(app.m_currentImage && app.m_currentImage->filePath==b.wstring(),"latest generation wins");
        Check(app.m_currentImage->HasGpuBitmap(),"UI thread uploaded current image");
        app.m_settings.sort_mode=1; // A sort change while the earlier scan is pending must be retried off UI.
        Until([&] { app.m_folderData.ProcessPending(); return !app.m_fileList.empty(); },"folder navigation becomes ready asynchronously");
        Check(app.m_fileList[app.m_currentIndex]==b.wstring(),"snapshot keeps explicit requested selection");
        Check(app.m_fileList.front()==b.wstring(),"pending snapshot honors the latest sort mode");
        auto previous=app.m_currentImage;
        app.LoadImage((root/L"missing.jpg").wstring());
        auto missing=std::move(app.m_imageLoader.m_foregroundQueue.front()); app.m_imageLoader.m_foregroundQueue.pop(); missing();
        app.m_imageLoader.ProcessPendingTextures(app.m_renderer);
        Check(app.m_currentImage==previous,"failed new image retains previous original");
        const auto pendingFolder=root/L"pending"; std::filesystem::create_directory(pendingFolder);
        std::filesystem::copy_file(b,pendingFolder/L"b.jpg");
        app.m_folderData.AddFolder(pendingFolder.wstring(),true,false);
        Check(app.m_folderData.HasPendingFolders(),"session has unresolved folder descriptor");
        Check(app.m_folderData.SaveSession(),"session can save during pending restoration");
        std::ifstream session(app.m_folderData.GetSessionFilePath());
        const std::string text((std::istreambuf_iterator<char>(session)),{});
        Check(text.find("pending")!=std::string::npos,"unresolved folder survives early save");
        auto staleSnapshot=FolderScanService::Scan(pendingFolder.wstring(),app.m_settings.sort_mode);
        std::filesystem::remove(pendingFolder/L"b.jpg");
        app.m_folderData.NotifyImageRemoved((pendingFolder/L"b.jpg").wstring());
        app.m_folderData.CompleteFolder(std::make_shared<const std::wstring>(pendingFolder.wstring()),std::move(staleSnapshot));
        Check(std::none_of(app.m_folders.begin(),app.m_folders.end(),[&](const FolderInfo& folder) { return PathEquals(folder.folderPath,pendingFolder.wstring()); }),
            "stale pre-deletion snapshot cannot resurrect a removed image");
        auto& strip=app.m_thumbnailStrip;
        std::vector<std::wstring> paths; for(int i=0;i<7749;++i) paths.push_back(std::to_wstring(i));
        strip.SetFileList(paths); strip.m_scrollX=0; strip.m_targetScrollX=strip.GetCenteredScroll(7000);
        const auto destinationRange=strip.GetVisibleRange(800,strip.m_targetScrollX);
        Check(destinationRange.first>6900 && destinationRange.second<7100,"target viewport independent of animated current position");
        strip.m_scrollX=-332349.875f; strip.m_targetScrollX=-332350.0f;
        strip.Update(1.0/144); strip.Update(1.0/144);
        Check(strip.m_scrollX==-332349.875f && strip.m_scrollStableFrames==2,
            "diagnostic recognizes a representable large-offset lerp plateau without altering animation");
        app.m_folderData.StopScanning();
    }
    // Runs each behavior and leaves its isolated evidence on disk for diagnosis.
    int Run() { Cache(); Scanning(); Scheduling(); Generations(); std::cout<<"startup/cache checks: "<<checks<<"\n"; return 0; }
private:
    std::filesystem::path root,a,b;
    size_t checks=0;
};
}
// CLI entry for the shared test EXE; initializes COM before WIC/WARP and returns explicit failure.
int RunStartupCacheTests() {
    try {
        kv::ScopedComInit com;
        int argc=0; auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
        if (!argv) throw std::runtime_error("Unicode arguments");
        if(argc==4 && std::wstring_view(argv[1])==L"--startup-cache-peer") {
            const std::filesystem::path root=argv[2]; const std::wstring source=argv[3]; LocalFree(argv);
            kv::ImageLoader loader; if(!loader.Initialize(0)) return 2;
            auto original=loader.LoadSync(source,100,100);
            kv::ThumbnailCache cache; cache.Initialize(root);
            auto cached=cache.Read(source,kv::ThumbnailCache::Signature(source),100);
            return original && cached && original->pixels==cached->pixels ? 0 : 3;
        }
        LocalFree(argv);
        kv::StartupCacheTests tests; return tests.Run();
    }
    catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
