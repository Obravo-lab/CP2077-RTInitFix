/**
 * sl_interposer_proxy4.cpp
 * Proxy sl.interposer.dll v4 — sans sl.interposer_real.dll
 *
 * Stratégie :
 *   - Ne PAS charger sl.interposer_real.dll (évite l'init NVIDIA via D3D12/NVAPI)
 *   - Charger System32/dxgi.dll directement pour les fonctions DXGI
 *   - Patcher le vtable de la factory DXGI pour filtrer NVIDIA
 *   - Stubber toutes les fonctions sl* (retourner succès ou "non supporté")
 *   - vk* trampolines restent null (jeu DX12, jamais appelés)
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>

static const UINT NVIDIA_VENDOR = 0x10DE;
static const UINT WARP_VENDOR   = 0x1414; // Microsoft Basic Render Driver

typedef HRESULT(WINAPI* PFN_CreateDXGIFactory )(REFIID, void**);
typedef HRESULT(WINAPI* PFN_CreateDXGIFactory1)(REFIID, void**);
typedef HRESULT(WINAPI* PFN_CreateDXGIFactory2)(UINT, REFIID, void**);
typedef HRESULT(WINAPI* PFN_DXGIGetDebugInterface1)(UINT, REFIID, void**);

static PFN_CreateDXGIFactory          real_F    = nullptr;
static PFN_CreateDXGIFactory1         real_F1   = nullptr;
static PFN_CreateDXGIFactory2         real_F2   = nullptr;
static PFN_DXGIGetDebugInterface1     real_DbgI = nullptr;

// ─── Log debug ───────────────────────────────────────────────────────────────

static FILE* g_log = nullptr;

static void LogInit(const char* dllPath) {
    char logPath[MAX_PATH];
    strncpy(logPath, dllPath, MAX_PATH - 1);
    logPath[MAX_PATH - 1] = '\0';
    char* last = strrchr(logPath, '\\');
    if (last) strcpy(last + 1, "sl_proxy_debug.txt");
    else      strcpy(logPath, "sl_proxy_debug.txt");
    g_log = fopen(logPath, "a");
    if (!g_log) {
        char tmp[MAX_PATH];
        GetTempPathA(MAX_PATH, tmp);
        strncat(tmp, "sl_proxy_debug.txt", MAX_PATH - strlen(tmp) - 1);
        g_log = fopen(tmp, "a");
    }
}

static void Log(const char* fmt, ...) {
    if (!g_log) return;
    va_list ap; va_start(ap, fmt); vfprintf(g_log, fmt, ap); va_end(ap);
    fflush(g_log);
}

// ─── Trampolines vk*, D3D11*, D3D12* ────────────────────────────────────────
#include "sl_trampolines_v4.inc"

// ─── Filtre adaptateur ───────────────────────────────────────────────────────

static bool ShouldSkip(IDXGIAdapter* a) {
    if (!a) return false;
    DXGI_ADAPTER_DESC d = {};
    if (FAILED(a->GetDesc(&d))) return false;
    return d.VendorId == NVIDIA_VENDOR || d.VendorId == WARP_VENDOR;
}

static bool IsNvidia(IDXGIAdapter* a) {
    if (!a) return false;
    DXGI_ADAPTER_DESC d = {};
    return SUCCEEDED(a->GetDesc(&d)) && d.VendorId == NVIDIA_VENDOR;
}

// ─── Hooks vtable ────────────────────────────────────────────────────────────

typedef HRESULT(STDMETHODCALLTYPE* PFN_EnumAdapters )(IDXGIFactory*, UINT, IDXGIAdapter**);
typedef HRESULT(STDMETHODCALLTYPE* PFN_EnumAdapters1)(IDXGIFactory1*, UINT, IDXGIAdapter1**);
typedef HRESULT(STDMETHODCALLTYPE* PFN_EnumAdapterByGpuPref)(IDXGIFactory6*, UINT, DXGI_GPU_PREFERENCE, REFIID, void**);

static PFN_EnumAdapters         orig_EnumAdapters         = nullptr;
static PFN_EnumAdapters1        orig_EnumAdapters1        = nullptr;
static PFN_EnumAdapterByGpuPref orig_EnumAdapterByGpuPref = nullptr;
static bool                     g_patched                 = false;

static HRESULT STDMETHODCALLTYPE Hook_EnumAdapters(IDXGIFactory* self, UINT idx, IDXGIAdapter** pp) {
    Log("Hook_EnumAdapters idx=%u\n", idx);
    for (UINT i = 0, found = 0; ; i++) {
        IDXGIAdapter* a = nullptr;
        HRESULT hr = orig_EnumAdapters(self, i, &a);
        if (hr == DXGI_ERROR_NOT_FOUND) { Log("  -> NOT_FOUND (found=%u)\n", found); return DXGI_ERROR_NOT_FOUND; }
        if (FAILED(hr)) return hr;
        DXGI_ADAPTER_DESC d = {}; a->GetDesc(&d);
        bool skip = ShouldSkip(a);
        Log("  adapter[%u] vendor=0x%04x skip=%d\n", i, d.VendorId, (int)skip);
        if (!skip) {
            if (found == idx) { *pp = a; Log("  -> returning adapter[%u]\n", i); return S_OK; }
            ++found; a->Release();
        } else { a->Release(); }
    }
}

static HRESULT STDMETHODCALLTYPE Hook_EnumAdapters1(IDXGIFactory1* self, UINT idx, IDXGIAdapter1** pp) {
    Log("Hook_EnumAdapters1 idx=%u\n", idx);
    for (UINT i = 0, found = 0; ; i++) {
        IDXGIAdapter1* a = nullptr;
        HRESULT hr = orig_EnumAdapters1(self, i, &a);
        if (hr == DXGI_ERROR_NOT_FOUND) { Log("  -> NOT_FOUND (found=%u)\n", found); return DXGI_ERROR_NOT_FOUND; }
        if (FAILED(hr)) return hr;
        DXGI_ADAPTER_DESC d = {}; a->GetDesc(&d);
        bool skip = ShouldSkip(reinterpret_cast<IDXGIAdapter*>(a));
        Log("  adapter1[%u] vendor=0x%04x skip=%d\n", i, d.VendorId, (int)skip);
        if (!skip) {
            if (found == idx) { *pp = a; Log("  -> returning adapter1[%u]\n", i); return S_OK; }
            ++found; a->Release();
        } else { a->Release(); }
    }
}

static HRESULT STDMETHODCALLTYPE Hook_EnumAdapterByGpuPref(IDXGIFactory6* self, UINT idx,
        DXGI_GPU_PREFERENCE pref, REFIID riid, void** pp) {
    Log("Hook_EnumAdapterByGpuPref idx=%u pref=%u\n", idx, (unsigned)pref);
    for (UINT i = 0, found = 0; ; i++) {
        IDXGIAdapter* a = nullptr;
        HRESULT hr = orig_EnumAdapterByGpuPref(self, i, pref, __uuidof(IDXGIAdapter), (void**)&a);
        if (hr == DXGI_ERROR_NOT_FOUND) { Log("  -> NOT_FOUND (found=%u)\n", found); return DXGI_ERROR_NOT_FOUND; }
        if (FAILED(hr)) return hr;
        DXGI_ADAPTER_DESC d = {}; a->GetDesc(&d);
        bool skip = ShouldSkip(a);
        Log("  adapterGpu[%u] vendor=0x%04x skip=%d\n", i, d.VendorId, (int)skip);
        a->Release();
        if (!skip) {
            if (found == idx) { Log("  -> returning adapterGpu[%u]\n", i); return orig_EnumAdapterByGpuPref(self, i, pref, riid, pp); }
            ++found;
        }
    }
}

static void PatchVTable(void* factory) {
    if (g_patched) return;
    g_patched = true;

    void** vtbl = *reinterpret_cast<void***>(factory);
    Log("PatchVTable vtbl=%p\n", vtbl);

    DWORD oldProt = 0;
    VirtualProtect(vtbl, 35 * sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProt);

    orig_EnumAdapters = reinterpret_cast<PFN_EnumAdapters>(vtbl[7]);
    vtbl[7] = reinterpret_cast<void*>(&Hook_EnumAdapters);

    orig_EnumAdapters1 = reinterpret_cast<PFN_EnumAdapters1>(vtbl[12]);
    vtbl[12] = reinterpret_cast<void*>(&Hook_EnumAdapters1);

    IDXGIFactory6* f6 = nullptr;
    if (SUCCEEDED(reinterpret_cast<IUnknown*>(factory)->QueryInterface(__uuidof(IDXGIFactory6), (void**)&f6)) && f6) {
        orig_EnumAdapterByGpuPref = reinterpret_cast<PFN_EnumAdapterByGpuPref>(vtbl[29]);
        vtbl[29] = reinterpret_cast<void*>(&Hook_EnumAdapterByGpuPref);
        f6->Release();
    }

    VirtualProtect(vtbl, 35 * sizeof(void*), oldProt, &oldProt);
    Log("PatchVTable done\n");
}

// ─── Exports DXGI interceptés ────────────────────────────────────────────────

extern "C" {

__declspec(dllexport) HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** pp) {
    Log("CreateDXGIFactory\n");
    HRESULT hr = real_F(riid, pp);
    if (SUCCEEDED(hr) && *pp) PatchVTable(*pp);
    return hr;
}

__declspec(dllexport) HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** pp) {
    Log("CreateDXGIFactory1\n");
    HRESULT hr = real_F1(riid, pp);
    if (SUCCEEDED(hr) && *pp) PatchVTable(*pp);
    return hr;
}

__declspec(dllexport) HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, void** pp) {
    Log("CreateDXGIFactory2\n");
    HRESULT hr = real_F2(Flags, riid, pp);
    if (SUCCEEDED(hr) && *pp) PatchVTable(*pp);
    return hr;
}

__declspec(dllexport) HRESULT WINAPI DXGIGetDebugInterface1(UINT Flags, REFIID riid, void** pp) {
    Log("DXGIGetDebugInterface1\n");
    if (real_DbgI) return real_DbgI(Flags, riid, pp);
    return E_NOTIMPL;
}

// ─── D3D12CreateDevice wrappé + hook CheckFeatureSupport ────────────────────

typedef HRESULT(WINAPI* PFN_D3D12CreateDevice)(IUnknown*, int, REFIID, void**);
static PFN_D3D12CreateDevice real_D3D12CreateDevice = nullptr;

// Hook CheckFeatureSupport pour masquer DXR (RaytracingTier -> NOT_SUPPORTED)
// D3D12_FEATURE_D3D12_OPTIONS5 = 27
// D3D12_FEATURE_DATA_D3D12_OPTIONS5: BOOL(4) + RenderPassTier(4) + RaytracingTier(4) → offset 8
#define D3D12_FEATURE_D3D12_OPTIONS5_VAL 27
typedef HRESULT(WINAPI* PFN_CheckFeatureSupport)(void*, UINT, void*, UINT);
static PFN_CheckFeatureSupport real_CheckFeatureSupport = nullptr;
static bool g_vtablePatched = false;

static HRESULT WINAPI Hook_CheckFeatureSupport(void* pThis, UINT Feature, void* pData, UINT DataSize) {
    HRESULT hr = real_CheckFeatureSupport(pThis, Feature, pData, DataSize);
    if (SUCCEEDED(hr) && Feature == D3D12_FEATURE_D3D12_OPTIONS5_VAL && pData && DataSize >= 12) {
        UINT* raytracingTier = (UINT*)((char*)pData + 8);
        Log("CheckFeatureSupport OPTIONS5: RaytracingTier=%u -> 0 (NOT_SUPPORTED)\n", *raytracingTier);
        *raytracingTier = 0;
    }
    return hr;
}

static void PatchD3D12DeviceVTable(void* pDevice) {
    if (g_vtablePatched) return;
    void** vtbl = *(void***)pDevice;
    // vtable[13] = CheckFeatureSupport
    DWORD old;
    if (VirtualProtect(&vtbl[13], sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) {
        real_CheckFeatureSupport = (PFN_CheckFeatureSupport)vtbl[13];
        vtbl[13] = (void*)Hook_CheckFeatureSupport;
        VirtualProtect(&vtbl[13], sizeof(void*), old, &old);
        g_vtablePatched = true;
        Log("PatchD3D12DeviceVTable: vtbl[13]=CheckFeatureSupport hooked\n");
    } else {
        Log("PatchD3D12DeviceVTable: VirtualProtect failed err=%lu\n", GetLastError());
    }
}

__declspec(dllexport) HRESULT WINAPI D3D12CreateDevice(IUnknown* pAdapter, int MinFeatureLevel, REFIID riid, void** ppDevice) {
    Log("D3D12CreateDevice adapter=%p featureLevel=0x%x\n", pAdapter, MinFeatureLevel);
    if (!real_D3D12CreateDevice) { Log("  real_D3D12CreateDevice is NULL!\n"); return E_FAIL; }
    HRESULT hr = real_D3D12CreateDevice(pAdapter, MinFeatureLevel, riid, ppDevice);
    Log("  D3D12CreateDevice -> hr=0x%08x device=%p\n", (unsigned)hr, ppDevice ? *ppDevice : nullptr);
    if (SUCCEEDED(hr) && ppDevice && *ppDevice) {
        PatchD3D12DeviceVTable(*ppDevice);
    }
    // Si ppDevice=NULL (feature-check call), créer un device temporaire pour patcher le vtable global
    if (!g_vtablePatched && pAdapter && !ppDevice) {
        void* pTempDev = nullptr;
        HRESULT hrTmp = real_D3D12CreateDevice(pAdapter, MinFeatureLevel, riid, &pTempDev);
        Log("  TempDevice for vtable patch -> hr=0x%08x dev=%p\n", (unsigned)hrTmp, pTempDev);
        if (SUCCEEDED(hrTmp) && pTempDev) {
            PatchD3D12DeviceVTable(pTempDev);
            ((IUnknown*)pTempDev)->Release();
        }
    }
    return hr;
}

// ─── Stubs sl* ───────────────────────────────────────────────────────────────
// sl::Result::eOk = 0, toute valeur != 0 = erreur (feature non supportée)

// slInit : retourne 0 (ok) pour que le jeu fasse ensuite slIsFeatureSupported
// slIsFeatureSupported retournera 4 (no adapter) pour DLSS/RT => DLSS_UNSUPPORTED=true
__declspec(dllexport) int32_t WINAPI slInit(const void* /*pref*/, uint64_t /*sdkVersion*/) {
    Log("slInit -> 4 (eError_NoSupportedAdapterFound)\n");
    return 4; // eError_NoSupportedAdapterFound - pas de GPU NVIDIA
}

