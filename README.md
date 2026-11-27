# Teardown VR 2.0

A community VR mod for Teardown, built on Nahu's original work and continued with AI-assisted development. I've wanted to bring this project back for years, and now it's available for everyone to enjoy for free.

**[Download Teardown VR 2.0 — r7.34](https://github.com/StrainZex/Teardown-VR/releases/tag/v2.0-r7.34)**

Choose **Teardown.VR.zip** to install and play, or **Teardown.VR.Source.zip** for the original source archive. The source is also available to browse in this repository. See [BUILDING.md](BUILDING.md) for build instructions. Both downloads are free.

[The story](#the-story-behind-the-mod) · [Headset testing](#headset-and-controller-testing) · [Installation](#installation) · [Performance](#performance-settings) · [How to play](#how-to-play) · [Controls](#controls) · [Uninstall](#uninstall) · [Feedback](#feedback-and-community) · [Credits](#credits)

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

See the [full controls guide](#controls) below for movement, tools, grabbing, menus, vehicles, the tablet, holsters, and the hYve gun pack. Run **`Disable-VR.cmd`** to turn VR off for the current session.

There may be a brief pause while VR starts. Freezing and performance issues are not guaranteed to be resolved on every setup; please report problems with your headset, controllers, game version, and performance setting.

## Controls

**Current build: r7.34**

These controls are for **Quest controllers through Virtual Desktop and SteamVR**. Testing has been limited to the Quest 3S headset.

Older compatibility builds may have different controls.

### Movement and tools

- Left stick forward/backward: Walk forward or backward.
- Left stick left/right: Strafe left or right.
- Hold left stick clicked in: Sprint using the game’s Shift action.
- Right stick left/right: Smooth turn.
- Right stick up/down: Scroll through weapons and tools. Hold it to continue scrolling.
- Hold right stick clicked in: Crouch.
- A: Jump.
- Right trigger: Fire or use the equipped tool’s primary action.
- Left trigger: Use the equipped tool’s secondary/right-click action, where supported.
- Tap X with no interaction prompt: Reload weapons that use R. Reload happens when X is released.
- X at an ordinary interaction prompt: Interact immediately, including opening the campaign computer.
- Y: Toggle the game flashlight.
- Move your right controller: Aim the equipped weapon or tool.

### Grabbing, throwing, and physical interactions

The following controls use “Grab mode: separate hands.”

- Hold left grip: Grab an eligible object with your left hand.
- Hold right grip: Grab an eligible object with your right hand.
- Release a grip: Release that hand’s object.
- Move your hand and release its grip: Throw using your hand’s movement.
- While gripping an object, hold that hand’s trigger, then release the trigger: Charge and throw in that controller’s pointing direction. Charge increases for approximately one second.
- Grip the same object with both hands: Hold it with both hands. Releasing one hand leaves the other attached.
- Bring your left hand to the equipped tool and hold left grip: Support the tool with your second hand.
- Release left grip: Release the supporting hand.
- Push with your hands or tools: Physical contact when “Experimental physical contact” is enabled.
- Swing a recognized melee tool: Activate its normal attack when “Physical melee” is enabled.
- Punch with an empty hand: Perform weak physical melee when that option is enabled.
- Select “VR hands” from the tool selection: Use your hands without an equipped weapon model.

In “normal game grab” mode, either grip activates the game’s normal right-click action instead of independent hand grabbing. Behavior can differ between tools.

### B button, menus, and recentering

- Tap B once: Open or close VR settings. There is a short delay to distinguish a double tap.
- Double-tap B quickly: Open or close the game pause menu.
- Hold B for one second: Toggle the map.
- Hold B while using the tablet: Toggle the tablet map page, except while its spawn page is active.
- Point either controller at VR settings and press its trigger: Select the pointed setting.
- Point the right controller at ordinary game menus and press right trigger: Click buttons, including campaign computer level selections.
- Hold the pointer trigger while moving: Hold the mouse button for controls that support dragging.
- Thumbstick up/down while pointing at a game menu: Scroll continuously. Ordinary menus accept either stick.
- X inside an ordinary game menu: Back or close using the game’s Escape action.
- X while using the campaign computer: Exit the computer.
- Hold both thumbsticks clicked in for 1.5 seconds: Recenter head position and heading.
- Keyboard F8: Recenter immediately.
- VR menu → Recenter position and heading: Recenter through the menu.

The mod also detects SteamVR tracking-origin resets.

### Vehicles and mounted controls

These mappings apply when the game recognizes you as controlling a vehicle or mounted object.

- Hold X for one second at a vehicle-entry prompt: Enter. Keep pointing at the same target while holding.
- Tap X while inside: Exit.
- Right trigger: Accelerate or drive forward.
- Left trigger: Reverse.
- Left stick left/right: Steer.
- A: Send the vehicle’s Space action—normally brake or handbrake.
- Right stick left/right: Turn the vehicle camera.
- Hold right stick up/down: Continuously adjust vehicle camera zoom, including moving into or out of first person where supported.
- Right grip: Vehicle primary/left-click action.
- Left grip: Vehicle secondary/right-click action.
- Y: Send the game’s flashlight/F action.

The exact primary, secondary, and Space actions depend on the vehicle.

Physical steering-wheel grabbing is not mapped in this build. Steering uses the left stick.

### Vehicle camera adjustment

- Select “Adjust vehicle camera (3 seconds)”: Recalibrate seated camera height after three seconds.
- Select “Vehicle / mounted camera: left stick” while inside: Enter manual adjustment mode.
- Left stick up/down during adjustment: Move the camera up or down.
- Left stick right during adjustment: Move the camera forward.
- Left stick left during adjustment: Move the camera backward.
- Tap B or select “Finish camera adjustment (save)”: Finish and save.
- Select “Reset vehicle camera position”: Clear the vehicle camera offsets.

### Tablet

- Reach to the outside of your left thigh: Find the tablet. A vibration signals proximity.
- Press left grip near it: Pick it up. Another vibration confirms the grab.
- Press right grip near it while “VR hands” is selected: Pick it up with your right hand instead.
- Point your free controller at the screen and press its trigger: Select tablet buttons.
- Select “Map”: Display the game map on the tablet.
- Select “Spawn menu”: Open the spawn interface on the tablet.
- Move your free hand’s stick up/down while pointing: Scroll the displayed menu.
- Click the search field: Bring up the tablet keyboard.
- Point at keyboard keys and press the trigger: Type letters or numbers, or select Space, Backspace, and Enter.
- X on a tablet page: Leave that page and return toward the tablet home screen.
- Release the holding grip outside an active page: Drop the tablet.
- Move away from a dropped tablet: It returns to your left thigh once sufficiently far away.

### Shoulder holsters

- Hold right grip, move the equipped tool behind either shoulder, then release right grip: Holster that tool.
- Reach to the occupied shoulder and press either grip: Retrieve the holstered tool.
- VR menu → Retrieve holstered tool: Emergency retrieval if reaching it is difficult.

While a tool is holstered, the mod selects “VR hands” and locks normal tool scrolling until retrieval.

### hYve gun pack

- VR menu → hYve gun pack ON: Add its five weapons alongside your normal tools.
- Right stick up/down: Select the Glock, G36K, Magnum, Shotgun, or Flamethrower.
- Right trigger: Fire the selected weapon. Holding repeats fire at its configured rate.
- Successfully support the Glock with left grip, then press left trigger: Toggle its flashlight.
- Successfully support the G36K with left grip, then press left trigger: Toggle its red laser dot.
- Release and press left trigger again while supporting: Toggle the attachment again.

Merely holding left grip away from the weapon does not activate its attachment toggle.

### VR settings menu options

- Head follows view: Enable or disable head-directed body heading.
- Smooth turn speed: Change on-foot turning speed.
- Recenter position and heading: Reset your VR position and facing direction.
- Open game pause menu: Open the game’s pause menu.
- Point at game menus / campaign computer: Manually enable the game-menu pointer.
- Rendering: Switch between stereoscopic rendering and single-pass compatibility without stereo depth.
- VR HUD: Show or hide the headset HUD.
- Grab mode: Switch between separate hands and normal game grab.
- Experimental physical contact: Enable or disable hand/tool pushing interactions.
- VR hands plus gun pack: Restrict the selectable inventory to hands and the gun pack.
- Vehicle turn speed: Change vehicle camera turning speed.
- Adjust vehicle camera (3 seconds): Recalibrate seated camera height after a three-second delay.
- Reset vehicle camera position: Clear vehicle camera offsets.
- God mode: Prevent native player damage.
- Unlimited native ammo: Preserve ammo handled by the game’s ammo system. Custom mod ammo counters may differ.
- Retrieve holstered tool: Recover a holstered tool without reaching behind you.
- Return tablet to home: Return the tablet interface to its home page.
- Physical melee: Enable or disable swing and punch attacks.
- Normal VR view / Wider VR view: Change the VR view width.
- Swing attack for this tool: Cycle AUTO → ON → OFF for the selected tool.
- Close settings: Dismiss VR settings.
- Vehicle / mounted camera: left stick: Enter manual vehicle camera adjustment. While adjusting, this changes to “Finish camera adjustment (save).”
- hYve gun pack: Enable or disable the pack in the normal inventory.

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
