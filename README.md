# WIP audio recording
works, but its still buggy and ugly

# ReplaySorcery
## An open-source, instant-replay solution for Linux.
Back when I used to use windows I used AMD ReLive alot. It, and the nVidia version ShadowPlay Instant Replay, will constantly record the screen without using too much computer resources and at the press of a keycombo will save the last 30 seconds.

I wanted something like this for Linux...

I got tired waiting for someone else to do it.

## What is wrong with OBS?
A lot of people online suggest using OBS's replay buffer feature. However this requires opening OBS and start recording. I do not know when something will happen that I want to share. Might not even happen while playing a game. I just want something in the background (like a `systemd` service) so that whenever something happens I can record it.

## Why JPEG? Why not hardware-accelerated encoding?
You might notice that this uses JPEG to encode frames. Initially the plan was to use hardware-accelerated encoding. However, since there is currently no way to grab frames directly on the GPU, sending frames to the GPU and encoded packets back became a huge bottleneck that limited me to no more than ~40-50 FPS and lagged basically any game I tried playing. I changed plan of attack based on the idea that:
- most of the frames will probably get discarded so why bother working hard on them?
- we can reencode them properly when we need to save
- the inital encoding of the frames can be cheap and is more a form a memory-compression
- turns out that an image format from 1992 is very easy for modern computers

Thus, this program _does_ use JPEG (specifically `libjpeg-turbo`) for encoding frames, but then switches to `x264` when you want to save it. The compressed frames are stored temporarily in memory inside a circle buffer which will automatically grow and shrink depending on the compressability of the frames, while also allowing space for future growth without many resizes.

# Documentation
## Installing
### Arch
There is an official AUR package that gets updated from the CI (thanks to [Bennett Hardwick](https://github.com/bennetthardwick)): [replay-sorcery](https://aur.archlinux.org/packages/replay-sorcery).

[Sergey A.](https://github.com/murlakatamenka) has also setup a `-git` AUR package: [replay-sorcery-git](https://aur.archlinux.org/packages/replay-sorcery-git).

## Building from Source
This project needs `cmake`, `make` and `nasm` (used by x264) for compiling. All dependencies (other than X11 development files) are built as part of the binary and statically linked in. Don't @ me it makes distribution easier.
```
$ git submodule update --init
$ cmake -B bin -DCMAKE_BUILD_TYPE=Release
$ make -C bin
$ sudo make -C bin install
```

## Running
It can be enabled as a user systemd service:
```
$ systemctl --user enable --now replay-sorcery
```

Once it is running, just press Ctrl+Super+R to save the last 30 seconds.

You can also use systemd to look at the output:
```
$ journalctl --user -fu replay-sorcery
```

## Configuration
The config files use the [XDG base directory specification](https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html) with a filename of `replay-sorcery.conf`. If you do not know what that means you can probably assume that the program will look for configuration files in the following spots:
- `/etc/xdg/replay-sorcery.conf`
- `~/.config/replay-sorcery.conf`
- `./replay-sorcery.conf`

The configuration files themselves are simple `<key> = <value> # <optional comment>` pairs. An example config could be:

```
# 1440p monitor...
width = 2560
height = 1440

# ...to the right of a 1080p monitor
offsetX = 1920

# Reduce memory usage
compressQuality = 50

# Output files into their own folder
outputFile = ~/Videos/ReplaySorcery/%F_%H-%M-%S.mp4
```

### `offsetX`, `offsetY`, `width`, and `height`
These four options work together to specify the rectangle of the display to grab frames from. This allows you to select which monitor to read from or potentially just read part of a monitor. `width` and `height` can be set to `auto` to specify the whole screen. Default is 0, 0, `auto`, and `auto` respectively.

### `framerate` and `duration`
`framerate` is the framerate to record at while `duration` is the maximum duration of the replay buffer in seconds, before it starts to overwrite itself. Together they create the maximum size of the replay buffer (`framerate * duration`). Default is 30 and 30 respectively.

### `compressQuality`
A value from 1-100 to specify the JPEG compression quality. Don't be afraid to lower this value for higher resolutions. The quality loss isn't that noticable most of the time and it greatly reduces the memory usage. Default is 70.

### `keyCombo`
The key combo to use. Must be a series of modifiers followed by an X11 key name, all seperated by `+`. Due to limitations with `XStringToKeysym`, the keys are case sensitive. Valid modifiers are `Ctrl`, `Shift`, `Alt`, and `Super`. Default is `Ctrl+Super+R`.

### `outputFile`
The path to output the videos to. Can use [`strftime`](https://en.cppreference.com/w/c/chrono/strftime) formatting. If you care about folder organization it is probably a good idea to make ReplaySorcery output into a subfolder inside `Videos`, for example `~/Videos/ReplaySorcery/%F_%H-%M-%S.mp4`. This is not the default since currently ReplaySorcery cannot create folders and thus you have to make sure the folder exists before hand. Default is `~/Videos/ReplaySorcery_%F_%H-%M-%S.mp4`.

### `preOutputCommand` and `postOutputCommand`
These options can be used to run commands before or after outputting a video, for instance generating notifications, playing sounds or running post processing. Failures from these commands do not stop ReplaySorcery. The default is setup to show a notification when the output is finished, but it requires `libnotify` to be installed. Default is an empty string and `notify-send ReplaySorcery "Video saved!"` respectively.

# Issues
- Does not track frame timing properly so if there is lag or dropped frames the output will be played back faster.
- Can be a bit resource hungry at times (in particular memory)

# TODO
- Document code better
- Add audio support
  - For my personal requirements it would need to support hot plugging (I use a bluetooth headset).
  - Alot of the time visuals alone are enough to tell a story.
  - (not crossing it out entirely just might take time).
- Cross-platform support
  - Doubt there is any demand though
  - Maybe for Intel devices if they are fast enough?
- Smaller tweaks:
  - Add sound effect for when it saves
  - Add configuration option for changing hotkey
  - Allow downscaling before saving
