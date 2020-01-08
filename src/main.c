#include <Imlib2.h>

#include "wallpaper.h"
#include "debug.h"


int main(int argc, char** argv) {
    assert(argc == 2, "usage: %s <image_file>", argv[0]);

    Imlib_Image img = imlib_load_image(argv[1]);
    assert(img, "Unable to load %s", argv[1]);

    WESetWallpaper(img);

    return 0;
}