__declspec(dllexport) int32_t WINAPI slShutdown() {
    Log("slShutdown stubbed\n");
    return 0;
}

// Retourne l'objet natif (aucun wrapping effectué)
__declspec(dllexport) int32_t WINAPI slGetNativeInterface(void* proxyObject, void** nativeObject) {
    Log("slGetNativeInterface proxy=%p\n", proxyObject);
    if (nativeObject) *nativeObject = proxyObject;
    return 0;
}

// Retourne succès — on ne wrappe pas mais on ne bloque pas non plus l'init Streamline
__declspec(dllexport) int32_t WINAPI slUpgradeInterface(void** baseInterface) {
    Log("slUpgradeInterface iface=%p -> 0 (ok)\n", baseInterface ? *baseInterface : nullptr);
    return 0;
}

__declspec(dllexport) int32_t WINAPI slSetD3DDevice(void* d3dDevice) {
    Log("slSetD3DDevice dev=%p -> 0 (ok)\n", d3dDevice);
    return 0;
}

// Features : retourner "non supporté" (!=0) pour que le jeu ne tente pas de les utiliser
__declspec(dllexport) int32_t WINAPI slGetFeatureFunction(uint32_t feature, const char* funcName, void** func) {
    Log("slGetFeatureFunction feature=%u func=%s\n", feature, funcName ? funcName : "null");
    if (func) *func = nullptr;
    return 1; // eError
}

