/**
 * GFSDK_Aftermath_Lib.x64.dll — stub complet
 * Désactive NVIDIA Aftermath sur les systèmes sans GPU NVIDIA compatible
 * Retourne GFSDK_Aftermath_Result_NotAvailable (0x20070002) pour Initialize
 * et 0 pour les autres fonctions.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>

// Valeurs GFSDK_Aftermath_Result
static const uint32_t AFTERMATH_NOT_AVAILABLE = 0x20070002;
static const uint32_t AFTERMATH_OK            = 0x20070001;

extern "C" {

// DX11/DX12 Initialize — retourner NotAvailable pour que le jeu ne continue pas
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_DX11_Initialize(uint32_t, uint32_t, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_DX12_Initialize(uint32_t, uint32_t, void*) { return AFTERMATH_NOT_AVAILABLE; }

// CreateContextHandle
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_DX11_CreateContextHandle(void*, void**) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_DX12_CreateContextHandle(void*, void**) { return AFTERMATH_NOT_AVAILABLE; }

// DX12 resource tracking
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_DX12_RegisterResource(void*, void*, void**) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_DX12_UnregisterResource(void*) { return AFTERMATH_NOT_AVAILABLE; }

// GPU crash dumps
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_DisableGpuCrashDumps() { return AFTERMATH_OK; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_EnableGpuCrashDumps(uint32_t, uint32_t, void*, void*, void*, uint32_t, void*) { return AFTERMATH_NOT_AVAILABLE; }

// Context queries
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GetContextError(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GetData(uint32_t, const void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GetDeviceStatus(void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GetPageFaultInformation(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }

// Shader debug
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GetShaderDebugInfoIdentifier(uint32_t, const void*, uint32_t, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GetShaderDebugName(const void*, uint32_t, void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GetShaderDebugNameSpirv(const void*, uint32_t, void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GetShaderHash(const void*, uint32_t, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GetShaderHashSpirv(const void*, uint32_t, void*) { return AFTERMATH_NOT_AVAILABLE; }

// Crash dump decoder
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_CreateDecoder(uint32_t, const void*, uint32_t, void**) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GenerateJSON(void*, uint32_t, uint32_t, void*, void*, void*, void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfo(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfoCount(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetDescription(void*, uint32_t, uint32_t, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(void*, uint32_t, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetDeviceInfo(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetEventMarkersInfo(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetEventMarkersInfoCount(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetGpuInfo(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetGpuInfoCount(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetJSON(void*, uint32_t, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetPageFaultInfo(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_GpuCrashDump_GetSystemInfo(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }

// Context management
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_ReleaseContextHandle(void*) { return AFTERMATH_OK; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_SetEventMarker(void*, const void*, uint32_t) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GFSDK_Aftermath_SetShaderDebugInfoPaths(void*, void*) { return AFTERMATH_NOT_AVAILABLE; }

// Non-prefixed exports (indices 36-38)
__declspec(dllexport) uint32_t WINAPI GetShaderDebugName(const void*, uint32_t, void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GetShaderDebugNameSpirv(const void*, uint32_t, void*, void*) { return AFTERMATH_NOT_AVAILABLE; }
__declspec(dllexport) uint32_t WINAPI GetShaderHashSpirv(const void*, uint32_t, void*) { return AFTERMATH_NOT_AVAILABLE; }

} // extern "C"

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
