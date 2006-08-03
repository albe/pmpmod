/**
 * @file vc9data.h
 * VC9 tables.
 */

#ifndef VC9DATA_H
#define VC9DATA_H

/* bfraction is fractional, we scale to the GCD 3*5*7*8 = 840 */
const int16_t vc9_bfraction_lut[23] = {
  420 /*1/2*/, 280 /*1/3*/, 560 /*2/3*/, 210 /*1/4*/,
  630 /*3/4*/, 168 /*1/5*/, 336 /*2/5*/,
  504 /*3/5*/, 672 /*4/5*/, 140 /*1/6*/, 700 /*5/6*/,
  120 /*1/7*/, 240 /*2/7*/, 360 /*3/7*/, 480 /*4/7*/,
  600 /*5/7*/, 720 /*6/7*/, 105 /*1/8*/, 315 /*3/8*/,
  525 /*5/8*/, 735 /*7/8*/,
  -1 /*inv.*/, 0 /*BI fm*/
};
const uint8_t vc9_bfraction_bits[23] = {
    3, 3, 3, 3,
    3, 3, 3,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7,
    7, 7
};
const uint8_t vc9_bfraction_codes[23] = {
     0,   1,   2,   3,
     4,   5,   6,
   112, 113, 114, 115,
   116, 117, 118, 119,
   120, 121, 122, 123,
   124, 125,
   126, 127
};

//Same as H.264
static const AVRational vc9_pixel_aspect[16]={
 {0, 1},
 {1, 1},
 {12, 11},
 {10, 11},
 {16, 11},
 {40, 33},
 {24, 11},
 {20, 11},
 {32, 11},
 {80, 33},
 {18, 11},
 {15, 11},
 {64, 33},
 {160, 99},
 {0, 1},
 {0, 1}
};

/* BitPlane IMODE - such a small table... */
static const uint8_t vc9_imode_codes[7] = {
  0, 2, 1, 3, 1, 2, 3
};
static const uint8_t vc9_imode_bits[7] = {
  4, 2, 3, 2, 4, 3, 3
};

/* Normal-2 imode */
static const uint8_t vc9_norm2_codes[4] = {
  0, 4, 5, 3
};
static const uint8_t vc9_norm2_bits[4] = {
  1, 3, 3, 2
};

static const uint16_t vc9_norm6_codes[64] = {
0x001, 0x002, 0x003, 0x000, 0x004, 0x001, 0x002, 0x047, 0x005, 0x003, 0x004, 0x04B, 0x005, 0x04D, 0x04E, 0x30E, 
0x006, 0x006, 0x007, 0x053, 0x008, 0x055, 0x056, 0x30D, 0x009, 0x059, 0x05A, 0x30C, 0x05C, 0x30B, 0x30A, 0x037, 
0x007, 0x00A, 0x00B, 0x043, 0x00C, 0x045, 0x046, 0x309, 0x00D, 0x049, 0x04A, 0x308, 0x04C, 0x307, 0x306, 0x036, 
0x00E, 0x051, 0x052, 0x305, 0x054, 0x304, 0x303, 0x035, 0x058, 0x302, 0x301, 0x034, 0x300, 0x033, 0x032, 0x007, 
};

static const uint8_t vc9_norm6_bits[64] = {
 1,  4,  4,  8,  4,  8,  8, 10,  4,  8,  8, 10,  8, 10, 10, 13, 
 4,  8,  8, 10,  8, 10, 10, 13,  8, 10, 10, 13, 10, 13, 13,  9, 
 4,  8,  8, 10,  8, 10, 10, 13,  8, 10, 10, 13, 10, 13, 13,  9, 
 8, 10, 10, 13, 10, 13, 13,  9, 10, 13, 13,  9, 13,  9,  9,  6,
};
/* Normal-6 imode */
static const uint8_t vc9_norm6_spec[64][5] = {
{ 0,  1, 1        },
{ 1,  2, 4        },
{ 2,  3, 4        },
{ 3,  0, 8        },
{ 4,  4, 4        },
{ 5,  1, 8        },
{ 6,  2, 8        },
{ 7,  2, 5,  7, 5 },
{ 8,  5, 4        },
{ 9,  3, 8        },
{10,  4, 8        },
{11,  2, 5, 11, 5 },
{12,  5, 8        },
{13,  2, 5, 13, 5 },
{14,  2, 5, 14, 5 },
{15,  3, 5, 14, 8 },
{16,  6, 4        },
{17,  6, 8        },
{18,  7, 8        },
{19,  2, 5, 19, 5 },
{20,  8, 8        },
{21,  2, 5, 21, 5 },
{22,  2, 5, 22, 5 },
{23,  3, 5, 13, 8 },
{24,  9, 8        },
{25,  2, 5, 25, 5 },
{26,  2, 5, 26, 5 },
{27,  3, 5, 12, 8 },
{28,  2, 5, 28, 5 },
{29,  3, 5, 11, 8 },
{30,  3, 5, 10, 8 },
{31,  3, 5,  7, 4 },
{32,  7, 4        },
{33, 10, 8        },
{34, 11, 8        },
{35,  2, 5,  3, 5 },
{36, 12, 8        },
{37,  2, 5,  5, 5 },
{38,  2, 5,  6, 5 },
{39,  3, 5,  9, 8 },
{40, 13, 8        },
{41,  2, 5,  9, 5 },
{42,  2, 5, 10, 5 },
{43,  3, 5,  8, 8 },
{44,  2, 5, 12, 5 },
{45,  3, 5,  7, 8 },
{46,  3, 5,  6, 8 },
{47,  3, 5,  6, 4 },
{48, 14, 8        },
{49,  2, 5, 17, 5 },
{50,  2, 5, 18, 5 },
{51,  3, 5,  5, 8 },
{52,  2, 5, 20, 5 },
{53,  3, 5,  4, 8 },
{54,  3, 5,  3, 8 },
{55,  3, 5,  5, 4 },
{56,  2, 5, 24, 5 },
{57,  3, 5,  2, 8 },
{58,  3, 5,  1, 8 },
{59,  3, 5,  4, 4 },
{60,  3, 5,  0, 8 },
{61,  3, 5,  3, 4 },
{62,  3, 5,  2, 4 },
{63,  3, 5,  1, 1 },
};