__declspec(dllexport) int32_t WINAPI slGetFeatureRequirements(uint32_t feature, void* /*requirements*/) {
    Log("slGetFeatureRequirements feature=%u\n", feature);
    return 4; // eError_NoSupportedAdapterFound
}

__declspec(dllexport) int32_t WINAPI slGetFeatureVersion(uint32_t feature, void* /*version*/) {
    Log("slGetFeatureVersion feature=%u\n", feature);
    return 1;
}

// Token de frame : retourner un pointeur statique factice
static uint64_t g_dummyToken = 0;
__declspec(dllexport) int32_t WINAPI slGetNewFrameToken(void** token, const uint32_t* frameIndex) {
    Log("slGetNewFrameToken frameIndex=%u\n", frameIndex ? *frameIndex : 0xFFFFFFFF);
    if (token) *token = &g_dummyToken;
    return 0;
}

__declspec(dllexport) int32_t WINAPI slIsFeatureLoaded(uint32_t feature, bool* loaded) {
    Log("slIsFeatureLoaded feature=%u\n", feature);
    if (loaded) *loaded = false;
    return 0;
}

__declspec(dllexport) int32_t WINAPI slIsFeatureSupported(uint32_t feature, const void* /*adapterInfo*/) {
    Log("slIsFeatureSupported feature=%u\n", feature);
    return 4; // eError_NoSupportedAdapterFound - feature not available on this HW
}

