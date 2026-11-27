# Third-party notices and provenance

No blanket license is assigned to this repository. A license for the project's
own contributions has not been selected. Existing third-party terms remain in
effect; this file does not expand those permissions or transfer ownership.

| Component | Upstream | Included notice |
| --- | --- | --- |
| OpenVR | https://github.com/ValveSoftware/openvr | `third_party/openvr/LICENSE.txt` |
| Detours | https://github.com/microsoft/Detours | `third_party/detours/LICENSE.md` |
| GLEW | https://github.com/Perlmint/glew-cmake | `third_party/glew/LICENSE.txt` |
| GLM | https://github.com/g-truc/glm | `third_party/glm/LICENSE.txt` |

Exact dependency commits are recorded in `third_party/dependency-lock.json`.
Their source notices are retained. OpenVR's official x64 SDK import library and
runtime DLL are included for linking and runtime packaging.

## hYve weapon content

The weapon models, sounds and original weapon behavior come from **hYve mYnd
[TEST]** by **MythRillin** (local author credit: **SnakeyWakey**):
https://steamcommunity.com/sharedfiles/filedetails/?id=2667255756

The Workshop description checked November 26, 2026 permits reuse of its assets
and scripts in maps with attribution. It does not expressly state permission
for a standalone source repository redistribution. Preserve that attribution
and obtain clarification from the creator before publicly redistributing this
content beyond the stated permission. No MIT or other blanket license is
applied to `mods/sledge-vr-hyve/` by this project.

This adaptation separates the weapons from the ragdoll characters and adds VR
integration, grip placement, sights, attachments and input handling. Character
models, ragdoll logic, AI and the rest of the original Workshop map are absent.

## Game integration

Teardown and SteamVR are separate products owned by their respective owners.
This repository contains guarded integration offsets and small reversible Lua
patch hunks. It does not include the game, full game scripts or a license to
redistribute the game. This project is not endorsed by those product owners.
