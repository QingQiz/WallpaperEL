[![Build Status](https://travis-ci.org/QingQiz/WallpaperEL.svg?branch=master)](https://travis-ci.org/QingQiz/WallpaperEL)
# WallpaperEL

> *Wallpaper Engine for Linux (Undone)*

## Dependencies

- Imlib2
- libX11
- libXrandr

## Install

```shell
make
sudo make install
```

## Usage

```shell
we --help
```

## TODO

- Features
  - more filling method
  - video wallpaper
  - wallpaper with sound
    
- FIXME
  - can not show help message without X
  - excessive memory usage during `we -m0 <many_files> --fifo -t 2`