__declspec(dllexport) int32_t WINAPI slSetConstants(const void* /*consts*/, void* /*token*/, uint32_t viewport) {
    Log("slSetConstants viewport=%u\n", viewport);
    return 0;
}

__declspec(dllexport) int32_t WINAPI slSetFeatureLoaded(uint32_t feature, bool loaded) {
    Log("slSetFeatureLoaded feature=%u loaded=%d\n", feature, (int)loaded);
    return 0;
}

__declspec(dllexport) int32_t WINAPI slSetTag(const void* /*viewport*/, const void* /*tags*/, uint32_t count, void* /*cmdList*/) {
    Log("slSetTag count=%u\n", count);
    return 0;
}

__declspec(dllexport) int32_t WINAPI slSetVulkanInfo(const void* /*info*/) {
    Log("slSetVulkanInfo\n");
    return 0;
}

__declspec(dllexport) int32_t WINAPI slAllocateResources(void* /*cmdList*/, uint32_t feature, const void* /*viewport*/) {
    Log("slAllocateResources feature=%u\n", feature);
    return 1;
}

__declspec(dllexport) int32_t WINAPI slEvaluateFeature(void* /*cmdList*/, uint32_t feature, void* /*token*/, const void* /*inputs*/, uint32_t count) {
    Log("slEvaluateFeature feature=%u count=%u\n", feature, count);
    return 1;
}

