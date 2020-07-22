# ReplaySorcery
## An open-source, instant-replay solution for Linux.
Back when I used to use windows I used AMD ReLive alot. It, and the nVidia version ShadowPlay Instant Replay, will constantly record the screen without using too much computer resources and at the press of a keycombo will save the last 30 seconds.

I wanted something like this for Linux...

I got tired waiting for someone else to do it.

## What is wrong with OBS?
Alot of people online suggest using OBS's replay buffer feature. However this requires opening OBS and start recording. I do not know when something will happen that I want to share. Might not even happen while playing a game. I just want something in the background (like a `systemd` service) so that whenever something happens I can record it.

## Why JPEG? Why not hardware-accelerated encoding?
You might notice that this uses JPEG to encode frames. Initially the plan was to use hardware-accelerated encoding. However, since there is currently no way to grab frames directly on the GPU, sending frames to the GPU and encoded packets back became a huge buttleneck that limited me to no more than ~40-50 FPS and lagged basically any game I tried playing. I changed plan of attack based on the idea that:
- most of the frames will probably get discarded so why bother working hard on them?
- we can reencode them properly when we need to save
- the inital encoding of the frames can be cheap and is more a form a memory-compression
- turns out that an image format from 1992 is very easy for modern computers

Thus, this program _does_ use JPEG (specifically `libjpeg-turbo`) for encoding frames, but then switches to `x264` when you want to save it.

# Documentation
## Building
This project needs `cmake`, `make` and `nasm` (used by x264) for compiling. All dependencies (other than X11 development files) are built as part of the binary and statically linked in. Don't @ me it makes distribution easier.
```
$ cmake -B bin
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

The configuration files themselves are simple `<key> = <value> # <optional comment>` pairs. The available options are:
| Key               | Description                                                                                                   | Default                                  |
| ----------------- | ------------------------------------------------------------------------------------------------------------- | ---------------------------------------- |
| `offsetX`         | This, along with `offsetY`, `width` and `height` specify the rectangle of the display to read frames from.    | 0                                        |
| `offsetY`         | See `offsetX`                                                                                                 | 0                                        |
| `width`           | See `offsetX`                                                                                                 | 1920                                     |
| `height`          | See `offsetX`                                                                                                 | 1080                                     |
| `framerate`       | The framerate in which to record frames.                                                                      | 30                                       |
| `duration`        | The duration to record when saving, in seconds.                                                               | 30                                       |
| `compressQuality` | A value from 0-100 specifying the quality to use for JPEG compression                                         | 70                                       |
| `outputFile`      | The output file path to save to. Uses [strftime](https://en.cppreference.com/w/c/chrono/strftime) formatting. | `~/Videos/ReplaySorcery_%F_%H-%M-%S.mp4` |

As an example configuration, here is the configuration I use (my primary monitor is a 1440p monitor which is to the right of a 1080p monitor):
```
offsetX = 1920
width = 2560
height = 1440
outputFile = ~/Videos/ReplaySorcery/%F_%H-%M-%S.mp4
```

# Issues
- Code is a bit of a mess <_<
- The routine to convert JPEG's interleaved YUV to YUV420p for x264 is very slow and a huge bottleneck

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
  - Add configuration option for changing scaler options
  - Improve performance of scaler, potentially using swscale from FFmpeg
