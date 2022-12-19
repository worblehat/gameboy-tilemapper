# Game Boy Tilemapper

`gbtm` creates a tilemap based on a given image made up of tiles and a corresponding tileset.
The tilemap is written to a file in a binary format (one byte per tile) that can be directly
included and used in Game Boy games.

Uses the public domain, header-only [stb_image](https://github.com/nothings/stb) library for
PNG reading. No external dependencies. Only a C compiler is needed to build it.

Compiling:

```lang-shell
make
```

Usage:

```lang-shell
Usage: gbtm OPTION...
Options:
  -s, --tileset FILE  Tileset as PNG image (required)
  -i, --image FILE    PNG image made up of tiles from the tileset (required)
  -m, --tilemap FILE  Destination file for the generated tilemap (required)
  -h, --help          Print usage information
```

Example:

```lang-shell
./gbtm -s my-tileset.png -i my-level-background.png -m my-level-tilemap.bin
```
