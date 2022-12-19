#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "3rdparty/stb_image.h"

// Each pixel has 4 components (RGBA).
const int COMPONENTS = 4;
// Each component is represented in 1 byte.
const int COMPONENT_SIZE_B = 1;
// A pixel therefore requires 4 bytes.
const int PIXEL_SIZE_B = COMPONENTS * COMPONENT_SIZE_B;
// A tile consists of 8x8 pixels.
const int TILE_WIDTH_PX = 8;
const int TILE_HEIGHT_PX = 8;
const int TILE_SIZE_PX = TILE_WIDTH_PX * TILE_HEIGHT_PX;
// A tileset cannot have more than 256 tiles.
const int MAX_TILESET_SIZE = 256;

void print_help() {
    printf("Usage: gbtm OPTION...\n");
    printf("Options:\n");
    printf("  -s, --tileset FILE  Tileset as PNG image (required)\n");
    printf("  -i, --image FILE    PNG image made up of tiles from the tileset (required)\n");
    printf("  -m, --tilemap FILE  Destination file for the generated tilemap (required)\n");
    printf("  -h, --help          Print usage information\n");
}

struct options {
    char *tileset_path;
    char *image_path;
    char *tilemap_path;
};

void options_parse(int argc, char *argv[], struct options *opts) {
    if (argc == 1 ||
        (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0))) {
        print_help();
        exit(0);
    }
    // Currently the number of arguments is fixed.
    if (argc != 7) {
        printf("Invalid arguments\n");
        print_help();
        exit(-1);
    }
    for (int i = 1; i < argc; i += 2) {
        char *arg = argv[i];
        if (strcmp(arg, "--tileset") == 0 || strcmp(arg, "-s") == 0) {
            opts->tileset_path = argv[i + 1];
        } else if (strcmp(arg, "--image") == 0 || strcmp(arg, "-i") == 0) {
            opts->image_path = argv[i + 1];
        } else if (strcmp(arg, "--tilemap") == 0 || strcmp(arg, "-m") == 0) {
            opts->tilemap_path = argv[i + 1];
        }
    }
    if (!opts->tilemap_path || !opts->image_path || !opts->tilemap_path) {
        printf("Invalid arguments\n");
        print_help();
        exit(-1);
    }
}

struct tiled_image {
    int w_px;
    int h_px;
    int w_tiles;
    int h_tiles;
    int num_tiles;
    uint8_t *data;
};

void tiled_image_free(struct tiled_image *img) {
    stbi_image_free(img->data);
    img->data = NULL;
}

int tiled_image_load(struct tiled_image *img, char *path) {
    int components;
    img->data = stbi_load(path, &img->w_px, &img->h_px, &components, COMPONENTS);
    if (img->data == NULL) {
        printf("Error: Failed to load image %s\n", path);
        return -1;
    }

    if (img->w_px % TILE_WIDTH_PX != 0 || img->h_px % TILE_HEIGHT_PX != 0) {
        printf("Error: Image size %dx%dpx is not a multiple of the tile size %dx%dpx\n", img->w_px,
               img->h_px, TILE_WIDTH_PX, TILE_HEIGHT_PX);
        tiled_image_free(img);
        return -1;
    }

    img->w_tiles = img->w_px / TILE_WIDTH_PX;
    img->h_tiles = img->h_px / TILE_HEIGHT_PX;
    img->num_tiles = img->w_tiles * img->h_tiles;

    return 0;
}

uint8_t *tiled_image_get_pixel_row(struct tiled_image *img, int tile_idx, int pixel_row) {
    int tile_row = tile_idx / img->w_tiles;
    int tile_col = tile_idx % img->w_tiles;
    int offset_tile_row = tile_row * img->w_px * PIXEL_SIZE_B * TILE_HEIGHT_PX;
    int offset_tile_col = tile_col * TILE_WIDTH_PX * PIXEL_SIZE_B;
    int offset_pixel_row = pixel_row * img->w_px * PIXEL_SIZE_B;
    return img->data + offset_tile_row + offset_tile_col + offset_pixel_row;
}