__declspec(dllexport) int32_t WINAPI slFreeResources(uint32_t feature, const void* /*viewport*/) {
    Log("slFreeResources feature=%u\n", feature);
    return 1;
}

} // extern "C"

// ─── DllMain ─────────────────────────────────────────────────────────────────

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        char path[MAX_PATH];
        HMODULE hSelf = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)&DllMain, &hSelf);
        GetModuleFileNameA(hSelf, path, sizeof(path));
        LogInit(path);
        Log("=== v4 DllMain ATTACH (sans sl.interposer_real) ===\n");

        // Charger dxgi.dll depuis System32
        char sys32[MAX_PATH];
        GetSystemDirectoryA(sys32, MAX_PATH);
        char dxgiPath[MAX_PATH];
        snprintf(dxgiPath, MAX_PATH, "%s\\dxgi.dll", sys32);
        Log("Loading DXGI from: %s\n", dxgiPath);

        HMODULE hDxgi = LoadLibraryA(dxgiPath);
        if (!hDxgi) { Log("FAILED to load dxgi.dll err=%lu\n", GetLastError()); return FALSE; }

        real_F    = (PFN_CreateDXGIFactory         )GetProcAddress(hDxgi, "CreateDXGIFactory");
        real_F1   = (PFN_CreateDXGIFactory1        )GetProcAddress(hDxgi, "CreateDXGIFactory1");
        real_F2   = (PFN_CreateDXGIFactory2        )GetProcAddress(hDxgi, "CreateDXGIFactory2");
        real_DbgI = (PFN_DXGIGetDebugInterface1    )GetProcAddress(hDxgi, "DXGIGetDebugInterface1");
        Log("DXGI ok: F=%p F1=%p F2=%p\n", real_F, real_F1, real_F2);

        if (!real_F2) { Log("CreateDXGIFactory2 not found\n"); return FALSE; }

        // Charger d3d11.dll et d3d12.dll depuis System32
        char d3d11Path[MAX_PATH], d3d12Path[MAX_PATH];
        snprintf(d3d11Path, MAX_PATH, "%s\\d3d11.dll", sys32);
        snprintf(d3d12Path, MAX_PATH, "%s\\d3d12.dll", sys32);

        HMODULE hD3D11 = LoadLibraryA(d3d11Path);
        HMODULE hD3D12 = LoadLibraryA(d3d12Path);
        Log("D3D11=%p D3D12=%p\n", hD3D11, hD3D12);
        if (hD3D11) InitD3D11Forwarders(hD3D11);
        if (hD3D12) {
            InitD3D12Forwarders(hD3D12);
            // D3D12CreateDevice est géré en C++ (pas trampoline), récupérer manuellement
            real_D3D12CreateDevice = (PFN_D3D12CreateDevice)GetProcAddress(hD3D12, "D3D12CreateDevice");
            Log("real_D3D12CreateDevice=%p\n", real_D3D12CreateDevice);
        }

        // vk* optionnel
        HMODULE hVulkan = LoadLibraryA("vulkan-1.dll");
        if (hVulkan) InitVkForwarders(hVulkan);
    }
    return TRUE;
}