/* 4MV Block pattern VLC tables */
static const uint8_t vc9_4mv_block_pattern_codes[4][16] = {
  { 14, 58, 59, 25, 12, 26, 15, 15, 13, 24, 27,  0, 28,  1,  2,  2},
  {  8, 18, 19,  4, 20,  5, 30, 11, 21, 31,  6, 12,  7, 13, 14,  0},
  { 15,  6,  7,  2,  8,  3, 28,  9, 10, 29,  4, 11,  5, 12, 13,  0},
  {  0, 11, 12,  4, 13,  5, 30, 16, 14, 31,  6, 17,  7, 18, 19, 19}
};
static const uint8_t vc9_4mv_block_pattern_bits[4][16] = {
  { 5, 6, 6, 5, 5, 5, 5, 4, 5, 5, 5, 3, 5, 3, 3, 2},
  { 4, 5, 5, 4, 5, 4, 5, 4, 5, 5, 4, 4, 4, 4, 4, 2},
  { 4, 4, 4, 4, 4, 4, 5, 4, 4, 5, 4, 4, 4, 4, 4, 3},
  { 2, 4, 4, 4, 4, 4, 5, 5, 4, 5, 4, 5, 4, 5, 5, 4}
};

const uint8_t wmv3_dc_scale_table[32]={
    0, 4, 6, 8, 8, 8, 9, 9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21
};

/* P-Picture CBPCY VLC tables */
static const uint16_t vc9_cbpcy_p_codes[4][64] = {
  {
      0,   1,   1,   4,   5,   1,  12,   4,  13,  14,  10,  11,  12,   7,  13,   2,
     15,   1,  96,   1,  49,  97,   2, 100,   3,   4,   5, 101, 102,  52,  53,   4,
      6,   7,  54, 103,   8,   9,  10, 110,  11,  12, 111,  56, 114,  58, 115,   5,
     13,   7,   8,   9,  10,  11,  12,  30,  13,  14,  15, 118, 119,  62,  63,   3
  },
  {
      0,   1,   2,   1,   3,   1,  16,  17,   5,  18,  12,  19,  13,   1,  28,  58,
      1,   1,   1,   2,   3,   2,   3, 236, 237,   4,   5, 238,   6,   7, 239,   8,
      9, 240,  10,  11, 121, 122,  12,  13,  14,  15, 241, 246,  16,  17, 124,  63,
     18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31, 247, 125
  },
  {
      0,   1,   2,   3,   2,   3,   1,   4,   5,  24,   7,  13,  16,  17,   9,   5,
     25,   1,   1,   1,   2,   3,  96, 194,   1,   2,  98,  99, 195, 200, 101,  26,
    201, 102, 412, 413, 414,  54, 220, 111, 221,   3, 224, 113, 225, 114, 230,  29,
    231, 415, 240,   4, 241, 484,   5, 243,   3, 244, 245, 485, 492, 493, 247,  31
  },
  {
      0,   1,   1,   1,   2,   2,   3,   4,   3,   5,   6,   7,   8,   9,  10,  11,
     12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,
     28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
     44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  28,  29,  30,  31
   }
};
static const uint8_t vc9_cbpcy_p_bits[4][64] = {
  {
    13,  6,  5,  6,  6,  7,  7,  5,  7,  7,  6,  6,  6,  5,  6,  3,
     7,  8,  8, 13,  7,  8, 13,  8, 13, 13, 13,  8,  8,  7,  7,  3,
    13, 13,  7,  8, 13, 13, 13,  8, 13, 13,  8,  7,  8,  7,  8,  3,
    13, 12, 12, 12, 12, 12, 12,  6, 12, 12, 12,  8,  8,  7,  7,  2
  },
  {
    14,  3,  3,  5,  3,  4,  5,  5,  3,  5,  4,  5,  4,  6,  5,  6,
     8, 14, 13,  8,  8, 13, 13,  8,  8, 13, 13,  8, 13, 13,  8, 13,
    13,  8, 13, 13,  7,  7, 13, 13, 13, 13,  8,  8, 13, 13,  7,  6,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,  8,  7
  },
  {
    13,  5,  5,  5,  4,  4,  6,  4,  4,  6,  4,  5,  5,  5,  4,  3,
     6,  8, 10,  9,  8,  8,  7,  8, 13, 13,  7,  7,  8,  8,  7,  5,
     8,  7,  9,  9,  9,  6,  8,  7,  8, 13,  8,  7,  8,  7,  8,  5,
     8,  9,  8, 13,  8,  9, 13,  8, 12,  8,  8,  9,  9,  9,  8,  5
  },
  {
     9,  2,  3,  9,  2,  9,  9,  9,  2,  9,  9,  9,  9,  9,  9,  9,
     9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
     9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
     9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  8,  8,  8,  8
  }
};

