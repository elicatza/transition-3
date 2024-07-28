////////////////////////////////////////////////////////////////////////////////////////
//                                                                                    //
// ImageAsCode exporter v1.0 - Image pixel data exported as an array of bytes         //
//                                                                                    //
// more info and bugs-report:  github.com/raysan5/raylib                              //
// feedback and support:       ray[at]raylib.com                                      //
//                                                                                    //
// Copyright (c) 2018-2023 Ramon Santamaria (@raysan5)                                //
//                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////

// Image data information
#define WORLD_ATLAS_WIDTH    32
#define WORLD_ATLAS_HEIGHT   16
#define WORLD_ATLAS_FORMAT   4          // raylib internal pixel format

static unsigned char WORLD_ATLAS_DATA[1536] = { 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8,
0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7,
0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8,
0xf7, 0xa8, 0xb8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7,
0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8,
0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8,
0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0,
0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8,
0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0,
0x0, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7,
0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8,
0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7,
0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0x55, 0xcd, 0xfc,
0x55, 0xcd, 0xfc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8,
0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0xf7,
0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8,
0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8,
0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8,
0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8,
0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7,
0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8,
0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0x55,
0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8,
0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8,
0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0,
0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xa8, 0xb8,
0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd,
0xfc, 0x55, 0xcd, 0xfc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7,
0xa8, 0xb8, 0x0, 0x0, 0x0, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x0, 0x0, 0x0,
0x55, 0xcd, 0xfc, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8,
0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0,
0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0,
0x0, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0xf7,
0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8,
0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xf7, 0xa8,
0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0x55, 0xcd, 0xfc, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8,
0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7,
0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8,
0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xa8,
0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55,
0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8,
0x0, 0x0, 0x0, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd,
0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7,
0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8,
0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0x55,
0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc,
0x55, 0xcd, 0xfc, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8,
0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7,
0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xa8, 0xb8, 0x0, 0x0, 0x0, 0x55, 0xcd, 0xfc, 0x55, 0xcd,
0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0x55, 0xcd, 0xfc, 0xf7,
0xa8, 0xb8, 0x0, 0x0, 0x0, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8,
0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8,
0xb8, 0xf7, 0xa8, 0xb8, 0xf7, 0xa8, 0xb8, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
