========================================================
Cyberpunk 2077 - Ray Tracing Initialization Error Fix
Version : 1.0
Compatibility : Cyberpunk 2077 v2.21+ (GOG, Steam, Epic)
========================================================

PROBLEM SOLVED
--------------
On systems with a DXR-capable primary GPU + NVIDIA GPU (secondary),
Cyberpunk 2077 shows this dialog at startup:
  "Problem during ray-tracing initialization"
  "The current GPU may be incompatible [...]"

This fix permanently removes that dialog.

INSTALLATION
------------
1. Extract the archive to a temporary folder
2. Double-click : install.bat
3. Follow the on-screen instructions
4. Launch Cyberpunk 2077 normally

The installer will:
  - Auto-detect your game folder (GOG / Steam / Epic)
  - Back up your original files to bin\x64\backup_rtfix\
  - Install the fix DLLs
  - Patch UserSettings.json (DLSS_UNSUPPORTED flag)

UNINSTALLATION
--------------
Double-click : uninstall.bat
All original files will be restored from the backup.

HOW IT WORKS
------------
The fix operates at 3 levels:

1. sl.interposer.dll  (NVIDIA Streamline proxy)
   - Hooks DXGI EnumAdapters : hides the NVIDIA GPU from the game
   - Hooks D3D12Device::CheckFeatureSupport : returns RaytracingTier=NOT_SUPPORTED
   - Stubs all Streamline entry points (slInit, slIsFeatureSupported, etc.)

2. nvapi64.dll  (NvAPI stub)
   - Prevents any NvAPI calls from reaching the NVIDIA GPU

3. GFSDK_Aftermath_Lib.x64.dll  (Aftermath stub)
   - Disables the NVIDIA crash reporter

4. UserSettings.json
   - DLSS_UNSUPPORTED = true  (prevents DLSS initialization attempts)

ROOT CAUSE
----------
Any DXR-capable primary GPU natively supports hardware ray tracing.
Cyberpunk 2077 detects this via D3D12Device::CheckFeatureSupport(OPTIONS5),
then tries to initialize ray tracing through NVIDIA Streamline.
Since no NVIDIA primary GPU is present, Streamline fails (slInit error 4),
which triggers the dialog. This fix masks DXR support so the game never
attempts that initialization path.

REQUIREMENTS
------------
- Windows 10 / 11
- Cyberpunk 2077 v2.21 or later
- A DXR-capable GPU as primary adapter (see compatibility table below)
- Any NVIDIA GPU present as secondary adapter

COMPATIBILITY
-------------
The fix is GPU-agnostic by design. It filters adapters by vendor ID (0x10DE
for NVIDIA) and patches the standard D3D12 vtable offset, which never
changes regardless of GPU brand or model.

Primary GPU (non-NVIDIA, must support DXR hardware ray tracing):
  [OK] AMD Radeon RX 6000 series  (RDNA 2, DXR 1.1)  - tested RX 6800
  [OK] AMD Radeon RX 7000 series  (RDNA 3, DXR 1.1)  - expected
  [?]  Intel Arc  (Xe-HPG, DXR 1.1)                  - should work, untested
  [--] AMD Radeon RX 5000 series  (RDNA 1, no DXR)   - fix not needed
  [--] AMD Radeon RX 580 / Vega   (no DXR)            - fix not needed

Secondary GPU (any NVIDIA GPU that triggers the issue):
  [OK] NVIDIA GTX 1660  - tested
  [OK] Any NVIDIA GPU   - expected (fix is vendor-ID based, not model-based)

Store platforms:
  [OK] GOG
  [OK] Steam
  [OK] Epic Games

NOTE: If your primary GPU does NOT support DXR (e.g. RX 580), you will
never see this dialog and this fix is unnecessary.

NOTES
-----
- No game files are modified irreversibly
- Original files are backed up before any replacement
- Compatible with mods (CET, RED4ext, etc.)
- No network connection required

SOURCE CODE
-----------
Full source code is available in the src/ folder.
Build with MinGW-w64 under Linux / WSL:
  apt install mingw-w64
  cd src && bash build.sh