int is_tile_equal(struct tiled_image *img_1, int tile_idx_1, struct tiled_image *img_2,
                  int tile_idx_2) {
    for (int row = 0; row < TILE_HEIGHT_PX; row++) {
        uint8_t *img_1_data = tiled_image_get_pixel_row(img_1, tile_idx_1, row);
        uint8_t *img_2_data = tiled_image_get_pixel_row(img_2, tile_idx_2, row);
        if (memcmp(img_1_data, img_2_data, TILE_WIDTH_PX * PIXEL_SIZE_B) != 0) {
            return 0;
        }
    }

    return 1;
}

struct tilemap {
    size_t size;
    uint8_t *data;
};

void tilemap_new(struct tilemap *map, int num_tiles) {
    map->size = num_tiles;
    map->data = malloc(num_tiles);
}

void tilemap_free(struct tilemap *map) {
    free(map->data);
    map->data = NULL;
}

void tilemap_set_tile(struct tilemap *map, int tilemap_idx, uint8_t tileset_idx) {
    map->data[tilemap_idx] = tileset_idx;
}

int tilemap_generate(struct tilemap *map, struct tiled_image *image, struct tiled_image *tileset) {
    for (int i = 0; i < image->num_tiles; i++) {
        int match = 1;
        for (int j = 0; j < tileset->num_tiles; j++) {
            match = is_tile_equal(image, i, tileset, j);
            if (match) {
                tilemap_set_tile(map, i, j);
                break;
            }
        }
        if (!match) {
            printf("Error: Tile number %d in image is not part of the tileset.\n", i);
            return -1;
        }
    }
    return 0;
}

int tilemap_save(struct tilemap *map, const char *filename) {
    FILE *file = fopen(filename, "wx");
    if (!file) {
        printf("Error: Failed to open file %s (%s)\n", filename, strerror(errno));
        return -1;
    }

    size_t c = fwrite(map->data, 1, map->size, file);
    if (c != map->size) {
        printf("Error: Could not write complete tilemap to file %s (bytes written: %zu/%zu)\n",
               filename, c, map->size);
        fclose(file);
        return -1;
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    int retval = 0;
    int error;

    struct options opts = {0};
    struct tiled_image tileset = {0};
    struct tiled_image image = {0};
    struct tilemap map = {0};

    options_parse(argc, argv, &opts);

    error = tiled_image_load(&tileset, opts.tileset_path);
    if (error) {
        retval = error;
        goto exit;
    }
    if (tileset.num_tiles > MAX_TILESET_SIZE) {
        printf("Error: Tileset contains too many tiles (has %d, maximum %d allowed)\n",
               tileset.num_tiles, MAX_TILESET_SIZE);
        retval = -1;
        goto exit;
    }
    printf("Tileset loaded (%dx%d px, %dx%d tiles)\n", tileset.w_px, tileset.h_px, tileset.w_tiles,
           tileset.h_tiles);

    error = tiled_image_load(&image, opts.image_path);
    if (error) {
        retval = error;
        goto exit;
    }
    printf("Tiled image loaded (%dx%d px, %dx%d tiles)\n", image.w_px, image.h_px, image.w_tiles,
           image.h_tiles);

    tilemap_new(&map, image.num_tiles);

    error = tilemap_generate(&map, &image, &tileset);
    if (error) {
        retval = error;
        goto exit;
    }
    printf("%d tiles of image successfully mapped to tileset\n", image.num_tiles);

    error = tilemap_save(&map, opts.tilemap_path);
    if (error) {
        retval = error;
        goto exit;
    }
    printf("Tilemap written to %s\n", opts.tilemap_path);

exit:
    tiled_image_free(&tileset);
    tiled_image_free(&image);
    tilemap_free(&map);

    return retval;
}
