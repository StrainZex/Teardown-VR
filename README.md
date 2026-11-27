# Teardown VR 2.0

A community VR mod for Teardown, built on Nahu's original work and continued with AI-assisted development. I've wanted to bring this project back for years, and now it's available for everyone to enjoy for free.

**[Download Teardown VR 2.0 — r7.34](https://github.com/StrainZex/Teardown-VR/releases/tag/v2.0-r7.34)**

Choose **Teardown.VR.zip** to install and play, or **Teardown.VR.Source.zip** for the original source archive. The source is also available to browse in this repository. See [BUILDING.md](BUILDING.md) for build instructions. Both downloads are free.

[The story](#the-story-behind-the-mod) · [Headset testing](#headset-and-controller-testing) · [Installation](#installation) · [Performance](#performance-settings) · [How to play](#how-to-play) · [Uninstall](#uninstall) · [Feedback](#feedback-and-community) · [Credits](#credits)

## The story behind the mod

Hey guys! First off, sorry for the delay. I've been busy with work, a family matter came up, and then I got sick. Oh, and I got caught up playing GTA6, so yeah... that didn't help with the delay either. 😅 Thanks for sticking with me—it's taken a while to get here.

Before anything else, **this project would not be possible without Nahu**, who created the original mod. I knew them for a little while before they stepped away and left it unfinished. That was disappointing, of course, but completely understandable. A mod like this is incredibly difficult to make, and I don't blame them at all for deciding not to continue.

Nahu laid the groundwork that made all of this possible. Huge thanks to them for getting it started and giving this project a foundation to build on.

### Why I kept trying

Just so everyone knows, I'm not a coder or a modder. I don't plan to make mods like this regularly, but I'll do my best to keep this one updated. I've wanted to play Teardown in VR since 2021, and I've been trying to get this project made since 2022. My goal is to keep it working as Teardown gets updated, though I can't promise support for every new update right away.

I tried working with people to get the mod finished and spent a lot of money along the way. Unfortunately, those attempts led to delays, unanswered messages, prices I couldn't justify, and experiences where I felt scammed.

At the start of this year, I tried again through Fiverr, Freelancer, and other websites. Months went by without the results I was hoping for. I was getting close to giving up.

### How AI became part of the project

I want to be upfront: **I used AI to help fix and continue this mod. Nahu created the original mod and deserves credit for that work.**

A few months ago, I came across videos about ChatGPT and Claude. I tried Claude first because ChatGPT didn't have the features I needed at the time, but I couldn't get the results I was looking for. I kept working with someone I'd hired, but months went by and I spent more money without seeing real progress.

Then I saw a video about ChatGPT Astra and its ability to control a PC and carry out tasks. I was interested, but also hesitant. I wondered whether I could trust it and whether giving it access to my computer was a good idea. Eventually, I bought a month's subscription and spent weeks trying it out, from coding tasks to animations.

One of those experiments was making animations for my VRChat character. It was fun watching the tool put things together. The results weren't always perfect, but they impressed me enough to try the real test: getting this old Teardown VR mod working again.

I honestly didn't know whether it would be possible. Trying AI was a gamble, but after years of trying to get this project made, it finally helped me get the mod working in my own testing.

I know people have different views on AI, and I share some of those concerns. For me, this felt like a worthwhile use of the tool: helping fix and finish an existing mod that I'd spent years trying to bring back, so the community could finally enjoy it.

### A free project for the community

This mod is **completely free. It won't cost you a single dime.** I've wanted it to be something the community could enjoy together for a long time.

The source code is available alongside the mod download so you can see how it works. No project-wide open-source license has been selected. Please read [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and the included notices before reusing or redistributing any part of it.

I'd love to see people build on the project where the relevant permissions allow it. Please give credit to Nahu's original work and this repository when discussing or sharing the project.

It's been a long time coming. Welcome to **Teardown VR 2.0**—I hope you enjoy it!

## Headset and controller testing

**I've only tested this mod with a Meta Quest 3S headset.** I don't have any other headsets or controllers to test with, so I can't confirm how well it works on other setups.

If you're using a different headset or controllers, please let me know whether the mod works and whether the controls behave as expected. Include the headset and controllers you're using, along with anything that doesn't work, so we can get a better idea of which setups work and which need attention.

## Installation

You'll need a Windows x64 PC, your own Steam copy of Teardown, SteamVR, a connected PC VR headset, and the Microsoft Visual C++ x64 runtime. This package is for single-player use. Headset testing is limited to the Quest 3S, as explained above.

1. Download **Teardown.VR.zip** from the [release page](https://github.com/StrainZex/Teardown-VR/releases/tag/v2.0-r7.34) and save it wherever you'd like, such as your desktop or a folder of your choice.
2. Extract the entire ZIP file, then open the **Teardown VR** folder. Don't run the installer from inside the ZIP.
3. Launch and close Teardown once, then make sure it is closed before installing.
4. Double-click **`Install-VR.cmd`**. It finds Teardown through your Steam libraries and installs the VR mod files.
5. In Steam, open **Teardown → Properties → Launch Options** and enter **`-gfxapi OpenGL`**.
6. If you use a custom mod list, enable **Sledge VR accessories** and **Sledge VR hYve gun pack** in that list. The installer enables them in the Default list.

The package checks the game executable and scripts it supports. If a later Teardown update is rejected, it may need an updated VR build; a matching game version label alone does not guarantee compatibility.

### Optional gun pack and Workshop download

The VR package includes an adapted hYve gun pack. To use it, turn on **hYve gun pack** in the VR settings; it is off by default.

You can also download the original **[hYve mYnd \[TEST\]](https://steamcommunity.com/sharedfiles/filedetails/?id=2667255756)** from the **Teardown Steam Workshop** if you'd like. Credit for the original weapon assets and behavior goes to **MythRillin / SnakeyWakey**; see the included [gun-pack credits](mods/sledge-vr-hyve/CREDITS.txt).

## Performance settings

Before you start playing:

1. Open the folder where you extracted the mod.
2. Run `Performance.bat`.
3. Choose a VR performance setting and try it out to see how it runs on your setup.

**Close Teardown completely every time you want to change the performance setting.** Choose another option in `Performance.bat`, then restart the game to try it.

## How to play

1. Connect your headset using your usual PC VR software.
2. Open the extracted **Teardown VR** folder and double-click **`Enable-VR.cmd`**.
3. Choose your mods and load a map in Teardown. Keep the game window selected for controller input.
4. Wait for the launcher to detect the loaded OpenGL scene and enable VR.

The launcher can start Teardown first, then start SteamVR when the map is ready. If Teardown is already running, it uses that session. You can still load a map yourself first and then run `Enable-VR.cmd`.

On the tested Quest 3S controls, tap **B** for VR settings, double-tap **B** for pause, or hold **B** for the map. Run **`Disable-VR.cmd`** to turn VR off for the current session.

There may be a brief pause while VR starts. Freezing and performance issues are not guaranteed to be resolved on every setup; please report problems with your headset, controllers, game version, and performance setting.

## Uninstall

1. Close Teardown completely.
2. Run **`Uninstall-VR.cmd`** from the extracted folder.

The uninstaller removes the installed VR mods, reverses the known game-script patches, and removes the runtime files from the extracted package. **Extract the original ZIP again if you want to reinstall.**

Modified VR files and unknown game-script versions cause the uninstaller to stop rather than overwrite them. Saves, unrelated mods, and graphics settings are preserved. Any separate Workshop subscription is managed through Steam.

## Feedback and community

I'd love to see the Teardown community come up with creative VR mods for the Workshop and help this project improve.

Got an idea for an update, found a weird bug, or know how to fix something that isn't working? I'd love to hear from you. You can share feedback through this repository's [Issues page](https://github.com/StrainZex/Teardown-VR/issues).

When reporting a problem, please include:

- Your Teardown version.
- Your headset and controllers, including whether the controls work as expected.
- The performance option you selected.
- The other mods you had enabled.
- What happened and the steps that led to it.

I'm not an experienced developer, so I can't promise a fix for every issue or an immediate update. I'll do my best, and help from the community would mean a lot.

## Credits

- **Nahu** — Creator of the original Teardown VR mod. Their work and foundation made this continuation possible.
- **StrainZex** — Continued the project through testing, troubleshooting, and AI-assisted development.
- **MythRillin / SnakeyWakey** — Original hYve weapon assets and behavior. See the [gun-pack credits](mods/sledge-vr-hyve/CREDITS.txt).
- **The community** — Thank you to everyone who shares feedback, ideas, and help.

## Source and build information

See [BUILDING.md](BUILDING.md) for the original source documentation and build commands, [VALIDATION.md](VALIDATION.md) for the supplied validation record, and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for third-party notices. The validation record describes checks performed before this upload, not new tests run during publication.

`SOURCE_MANIFEST.json` describes the original source ZIP. Its original `README.md` is preserved here as `BUILDING.md`; this repository uses a separate player-facing README.
