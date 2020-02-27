[![Build Status](https://travis-ci.org/QingQiz/WallpaperEL.svg?branch=master)](https://travis-ci.org/QingQiz/WallpaperEL)
# WallpaperEL

> *Wallpaper Engine for Linux (Undone)*

## Dependencies

- Imlib2
- libX11
- libXrandr
- libasound2

## Install

```shell
make
sudo make install
```

## Usage

```shell
we --help
```

## Example

- use video as wallpaper (need ffmpeg)

```shell
mkdir -p video
ffmpeg -i video.mp4 -vf fps=25 video/frame%04d.png
ffmpeg -i video.mp4 video.wav
cd video
we -m0 frame* --bgm video.wav -t 0.04 --less-memory --max-preload 5
```

## Testing

```shell
make test
cd test
./test.sh
```

This requires you to observe whether the wallpaper switch is as expected.

## TODO

- Features
  - more filling method
    
- FIXME