/* MacroBlock Transform Type: 7.1.3.11, p89
 * 8x8:B
 * 8x4:B:btm  8x4:B:top  8x4:B:both,
 * 4x8:B:right  4x8:B:left  4x8:B:both
 * 4x4:B  8x8:MB
 * 8x4:MB:btm  8x4:MB:top  8x4,MB,both
 * 4x8,MB,right  4x8,MB,left
 * 4x4,MB                               */
static const uint16_t vc9_ttmb_codes[3][16] = {
  {
    0x0003,
    0x002E, 0x005F, 0x0000,
    0x0016, 0x0015, 0x0001,
    0x0004, 0x0014,
    0x02F1, 0x0179, 0x017B,
    0x0BC0, 0x0BC1, 0x05E1,
    0x017A
  },
  {
    0x0006,
    0x0006, 0x0003, 0x0007,
    0x000F, 0x000E, 0x0000,
    0x0002, 0x0002,
    0x0014, 0x0011, 0x000B,
    0x0009, 0x0021, 0x0015,
    0x0020
  },
  {
    0x0006,
    0x0000, 0x000E, 0x0005,
    0x0002, 0x0003, 0x0003,
    0x000F, 0x0002,
    0x0081, 0x0021, 0x0009,
    0x0101, 0x0041, 0x0011,
    0x0100
  }
};

static const uint8_t vc9_ttmb_bits[3][16] = {
  {
     2,
     6,  7,  2,
     5,  5,  2,
     3,  5,
    10,  9,  9,
    12, 12, 11,
     9
  },
  {
    3,
    4, 4, 4,
    4, 4, 3,
    3, 2,
    7, 7, 6,
    6, 8, 7,
    8
  },
  {
     3,
     3, 4, 5,
     3, 3, 4,
     4, 2,
    10, 8, 6,
    11, 9, 7,
    11
  }
};  

/* TTBLK (Transform Type per Block) tables */
static const uint8_t vc9_ttblk_codes[3][8] = {
  {  0,  1,  3,  5, 16, 17, 18, 19},
  {  3,  0,  1,  2,  3,  5,  8,  9},
  {  1,  0,  1,  4,  6,  7, 10, 11}
};
static const uint8_t vc9_ttblk_bits[3][8] = {
  {  2,  2,  2,  3,  5,  5,  5,  5},
  {  2,  3,  3,  3,  3,  3,  4,  4},
  {  2,  3,  3,  3,  3,  3,  4,  4}
};

/* SUBBLKPAT tables, p93-94, reordered */
static const uint8_t vc9_subblkpat_codes[3][15] = {
  { 14, 12,  7, 11,  9, 26,  2, 10, 27,  8,  0,  6,  1, 15,  1},
  { 14,  0,  8, 15, 10,  4, 23, 13,  5,  9, 25,  3, 24, 22,  1},
  {  5,  6,  2,  2,  8,  0, 28,  3,  1,  3, 29,  1, 19, 18, 15}
};
static const uint8_t vc9_subblkpat_bits[3][15] = {
  {  5,  5,  5,  5,  5,  6,  4,  5,  6,  5,  4,  5,  4,  5,  1},
  {  4,  3,  4,  4,  4,  5,  5,  4,  5,  4,  5,  4,  5,  5,  2},
  {  3,  3,  4,  3,  4,  5,  5,  3,  5,  4,  5,  4,  5,  5,  4}
};

