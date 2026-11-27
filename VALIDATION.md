# Source archive validation

Checked November 26, 2026 on Windows x64 using Visual Studio 2022 and Python.

- Compiled all three runtimes from the included source and dependency files,
  without using pre-existing project object files or game executables.
- Ran 276 stereo, camera, input and signature-guard checks. Default signature
  tests used synthetic data; they do not establish compatibility with an
  unknown real executable.
- Ran 22 grab/contact-path checks with mocked engine APIs, nonblocking pose
  snapshot and avatar checks, and player-assist checks.
- Passed the live OpenGL both-eye HUD, controller cursor and state-restoration test.
- Passed Lua accessory/menu and weapon-behavior tests, including buffered firing,
  complete shotgun volleys, original impacts/tracers and bounded inventory writes.
- Passed tablet asset checks and 12 graphics-preset/restore checks.
- Generated the portable runtime ZIP from this source tree.
- All packaged PowerShell scripts passed syntax parsing.

This source export changes packaging and build/test paths, not native gameplay
code or the mod's Lua/model/sound content. Those files match the existing r7.34
source and release. Source archive hashes are listed in `SOURCE_MANIFEST.json`.

No new headset session was performed for this source export. The last available
r7.34 game log still contained frame gaps of at least 250 ms and watchdog warnings.
Loading/focus changes can contribute to those measurements; their cause is not
established by the log alone. Freezing is not claimed resolved. Private session
logs and game-derived analysis fixtures are not included.
