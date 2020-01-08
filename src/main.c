#include <Imlib2.h>

#include "wallpaper.h"
#include "debug.h"
#include "imtools.h"


int main(int argc, char** argv) {
    assert(argc == 2, "usage: %s <image_file>", argv[0]);

    init_x_and_imtools();

    Imlib_Image img = imlib_load_image(argv[1]);
    assert(img, "Unable to load %s", argv[1]);

    WESetWallpaper(img);

    destruct_imtools();
    return 0;
}

