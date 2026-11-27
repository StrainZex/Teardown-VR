Teardown VR - r7.34

Windows x64, single-player. Requires your own Steam copy of Teardown,
SteamVR, a connected PC VR headset and the Microsoft Visual C++ x64 runtime.

SETUP
1. Extract this entire folder somewhere writable. Do not run from inside the ZIP.
2. Launch and close Teardown once, then close it before installation.
3. Double-click Install-VR.cmd. It finds Teardown through Steam's libraries,
   installs the two included VR mods and applies small checked game-script patches.
4. In Steam > Teardown > Properties > Launch Options, enter: -gfxapi OpenGL
5. Double-click Enable-VR.cmd and load a level. The launcher starts Teardown
   before SteamVR, then enables VR. Connect the headset with your usual PC VR software.
6. If you use a different mod list, enable Sledge VR accessories and Sledge VR
   hYve gun pack in that list. The installer enables them in the Default list.

Tap B for VR settings; double-tap B for pause; hold B for the map.
Enable hYve gun pack in VR settings to use the included weapons. Its default
is off; no personal VR settings are supplied. The isolated hands/gun-pack mode
is optional. Performance.bat provides optional graphics presets.
Disable-VR.cmd turns VR off for the current session.
Uninstall-VR.cmd removes the installed VR mods, reverses the two known game
patches and removes this folder's runtime binaries. Extract the ZIP again to reinstall.
Unknown game-script versions or modified mod files are not overwritten by uninstall.

SUPPORTED EXECUTABLES
Steam build 26758720 (current r7.34), Steam build 26320217 (retained r7.23),
and the retained 2.1 executable (r7.20). Older runtimes retain their older
feature set; the hYve gun pack requires the 2.2 bridge. Native signatures are
checked before attaching. The installer also requires matching game scripts.
Other updates may require a new VR build. No game executable is supplied.

CONTENTS
Only VR launch/install/uninstall/performance scripts, runtime binaries,
the accessories/tablet mod, the extracted hYve weapon pack, reversible patches,
file checksums and required credits/licenses are included.
No saves, account information, user profiles, personal settings, logs, backups,
full game scripts, Workshop library, development source or debug symbols are included.
Personal build-path metadata has been removed from the VR binaries.

Installation creates restore copies from the recipient's own game under
LocalAppData/TeardownVR. Runtime logs/settings may be created after use; none
are present in this ZIP. Saves and unrelated mods are not included or replaced.

The current gun pack uses original hYve bullets with buffered trigger taps,
complete shotgun volleys and the green Glock sight. The latest stall-reduction
changes are retained, but freezing is not guaranteed resolved on every setup.

Weapon assets and original behavior: MythRillin / SnakeyWakey, hYve mYnd [TEST].
See the weapon pack's CREDITS.txt and the licenses folder for third-party notices.
