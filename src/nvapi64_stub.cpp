// Stub nvapi64.dll — masque complètement NVIDIA au jeu
// nvapi_QueryInterface(id) -> NULL = "pas de GPU NVIDIA"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

extern "C" {

// Export principal — toutes les fonctions NVAPI passent par là
__declspec(dllexport) void* __cdecl nvapi_QueryInterface(uint32_t /*id*/) {
    return nullptr;
}

// Certains binaires importent aussi ces exports nommés
__declspec(dllexport) int __cdecl nvapi_Initialize() { return -1; }  // NVAPI_ERROR
__declspec(dllexport) int __cdecl nvapi_Unload()     { return -1; }

} // extern "C"

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
