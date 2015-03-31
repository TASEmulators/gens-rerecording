#.gmv is the movie capture format of Gens, a Sega Genesis/Megadrive emulator.

## GMV file format description ##
GMV file consists of a 64-byte header and the movie data.

### Header format ###
```
000 16-byte signature and format version: "Gens Movie TEST9"
00F ASCII-encoded GMV file format version. The most recent is 'A'. (?)
010 4-byte little-endian unsigned int: rerecord count
014 ASCII-encoded controller config for player 1. '3' or '6'.
015 ASCII-encoded controller config for player 2. '3' or '6'.
016 special flags (Version A and up only):
    bit 7(most significant): if "1", movie runs at 50 frames per second; if "0", movie runs at 60 frames per second
    bit 6: if "1", movie requires a savestate.
    bit 5: if "1", movie is 3-player movie; if "0", movie is 2-player movie
018 40-byte zero-terminated ASCII movie name string
040 frame data
```

### Controller Data ###
Each frame consists of 3 bytes.

| **000** | **001** | **002** |
|:--------|:--------|:--------|
| Controller 1 | Controller 2 | `*` |

where `*` is controller 3 if a 3-player movie, or XYZ-mode if a 2-player movie.

For controller bytes, each value is determined by OR-ing together values for whichever of the following are **left unpressed**:
```
  0x01 Up
  0x02 Down
  0x04 Left
  0x08 Right
  0x10 A
  0x20 B
  0x40 C
  0x80 Start
```

For XYZ-mode, each value is determined by OR-ing together values for whichever of the following are **left unpressed**:
```
  0x01 Controller 1 X
  0x02 Controller 1 Y
  0x04 Controller 1 Z
  0x08 Controller 1 Mode
  0x10 Controller 2 X
  0x20 Controller 2 Y
  0x40 Controller 2 Z
  0x80 Controller 2 Mode
```
The file has no terminator byte or frame count. The number of frames is the <filesize minus 64> divided by 3.

The file format has no means of identifying NTSC/PAL, but the FPS can still be derived from the header.