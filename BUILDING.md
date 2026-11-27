# Teardown VR

Source for the experimental Sledge-based Teardown VR bridge, revision **r7.34**.
This is the source used for the current VR adaptation and the retained compatibility
runtimes, not a recovered archive of the original Sledge 1.0 source. It is an
unofficial Windows x64, single-player mod using OpenGL and SteamVR/OpenVR.

## Build

Install Visual Studio 2022 or its Build Tools with **Desktop development with C++**
and a Windows SDK, plus Python 3.10 or newer. The exact native dependency files
used by this project are included, with their notices and upstream commit IDs.
No game installation or headset is needed to compile.

From this folder:

```bat
Build.cmd
Build.cmd --all
Build.cmd --package
```

The first builds the current runtime. `--all` also builds the two retained game
versions. `--package` builds all three and creates `dist/Teardown VR.zip`, including
the portable installer, launchers, accessories, gun pack, checksums and credits.
Extract that release ZIP and follow its `README.txt` to install and launch.
Building the source does not install anything or change your game.

For an existing **x64 Native Tools Command Prompt for VS 2022**, the equivalent is:

```bat
python tools/build.py --package
```

Generated objects, logs and DLLs go under `build/`. They are ignored by Git.
Build commands use relative source paths and do not generate private PDB paths.

## Tests

```bat
py -3 -m pip install -r tests/requirements.txt
Build.cmd --test
Build.cmd --test --test-opengl
```

Tests cover stereo/camera/input maths, signature rejection, contact and grabbing,
nonblocking pose snapshots, player assists, Lua accessory and gun behavior,
tablet geometry, and graphics preset restoration. `--test-opengl` additionally
needs a working desktop OpenGL context to verify both-eye HUD rendering and
cursor/state restoration. Lua tests use a mocked engine, not the game itself.

Signature tests default to synthetic data. For optional checks against your own
two executable files, run `build/tests/stereo_tests.exe OLD_EXE CURRENT_EXE`.
Do not commit game executables, memory dumps or analysis fixtures.
Real game compatibility, headset alignment and performance still need a fresh
in-game test. Offline checks cannot prove that freezing has been eliminated.

To inspect an actual session log:

```powershell
powershell -NoProfile -File tests/check_freeze_log.ps1 -LogPath "PATH_TO_LOG"
```

## Supported executable snapshots

| Source | Executable | Runtime revision |
| --- | --- | --- |
| `src/current` | Teardown 2.2, Steam build 26758720 | r7.34 |
| `src/compat/26320217` | Teardown 2.2, Steam build 26320217 | r7.23 |
| `src/compat/legacy21` | Retained Teardown 2.1 executable | r7.20 |

The compatibility directories are source for supported game versions, not saved
user profiles or backups. Older runtimes keep their older feature sets. The hYve
gun pack needs the 2.2 bridge. Native signature checks reject unknown builds;
the installer separately requires recognized game-script hashes. Version labels
alone are not sufficient to establish compatibility with another game update.

## Repository contents

- `src/`: renderer bridge, launcher, VR input, HUD, hands, contacts and menus.
- `mods/sledge-vr-accessories/`: tablet/accessories Lua and voxel model.
- `mods/sledge-vr-hyve/`: VR gun pack scripts, prefabs, weapon models and sounds.
- `package/`: install/uninstall/launch/performance scripts and reversible game-script patches.
- `third_party/`: minimal native build dependencies and original license notices.
- `tests/` and `tools/`: automated checks and portable build/package tooling.

The OpenVR SDK includes its official x64 import library and runtime DLL; those
are the only prebuilt native dependencies in this source archive. No Teardown
executable, full base-game scripts, saves, account data, user settings, logs,
development backups, compiled VR bridge or private debug symbols are included.

## Sharing on GitHub

Extract this archive and use the contents of this folder as the repository root.
Keep `.gitignore`, this README, credits, and all third-party license files.
The source archive does not contain Git history, credentials or a configured remote.
Put compiled release ZIPs in GitHub Releases rather than committing build output.

No new project-wide open-source license has been selected. See
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) before choosing one. In particular,
the gun pack is adapted from another creator's work; the cited permission covers
reuse in maps with credit and does not expressly address a standalone GitHub
redistribution. Do not describe it as your original work or relicense it under
a blanket license without the relevant permission.
