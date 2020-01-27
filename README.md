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
  - --loop has some bugs (`we -m0 a.png && we -m0 b.png c.png --fifo -d 2 --loop`)
  - excessive memory usage (`we -m0 <a_lot_of_files> --fifo -t 2`)

