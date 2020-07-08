# ReplaySorcery
## An open-source, instant-replay solution for Linux.
Back when I used to use windows I used AMD ReLive alot. It, and the nVidia version ShadowPlay Instant Replay, will constantly record the screen without using too much computer resources and at the press of a keycombo will save the last 30 seconds.

I wanted something like this for Linux...

I got tired waiting for someone else to do it.

## What is wrong with OBS?
Alot of people online suggest using OBS's replay buffer feature. However this requires opening OBS and start recording. I do not know when something will happen that I want to share. Might not even happen while playing a game. I just want something in the background (like a `systemd` service) so that whenever something happens I can record it.

# Building
This project uses Meson and Ninja for compiling. It also depends on FFmpeg and X11 development libraries. `libunwind` is not directly needed but is recommended.
```
$ meson bin
$ ninja -C bin
```

To build with x264 support, which potentially (IANAL) makes the binary GPL-licensed, run:
```
$ meson bin
$ meson configure bin -Dx264-gpl=true
$ ninja -C bin
```

## Ubuntu Dependencies
To install the required dependencies on Ubuntu run:
```
$ apt install python3-pip ninja-build libavutil-dev libavformat-dev libavdevice-dev \
              libavcodec-dev libswscale-dev libx11-dev libunwind-dev
$ pip3 install meson
```

## Fedora Dependencies
To install the required dependencies on Fedora run:
```
$ dnf install python3-pip ninja-build ffmpeg-devel libX11-devel libunwind-devel
$ pip install meson
```

## Arch Dependencies
To install the required dependencies on Arch run:
```
$ pacman -S python-pip ninja ffmpeg libx11 libunwind
$ pip install meson
```

# Running
Currently can be run as `bin/replay-sorcery`. Pressing enter will save the last 30 seconds. Early stages :)

# Documentation
Check out `src/config.h` for documentation about the configuration files.

The code itself is partway through getting better documentation and commenting. If you want to jump into it I would recommend looking at these source files and headers inparticular:
- `src/input/input`: handles different input devices.
- `src/input/video`: finds a video input device.
- `src/encoder/encoder`: handles different encoders.
- `src/encoder/video`: finds a video encoder.
- `src/save`: saves into an `mp4` container.
- `src/pktcircle`: a multithreaded circle buffer for packets.

If theres any part of user or developer documentation that you feel needs to be improved. Feel free to [let me know](https://github.com/matanui159/ReplaySorcery/issues).

# TODO
- Document code better
- Add X11 frontend to listen to key presses (right now only the `debug` frontend is available)
- Make a `systemd` service
- Implement different quality levels
- Add audio support
  - On first look reading from audio output does not seem supported by FFmpeg.
  - For my personal requirements it would need to support hot plugging (I use a bluetooth headset).
  - Alot of the time visuals alone are enough to tell a story.
  - (not crossing it out entirely just might take time).
- Cross-platform support
  - Theoretically possible since FFmpeg supports multiple platforms and provides inputs & encoders for them as well
  - Doubt there is any demand though
  - Maybe for Intel devices if they are fast enough?
- Smaller tweaks:
  - Add configuration option for changing scaler options
