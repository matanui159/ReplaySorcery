# ReplaySorcery
## An open-source, instant-replay solution for Linux.
Back when I used to use windows I used AMD ReLive alot. It, and the nVidia version ShadowPlay Instant Replay, will constantly record the screen without using too much computer resources and at the press of a keycombo will save the last 30 seconds.

I wanted something like this for Linux...

I got tired waiting for someone else to do it.

# Documentation
- [Installing](#installing)
  - [Arch](#arch)
- [Building from Source](#building-from-source)
  - [Required Dependencies](#required-dependencies)
  - [Optional Dependencies](#optional-dependencies)
- [Running](#running)
- [Configuration](#configuration)
- [Hardware Acceleration](#hardware-acceleration)
  - [Using `replay-sorcery-kms`](#using-replay-sorcery-kms)
  - [nVidia Support](#nvidia-support)
- [Wayland Support](#wayland-support)

## Installing
### Arch
There is an official AUR package that gets updated from the CI (thanks to [Bennett Hardwick](https://github.com/bennetthardwick)): [replay-sorcery](https://aur.archlinux.org/packages/replay-sorcery).

[Sergey A.](https://github.com/murlakatamenka) has also setup a `-git` AUR package: [replay-sorcery-git](https://aur.archlinux.org/packages/replay-sorcery-git).

## Building from Source
```
$ git submodule update --init
$ cmake -B bin -DCMAKE_BUILD_TYPE=Release
$ make -C bin
$ sudo make -C bin install
```

### Required dependencies
- CMake
- FFmpeg

### Optional dependencies
- Xlib and xcb (for software screen recording and keyboard shortcuts on X11)
- PulseAudio (for audio recording)
- `libdrm` (for listing `kms` devices)

## Running
It can be enabled as a user systemd service:
```
$ systemctl --user enable --now replay-sorcery
```
Once it is running, just press Ctrl+Super+R to save the last 30 seconds.

When the configuration file has changed, the service must be reloaded by running:
```
$ systemctl --user restart replay-sorcery
```

You can also use systemd to look at the output:
```
$ journalctl --user -fu replay-sorcery
```

## Configuration
The config file location and options has completely changed since version 0.3.x

There are two config files:
- The global config file is located at `@CMAKE_INSTALL_PREFIX@/etc/replay-sorcery.conf` (`/usr/local/etc/replay-sorcery.conf` by default).
- The local config file is located at `~/.config/replay-sorcery.conf`. Options in this file will overwrite options in the global file.

See [`sys/replay-sorcery.conf`](sys/replay-sorcery.conf) for the default values along with documentation. This file is installed into the global config file location.

## Hardware Acceleration
### Using `replay-sorcery-kms`
Due to hardware acceleration requiring root permissions, the recommended way of enabling hardware acceleration is by using the KMS service. Start the service by running:
```
$ sudo systemctl enable --now replay-sorcery-kms
```

Then set `videoInput` in the configuration file to either `hwaccel` or `kms_service` and start or restart the user service as documented above.

### nVidia Support
The Nouveau open source drivers are supported but sadly the proprietary nVidia drivers do not support VA-API which is currently required for hardware acceleration. In the future NVENC might be supported. Software encoding is always supported and tries to use as little CPU as possible.

## Wayland Support
Wayland screen grabbing is not currently supported, however hardware accelerated screen grabbing works fine. See above for steps for enabling that.

Wayland also does not allow listening to keyboard events unless you are the active window. To get around this you can set `controller` in the configuration file to `command` and setup a shortcut in your window manager to run:
```
$ replay-sorcery save
```

# TODO
- Support NVENC API
- Document code better
- Cross-platform support
  - Doubt there is any demand though
  - Maybe for Intel devices if they are fast enough?