/* MV differential tables, p265 */
static const uint16_t vc9_mv_diff_codes[4][73] = {
  {
       0,    2,    3,    8,  576,    3,    2,    6,
       5,  577,  578,    7,    8,    9,   40,   19,
      37,   82,   21,   22,   23,  579,  580,  166,
      96,  167,   49,  194,  195,  581,  582,  583,
     292,  293,  294,   13,    2,    7,   24,   50,
     102,  295,   13,    7,    8,   18,   50,  103,
      38,   20,   21,   22,   39,  204,  103,   23,
      24,   25,  104,  410,  105,  106,  107,  108,
     109,  220,  411,  442,  222,  443,  446,  447,
       7 /* 73 elements */
  },
  {
       0,    4,    5,    3,    4,    3,    4,    5,
      20,    6,   21,   44,   45,   46, 3008,   95,
     112,  113,   57, 3009, 3010,  116,  117, 3011,
     118, 3012, 3013, 3014, 3015, 3016, 3017, 3018,
    3019, 3020, 3021, 3022,    1,    4,   15,  160,
     161,   41,    6,   11,   42,  162,   43,  119,
      56,   57,   58,  163,  236,  237, 3023,  119,
     120,  242,  122,  486, 1512,  487,  246,  494,
    1513,  495, 1514, 1515, 1516, 1517, 1518, 1519,
      31 /* 73 elements */
  },
  {
       0,  512,  513,  514,  515,    2,    3,  258,
     259,  260,  261,  262,  263,  264,  265,  266,
     267,  268,  269,  270,  271,  272,  273,  274,
     275,  276,  277,  278,  279,  280,  281,  282,
     283,  284,  285,  286,    1,    5,  287,  288,
     289,  290,    6,    7,  291,  292,  293,  294,
     295,  296,  297,  298,  299,  300,  301,  302,
     303,  304,  305,  306,  307,  308,  309,  310,
     311,  312,  313,  314,  315,  316,  317,  318,
     319 /* 73 elements */
  },
  {
       0,    1,    1,    2,    3,    4,    1,    5,
       4,    3,    5,    8,    6,    9,   10,   11,
      12,    7,  104,   14,  105,    4,   10,   15,
      11,    6,   14,    8,  106,  107,  108,   15,
     109,    9,   55,   10,    1,    2,    1,    2,
       3,   12,    6,    2,    6,    7,   28,    7,
      15,    8,    5,   18,   29,  152,   77,   24,
      25,   26,   39,  108,   13,  109,   55,   56,
      57,  116,   11,  153,  234,  235,  118,  119,
      15 /* 73 elements */
  }
};
static const uint8_t vc9_mv_diff_bits[4][73] = {
  {
     6,  7,  7,  8, 14,  6,  5,  6,  7, 14, 14,  6,  6,  6,  8,  9,
    10,  9,  7,  7,  7, 14, 14, 10,  9, 10,  8, 10, 10, 14, 14, 14,
    13, 13, 13,  6,  3,  5,  6,  8,  9, 13,  5,  4,  4,  5,  7,  9,
     6,  5,  5,  5,  6,  9,  8,  5,  5,  5,  7, 10,  7,  7,  7,  7,
     7,  8, 10,  9,  8,  9,  9,  9,  3 /* 73 elements */
  },
  {
     5,  7,  7,  6,  6,  5,  5,  6,  7,  5,  7,  8,  8,  8, 14,  9,
     9,  9,  8, 14, 14,  9,  9, 14,  9, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14,  2,  3,  6,  8,  8,  6,  3,  4,  6,  8,  6,  9,
     6,  6,  6,  8,  8,  8, 14,  7,  7,  8,  7,  9, 13,  9,  8,  9,
    13,  9, 13, 13, 13, 13, 13, 13,  5 /* 73 elements */
     
  },
  {
     3, 12, 12, 12, 12,  3,  4, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11,  1,  5, 11, 11, 11, 11,  4,  4, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11 /* 73 elements */
  },
  {
    15, 11, 15, 15, 15, 15, 12, 15, 12, 11, 12, 12, 15, 12, 12, 12,
    12, 15, 15, 12, 15, 10, 11, 12, 11, 10, 11, 10, 15, 15, 15, 11,
    15, 10, 14, 10,  4,  4,  5,  7,  8,  9,  5,  3,  4,  5,  6,  8,
     5,  4,  3,  5,  6,  8,  7,  5,  5,  5,  6,  7,  9,  7,  6,  6,
     6,  7, 10,  8,  8,  8,  7,  7,  4 /* 73 elements */
  }
};

/* DC differentials low+hi-mo, p217 are the same as in msmpeg4data .h */

/* Scantables/ZZ scan are at 11.9 (p262) and 8.1.1.12 (p10) */

#endif /* VC9DATA_H */