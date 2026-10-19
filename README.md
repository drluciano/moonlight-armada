# Moonlight Armada

[![ArmadaOS AppImage](https://img.shields.io/github/actions/workflow/status/drluciano/moonlight-armada/build-armada-appimage.yml?branch=main&label=ArmadaOS%20AppImage)](https://github.com/drluciano/moonlight-armada/actions/workflows/build-armada-appimage.yml)
[![License: GPL v3](https://img.shields.io/badge/license-GPLv3-blue.svg)](LICENSE)

**Moonlight Armada** is a game streaming client for Snapdragon handhelds running [ArmadaOS](#about-armadaos). It is built on [moonlight-qt](https://github.com/moonlight-stream/moonlight-qt) and adds an **ultra low latency decoding mode** for Qualcomm's hardware video decoders, a **modernized interface**, and **one-file AppImage builds** made for ArmadaOS.

The ultra low latency mode is a Linux port of the Snapdragon low latency work in [ClassicOldSong/moonlight-android](https://github.com/ClassicOldSong/moonlight-android). On an AYN Odin running ArmadaOS in Desktop Mode, H.264 decodes in **under 1 ms per frame**.

![The Computers screen in Moonlight Armada](docs/screenshots/computers.png)

## Contents

- [Highlights](#highlights)
- [Screenshots](#screenshots)
- [Installing on ArmadaOS](#installing-on-armadaos)
- [Using ultra low latency mode](#using-ultra-low-latency-mode)
- [How ultra low latency mode works](#how-ultra-low-latency-mode-works)
- [The Iris kernel patch](#the-iris-kernel-patch)
- [The new interface](#the-new-interface)
- [Building from source](#building-from-source)
- [How the AppImage is packaged](#how-the-appimage-is-packaged)
- [Troubleshooting](#troubleshooting)
- [Roadmap](#roadmap)
- [What changed from moonlight-qt](#what-changed-from-moonlight-qt)
- [Credits and license](#credits-and-license)

## Highlights

- **Ultra low latency decoding.** One switch tunes the whole decode path for Snapdragon's V4L2 hardware decoders (Venus and Iris). It asks the decoder to return frames as soon as they are decoded, enables any vendor low latency controls the driver exposes, turns off frame pacing, and runs the decoder thread at real-time priority.
- **Works on today's ArmadaOS kernel.** Mainline Iris doesn't yet expose the decode order control, so Moonlight Armada detects that and prefers H.264, which outputs frames immediately without it. A [kernel patch](contrib/kernel/) is included for the ArmadaOS team to add the control.
- **Honest diagnostics.** The performance overlay shows exactly what ultra low latency mode could enable on your hardware.
- **Modernized interface.** A new dark theme, card-based layout, status chips, and a cleaner toolbar and settings page, designed for handheld screens and gamepad navigation.
- **One-file install.** Each build is a single aarch64 AppImage that runs on ArmadaOS without installing anything.
- **Automated builds.** GitHub Actions builds, smoke-tests and publishes the AppImage on every change.

## Screenshots

These were taken with three example computers saved and no host running, so they show as Offline.

| Computers | Settings |
|---|---|
| ![Computers screen with host cards and status chips](docs/screenshots/computers.png) | ![Settings with the new Latency card and ultra low latency enabled](docs/screenshots/settings.png) |

### Before and after

Stock moonlight-qt 6.2.0 (left) next to Moonlight Armada (right), with the same saved computers and settings:

| Stock moonlight-qt | Moonlight Armada |
|---|---|
| ![Stock moonlight-qt Computers screen](docs/screenshots/stock-computers.png) | ![Moonlight Armada Computers screen](docs/screenshots/computers.png) |
| ![Stock moonlight-qt Settings screen](docs/screenshots/stock-settings.png) | ![Moonlight Armada Settings screen](docs/screenshots/settings.png) |

## Installing on ArmadaOS

1. Download the latest `Moonlight-Armada-*-aarch64.AppImage`:
   - From [Releases](https://github.com/drluciano/moonlight-armada/releases) for tagged versions, or
   - From the **Artifacts** section of the latest [ArmadaOS AppImage build](https://github.com/drluciano/moonlight-armada/actions/workflows/build-armada-appimage.yml). GitHub wraps artifacts in a zip, so unzip it first.
2. Switch to Desktop Mode, open a terminal in the download folder, and make it executable:
   ```sh
   chmod +x Moonlight-Armada-*-aarch64.AppImage
   ```
3. Run it by double-clicking it or from the terminal:
   ```sh
   ./Moonlight-Armada-*-aarch64.AppImage
   ```
4. To launch it from Game Mode, see [Adding it to Game Mode](#adding-it-to-game-mode).
5. Pair with your gaming PC running [Sunshine](https://github.com/LizardByte/Sunshine) (or GeForce Experience), the same as stock Moonlight.

If your system can't mount AppImages (no FUSE), run it with `--appimage-extract-and-run`.

### Adding it to Game Mode

Open the AppImage once in Desktop Mode (if double-clicking does nothing, right-click it, choose **Properties → Permissions**, tick **Is executable**, and try again). The first time it opens, it installs itself:

- it copies the app to `~/.local/share/moonlight-armada`, so Steam launches a normal installed app rather than the AppImage,
- it adds **Moonlight Armada** to the app menu,
- and it adds **Moonlight Armada** to Steam as a non-Steam game for every Steam account on the device. If Steam is open, it closes Steam while it does this and reopens it afterwards, because Steam would otherwise overwrite the change. A backup of Steam's shortcut list is kept next to it as `shortcuts.vdf.armada-backup`.

To update, open a newer AppImage once the same way. If the Steam entry is missing, use **Add a Non-Steam Game** in Desktop Mode and tick **Moonlight Armada**.

If it still doesn't start:

- In the shortcut's **Properties → Compatibility**, make sure **Force the use of a specific Steam Play compatibility tool** is off. Moonlight is a native Linux app, and Proton can't run it.
- Check `~/.cache/moonlight-armada/last-run.log`. Every launch writes it, so if it wasn't updated, Steam never reached the launcher.

In Game Mode the launcher runs Moonlight on X11, because gamescope only shows X11 windows. Set `QT_QPA_PLATFORM` in the launch options to override that.

## Using ultra low latency mode

1. Open **Settings**. The **Latency** card is at the top.
2. Turn on **Ultra low latency decoding (experimental)**.
3. Optionally, turn on **Show performance stats while streaming** to see what was enabled.
4. Start a stream.

Turning on ultra low latency mode also turns off and greys out **frame pacing**, since frame pacing deliberately holds frames back to smooth delivery.

From the command line, use `--ultra-low-latency` or `--no-ultra-low-latency` with `moonlight stream`.

### Reading the performance overlay

The overlay gains an **Ultra low latency** line that says what took effect:

| Overlay text | Meaning |
|---|---|
| `active, Decode order output (qcom-venus)` | The decoder was switched to decode order output (and any vendor low latency controls listed). This is the full mode. |
| `active, H.264 fast path (iris_driver)` | The decoder has no decode order control (mainline Iris today), but the stream is H.264, which already outputs frames immediately. Still the full benefit. |
| `partial, iris_driver lacks decode order control` | HEVC or AV1 on a decoder without the control. Pacing and thread priority still apply, but the decoder may hold frames. Use H.264 or apply the [kernel patch](#the-iris-kernel-patch). |
| `active (generic)` | A non-V4L2 decoder (for example VA-API or software). Only the generic tuning applies. |

The **Video decoder** line shows which decoder is in use and whether it's hardware or software. On Snapdragon it should read `h264_v4l2m2m (hardware)` or `hevc_v4l2m2m (hardware)`.

The **average decoding time** line is the number to watch. On an AYN Odin with mainline Iris in Desktop Mode, H.264 decodes in under 1 ms. Keep in mind that this is decode time only. Network and display time are added on top of it.

> **Game Mode vs Desktop Mode.** Stock Moonlight on ArmadaOS measures 6–7 ms decode time in Game Mode but under 1 ms in Desktop Mode, so the session you stream from matters as much as the decoder settings. We're still working out why, and how much ultra low latency mode adds on top of each. Check the **Video decoder** line first: a `(software)` decoder in Game Mode would explain the gap.

## How ultra low latency mode works

### From Android to Linux

On Android, the Snapdragon low latency mode in moonlight-android sets Qualcomm's MediaCodec vendor keys. On Linux, the same hardware is driven through V4L2 stateful (memory-to-memory) decoders, which FFmpeg exposes as `h264_v4l2m2m` and `hevc_v4l2m2m`. Moonlight Armada maps each Android setting to its Linux counterpart:

| Android (MediaCodec) | Linux (V4L2) | What it does |
|---|---|---|
| `vendor.qti-ext-dec-picture-order.enable` | `V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY_ENABLE` = 1 with `..._DISPLAY_DELAY` = 0 | Return frames in decode order as soon as they're decoded, instead of holding them for display order |
| `vendor.qti-ext-dec-low-latency.enable` | Any boolean driver control whose name contains "low latency" or "decode order" | Vendor kernels (such as Qualcomm's downstream `msm_vidc`) expose their low latency switches as private controls |
| `KEY_PRIORITY` / operating rate | Decoder thread at `SDL_THREAD_PRIORITY_TIME_CRITICAL` | Keep the decode thread from being preempted |
| (pacing off in the Android client) | Frame pacing disabled | Present each frame as soon as it's ready |

Moonlight never sends B-frames, so decode order is the same as display order and this tuning doesn't change the picture.

### Setting controls behind FFmpeg's back

FFmpeg doesn't expose V4L2 controls, and V4L2 controls belong to the file handle that opened the decoder. So `app/streaming/video/v4l2lowlatency.cpp`:

1. Records the open `/dev/video*` file descriptors (from `/proc/self/fd`) just before `avcodec_open2()`.
2. After the decoder opens, finds the new descriptor and confirms it's an M2M decoder with `VIDIOC_QUERYCAP`.
3. Sets the display delay controls, then walks every control on the device (`V4L2_CTRL_FLAG_NEXT_CTRL`) to enable matching vendor low latency toggles.

This works because FFmpeg's v4l2m2m wrapper only starts streaming when the first packet arrives, so controls set right after opening still take effect.

### Mainline Iris and the H.264 fast path

The Snapdragon 8 Gen 2, 8 Gen 3 and 8 Elite use Qualcomm's newer **Iris** video core. The mainline `iris` driver, which reports itself as `iris_driver`, doesn't yet expose the display delay controls, so the decode order switch isn't available on today's ArmadaOS kernel.

For H.264, Moonlight already rewrites each stream's SPS to set `max_dec_frame_buffering = 1`, so the decoder outputs every frame immediately anyway. When ultra low latency mode is on, HDR is off, and an Iris decoder without the control is found, Moonlight Armada therefore moves H.264 to the front of the codec list. HEVC needs the kernel patch to get the same behavior.

### What it does per decoder

| Decoder | H.264 | HEVC |
|---|---|---|
| Venus (`qcom-venus`, older Snapdragons) | Full mode | Full mode |
| Iris, mainline (`iris_driver`) | Full mode via the H.264 fast path (preferred automatically) | Partial |
| Iris with the [kernel patch](#the-iris-kernel-patch) | Full mode | Full mode |
| Downstream `msm_vidc` vendor kernels | Full mode, plus vendor low latency controls | Full mode, plus vendor low latency controls |
| Any other decoder | Generic tuning | Generic tuning |

## The Iris kernel patch

[`contrib/kernel/0001-media-qcom-iris-add-decode-order-output-for-gen2-dec.patch`](contrib/kernel/) adds the standard V4L2 display delay controls to the mainline Iris decoder on gen2 SoCs (SM8550, SM8650, SM8750 and others sharing the `inst_fw_cap_sm8550_dec` table). When display delay is enabled and set to 0, the driver sends `HFI_PROP_DECODE_ORDER_OUTPUT` (`0x0300015b`) to the firmware. That's the same property Qualcomm's downstream `msm_vidc` driver uses for this feature.

With the patch applied, Moonlight Armada sets the controls automatically and HEVC gets the full mode too. No app change is needed.

The patch is compile-tested for arm64 against mainline with `W=1`, but not yet tested on hardware. It's meant for the ArmadaOS kernel maintainers, and it needs a `Signed-off-by` from whoever submits it upstream. See [`contrib/kernel/README.md`](contrib/kernel/README.md).

## The new interface

The interface keeps Moonlight's layout and gamepad navigation but restyles everything:

- **Theme.** `app/gui/Theme.qml` is a singleton design system: a deep blue-black palette (`#0D1017` to `#151A2B`), raised surfaces, a cyan accent (`#4FC3F7`), and shared corner radii, spacing, type sizes and animation timings. Qt's Material style is tinted to match.
- **Cards.** `Card.qml` and `NavigableItemDelegate.qml` give computers and apps rounded cards with a hover and focus lift and a clear accent focus ring for gamepad and keyboard navigation.
- **Status chips.** `StatusChip.qml` shows each computer's state at a glance: Checking…, Online, Not paired or Offline. Running apps get a "Running" chip.
- **Toolbar.** The toolbar is now a flat surface with a hairline divider, the logo, a left-aligned page title, a "Moonlight Armada" caption, and a muted version label.
- **Settings.** Settings are grouped into rounded `SettingsCard` panels with accent titles, and collapse to a single column under 900 px wide. A new **Latency** card at the top holds the ultra low latency switch.
- **Dialogs.** Modal dialogs dim the background with a darker scrim.

## Building from source

### Requirements

Moonlight Armada builds the same way as moonlight-qt. On Ubuntu 24.04 (arm64 or x86_64), install:

```sh
sudo apt install build-essential pkg-config \
  qt6-base-dev qt6-declarative-dev qt6-svg-dev qt6-wayland qt6-qpa-plugins qt6-image-formats-plugins \
  qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-templates qml6-module-qtquick-layouts \
  qml6-module-qtquick-window qml6-module-qtquick-dialogs qml6-module-qtqml-workerscript \
  libsdl2-dev libsdl2-ttf-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev \
  libva-dev libvdpau-dev libopus-dev libssl-dev libdrm-dev libegl-dev libgbm-dev libwayland-dev libx11-dev
```

On Arch or ArmadaOS, install the equivalents: `qt6-base qt6-declarative qt6-svg qt6-wayland sdl2 sdl2_ttf ffmpeg libva libvdpau opus openssl libdrm`.

### Build

```sh
git clone --recursive https://github.com/drluciano/moonlight-armada.git
cd moonlight-armada
mkdir build && cd build
qmake6 ../moonlight-qt.pro CONFIG+=release
make -j$(nproc)
```

The binary is `build/app/moonlight`.

### Package an AppImage

On an arm64 machine, with [appimagetool](https://github.com/AppImage/appimagetool) on your `PATH`:

```sh
scripts/build-appimage-armada.sh build out/Moonlight-Armada-aarch64.AppImage
```

The script also works for cross-builds from x86_64 using Ubuntu's multiarch arm64 packages. In that case, set `CROSS=aarch64-linux-gnu-`.

### Continuous integration and releases

[`.github/workflows/build-armada-appimage.yml`](.github/workflows/build-armada-appimage.yml) runs on GitHub's native `ubuntu-24.04-arm` runners:

- **Pull requests and pushes to `main`** build Moonlight, package the AppImage, check that it starts without missing libraries, and upload it as a build artifact.
- **Tags starting with `v`** (for example `v0.1.0`) also publish the AppImage as a GitHub Release:
  ```sh
  git tag v0.1.0 && git push origin v0.1.0
  ```

## How the AppImage is packaged

ArmadaOS is Arch-based and can be lean, so the AppImage bundles everything except a core set of libraries that every Arch desktop or Steam install provides: glibc and the C++ runtime, Mesa/GL, X11/xcb, Wayland, xkbcommon, glib, harfbuzz, fontconfig/freetype, ALSA, dbus and systemd/udev. These choices came out of on-device testing:

- **OpenSSL is bundled as a matched pair.** Qt loads libssl at runtime, so it never shows up as a link dependency. Pairing the system's libssl with an older bundled libcrypto broke pairing-key generation, so both ship together.
- **libproxy is replaced by a stub.** The real library pulls in curl, gnutls and p11-kit, which load system plugins at startup and can crash on a mismatched system. The stub ([`app/deploy/linux/armada/libproxy-stub.c`](app/deploy/linux/armada/libproxy-stub.c)) always reports a direct connection.
- **libva prefers the system copy.** As in upstream's AppImage, VA-API drivers only load with a matching libva, so the bundled libva is only a fallback for systems without one.
- **Wayland stays on.** Upstream's AppImage disables Wayland because it bundles libwayland-client. This one uses the system's, which matters for Gamescope and KDE.
- **Every run is logged.** The AppRun writes each run to `~/.cache/moonlight-armada/last-run.log`. If a library is missing, the log lists every missing library in one pass.

## Troubleshooting

- **Nothing happens when I launch it.** Run it from a terminal and check `~/.cache/moonlight-armada/last-run.log`. Missing libraries are listed at the end.
- **"Ultra low latency: partial".** You're streaming HEVC or AV1 on mainline Iris. Choose H.264 under **Settings → Video codec** (or leave it on Automatic with HDR off), or apply the [kernel patch](#the-iris-kernel-patch).
- **Software decoding instead of hardware.** Your user needs access to `/dev/video*`. Check with `ls -l /dev/video*`; on most setups the `video` group or the seat's uaccess rule grants it.
- **Hardware decoder check.** `v4l2-ctl --list-devices` (from `v4l-utils`) should list the Qualcomm decoder, for example `qcom-iris-decoder` on Iris devices.
- **No title bar in Desktop Mode windows.** The bundled libdecor can't find the system's decoration plugins. This doesn't affect fullscreen or Game Mode.

## Roadmap

- **Zero-copy frames.** Mainline FFmpeg's v4l2m2m decoder returns frames in system memory (NV12), so each frame is copied before display. An FFmpeg build with DRM-PRIME output for V4L2 (as in the Raspberry Pi and LibreELEC forks) would remove that copy.
- **Upstream the Iris patch** through the ArmadaOS kernel and then mainline Linux, so HEVC gets the full mode everywhere.
- **Vulkan renderer and HDR** by bundling libplacebo.
- **Flatpak** for a more integrated install on SteamOS-like systems.

## What changed from moonlight-qt

| Area | Files |
|---|---|
| Ultra low latency decoding | `app/streaming/video/v4l2lowlatency.{h,cpp}` (new), `ffmpeg.{h,cpp}`, `decoder.h`, `app/streaming/session.{h,cpp}` |
| Setting and CLI flag | `app/settings/streamingpreferences.{h,cpp}`, `app/cli/commandlineparser.cpp`, `app/gui/SettingsView.qml` |
| Interface | `app/gui/Theme.qml`, `Card.qml`, `StatusChip.qml`, `SettingsCard.qml` (new), `main.qml`, `PcView.qml`, `AppView.qml`, `NavigableItemDelegate.qml`, `app/main.cpp` |
| Kernel patch | `contrib/kernel/` (new) |
| Packaging and CI | `scripts/build-appimage-armada.sh`, `app/deploy/linux/armada/` (new), `.github/workflows/build-armada-appimage.yml` (replaces upstream's Windows, macOS, Steam Link and x86_64 workflows) |

Everything else is upstream moonlight-qt. For upstream's features, supported platforms and build instructions for other systems, see the [moonlight-qt README](https://github.com/moonlight-stream/moonlight-qt#readme).

## About ArmadaOS

ArmadaOS is an Arch-based (CachyOS-derived), SteamOS-like Linux distribution for ARM handhelds such as the AYN Odin and Thor series. It boots into Steam and Gamescope on Snapdragon hardware, with a separate Desktop Mode.

## Credits and license

- [moonlight-stream/moonlight-qt](https://github.com/moonlight-stream/moonlight-qt) by the Moonlight Game Streaming Project, the base of this project.
- [ClassicOldSong/moonlight-android](https://github.com/ClassicOldSong/moonlight-android), whose Snapdragon low latency mode inspired the Linux implementation.
- Qualcomm's `msm_vidc` video driver (CodeLinaro), which documents the decode order output property used by the kernel patch.

Moonlight Armada is licensed under the [GNU General Public License v3.0](LICENSE), the same as moonlight-qt.
