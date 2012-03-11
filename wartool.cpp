//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//   Utility for Stratagus - A free fantasy real time strategy game engine
//
/**@name wartool.c - Extract files from war archives. */
//
//      (c) Copyright 1999-2011 by Lutz Sammer, Nehal Mistry, Jimmy Salmon
//                              and Pali Rohár
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; version 2 dated June, 1991.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

//@{

/*----------------------------------------------------------------------------
--  General
----------------------------------------------------------------------------*/

#define VERSION "1.3" // Version of extractor wartool

const char NameLine[] = "wartool V" VERSION " for Stratagus, (c) 1998-2011 by The Stratagus Project.\n"\
"  Written by Lutz Sammer, Nehal Mistry, and Jimmy Salmon and Pali Rohar.\n"\
"  https://launchpad.net/wargus";

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <ctype.h>
#include <png.h>

#if defined(_MSC_VER) || defined(WIN32)
#include <windows.h>
#endif

#ifdef _MSC_VER
#define inline __inline
#define strdup _strdup
#define DEBUG _DEBUG
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "endian.h"
#include "xmi2mid.h"
#include "rip_music.h"
#include "pud.h"

#ifndef __GNUC__
#define __attribute__(args)  // Does nothing for non GNU CC
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

//----------------------------------------------------------------------------
//  Config
//----------------------------------------------------------------------------

/**
**		Destination directory of the graphics
*/
char* Dir;

/**
**		Path to the tileset graphics. (default=$DIR/graphics/tilesets)
*/
#define TILESET_PATH		"graphics/tilesets"

/**
**		Path to the unit graphics. (default=$DIR/graphics)
*/
#define GRAPHICS_PATH		"graphics"

/**
**		Path the pud files. (default=$DIR)
*/
#define PUD_PATH		"."

/**
**		Path the font files. (default=$DIR/graphics/ui/fonts)
*/
#define FONT_PATH		"graphics/ui/fonts"

/**
**		Path the cursor files. (default=$DIR/graphic/ui/)
*/
#define CURSOR_PATH		"graphics/ui"

/**
**		Path the graphic files. (default=$DIR/graphic)
*/
#define GRAPHIC_PATH		"graphics"

/**
**		Path the sound files. (default=$DIR/sounds)
*/
#define SOUND_PATH		"sounds"

/**
**		Path the sound files. (default=$DIR/music)
*/
#define MUSIC_PATH		"music"

/**
**		Path the text files. (default=$DIR/texts)
*/
#define TEXT_PATH		"campaigns"

/**
**		How much tiles are stored in a row.
*/
#define TILE_PER_ROW		16

//----------------------------------------------------------------------------

/**
**  Conversion control sturcture.
*/
typedef struct _control_ {
	int   Type;       /// Entry type
	int   Version;    /// Only in this version
	char* File;       /// Save file
	int   Arg1;       /// Extra argument 1
	int   Arg2;       /// Extra argument 2
	int   Arg3;       /// Extra argument 3
	int   Arg4;       /// Extra argument 4
} Control;

/**
**  Palette N27, for credits cursor
*/
static unsigned char* Pal27;

/**
**  Original archive buffer.
*/
static unsigned char* ArchiveBuffer;

/**
**  Offsets for each entry into original archive buffer.
*/
static unsigned char** ArchiveOffsets;

/**
**  Possible entry types of archive file.
*/
enum _archive_type_ {
	S,    // Setup
	F,    // File                          (name)
	T,    // Tileset                       (name,pal,mega,mini,map)
	R,    // RGB -> gimp                   (name,rgb)
	G,    // Graphics                      (name,pal,gfx)
	U,    // Uncompressed Graphics         (name,pal,gfu)
	D,    // Grouped Uncompressed Graphic  (name,pal,gfu,glist)
	P,    // Pud                           (name,idx)
	N,    // Font                          (name,idx)
	I,    // Image                         (name,pal,img)
	W,    // Wav                           (name,wav)
	M,    // XMI Midi Sound                (name,xmi)
	X,    // Text                          (name,text,ofs)
	C,    // Cursor                        (name,cursor)
	V,    // Video                         (name,video)
	L,    // Campaign Levels
};

static char* ArchiveDir;

#define CD_MAC        (1)
#define CD_EXPANSION  (1 << 1)
#define CD_US         (1 << 4)
#define CD_SPANISH    (1 << 5)
#define CD_GERMAN     (1 << 6)
#define CD_UK         (1 << 7)  // also Australian
#define CD_ITALIAN    (1 << 8)
#define CD_PORTUGUESE (1 << 9)
#define CD_FRENCH     (1 << 10)

#define CD_UPPER      (1 << 12) // Filenames on CD are upper

/**
**  What CD Type is it?
*/
static int CDType;

// Width of game font
static int game_font_width;

/**
**  What, where, how to extract.
**
**  FIXME: version alpha, demo, 1.00, 1.31, 1.40, 1.50 dependend!
*/
static Control Todo[] = {
#define __ ,0,0,0
#define _2 ,0,0,

///////////////////////////////////////////////////////////////////////////////
//		TEXT		(must be done for all others!)
///////////////////////////////////////////////////////////////////////////////

{F,0,"rezdat.war",                                     3000 __},
{I,0,"ui/Credits_background",                          27, 28 _2},

{F,0,"strdat.war",                                     4000 __},
{S,0,"unit_names",                                     1 __},
{L,0,"objectives",                                     54, 236 _2},
//{X,0,"human/dialog",                                   55 __},
//{X,0,"orc/dialog",                                     56 __},
{X,0,"credits",                                        58, 4 _2},

{X,0,"human/level01h",                                 65, 4 _2},
{X,0,"orc/level01o",                                   66, 4 _2},
{X,0,"human/level02h",                                 67, 4 _2},
{X,0,"orc/level02o",                                   68, 4 _2},
{X,0,"human/level03h",                                 69, 4 _2},
{X,0,"orc/level03o",                                   70, 4 _2},
{X,0,"human/level04h",                                 71, 4 _2},
{X,0,"orc/level04o",                                   72, 4 _2},
{X,0,"human/level05h",                                 73, 4 _2},
{X,0,"orc/level05o",                                   74, 4 _2},
{X,0,"human/level06h",                                 75, 4 _2},
{X,0,"orc/level06o",                                   76, 4 _2},
{X,0,"human/level07h",                                 77, 4 _2},
{X,0,"orc/level07o",                                   78, 4 _2},
{X,0,"human/level08h",                                 79, 4 _2},
{X,0,"orc/level08o",                                   80, 4 _2},
{X,0,"human/level09h",                                 81, 4 _2},
{X,0,"orc/level09o",                                   82, 4 _2},
{X,0,"human/level10h",                                 83, 4 _2},
{X,0,"orc/level10o",                                   84, 4 _2},
{X,0,"human/level11h",                                 85, 4 _2},
{X,0,"orc/level11o",                                   86, 4 _2},
{X,0,"human/level12h",                                 87, 4 _2},
{X,0,"orc/level12o",                                   88, 4 _2},
{X,0,"human/level13h",                                 89, 4 _2},
{X,0,"orc/level13o",                                   90, 4 _2},
{X,0,"human/level14h",                                 91, 4 _2},
{X,0,"orc/level14o",                                   92, 4 _2},
{X,2,"human-exp/levelx01h",                            99, 4 _2},
{X,2,"orc-exp/levelx01o",                              100, 4 _2},
{X,2,"human-exp/levelx02h",                            101, 4 _2},
{X,2,"orc-exp/levelx02o",                              102, 4 _2},
{X,2,"human-exp/levelx03h",                            103, 4 _2},
{X,2,"orc-exp/levelx03o",                              104, 4 _2},
{X,2,"human-exp/levelx04h",                            105, 4 _2},
{X,2,"orc-exp/levelx04o",                              106, 4 _2},
{X,2,"human-exp/levelx05h",                            107, 4 _2},
{X,2,"orc-exp/levelx05o",                              108, 4 _2},
{X,2,"human-exp/levelx06h",                            109, 4 _2},
{X,2,"orc-exp/levelx06o",                              110, 4 _2},
{X,2,"human-exp/levelx07h",                            111, 4 _2},
{X,2,"orc-exp/levelx07o",                              112, 4 _2},
{X,2,"human-exp/levelx08h",                            113, 4 _2},
{X,2,"orc-exp/levelx08o",                              114, 4 _2},
{X,2,"human-exp/levelx09h",                            115, 4 _2},
{X,2,"orc-exp/levelx09o",                              116, 4 _2},
{X,2,"human-exp/levelx10h",                            117, 4 _2},
{X,2,"orc-exp/levelx10o",                              118, 4 _2},
{X,2,"human-exp/levelx11h",                            119, 4 _2},
{X,2,"orc-exp/levelx11o",                              120, 4 _2},
{X,2,"human-exp/levelx12h",                            121, 4 _2},
{X,2,"orc-exp/levelx12o",                              122, 4 _2},
{X,2,"credits2",                                       123, 4 _2},
//{X,2,"credits-ext",                                    124 __},
{X,3,"human/victory",                                  93, 6 _2},
{X,3,"orc/victory",                                    93, 556 _2},
{X,2,"human/victory",                                  93, 10 _2},
{X,2,"orc/victory",                                    93, 560 _2},
{X,2,"human-exp/victory",                              93, 1086 _2},
{X,2,"orc-exp/victory",                                93, 1840 _2},

///////////////////////////////////////////////////////////////////////////////
//		MOST THINGS
///////////////////////////////////////////////////////////////////////////////

{F,0,"maindat.war",                                    1000 __},

{R,0,"summer/summer",                                  2 __},
{T,0,"summer/terrain/summer",                          2, 3, 4, 5},
{R,0,"wasteland/wasteland",                            10 __},
{T,0,"wasteland/terrain/wasteland",                    10, 11, 12, 13},
{R,3,"swamp/swamp",                                    10 __},
{T,3,"swamp/terrain/swamp",                            10, 11, 12, 13},
{R,0,"winter/winter",                                  18 __},
{T,0,"winter/terrain/winter",                          18, 19, 20, 21},

{G,0,"human/units/%16",                                2, 33 _2},
{G,0,"orc/units/%17",                                  2, 34 _2},
{G,0,"human/units/%44",                                2, 35 _2},
{G,0,"orc/units/%45",                                  2, 36 _2},
{G,0,"orc/units/%47",                                  2, 37 _2},
{G,0,"human/units/%42",                                2, 38 _2},
{G,0,"human/units/%-30",                               2, 39 _2},
{G,0,"orc/units/%-31",                                 2, 40 _2},
{G,0,"human/units/%34",                                2, 41 _2},
{G,0,"orc/units/%35",                                  2, 42 _2},
{G,0,"human/units/%40",                                2, 43 _2},
{G,0,"orc/units/%41",                                  2, 44 _2},
{G,0,"human/units/%2",                                 2, 45 _2},
{G,0,"orc/units/%3",                                   2, 46 _2},
{G,0,"human/units/%4",                                 2, 47 _2},
{G,0,"orc/units/%5",                                   2, 48 _2},
{G,0,"human/units/%6",                                 2, 49 _2},
{G,0,"orc/units/%7",                                   2, 50 _2},
{G,0,"human/units/%8",                                 2, 51 _2},
{G,0,"orc/units/%9",                                   2, 52 _2},
{G,0,"human/units/%10",                                2, 53 _2},
{G,0,"orc/units/%11",                                  2, 54 _2},
{G,0,"human/units/%12",                                2, 55 _2},
{G,0,"orc/units/%13",                                  2, 58 _2},
{G,0,"human/units/%-28_empty",                         2, 59 _2},
{G,0,"orc/units/%-29_empty",                           2, 60 _2},
{G,0,"human/units/%32",                                2, 61 _2},
{G,0,"orc/units/%33",                                  2, 62 _2},
{G,0,"orc/units/%43",                                  2, 63 _2},
{G,0,"tilesets/summer/neutral/units/%59",              2, 64 _2},
{G,0,"tilesets/wasteland/neutral/units/%59",           10, 65 _2},
{G,3,"tilesets/swamp/neutral/units/%59",               10, 65 _2},
{G,0,"tilesets/winter/neutral/units/%59",              18, 66 _2},
{G,0,"neutral/units/%57",                              2, 69 _2},
{G,0,"neutral/units/%58",                              2, 70 _2},
// 71-79 unknown
{G,0,"tilesets/summer/human/buildings/%-98",           2, 80 _2},
{G,0,"tilesets/summer/orc/buildings/%-99",             2, 81 _2},
{G,0,"tilesets/summer/human/buildings/%-100",          2, 82 _2},
{G,0,"tilesets/summer/orc/buildings/%-101",            2, 83 _2},
{G,0,"tilesets/summer/human/buildings/%82",            2, 84 _2},
{G,0,"tilesets/summer/orc/buildings/%83",              2, 85 _2},
{G,0,"tilesets/summer/human/buildings/%90",            2, 86 _2},
{G,0,"tilesets/summer/orc/buildings/%91",              2, 87 _2},
{G,0,"tilesets/summer/human/buildings/%72",            2, 88 _2},
{G,0,"tilesets/summer/orc/buildings/%73",              2, 89 _2},
{G,0,"tilesets/summer/human/buildings/%70",            2, 90 _2},
{G,0,"tilesets/summer/orc/buildings/%71",              2, 91 _2},
{G,0,"tilesets/summer/human/buildings/%60",            2, 92 _2},
{G,0,"tilesets/summer/orc/buildings/%61",              2, 93 _2},
{G,0,"tilesets/summer/human/buildings/%-62",           2, 94 _2},
{G,0,"tilesets/summer/orc/buildings/%-63",             2, 95 _2},
{G,0,"tilesets/summer/human/buildings/%64",            2, 96 _2},
{G,0,"tilesets/summer/orc/buildings/%65",              2, 97 _2},
{G,0,"tilesets/summer/human/buildings/%66",            2, 98 _2},
{G,0,"tilesets/summer/orc/buildings/%67",              2, 99 _2},
{G,0,"tilesets/summer/human/buildings/%76",            2, 100 _2},
{G,0,"tilesets/summer/orc/buildings/%77",              2, 101 _2},
{G,0,"tilesets/summer/human/buildings/%78",            2, 102 _2},
{G,0,"tilesets/summer/orc/buildings/%79",              2, 103 _2},
{G,0,"tilesets/summer/human/buildings/%68",            2, 104 _2},
{G,0,"tilesets/summer/orc/buildings/%69",              2, 105 _2},
{G,0,"tilesets/summer/human/buildings/%-84",           2, 106  _2},
{G,0,"tilesets/summer/orc/buildings/%-85",             2, 107 _2},
{G,0,"tilesets/summer/human/buildings/%-74",           2, 108 _2},
{G,0,"tilesets/summer/orc/buildings/%-75",             2, 109  _2},
{G,0,"tilesets/summer/human/buildings/%-80",           2, 110  _2},
{G,0,"tilesets/summer/orc/buildings/%-81",             2, 111  _2},
{G,0,"tilesets/summer/human/buildings/%-86",           2, 112  _2},
{G,0,"tilesets/summer/orc/buildings/%-87",             2, 113  _2},
{G,0,"tilesets/summer/human/buildings/%-88",           2, 114  _2},
{G,0,"tilesets/summer/orc/buildings/%-89",             2, 115  _2},
{G,0,"tilesets/summer/human/buildings/%92",            2, 116  _2},
{G,0,"tilesets/summer/orc/buildings/%93",              2, 117  _2},
{G,0,"tilesets/summer/neutral/buildings/%95",          2, 118  _2},
{G,0,"tilesets/summer/neutral/buildings/%94",          2, 119  _2},
{G,3,"tilesets/swamp/human/buildings/%-98",            2, 80 _2},
{G,3,"tilesets/swamp/orc/buildings/%-99",              2, 81 _2},
{G,3,"tilesets/swamp/human/buildings/%-100",           2, 82 _2},
{G,3,"tilesets/swamp/orc/buildings/%-101",             2, 83 _2},
{G,3,"tilesets/swamp/human/buildings/%82",             2, 84 _2},
{G,3,"tilesets/swamp/orc/buildings/%83",               2, 85 _2},
{G,3,"tilesets/swamp/human/buildings/%90",             2, 86 _2},
{G,3,"tilesets/swamp/orc/buildings/%91",               2, 87 _2},
{G,3,"tilesets/swamp/human/buildings/%72",             2, 88 _2},
{G,3,"tilesets/swamp/orc/buildings/%73",               2, 89 _2},
{G,3,"tilesets/swamp/human/buildings/%70",             2, 90 _2},
{G,3,"tilesets/swamp/orc/buildings/%71",               2, 91 _2},
{G,3,"tilesets/swamp/human/buildings/%60",             2, 92 _2},
{G,3,"tilesets/swamp/orc/buildings/%61",               2, 93 _2},
{G,3,"tilesets/swamp/human/buildings/%-62",            2, 94 _2},
{G,3,"tilesets/swamp/orc/buildings/%-63",              2, 95 _2},
{G,3,"tilesets/swamp/human/buildings/%64",             2, 96 _2},
{G,3,"tilesets/swamp/orc/buildings/%65",               2, 97 _2},
{G,3,"tilesets/swamp/human/buildings/%66",             2, 98 _2},
{G,3,"tilesets/swamp/orc/buildings/%67",               2, 99 _2},
{G,3,"tilesets/swamp/human/buildings/%76",             2, 100 _2},
{G,3,"tilesets/swamp/orc/buildings/%77",               2, 101 _2},
{G,3,"tilesets/swamp/human/buildings/%78",             2, 102 _2},
{G,3,"tilesets/swamp/orc/buildings/%79",               2, 103 _2},
{G,3,"tilesets/swamp/human/buildings/%68",             2, 104 _2},
{G,3,"tilesets/swamp/orc/buildings/%69",               2, 105 _2},
{G,3,"tilesets/swamp/human/buildings/%-84",            2, 106 _2},
{G,3,"tilesets/swamp/orc/buildings/%-85",              2, 107 _2},
{G,3,"tilesets/swamp/human/buildings/%-74",            2, 108 _2},
{G,3,"tilesets/swamp/orc/buildings/%-75",              2, 109 _2},
{G,3,"tilesets/swamp/human/buildings/%-80",            2, 110 _2},
{G,3,"tilesets/swamp/orc/buildings/%-81",              2, 111 _2},
{G,3,"tilesets/swamp/human/buildings/%-86",            2, 112 _2},
{G,3,"tilesets/swamp/orc/buildings/%-87",              2, 113 _2},
{G,3,"tilesets/swamp/human/buildings/%-88",            2, 114 _2},
{G,3,"tilesets/swamp/orc/buildings/%-89",              2, 115 _2},
{G,3,"tilesets/swamp/human/buildings/%92",             2, 116 _2},
{G,3,"tilesets/swamp/orc/buildings/%93",               2, 117 _2},
{G,0,"neutral/units/corpses",                          2, 120 _2},
{G,0,"tilesets/summer/neutral/buildings/destroyed_site",
                                                       2, 121  _2},
	// Hardcoded support for worker with resource repairing
{G,0,"human/units/%4_with_wood",                       2, 122, 47, 25},
{G,0,"orc/units/%5_with_wood",                         2, 123, 48, 25},
{G,0,"human/units/%4_with_gold",                       2, 124, 47, 25},
{G,0,"orc/units/%5_with_gold",                         2, 125, 48, 25},
{G,0,"human/units/%-28_full",                          2, 126 _2},
{G,0,"orc/units/%-29_full",                            2, 127 _2},
{G,0,"tilesets/winter/human/buildings/%90",            18, 128 _2},
{G,0,"tilesets/winter/orc/buildings/%91",              18, 129 _2},
{G,0,"tilesets/winter/human/buildings/%72",            18, 130 _2},
{G,0,"tilesets/winter/orc/buildings/%73",              18, 131 _2},
{G,0,"tilesets/winter/human/buildings/%70",            18, 132 _2},
{G,0,"tilesets/winter/orc/buildings/%71",              18, 133 _2},
{G,0,"tilesets/winter/human/buildings/%60",            18, 134 _2},
{G,0,"tilesets/winter/orc/buildings/%61",              18, 135 _2},
{G,0,"tilesets/winter/human/buildings/%-62",           18, 136 _2},
{G,0,"tilesets/winter/orc/buildings/%-63",             18, 137 _2},
{G,0,"tilesets/winter/human/buildings/%64",            18, 138 _2},
{G,0,"tilesets/winter/orc/buildings/%65",              18, 139 _2},
{G,0,"tilesets/winter/human/buildings/%66",            18, 140 _2},
{G,0,"tilesets/winter/orc/buildings/%67",              18, 141 _2},
{G,0,"tilesets/winter/human/buildings/%76",            18, 142 _2},
{G,0,"tilesets/winter/orc/buildings/%77",              18, 143 _2},
{G,0,"tilesets/winter/human/buildings/%78",            18, 144 _2},
{G,0,"tilesets/winter/orc/buildings/%79",              18, 145 _2},
{G,0,"tilesets/winter/human/buildings/%68",            18, 146 _2},
{G,0,"tilesets/winter/orc/buildings/%69",              18, 147 _2},
{G,0,"tilesets/winter/human/buildings/%-84",           18, 148 _2},
{G,0,"tilesets/winter/orc/buildings/%-85",             18, 149 _2},
{G,0,"tilesets/winter/human/buildings/%-74",           18, 150 _2},
{G,0,"tilesets/winter/orc/buildings/%-75",             18, 151 _2},
{G,0,"tilesets/winter/human/buildings/%-80",           18, 152 _2},
{G,0,"tilesets/winter/orc/buildings/%-81",             18, 153 _2},
{G,0,"tilesets/winter/human/buildings/%-86",           18, 154 _2},
{G,0,"tilesets/winter/orc/buildings/%-87",             18, 155 _2},
{G,0,"tilesets/winter/human/buildings/%-88",           18, 156 _2},
{G,0,"tilesets/winter/orc/buildings/%-89",             18, 157 _2},
{G,0,"tilesets/winter/human/buildings/%92",            18, 158 _2},
{G,0,"tilesets/winter/orc/buildings/%93",              18, 159 _2},
{G,0,"tilesets/winter/human/buildings/%82",            18, 160 _2},
{G,0,"tilesets/winter/orc/buildings/%83",              18, 161 _2},
{G,0,"tilesets/winter/neutral/buildings/%94",          18, 162 _2},
{G,0,"tilesets/winter/neutral/buildings/destroyed_site",
                                                       18, 163 _2},
{G,0,"human/x_startpoint",                             2, 164 _2},
{G,0,"orc/o_startpoint",                               2, 165 _2},
{G,0,"neutral/buildings/%102",                         2, 166 _2},
{G,0,"tilesets/summer/neutral/buildings/%103",         2, 167 _2},
{G,0,"tilesets/summer/neutral/buildings/%105",         2, 168 _2},
{G,0,"tilesets/winter/human/buildings/%-98",           18, 169 _2},
{G,0,"tilesets/winter/orc/buildings/%-99",             18, 170 _2},
{G,0,"tilesets/winter/human/buildings/%-100",          18, 171 _2},
{G,0,"tilesets/winter/orc/buildings/%-101",            18, 172 _2},
{G,0,"tilesets/wasteland/human/buildings/%60",         10, 173 _2},
{G,0,"tilesets/wasteland/orc/buildings/%61",           10, 174 _2},
{G,0,"tilesets/wasteland/human/buildings/%78",         10, 175 _2},
{G,0,"tilesets/wasteland/orc/buildings/%79",           10, 176 _2},
{G,0,"tilesets/wasteland/human/buildings/%-88",        10, 177 _2},
{G,0,"tilesets/wasteland/orc/buildings/%-89",          10, 178 _2},
{G,0,"tilesets/wasteland/neutral/buildings/%94",       10, 179 _2},
{G,0,"tilesets/wasteland/neutral/buildings/%95",       10, 180 _2},
{G,3,"tilesets/swamp/human/buildings/%60",             10, 173 _2},
{G,3,"tilesets/swamp/orc/buildings/%61",               10, 174 _2},
{G,3,"tilesets/swamp/human/buildings/%78",             10, 175 _2},
{G,3,"tilesets/swamp/orc/buildings/%79",               10, 176 _2},
{G,3,"tilesets/swamp/human/buildings/%-88",            10, 177 _2},
{G,3,"tilesets/swamp/orc/buildings/%-89",              10, 178 _2},
{G,3,"tilesets/swamp/neutral/buildings/%94",           10, 179 _2},
{G,3,"tilesets/swamp/neutral/buildings/%95",           10, 180 _2},
{G,0,"neutral/buildings/%104",                         2, 181 _2},
{G,0,"tilesets/wasteland/human/units/%40",             10, 182 _2},
{G,0,"tilesets/wasteland/orc/units/%41",               10, 183 _2},
{G,3,"tilesets/swamp/human/units/%40",                 10, 182 _2},
{G,3,"tilesets/swamp/orc/units/%41",                   10, 183 _2},
{G,0,"tilesets/winter/neutral/buildings/%103",         18, 184 _2},
{G,0,"tilesets/wasteland/neutral/buildings/%103",      10, 185 _2},
{G,3,"tilesets/swamp/neutral/buildings/%103",          10, 185 _2},
{G,3,"tilesets/swamp/neutral/buildings/%104",          2, 181 _2},
{G,0,"tilesets/winter/neutral/buildings/%104",         18, 186 _2},
{U,0,"ui/gold,wood,oil,mana",                          2, 187 _2},
{G,0,"tilesets/wasteland/neutral/buildings/small_destroyed_site",
                                                       10, 188 _2},
{G,3,"tilesets/swamp/neutral/buildings/small_destroyed_site",
                                                       10, 188 _2},
{G,0,"tilesets/summer/neutral/buildings/small_destroyed_site",
                                                       2, 189 _2},
{G,0,"tilesets/winter/neutral/buildings/small_destroyed_site",
                                                       18, 190 _2},
{G,0,"tilesets/wasteland/neutral/buildings/destroyed_site",
                                                       10, 191 _2},
{G,3,"tilesets/swamp/neutral/buildings/destroyed_site",
                                                       10, 191 _2},
//--------------------------------------------------
{P,0,"campaigns/human/level01h",                       192 __},
{P,0,"campaigns/orc/level01o",                         193 __},
{P,0,"campaigns/human/level02h",                       194 __},
{P,0,"campaigns/orc/level02o",                         195 __},
{P,0,"campaigns/human/level03h",                       196 __},
{P,0,"campaigns/orc/level03o",                         197 __},
{P,0,"campaigns/human/level04h",                       198 __},
{P,0,"campaigns/orc/level04o",                         199 __},
{P,0,"campaigns/human/level05h",                       200 __},
{P,0,"campaigns/orc/level05o",                         201 __},
{P,0,"campaigns/human/level06h",                       202 __},
{P,0,"campaigns/orc/level06o",                         203 __},
{P,0,"campaigns/human/level07h",                       204 __},
{P,0,"campaigns/orc/level07o",                         205 __},
{P,0,"campaigns/human/level08h",                       206 __},
{P,0,"campaigns/orc/level08o",                         207 __},
{P,0,"campaigns/human/level09h",                       208 __},
{P,0,"campaigns/orc/level09o",                         209 __},
{P,0,"campaigns/human/level10h",                       210 __},
{P,0,"campaigns/orc/level10o",                         211 __},
{P,0,"campaigns/human/level11h",                       212 __},
{P,0,"campaigns/orc/level11o",                         213 __},
{P,0,"campaigns/human/level12h",                       214 __},
{P,0,"campaigns/orc/level12o",                         215 __},
{P,0,"campaigns/human/level13h",                       216 __},
{P,0,"campaigns/orc/level13o",                         217 __},
{P,0,"campaigns/human/level14h",                       218 __},
{P,0,"campaigns/orc/level14o",                         219 __},
// --------------------------------------------------
{P,0,"maps/multi/(6)gold-separates-east-west",         220 __},
{P,0,"maps/multi/(4)oil-is-the-key",                   221 __},
{P,0,"maps/multi/(8)bridge-to-bridge-combat",          222 __},
{P,0,"maps/multi/(8)plains-of-snow",                   223 __},
{P,0,"maps/multi/(3)three-ways-to-cross",              224 __},
{P,0,"maps/multi/(8)a-continent-to-explore",           225 __},
{P,0,"maps/multi/(8)high-seas-combat",                 226 __},
{P,0,"maps/multi/(4)islands-in-the-stream",            227 __},
{P,0,"maps/multi/(4)opposing-city-states",             228 __},
{P,0,"maps/multi/(4)death-in-the-middle",              229 __},
{P,0,"maps/multi/(5)the-spiral",                       230 __},
{P,0,"maps/multi/(2)mysterious-dragon-isle",           231 __},
{P,0,"maps/multi/(4)the-four-corners",                 232 __},
{P,0,"maps/multi/(6)no-way-out-of-this-maze",          233 __},
{P,0,"maps/multi/(8)fierce-ocean-combat",              234 __},
{P,0,"maps/multi/(2)cross-the-streams",                235 __},
{P,0,"maps/multi/(5)how-much-can-you-mine",            236 __},
{P,0,"maps/multi/(2)x-marks-the-spot",                 237 __},
{P,0,"maps/multi/(2)one-way-in-one-way-out",           238 __},
{P,0,"maps/multi/(4)nowhere-to-run",                   239 __},
{P,0,"maps/multi/(2)2-players",                        240 __},
{P,0,"maps/multi/(7)and-the-rivers-ran-as-blood",      241 __},
{P,0,"maps/multi/(3)critter-attack",                   242 __},
{P,0,"maps/multi/(6)skull-isle",                       243 __},
{P,0,"maps/multi/(8)cross-over",                       244 __},
{P,0,"maps/multi/(8)garden-of-war",                    245 __},
{P,0,"maps/multi/(4)mine-the-center",                  246 __},
{P,0,"maps/multi/(3)river-fork",                       247 __},
// -------------------------------------------------
{P,0,"maps/demo/demo01",                               248 __},
{P,0,"maps/demo/demo02",                               249 __},
{P,0,"maps/demo/demo03",                               250 __},
{P,0,"maps/demo/demo04",                               251 __},
// --------------------------------------------------
{G,0,"neutral/buildings/land_construction_site",       2, 252 _2},
{G,0,"human/buildings/shipyard_construction_site",     2, 253 _2},
{G,0,"orc/buildings/shipyard_construction_site",       2, 254 _2},
{G,3,"tilesets/swamp/neutral/buildings/land_construction_site",
                                                       2, 252 _2},
{G,3,"tilesets/swamp/human/buildings/shipyard_construction_site",
                                                       2, 253 _2},
{G,3,"tilesets/swamp/orc/buildings/shipyard_construction_site",
                                                       2, 254 _2},
{G,0,"tilesets/summer/human/buildings/oil_well_construction_site",
                                                       2, 255 _2},
{G,0,"tilesets/summer/orc/buildings/oil_well_construction_site",
                                                       2, 256 _2},
{G,0,"human/buildings/refinery_construction_site",     2, 257 _2},
{G,0,"orc/buildings/refinery_construction_site",       2, 258 _2},
{G,0,"human/buildings/foundry_construction_site",      2, 259 _2},
{G,0,"orc/buildings/foundry_construction_site",        2, 260 _2},
{G,3,"tilesets/swamp/human/buildings/refinery_construction_site",
                                                       2, 257 _2},
{G,3,"tilesets/swamp/orc/buildings/refinery_construction_site",
                                                       2, 258 _2},
{G,3,"tilesets/swamp/human/buildings/foundry_construction_site",
                                                       2, 259 _2},
{G,3,"tilesets/swamp/orc/buildings/foundry_construction_site",
                                                       2, 260 _2},
{G,0,"tilesets/summer/neutral/buildings/wall_construction_site",
                                                       2, 261 _2},
{G,0,"tilesets/winter/neutral/buildings/land_construction_site",
                                                       18, 262 _2},
{G,0,"tilesets/winter/human/buildings/shipyard_construction_site",
                                                       18, 263 _2},
{G,0,"tilesets/winter/orc/buildings/shipyard_construction_site",
                                                       18, 264 _2},
{G,0,"tilesets/winter/human/buildings/oil_well_construction_site",
                                                       18, 265 _2},
{G,0,"tilesets/winter/orc/buildings/oil_well_construction_site",
                                                       18, 266 _2},
{G,0,"tilesets/winter/human/buildings/refinery_construction_site",
                                                       18, 267 _2},
{G,0,"tilesets/winter/orc/buildings/refinery_construction_site",
                                                       18, 268 _2},
{G,0,"tilesets/winter/human/buildings/foundry_construction_site",
                                                       18, 269 _2},
{G,0,"tilesets/winter/orc/buildings/foundry_construction_site",
                                                       18, 270 _2},
{G,0,"tilesets/wasteland/human/buildings/oil_well_construction_site",
                                                       10, 271 _2},
{G,0,"tilesets/wasteland/orc/buildings/oil_well_construction_site",
                                                       10, 272 _2},
{G,3,"tilesets/swamp/human/buildings/oil_platform_construction_site",
                                                       10, 271 _2},
{G,3,"tilesets/swamp/orc/buildings/oil_platform_construction_site",
                                                       10, 272 _2},
{G,0,"tilesets/winter/neutral/buildings/wall",         18, 273 _2},
{G,0,"tilesets/wasteland/neutral/buildings/wall",      10, 274 _2},
{G,3,"tilesets/swamp/neutral/buildings/wall",          10, 274 _2},
{G,0,"tilesets/winter/neutral/buildings/wall_construction_site",
                                                       18, 275 _2},
{G,0,"tilesets/wasteland/neutral/buildings/wall_construction_site",
                                                       10, 276 _2},
{G,3,"tilesets/swamp/neutral/buildings/wall_construction_site",
                                                       10, 276 _2},
// 277 Control programs for computer player AI
// 278 Control programs for unit movement
// --------------------------------------------------
{N,0,"large_episode_titles",                           279 __},
{N,0,"small_episode_titles",                           280 __},
{N,0,"large",                                          281 __},
{N,0,"game",                                           282 __},
{N,0,"small",                                          283 __},
// --------------------------------------------------
{I,0,"ui/human/menubutton",                            2, 293 _2},
{I,0,"ui/orc/menubutton",                              2, 294 _2},
{I,0,"ui/human/minimap",                               2, 295 _2},
{I,0,"ui/orc/minimap",                                 2, 296 _2},
{I,0,"ui/title",                                       300, 299 _2},
// 284-286 unknown
// --------------------------------------------------
{I,0,"ui/human/640x480/resource",                      2, 287 _2},
{I,0,"ui/orc/640x480/resource",                        2, 288 _2},
{I,0,"ui/human/640x480/filler-right",                  2, 289 _2},
{I,0,"ui/orc/640x480/filler-right",                    2, 290 _2},
{I,0,"ui/human/640x480/statusline",                    2, 291 _2},
{I,0,"ui/orc/640x480/statusline",                      2, 292 _2},
{I,0,"ui/human/640x480/buttonpanel",                   2, 297 _2},
{I,0,"ui/orc/640x480/buttonpanel",                     2, 298 _2},
//---------------------------------------------------
{I,0,"ui/human/800x480/resource",                      2, 287, 608, 16},
{I,0,"ui/orc/800x480/resource",                        2, 288, 608, 16},
{I,0,"ui/human/800x480/filler-right",                  2, 289, 16, 480},
{I,0,"ui/orc/800x480/filler-right",                    2, 290, 16, 480},
{I,0,"ui/human/800x480/statusline",                    2, 291, 608, 16},
{I,0,"ui/orc/800x480/statusline",                      2, 292, 608, 16},
{I,0,"ui/human/800x480/buttonpanel",                   2, 297, 176, 144},
{I,0,"ui/orc/800x480/buttonpanel",                     2, 298, 176, 144},
//---------------------------------------------------
{I,0,"ui/human/800x600/resource",                      2, 287, 608, 16}, //w-192 16
{I,0,"ui/orc/800x600/resource",                        2, 288, 608, 16}, //w-192 16
{I,0,"ui/human/800x600/filler-right",                  2, 289, 16, 600}, //16 h
{I,0,"ui/orc/800x600/filler-right",                    2, 290, 16, 600}, //16 h
{I,0,"ui/human/800x600/statusline",                    2, 291, 608, 16}, //w-192 16
{I,0,"ui/orc/800x600/statusline",                      2, 292, 608, 16}, //w-192 16
{I,0,"ui/human/800x600/buttonpanel",                   2, 297, 176, 264},//176, h-336
{I,0,"ui/orc/800x600/buttonpanel",                     2, 298, 176, 264},//176, h-336
//---------------------------------------------------
{I,0,"ui/human/1024x768/resource",                     2, 287, 832, 16},
{I,0,"ui/orc/1024x768/resource",                       2, 288, 832, 16},
{I,0,"ui/human/1024x768/filler-right",                 2, 289, 16, 768},
{I,0,"ui/orc/1024x768/filler-right",                   2, 290, 16, 768},
{I,0,"ui/human/1024x768/statusline",                   2, 291, 832, 16},
{I,0,"ui/orc/1024x768/statusline",                     2, 292, 832, 16},
{I,0,"ui/human/1024x768/buttonpanel",                  2, 297, 176, 432},
{I,0,"ui/orc/1024x768/buttonpanel",                    2, 298, 176, 432},
// --------------------------------------------------
{I,0,"ui/human/1280x800/resource",                     2, 287, 1088, 16},
{I,0,"ui/orc/1280x800/resource",                       2, 288, 1088, 16},
{I,0,"ui/human/1280x800/filler-right",                 2, 289, 16, 800},
{I,0,"ui/orc/1280x800/filler-right",                   2, 290, 16, 800},
{I,0,"ui/human/1280x800/statusline",                   2, 291, 1088, 16},
{I,0,"ui/orc/1280x800/statusline",                     2, 292, 1088, 16},
{I,0,"ui/human/1280x800/buttonpanel",                  2, 297, 176, 464},
{I,0,"ui/orc/1280x800/buttonpanel",                    2, 298, 176, 464},
// --------------------------------------------------
{I,0,"ui/human/1280x960/resource",                     2, 287, 1088, 16},
{I,0,"ui/orc/1280x960/resource",                       2, 288, 1088, 16},
{I,0,"ui/human/1280x960/filler-right",                 2, 289, 16, 960},
{I,0,"ui/orc/1280x960/filler-right",                   2, 290, 16, 960},
{I,0,"ui/human/1280x960/statusline",                   2, 291, 1088, 16},
{I,0,"ui/orc/1280x960/statusline",                     2, 292, 1088, 16},
{I,0,"ui/human/1280x960/buttonpanel",                  2, 297, 176, 624},
{I,0,"ui/orc/1280x960/buttonpanel",                    2, 298, 176, 624},
// --------------------------------------------------
{I,0,"ui/human/1280x1024/resource",                     2, 287, 1088, 16},
{I,0,"ui/orc/1280x1024/resource",                       2, 288, 1088, 16},
{I,0,"ui/human/1280x1024/filler-right",                 2, 289, 16, 1024},
{I,0,"ui/orc/1280x1024/filler-right",                   2, 290, 16, 1024},
{I,0,"ui/human/1280x1024/statusline",                   2, 291, 1088, 16},
{I,0,"ui/orc/1280x1024/statusline",                     2, 292, 1088, 16},
{I,0,"ui/human/1280x1024/buttonpanel",                  2, 297, 176, 688},
{I,0,"ui/orc/1280x1024/buttonpanel",                    2, 298, 176, 688},
// --------------------------------------------------
{I,0,"ui/human/1400x1050/resource",                     2, 287, 1208, 16},
{I,0,"ui/orc/1400x1050/resource",                       2, 288, 1208, 16},
{I,0,"ui/human/1400x1050/filler-right",                 2, 289, 16, 1050},
{I,0,"ui/orc/1400x1050/filler-right",                   2, 290, 16, 1050},
{I,0,"ui/human/1400x1050/statusline",                   2, 291, 1208, 16},
{I,0,"ui/orc/1400x1050/statusline",                     2, 292, 1208, 16},
{I,0,"ui/human/1400x1050/buttonpanel",                  2, 297, 176, 714},
{I,0,"ui/orc/1400x1050/buttonpanel",                    2, 298, 176, 714},
// --------------------------------------------------
{I,0,"ui/human/1600x1200/resource",                    2, 287, 1408, 16},
{I,0,"ui/orc/1600x1200/resource",                      2, 288, 1408, 16},
{I,0,"ui/human/1600x1200/filler-right",                2, 289, 16, 1200},
{I,0,"ui/orc/1600x1200/filler-right",                  2, 290, 16, 1200},
{I,0,"ui/human/1600x1200/statusline",                  2, 291, 1408, 16},
{I,0,"ui/orc/1600x1200/statusline",                    2, 292, 1408, 16},
{I,0,"ui/human/1600x1200/buttonpanel",                 2, 297, 176, 864},
{I,0,"ui/orc/1600x1200/buttonpanel",                   2, 298, 176, 864},
// --------------------------------------------------
{I,0,"ui/human/1680x1050/resource",                    2, 287, 1488, 16},
{I,0,"ui/orc/1680x1050/resource",                      2, 288, 1488, 16},
{I,0,"ui/human/1680x1050/filler-right",                2, 289, 16, 1050},
{I,0,"ui/orc/1680x1050/filler-right",                  2, 290, 16, 1050},
{I,0,"ui/human/1680x1050/statusline",                  2, 291, 1488, 16},
{I,0,"ui/orc/1680x1050/statusline",                    2, 292, 1488, 16},
{I,0,"ui/human/1680x1050/buttonpanel",                 2, 297, 176, 714},
{I,0,"ui/orc/1680x1050/buttonpanel",                   2, 298, 176, 714},
// --------------------------------------------------
{C,0,"human/cursors/human_gauntlet",                   2, 301 _2},
{C,0,"orc/cursors/orcish_claw",                        2, 302 _2},
{C,0,"human/cursors/human_don't_click_here",           2, 303 _2},
{C,0,"orc/cursors/orcish_don't_click_here",            2, 304 _2},
{C,0,"human/cursors/yellow_eagle",                     2, 305 _2},
{C,0,"orc/cursors/yellow_crosshairs",                  2, 306 _2},
{C,0,"human/cursors/red_eagle",                        2, 307 _2},
{C,0,"orc/cursors/red_crosshairs",                     2, 308 _2},
{C,0,"human/cursors/green_eagle",                      2, 309 _2},
{C,0,"orc/cursors/green_crosshairs",                   2, 310 _2},
{C,0,"cursors/magnifying_glass",                       2, 311 _2},
{C,0,"cursors/small_green_cross",                      2, 312 _2},
{C,0,"cursors/hourglass",                              2, 313 _2},
{C,0,"cursors/credits_arrow",                          27, 314 _2},
{C,0,"cursors/arrow_N",                                2, 315 _2},
{C,0,"cursors/arrow_NE",                               2, 316 _2},
{C,0,"cursors/arrow_E",                                2, 317 _2},
{C,0,"cursors/arrow_SE",                               2, 318 _2},
{C,0,"cursors/arrow_S",                                2, 319 _2},
{C,0,"cursors/arrow_SW",                               2, 320 _2},
{C,0,"cursors/arrow_W",                                2, 321 _2},
{C,0,"cursors/arrow_NW",                               2, 322 _2},
{U,0,"ui/bloodlust,haste,slow,invisible,shield",       2, 323 _2},
// --------------------------------------------------------
{G,0,"missiles/lightning",                             2, 324 _2},
{G,0,"missiles/gryphon_hammer",                        2, 325 _2},
// "fireball (also dragon breath)"
{G,0,"missiles/dragon_breath",                         2, 326 _2},
// "fireball (when casting flameshield)"
{G,0,"missiles/fireball",                              2, 327 _2},
{G,0,"missiles/flame_shield",                          2, 328 _2},
{G,0,"missiles/blizzard",                              2, 329 _2},
{G,0,"missiles/death_and_decay",                       2, 330 _2},
{G,0,"missiles/big_cannon",                            2, 331 _2},
{G,0,"missiles/exorcism",                              2, 332 _2},
{G,0,"missiles/heal_effect",                           2, 333 _2},
{G,0,"missiles/touch_of_death",                        2, 334 _2},
{G,0,"missiles/rune",                                  2, 335 _2},
{G,0,"missiles/tornado",                               2, 336 _2},
{G,0,"missiles/catapult_rock",                         2, 337 _2},
{G,0,"missiles/ballista_bolt",                         2, 338 _2},
{G,0,"missiles/arrow",                                 2, 339 _2},
{G,0,"missiles/axe",                                   2, 340 _2},
{G,0,"missiles/submarine_missile",                     2, 341 _2},
{G,0,"missiles/turtle_missile",                        2, 342 _2},
{G,0,"missiles/small_fire",                            2, 343 _2},
{G,0,"missiles/big_fire",                              2, 344 _2},
{G,0,"missiles/ballista-catapult_impact",              2, 345 _2},
{G,0,"missiles/normal_spell",                          2, 346 _2},
{G,0,"missiles/explosion",                             2, 347 _2},
{G,0,"missiles/cannon",                                2, 348 _2},
{G,0,"missiles/cannon_explosion",                      2, 349 _2},
{G,0,"missiles/cannon-tower_explosion",                2, 350 _2},
{G,0,"missiles/daemon_fire",                           2, 351 _2},
{G,0,"missiles/green_cross",                           2, 352 _2},
{G,0,"missiles/unit_shadow",                           2, 353 _2},
{U,0,"ui/human/infopanel",                             2, 354 _2},
{U,0,"ui/orc/infopanel",                               2, 355 _2},
{G,0,"tilesets/summer/icons",                          2, 356 _2},
{G,0,"tilesets/winter/icons",                          18, 357 _2},
{G,0,"tilesets/wasteland/icons",                       10, 358 _2},
{G,3,"tilesets/swamp/icons",                           10, 358 _2},

{I,0,"ui/human/victory",                               363, 359 _2},
{I,0,"ui/orc/victory",                                 364, 360 _2},
{I,0,"ui/human/defeat",                                365, 361 _2},
{I,0,"ui/orc/defeat",                                  366, 362 _2},

{I,0,"../campaigns/human/interface/introscreen1",      367, 369 _2},
{I,0,"../campaigns/orc/interface/introscreen1",        368, 370 _2},
{I,0,"../campaigns/orc/interface/introscreen2",        368, 371 _2},
{I,0,"../campaigns/orc/interface/introscreen3",        368, 372 _2},
{I,0,"../campaigns/orc/interface/introscreen4",        368, 373 _2},
{I,0,"../campaigns/orc/interface/introscreen5",        368, 374 _2},
{I,0,"../campaigns/human/interface/introscreen2",      367, 375 _2},
{I,0,"../campaigns/human/interface/introscreen3",      367, 376 _2},
{I,0,"../campaigns/human/interface/introscreen4",      367, 377 _2},
{I,0,"../campaigns/human/interface/introscreen5",      367, 378 _2},

{W,0,"ui/click",                                       432 __},
{W,0,"human/act",                                      433 __},
{W,0,"orc/act",                                        434 __},
{W,0,"ui/highclick",                                   435 __},
{W,0,"ui/statsthump",                                  436 __},

{M,0,"Human Battle 1",                                 413 __},
{M,0,"Human Battle 2",                                 414 __},
{M,0,"Human Battle 3",                                 415 __},
{M,0,"Human Battle 4",                                 416 __},
{M,0,"Orc Battle 1",                                   417 __},
{M,0,"Orc Battle 2",                                   418 __},
{M,0,"Orc Battle 3",                                   419 __},
{M,0,"Orc Battle 4",                                   420 __},
{M,0,"Human Defeat",                                   421 __},
{M,0,"Orc Defeat",                                     422 __},
{M,0,"Human Victory",                                  423 __},
{M,0,"Orc Victory",                                    424 __},
{M,0,"Human Briefing",                                 425 __},
{M,0,"I'm a Medieval Man",                             426 __},
{M,0,"Human Battle 5",                                 427 __},
{M,0,"Orc Battle 5",                                   428 __},
{M,0,"Orc Briefing",                                   429 __},

{V,0,"videos/logo",                                    430 __},

{R,2,"swamp/swamp",                                    438 __},
{T,2,"swamp/terrain/swamp",                            438, 439, 440, 441},

// --------------------------------------------------
{P,2,"campaigns/human-exp/levelx01h",                  446 __},
{P,2,"campaigns/orc-exp/levelx01o",                    447 __},
{P,2,"campaigns/human-exp/levelx02h",                  448 __},
{P,2,"campaigns/orc-exp/levelx02o",                    449 __},
{P,2,"campaigns/human-exp/levelx03h",                  450 __},
{P,2,"campaigns/orc-exp/levelx03o",                    451 __},
{P,2,"campaigns/human-exp/levelx04h",                  452 __},
{P,2,"campaigns/orc-exp/levelx04o",                    453 __},
{P,2,"campaigns/human-exp/levelx05h",                  454 __},
{P,2,"campaigns/orc-exp/levelx05o",                    455 __},
{P,2,"campaigns/human-exp/levelx06h",                  456 __},
{P,2,"campaigns/orc-exp/levelx06o",                    457 __},
{P,2,"campaigns/human-exp/levelx07h",                  458 __},
{P,2,"campaigns/orc-exp/levelx07o",                    459 __},
{P,2,"campaigns/human-exp/levelx08h",                  460 __},
{P,2,"campaigns/orc-exp/levelx08o",                    461 __},
{P,2,"campaigns/human-exp/levelx09h",                  462 __},
{P,2,"campaigns/orc-exp/levelx09o",                    463 __},
{P,2,"campaigns/human-exp/levelx10h",                  464 __},
{P,2,"campaigns/orc-exp/levelx10o",                    465 __},
{P,2,"campaigns/human-exp/levelx11h",                  466 __},
{P,2,"campaigns/orc-exp/levelx11o",                    467 __},
{P,2,"campaigns/human-exp/levelx12h",                  468 __},
{P,2,"campaigns/orc-exp/levelx12o",                    469 __},
// ------------------------------------------
{G,2,"tilesets/swamp/neutral/units/%59",               438, 470 _2},
{G,2,"tilesets/swamp/icons",                           438, 471 _2},
// 472: default UDTA for expansion PUDs
{G,2,"tilesets/swamp/human/buildings/%90",             438, 473 _2},
{G,2,"tilesets/swamp/orc/buildings/%91",               438, 474 _2},
{G,2,"tilesets/swamp/human/buildings/%72",             438, 475 _2},
{G,2,"tilesets/swamp/orc/buildings/%73",               438, 476 _2},
{G,2,"tilesets/swamp/human/buildings/%70",             438, 477 _2},
{G,2,"tilesets/swamp/orc/buildings/%71",               438, 478 _2},
{G,2,"tilesets/swamp/human/buildings/%60",             438, 479 _2},
{G,2,"tilesets/swamp/orc/buildings/%61",               438, 480 _2},
{G,2,"tilesets/swamp/human/buildings/%-62",            438, 481 _2},
{G,2,"tilesets/swamp/orc/buildings/%-63",              438, 482 _2},
{G,2,"tilesets/swamp/human/buildings/%64",             438, 483 _2},
{G,2,"tilesets/swamp/orc/buildings/%65",               438, 484 _2},
{G,2,"tilesets/swamp/human/buildings/%66",             438, 485 _2},
{G,2,"tilesets/swamp/orc/buildings/%67",               438, 486 _2},
{G,2,"tilesets/swamp/human/buildings/%76",             438, 487 _2},
{G,2,"tilesets/swamp/orc/buildings/%77",               438, 488 _2},
{G,2,"tilesets/swamp/human/buildings/%78",             438, 489 _2},
{G,2,"tilesets/swamp/orc/buildings/%79",               438, 490 _2},
{G,2,"tilesets/swamp/human/buildings/%68",             438, 491 _2},
{G,2,"tilesets/swamp/orc/buildings/%69",               438, 492 _2},
{G,2,"tilesets/swamp/human/buildings/%-84",            438, 493 _2},
{G,2,"tilesets/swamp/orc/buildings/%-85",              438, 494 _2},
{G,2,"tilesets/swamp/human/buildings/%-74",            438, 495 _2},
{G,2,"tilesets/swamp/orc/buildings/%-75",              438, 496 _2},
{G,2,"tilesets/swamp/human/buildings/%-80",            438, 497 _2},
{G,2,"tilesets/swamp/orc/buildings/%-81",              438, 498 _2},
{G,2,"tilesets/swamp/human/buildings/%-86",            438, 499 _2},
{G,2,"tilesets/swamp/orc/buildings/%-87",              438, 500 _2},
{G,2,"tilesets/swamp/human/buildings/%-88",            438, 501 _2},
{G,2,"tilesets/swamp/orc/buildings/%-89",              438, 502 _2},
{G,2,"tilesets/swamp/human/buildings/%92",             438, 503 _2},
{G,2,"tilesets/swamp/orc/buildings/%93",               438, 504 _2},
{G,2,"tilesets/swamp/human/buildings/%82",             438, 505 _2},
{G,2,"tilesets/swamp/orc/buildings/%83",               438, 506 _2},
{G,2,"tilesets/swamp/human/buildings/%-98",            438, 507 _2},
{G,2,"tilesets/swamp/orc/buildings/%-99",              438, 508 _2},
{G,2,"tilesets/swamp/human/buildings/%-100",           438, 509 _2},
{G,2,"tilesets/swamp/orc/buildings/%-101",             438, 510 _2},
{G,2,"tilesets/swamp/neutral/buildings/%94",           438, 511 _2},
{G,2,"tilesets/swamp/neutral/buildings/destroyed_site",
                                                       438, 512 _2},
{G,2,"tilesets/swamp/neutral/buildings/%103",          438, 513 _2},
{G,2,"tilesets/swamp/neutral/buildings/%104",          438, 514 _2},
{G,2,"tilesets/swamp/neutral/buildings/%95",           438, 515 _2},
{G,2,"tilesets/swamp/human/buildings/%-74_construction_site",
                                                       438, 516 _2},
{G,2,"tilesets/swamp/orc/buildings/%-75_construction_site",
                                                       438, 517 _2},
{G,2,"tilesets/swamp/human/buildings/%-88_construction_site",
                                                       438, 518 _2},
{G,2,"tilesets/swamp/orc/buildings/%-89_construction_site",
                                                       438, 519 _2},
{G,2,"tilesets/swamp/human/buildings/%-86_construction_site",
                                                       438, 520 _2},
{G,2,"tilesets/swamp/orc/buildings/%-87_construction_site",
                                                       438, 521 _2},
{G,2,"tilesets/swamp/human/buildings/%-80_construction_site",
                                                       438, 522 _2},
{G,2,"tilesets/swamp/orc/buildings/%-81_construction_site",
                                                       438, 523 _2},
{G,2,"tilesets/swamp/neutral/buildings/small_destroyed_site",
                                                       438, 524 _2},
{G,2,"tilesets/swamp/neutral/buildings/%102",          438, 525 _2},
{G,2,"tilesets/swamp/human/units/%40",                 438, 526 _2},
{G,2,"tilesets/swamp/orc/units/%41",                   438, 527 _2},

///////////////////////////////////////////////////////////////////////////////
//		SOUNDS
///////////////////////////////////////////////////////////////////////////////

{F,0,"sfxdat.sud",                                     5000 __},

// 0 file length
// 1 description
{W,0,"ui/placement_error",                             2 __},
{W,0,"ui/placement_success",                           3 __},
{W,0,"misc/building_construction",                     4 __},
{W,0,"human/basic_voices/selected/1",                  5 __},
{W,0,"orc/basic_voices/selected/1",                    6 __},
{W,0,"human/basic_voices/selected/2",                  7 __},
{W,0,"orc/basic_voices/selected/2",                    8 __},
{W,0,"human/basic_voices/selected/3",                  9 __},
{W,0,"orc/basic_voices/selected/3",                    10 __},
{W,0,"human/basic_voices/selected/4",                  11 __},
{W,0,"orc/basic_voices/selected/4",                    12 __},
{W,0,"human/basic_voices/selected/5",                  13 __},
{W,0,"orc/basic_voices/selected/5",                    14 __},
{W,0,"human/basic_voices/selected/6",                  15 __},
{W,0,"orc/basic_voices/selected/6",                    16 __},
{W,0,"human/basic_voices/annoyed/1",                   17 __},
{W,0,"orc/basic_voices/annoyed/1",                     18 __},
{W,0,"human/basic_voices/annoyed/2",                   19 __},
{W,0,"orc/basic_voices/annoyed/2",                     20 __},
{W,0,"human/basic_voices/annoyed/3",                   21 __},
{W,0,"orc/basic_voices/annoyed/3",                     22 __},
{W,0,"human/basic_voices/annoyed/4",                   23 __},
{W,0,"orc/basic_voices/annoyed/4",                     24 __},
{W,0,"human/basic_voices/annoyed/5",                   25 __},
{W,0,"orc/basic_voices/annoyed/5",                     26 __},
{W,0,"human/basic_voices/annoyed/6",                   27 __},
{W,0,"orc/basic_voices/annoyed/6",                     28 __},
{W,0,"human/basic_voices/annoyed/7",                   29 __},
{W,0,"orc/basic_voices/annoyed/7",                     30 __},
{W,0,"misc/explosion",                                 31 __},
{W,0,"human/basic_voices/acknowledgement/1",           32 __},
{W,0,"orc/basic_voices/acknowledgement/1",             33 __},
{W,0,"human/basic_voices/acknowledgement/2",           34 __},
{W,0,"orc/basic_voices/acknowledgement/2",             35 __},
{W,0,"human/basic_voices/acknowledgement/3",           36 __},
{W,0,"orc/basic_voices/acknowledgement/3",             37 __},
{W,0,"human/basic_voices/acknowledgement/4",           38 __},
{W,0,"orc/basic_voices/acknowledgement/4",             39 __},
{W,0,"human/basic_voices/work_complete",               40 __},
{W,0,"orc/basic_voices/work_complete",                 41 __},
{W,0,"human/units/peasant/work_complete",              42 __},
{W,0,"human/basic_voices/ready",                       43 __},
{W,0,"orc/basic_voices/ready",                         44 __},
{W,0,"human/basic_voices/help/1",                      45 __},
{W,0,"orc/basic_voices/help/1",                        46 __},
{W,0,"human/basic_voices/help/2",                      47 __},
{W,0,"orc/basic_voices/help/2",                        48 __},
{W,0,"human/basic_voices/dead",                        49 __},
{W,0,"orc/basic_voices/dead",                          50 __},
{W,0,"ships/sinking",                                  51 __},
{W,0,"misc/building_explosion/1",                      52 __},
{W,0,"misc/building_explosion/2",                      53 __},
{W,0,"misc/building_explosion/3",                      54 __},
{W,0,"missiles/catapult-ballista_attack",              55 __},
{W,0,"misc/tree_chopping/1",                           56 __},
{W,0,"misc/tree_chopping/2",                           57 __},
{W,0,"misc/tree_chopping/3",                           58 __},
{W,0,"misc/tree_chopping/4",                           59 __},
{W,0,"missiles/sword_attack/1",                        60 __},
{W,0,"missiles/sword_attack/2",                        61 __},
{W,0,"missiles/sword_attack/3",                        62 __},
{W,0,"missiles/punch",                                 63 __},
{W,0,"missiles/fireball_hit",                          64 __},
{W,0,"missiles/fireball_throw",                        65 __},
{W,0,"missiles/bow_throw",                             66 __},
{W,0,"missiles/bow_hit",                               67 __},
{W,0,"spells/basic_spell_sound",                       68 __},
{W,0,"buildings/blacksmith",                           69 __},
{W,0,"human/buildings/church",                         70 __},
{W,0,"orc/buildings/altar_of_storms",                  71 __},
{W,0,"human/buildings/stables",                        72 __},
{W,0,"orc/buildings/ogre_mound",                       73 __},
{W,0,"human/buildings/farm",                           74 __},
{W,0,"orc/buildings/pig_farm",                         75 __},
{W,0,"neutral/buildings/gold_mine",                    76 __},
{W,0,"missiles/axe_throw",                             77 __},
{W,0,"ships/tanker/acknowledgement/1",                 78 __},
{W,0,"missiles/fist",                                  79 __},
{W,0,"buildings/shipyard",                             80 __},
{W,0,"human/units/peasant/attack",                     81 __},
{W,0,"buildings/oil_platform",                         82 __},
{W,0,"buildings/oil_refinery",                         83 __},
{W,0,"buildings/lumbermill",                           84 __},
{W,0,"misc/transport_docking",                         85 __},
{W,0,"misc/burning",                                   86 __},
{W,0,"human/buildings/gryphon_aviary",                 87 __},
{W,0,"orc/buildings/dragon_roost",                     88 __},
{W,0,"buildings/foundry",                              89 __},
{W,0,"human/buildings/gnomish_inventor",               90 __},
{W,0,"orc/buildings/goblin_alchemist",                 91 __},
{W,0,"human/buildings/mage_tower",                     92 __},
{W,0,"orc/buildings/temple_of_the_damned",             93 __},
{W,0,"human/capture",                                  94 __},
{W,0,"orc/capture",                                    95 __},
{W,0,"human/rescue",                                   96 __},
{W,0,"orc/rescue",                                     97 __},
{W,0,"spells/bloodlust",                               98 __},
{W,0,"spells/death_and_decay",                         99 __},
{W,0,"spells/death_coil",                              100 __},
{W,0,"spells/exorcism",                                101 __},
{W,0,"spells/flame_shield",                            102 __},
{W,0,"spells/haste",                                   103 __},
{W,0,"spells/healing",                                 104 __},
{W,0,"spells/holy_vision",                             105 __},
{W,0,"spells/blizzard",                                106 __},
{W,0,"spells/invisibility",                            107 __},
{W,0,"spells/eye_of_kilrogg",                          108 __},
{W,0,"spells/polymorph",                               109 __},
{W,0,"spells/slow",                                    110 __},
{W,0,"spells/lightning",                               111 __},
{W,0,"spells/touch_of_darkness",                       112 __},
{W,0,"spells/unholy_armor",                            113 __},
{W,0,"spells/whirlwind",                               114 __},
{W,0,"orc/peon/ready",                                 115 __},
{W,0,"orc/units/death_knight/annoyed/1",               116 __},
{W,0,"orc/units/death_knight/annoyed/2",               117 __},
{W,0,"orc/units/death_knight/annoyed/3",               118 __},
{W,0,"orc/units/death_knight/ready",                   119 __},
{W,0,"orc/units/death_knight/selected/1",              120 __},
{W,0,"orc/units/death_knight/selected/2",              121 __},
{W,0,"orc/units/death_knight/acknowledgement/1",       122 __},
{W,0,"orc/units/death_knight/acknowledgement/2",       123 __},
{W,0,"orc/units/death_knight/acknowledgement/3",       124 __},
{W,0,"human/units/dwarven_demolition_squad/annoyed/1", 125 __},
{W,0,"human/units/dwarven_demolition_squad/annoyed/2", 126 __},
{W,0,"human/units/dwarven_demolition_squad/annoyed/3", 127 __},
{W,0,"human/units/dwarven_demolition_squad/ready",
                                                       128 __},
{W,0,"human/units/dwarven_demolition_squad/selected/1",
                                                       129 __},
{W,0,"human/units/dwarven_demolition_squad/selected/2",
                                                       130 __},
{W,0,"human/units/dwarven_demolition_squad/acknowledgement/1",
                                                       131 __},
{W,0,"human/units/dwarven_demolition_squad/acknowledgement/2",
                                                       132 __},
{W,0,"human/units/dwarven_demolition_squad/acknowledgement/3",
                                                       133 __},
{W,0,"human/units/dwarven_demolition_squad/acknowledgement/4",
                                                       134 __},
{W,0,"human/units/dwarven_demolition_squad/acknowledgement/5",
                                                       135 __},
{W,0,"human/units/elven_archer-ranger/annoyed/1",      136 __},
{W,0,"human/units/elven_archer-ranger/annoyed/2",      137 __},
{W,0,"human/units/elven_archer-ranger/annoyed/3",      138 __},
{W,0,"human/units/elven_archer-ranger/ready",          139 __},
{W,0,"human/units/elven_archer-ranger/selected/1",     140 __},
{W,0,"human/units/elven_archer-ranger/selected/2",     141 __},
{W,0,"human/units/elven_archer-ranger/selected/3",     142 __},
{W,0,"human/units/elven_archer-ranger/selected/4",     143 __},
{W,0,"human/units/elven_archer-ranger/acknowledgement/1",
                                                       144 __},
{W,0,"human/units/elven_archer-ranger/acknowledgement/2",
                                                       145 __},
{W,0,"human/units/elven_archer-ranger/acknowledgement/3",
                                                       146 __},
{W,0,"human/units/elven_archer-ranger/acknowledgement/4",
                                                       147 __},
{W,0,"human/units/gnomish_flying_machine/annoyed/1",   148 __},
{W,0,"human/units/gnomish_flying_machine/annoyed/2",   149 __},
{W,0,"human/units/gnomish_flying_machine/annoyed/3",   150 __},
{W,0,"human/units/gnomish_flying_machine/annoyed/4",   151 __},
{W,0,"human/units/gnomish_flying_machine/annoyed/5",   152 __},
{W,0,"human/units/gnomish_flying_machine/ready",       153 __},
{W,0,"human/units/gnomish_flying_machine/acknowledgement/1",
                                                       154 __},
{W,0,"orc/units/goblin_sappers/annoyed/1",             155 __},
{W,0,"orc/units/goblin_sappers/annoyed/2",             156 __},
{W,0,"orc/units/goblin_sappers/annoyed/3",             157 __},
{W,0,"orc/units/goblin_sappers/ready",                 158 __},
{W,0,"orc/units/goblin_sappers/selected/1",            159 __},
{W,0,"orc/units/goblin_sappers/selected/2",            160 __},
{W,0,"orc/units/goblin_sappers/selected/3",            161 __},
{W,0,"orc/units/goblin_sappers/selected/4",            162 __},
{W,0,"orc/units/goblin_sappers/acknowledgement/1",     163 __},
{W,0,"orc/units/goblin_sappers/acknowledgement/2",     164 __},
{W,0,"orc/units/goblin_sappers/acknowledgement/3",     165 __},
{W,0,"orc/units/goblin_sappers/acknowledgement/4",     166 __},
{W,0,"orc/units/goblin_zeppelin/annoyed/1",            167 __},
{W,0,"orc/units/goblin_zeppelin/annoyed/2",            168 __},
{W,0,"orc/units/goblin_zeppelin/ready",                169 __},
{W,0,"orc/units/goblin_zeppelin/acknowledgement/1",    170 __},
{W,0,"human/units/knight/annoyed/1",                   171 __},
{W,0,"human/units/knight/annoyed/2",                   172 __},
{W,0,"human/units/knight/annoyed/3",                   173 __},
{W,0,"human/units/knight/ready",                       174 __},
{W,0,"human/units/knight/selected/1",                  175 __},
{W,0,"human/units/knight/selected/2",                  176 __},
{W,0,"human/units/knight/selected/3",                  177 __},
{W,0,"human/units/knight/selected/4",                  178 __},
{W,0,"human/units/knight/acknowledgement/1",           179 __},
{W,0,"human/units/knight/acknowledgement/2",           180 __},
{W,0,"human/units/knight/acknowledgement/3",           181 __},
{W,0,"human/units/knight/acknowledgement/4",           182 __},
{W,0,"human/units/paladin/annoyed/1",                  183 __},
{W,0,"human/units/paladin/annoyed/2",                  184 __},
{W,0,"human/units/paladin/annoyed/3",                  185 __},
{W,0,"human/units/paladin/ready",                      186 __},
{W,0,"human/units/paladin/selected/1",                 187 __},
{W,0,"human/units/paladin/selected/2",                 188 __},
{W,0,"human/units/paladin/selected/3",                 189 __},
{W,0,"human/units/paladin/selected/4",                 190 __},
{W,0,"human/units/paladin/acknowledgement/1",          191 __},
{W,0,"human/units/paladin/acknowledgement/2",          192 __},
{W,0,"human/units/paladin/acknowledgement/3",          193 __},
{W,0,"human/units/paladin/acknowledgement/4",          194 __},
{W,0,"orc/units/ogre/annoyed/1",                       195 __},
{W,0,"orc/units/ogre/annoyed/2",                       196 __},
{W,0,"orc/units/ogre/annoyed/3",                       197 __},
{W,0,"orc/units/ogre/annoyed/4",                       198 __},
{W,0,"orc/units/ogre/annoyed/5",                       199 __},
{W,0,"orc/units/ogre/ready",                           200 __},
{W,0,"orc/units/ogre/selected/1",                      201 __},
{W,0,"orc/units/ogre/selected/2",                      202 __},
{W,0,"orc/units/ogre/selected/3",                      203 __},
{W,0,"orc/units/ogre/selected/4",                      204 __},
{W,0,"orc/units/ogre/acknowledgement/1",               205 __},
{W,0,"orc/units/ogre/acknowledgement/2",               206 __},
{W,0,"orc/units/ogre/acknowledgement/3",               207 __},
{W,0,"orc/units/ogre-mage/annoyed/1",                  208 __},
{W,0,"orc/units/ogre-mage/annoyed/2",                  209 __},
{W,0,"orc/units/ogre-mage/annoyed/3",                  210 __},
{W,0,"orc/units/ogre-mage/ready",                      211 __},
{W,0,"orc/units/ogre-mage/selected/1",                 212 __},
{W,0,"orc/units/ogre-mage/selected/2",                 213 __},
{W,0,"orc/units/ogre-mage/selected/3",                 214 __},
{W,0,"orc/units/ogre-mage/selected/4",                 215 __},
{W,0,"orc/units/ogre-mage/acknowledgement/1",          216 __},
{W,0,"orc/units/ogre-mage/acknowledgement/2",          217 __},
{W,0,"orc/units/ogre-mage/acknowledgement/3",          218 __},
{W,0,"human/ships/annoyed/1",                          219 __},
{W,0,"orc/ships/annoyed/1",                            220 __},
{W,0,"human/ships/annoyed/2",                          221 __},
{W,0,"orc/ships/annoyed/2",                            222 __},
{W,0,"human/ships/annoyed/3",                          223 __},
{W,0,"orc/ships/annoyed/3",                            224 __},
{W,0,"human/ships/ready",                              225 __},
{W,0,"orc/ships/ready",                                226 __},
{W,0,"human/ships/selected/1",                         227 __},
{W,0,"orc/ships/selected/1",                           228 __},
{W,0,"human/ships/selected/2",                         229 __},
{W,0,"orc/ships/selected/2",                           230 __},
{W,0,"human/ships/selected/3",                         231 __},
{W,0,"orc/ships/selected/3",                           232 __},
{W,0,"human/ships/acknowledgement/1",                  233 __},
{W,0,"orc/ships/acknowledgement/1",                    234 __},
{W,0,"human/ships/acknowledgement/2",                  235 __},
{W,0,"orc/ships/acknowledgement/2",                    236 __},
{W,0,"human/ships/acknowledgement/3",                  237 __},
{W,0,"orc/ships/acknowledgement/3",                    238 __},
{W,0,"human/ships/gnomish_submarine/annoyed/1",        239 __},
{W,0,"human/ships/gnomish_submarine/annoyed/2",        240 __},
{W,0,"human/ships/gnomish_submarine/annoyed/3",        241 __},
{W,0,"human/ships/gnomish_submarine/annoyed/4",        242 __},
{W,0,"orc/units/troll_axethrower-berserker/annoyed/1", 243 __},
{W,0,"orc/units/troll_axethrower-berserker/annoyed/2", 244 __},
{W,0,"orc/units/troll_axethrower-berserker/annoyed/3", 245 __},
{W,0,"orc/units/troll_axethrower-berserker/ready",     246 __},
{W,0,"orc/units/troll_axethrower-berserker/selected/1",
                                                       247 __},
{W,0,"orc/units/troll_axethrower-berserker/selected/2",
                                                       248 __},
{W,0,"orc/units/troll_axethrower-berserker/selected/3",
                                                       249 __},
{W,0,"orc/units/troll_axethrower-berserker/acknowledgement/1",
                                                       250 __},
{W,0,"orc/units/troll_axethrower-berserker/acknowledgement/2",
                                                       251 __},
{W,0,"orc/units/troll_axethrower-berserker/acknowledgement/3",
                                                       252 __},
{W,0,"human/units/mage/annoyed/1",                     253 __},
{W,0,"human/units/mage/annoyed/2",                     254 __},
{W,0,"human/units/mage/annoyed/3",                     255 __},
{W,0,"human/units/mage/ready",                         256 __},
{W,0,"human/units/mage/selected/1",                    257 __},
{W,0,"human/units/mage/selected/2",                    258 __},
{W,0,"human/units/mage/selected/3",                    259 __},
{W,0,"human/units/mage/acknowledgement/1",             260 __},
{W,0,"human/units/mage/acknowledgement/2",             261 __},
{W,0,"human/units/mage/acknowledgement/3",             262 __},
{W,0,"human/units/peasant/ready",                      263 __},
{W,0,"human/units/peasant/annoyed/1",                  264 __},
{W,0,"human/units/peasant/annoyed/2",                  265 __},
{W,0,"human/units/peasant/annoyed/3",                  266 __},
{W,0,"human/units/peasant/annoyed/4",                  267 __},
{W,0,"human/units/peasant/annoyed/5",                  268 __},
{W,0,"human/units/peasant/annoyed/6",                  269 __},
{W,0,"human/units/peasant/annoyed/7",                  270 __},
{W,0,"human/units/peasant/selected/1",                 271 __},
{W,0,"human/units/peasant/selected/2",                 272 __},
{W,0,"human/units/peasant/selected/3",                 273 __},
{W,0,"human/units/peasant/selected/4",                 274 __},
{W,0,"human/units/peasant/acknowledgement/1",          275 __},
{W,0,"human/units/peasant/acknowledgement/2",          276 __},
{W,0,"human/units/peasant/acknowledgement/3",          277 __},
{W,0,"human/units/peasant/acknowledgement/4",          278 __},
{W,0,"orc/units/dragon/ready",                         279 __},
{W,0,"orc/units/dragon/selected/1",                    280 __},
{W,0,"orc/units/dragon/acknowledgement/1",             281 __},
{W,0,"orc/units/dragon/acknowledgement/2",             282 __},
{W,0,"human/units/gryphon_rider/selected/1",           283 __},
{W,0,"human/units/gryphon_rider/ready",                284 __},
{W,0,"human/units/gryphon_rider/acknowledgement/2",    285 __},
{W,0,"neutral/units/sheep/selected/1",                 286 __},
{W,0,"neutral/units/sheep/annoyed/1",                  287 __},
{W,0,"neutral/units/seal/selected/1",                  288 __},
{W,0,"neutral/units/seal/annoyed/1",                   289 __},
{W,0,"neutral/units/pig/selected/1",                   290 __},
{W,0,"neutral/units/pig/annoyed/1",                    291 __},
{W,0,"units/catapult-ballista/acknowledgement/1",      292 __},
{W,2,"human/units/alleria/annoyed/1",                  293 __},
{W,2,"human/units/alleria/annoyed/2",                  294 __},
{W,2,"human/units/alleria/annoyed/3",                  295 __},
{W,2,"human/units/alleria/selected/1",                 296 __},
{W,2,"human/units/alleria/selected/2",                 297 __},
{W,2,"human/units/alleria/selected/3",                 298 __},
{W,2,"human/units/alleria/acknowledgement/1",          299 __},
{W,2,"human/units/alleria/acknowledgement/2",          300 __},
{W,2,"human/units/alleria/acknowledgement/3",          301 __},
{W,2,"human/units/danath/annoyed/1",                   302 __},
{W,2,"human/units/danath/annoyed/2",                   303 __},
{W,2,"human/units/danath/annoyed/3",                   304 __},
{W,2,"human/units/danath/selected/1",                  305 __},
{W,2,"human/units/danath/selected/2",                  306 __},
{W,2,"human/units/danath/selected/3",                  307 __},
{W,2,"human/units/danath/acknowledgement/1",           308 __},
{W,2,"human/units/danath/acknowledgement/2",           309 __},
{W,2,"human/units/danath/acknowledgement/3",           310 __},
{W,2,"human/units/khadgar/annoyed/1",                  311 __},
{W,2,"human/units/khadgar/annoyed/2",                  312 __},
{W,2,"human/units/khadgar/annoyed/3",                  313 __},
{W,2,"human/units/khadgar/selected/1",                 314 __},
{W,2,"human/units/khadgar/selected/2",                 315 __},
{W,2,"human/units/khadgar/selected/3",                 316 __},
{W,2,"human/units/khadgar/acknowledgement/1",          317 __},
{W,2,"human/units/khadgar/acknowledgement/2",          318 __},
{W,2,"human/units/khadgar/acknowledgement/3",          319 __},
{W,2,"human/units/kurdan/annoyed/1",                   320 __},
{W,2,"human/units/kurdan/annoyed/2",                   321 __},
{W,2,"human/units/kurdan/annoyed/3",                   322 __},
{W,2,"human/units/kurdan/selected/1",                  323 __},
{W,2,"human/units/kurdan/selected/2",                  324 __},
{W,2,"human/units/kurdan/selected/3",                  325 __},
{W,2,"human/units/kurdan/acknowledgement/1",           326 __},
{W,2,"human/units/kurdan/acknowledgement/2",           327 __},
{W,2,"human/units/kurdan/acknowledgement/3",           328 __},
{W,2,"human/units/turalyon/annoyed/1",                 329 __},
{W,2,"human/units/turalyon/annoyed/2",                 330 __},
{W,2,"human/units/turalyon/annoyed/3",                 331 __},
{W,2,"human/units/turalyon/selected/1",                332 __},
{W,2,"human/units/turalyon/selected/2",                333 __},
{W,2,"human/units/turalyon/selected/3",                334 __},
{W,2,"human/units/turalyon/acknowledgement/1",         335 __},
{W,2,"human/units/turalyon/acknowledgement/2",         336 __},
{W,2,"human/units/turalyon/acknowledgement/3",         337 __},
{W,2,"orc/units/deathwing/annoyed/1",                  338 __},
{W,2,"orc/units/deathwing/annoyed/2",                  339 __},
{W,2,"orc/units/deathwing/annoyed/3",                  340 __},
{W,2,"orc/units/deathwing/selected/1",                 341 __},
{W,2,"orc/units/deathwing/selected/2",                 342 __},
{W,2,"orc/units/deathwing/selected/3",                 343 __},
{W,2,"orc/units/deathwing/acknowledgement/1",          344 __},
{W,2,"orc/units/deathwing/acknowledgement/2",          345 __},
{W,2,"orc/units/deathwing/acknowledgement/3",          346 __},
{W,2,"orc/units/dentarg/annoyed/1",                    347 __},
{W,2,"orc/units/dentarg/annoyed/2",                    348 __},
{W,2,"orc/units/dentarg/annoyed/3",                    349 __},
{W,2,"orc/units/dentarg/selected/1",                   350 __},
{W,2,"orc/units/dentarg/selected/2",                   351 __},
{W,2,"orc/units/dentarg/selected/3",                   352 __},
{W,2,"orc/units/dentarg/acknowledgement/1",            353 __},
{W,2,"orc/units/dentarg/acknowledgement/2",            354 __},
{W,2,"orc/units/dentarg/acknowledgement/3",            355 __},
{W,2,"orc/units/grom_hellscream/annoyed/1",            356 __},
{W,2,"orc/units/grom_hellscream/annoyed/2",            357 __},
{W,2,"orc/units/grom_hellscream/annoyed/3",            358 __},
{W,2,"orc/units/grom_hellscream/selected/1",           359 __},
{W,2,"orc/units/grom_hellscream/selected/2",           360 __},
{W,2,"orc/units/grom_hellscream/selected/3",           361 __},
{W,2,"orc/units/grom_hellscream/acknowledgement/1",    362 __},
{W,2,"orc/units/grom_hellscream/acknowledgement/2",    363 __},
{W,2,"orc/units/grom_hellscream/acknowledgement/3",    364 __},
{W,2,"orc/units/korgath_bladefist/annoyed/1",          365 __},
{W,2,"orc/units/korgath_bladefist/annoyed/2",          366 __},
{W,2,"orc/units/korgath_bladefist/annoyed/3",          367 __},
{W,2,"orc/units/korgath_bladefist/selected/1",         368 __},
{W,2,"orc/units/korgath_bladefist/selected/2",         369 __},
{W,2,"orc/units/korgath_bladefist/selected/3",         370 __},
{W,2,"orc/units/korgath_bladefist/acknowledgement/1",  371 __},
{W,2,"orc/units/korgath_bladefist/acknowledgement/2",  372 __},
{W,2,"orc/units/korgath_bladefist/acknowledgement/3",  373 __},
{W,2,"orc/units/teron_gorefiend/annoyed/1",            374 __},
{W,2,"orc/units/teron_gorefiend/annoyed/2",            375 __},
{W,2,"orc/units/teron_gorefiend/annoyed/3",            376 __},
{W,2,"orc/units/teron_gorefiend/selected/1",           377 __},
{W,2,"orc/units/teron_gorefiend/selected/2",           378 __},
{W,2,"orc/units/teron_gorefiend/selected/3",           379 __},
{W,2,"orc/units/teron_gorefiend/acknowledgement/1",    380 __},
{W,2,"orc/units/teron_gorefiend/acknowledgement/2",    381 __},
{W,2,"orc/units/teron_gorefiend/acknowledgement/3",    382 __},
{W,2,"neutral/units/warthog/selected/1",               383 __},
{W,2,"neutral/units/warthog/annoyed/1",                384 __},

///////////////////////////////////////////////////////////////////////////////
//		INTERFACE
///////////////////////////////////////////////////////////////////////////////

{F,0,"rezdat.war",                                     3000 __},

// palette 27 for the first 3 frames, 14 for the rest
{D,0,"ui/human/widgets",                               27, 0, 0, 0},
{D,0,"ui/orc/widgets",                                 27, 1, 0, 0},
// (correct palette is #2 in maindat)
{U,0,"ui/buttons_1",                                   14, 0 _2},
// (correct palette is #2 in maindat)
{U,0,"ui/buttons_2",                                   14, 1 _2},
{I,0,"ui/cd-icon",                                     14, 2 _2},
{I,0,"ui/human/panel_1",                               14, 3 _2},
{I,0,"ui/orc/panel_1",                                 14, 4 _2},
{I,0,"ui/human/panel_2",                               14, 5 _2},
{I,0,"ui/orc/panel_2",                                 14, 6 _2},
{I,0,"ui/human/panel_3",                               14, 7 _2},
{I,0,"ui/orc/panel_3",                                 14, 8 _2},
{I,0,"ui/human/panel_4",                               14, 9 _2},
{I,0,"ui/orc/panel_4",                                 14, 10 _2},
{I,0,"ui/human/panel_5",                               14, 11 _2},
{I,0,"ui/orc/panel_5",                                 14, 12 _2},
{I,0,"ui/Menu_background_with_title",                  14, 13 _2},
{I,0,"ui/Menu_background_without_title",               16, 15 _2},
{I,0,"../campaigns/human/interface/Act_I_-_Shores_of_Lordareon",
                                                       17, 19 _2},
{I,0,"../campaigns/orc/interface/Act_I_-_Seas_of_Blood",
                                                       17, 20 _2},
{I,0,"../campaigns/human/interface/Act_II_-_Khaz_Modan",
                                                       17, 21 _2},
{I,0,"../campaigns/orc/interface/Act_II_-_Khaz_Modan",
                                                       17, 22 _2},
{I,0,"../campaigns/human/interface/Act_III_-_The_Northlands",
                                                       17, 23 _2},
{I,0,"../campaigns/orc/interface/Act_III_-_Quel'Thalas",
                                                       17, 24 _2},
{I,0,"../campaigns/human/interface/Act_IV_-_Return_to_Azeroth",
                                                       17, 25 _2},
{I,0,"../campaigns/orc/interface/Act_IV_-_Tides_of_Darkness",
                                                       17, 26 _2},

{I,0,"ui/human/The_End",                               27, 29 _2},
{I,0,"ui/orc/Smashing_of_Lordaeron_scroll",            32, 30 _2},
{I,2,"ui/Patch",                                       14, 91 _2},
{I,2,"ui/Credits_for_extension_background",            93, 94 _2},
{I,2,"../campaigns/human-exp/interface/Act_I_-_A_Time_for_Heroes",
                                                       17, 96 _2},
{I,2,"../campaigns/orc-exp/interface/Act_I_-_Draenor,_the_Red_World",
                                                       17, 97 _2},
{I,2,"../campaigns/human-exp/interface/Act_II_-_Draenor,_the_Red_World",
                                                       17, 98 _2},
{I,2,"../campaigns/orc-exp/interface/Act_II_-_The_Burning_of_Azeroth",
                                                       17, 99 _2},
{I,2,"../campaigns/human-exp/interface/Act_III_-_War_in_the_Shadows",
                                                       17, 100 _2},
{I,2,"../campaigns/orc-exp/interface/Act_III_-_The_Great_Sea",
                                                       17, 101 _2},
{I,2,"../campaigns/human-exp/interface/Act_IV_-_The_Measure_of_Valor",
                                                       17, 102 _2},
{I,2,"../campaigns/orc-exp/interface/Act_IV_-_Prelude_to_New_Worlds",
                                                       17, 103 _2},

///////////////////////////////////////////////////////////////////////////////
//		SPEECH INTROS
///////////////////////////////////////////////////////////////////////////////

		// FIXME: this file contains different data, if expansion or not.
		// FIXME: Where and what are the expansion entries

{F,0,"snddat.war",                                     2000 __},
{W,0,"../campaigns/human/victory",                     2 __},
{W,0,"../campaigns/orc/victory",                       3 __},
{W,0,"../campaigns/human/level01h-intro1",             4 __},
{W,0,"../campaigns/human/level01h-intro2",             5 __},
{W,0,"../campaigns/human/level02h-intro1",             6 __},
{W,0,"../campaigns/human/level02h-intro2",             7 __},
{W,0,"../campaigns/human/level03h-intro1",             8 __},
{W,0,"../campaigns/human/level03h-intro2",             9 __},
{W,0,"../campaigns/human/level04h-intro1",             10 __},
{W,0,"../campaigns/human/level04h-intro2",             11 __},
{W,0,"../campaigns/human/level05h-intro1",             12 __},
{W,0,"../campaigns/human/level05h-intro2",             13 __},
{W,0,"../campaigns/human/level06h-intro1",             14 __},
{W,0,"../campaigns/human/level07h-intro1",             15 __},
{W,0,"../campaigns/human/level07h-intro2",             16 __},
{W,0,"../campaigns/human/level08h-intro1",             17 __},
{W,0,"../campaigns/human/level08h-intro2",             18 __},
{W,0,"../campaigns/human/level09h-intro1",             19 __},
{W,0,"../campaigns/human/level10h-intro1",             20 __},
{W,0,"../campaigns/human/level11h-intro1",             21 __},
{W,0,"../campaigns/human/level11h-intro2",             22 __},
{W,0,"../campaigns/human/level12h-intro1",             23 __},
{W,0,"../campaigns/human/level13h-intro1",             24 __},
{W,0,"../campaigns/human/level13h-intro2",             25 __},
{W,0,"../campaigns/human/level14h-intro1",             26 __},

{W,0,"../campaigns/orc/level01o-intro1",               27 __},
{W,0,"../campaigns/orc/level02o-intro1",               28 __},
{W,0,"../campaigns/orc/level03o-intro1",               29 __},
{W,0,"../campaigns/orc/level03o-intro2",               30 __},
{W,0,"../campaigns/orc/level04o-intro1",               31 __},
{W,0,"../campaigns/orc/level05o-intro1",               32 __},
{W,0,"../campaigns/orc/level06o-intro1",               33 __},
{W,0,"../campaigns/orc/level07o-intro1",               34 __},
{W,0,"../campaigns/orc/level07o-intro2",               35 __},
{W,0,"../campaigns/orc/level08o-intro1",               36 __},
{W,0,"../campaigns/orc/level09o-intro1",               37 __},
{W,0,"../campaigns/orc/level10o-intro1",               38 __},
{W,0,"../campaigns/orc/level11o-intro1",               39 __},
{W,0,"../campaigns/orc/level11o-intro2",               40 __},
{W,0,"../campaigns/orc/level12o-intro1",               41 __},
{W,0,"../campaigns/orc/level12o-intro2",               42 __},
{W,0,"../campaigns/orc/level13o-intro1",               43 __},
{W,0,"../campaigns/orc/level13o-intro2",               44 __},
{W,0,"../campaigns/orc/level14o-intro1",               45 __},

{W,2,"../campaigns/human-exp/victory-1",               50 __},
{W,2,"../campaigns/human-exp/victory-2",               51 __},
{W,2,"../campaigns/orc-exp/victory-1",                 52 __},
{W,2,"../campaigns/orc-exp/victory-2",                 53 __},
{W,2,"../campaigns/orc-exp/victory-3",                 54 __},
{W,2,"../campaigns/human-exp/levelx01h-intro1",        55 __},
{W,2,"../campaigns/human-exp/levelx01h-intro2",        56 __},
{W,2,"../campaigns/human-exp/levelx02h-intro1",        57 __},
{W,2,"../campaigns/human-exp/levelx02h-intro2",        58 __},
{W,2,"../campaigns/human-exp/levelx03h-intro1",        59 __},
{W,2,"../campaigns/human-exp/levelx03h-intro2",        60 __},
{W,2,"../campaigns/human-exp/levelx04h-intro1",        61 __},
{W,2,"../campaigns/human-exp/levelx04h-intro2",        62 __},
{W,2,"../campaigns/human-exp/levelx04h-intro3",        63 __},
{W,2,"../campaigns/human-exp/levelx05h-intro1",        64 __},
{W,2,"../campaigns/human-exp/levelx05h-intro2",        65 __},
{W,2,"../campaigns/human-exp/levelx06h-intro1",        66 __},
{W,2,"../campaigns/human-exp/levelx06h-intro2",        67 __},
{W,2,"../campaigns/human-exp/levelx06h-intro3",        68 __},
{W,2,"../campaigns/human-exp/levelx07h-intro1",        69 __},
{W,2,"../campaigns/human-exp/levelx07h-intro2",        70 __},
{W,2,"../campaigns/human-exp/levelx07h-intro3",        71 __},
{W,2,"../campaigns/human-exp/levelx08h-intro1",        72 __},
{W,2,"../campaigns/human-exp/levelx08h-intro2",        73 __},
{W,2,"../campaigns/human-exp/levelx09h-intro1",        74 __},
{W,2,"../campaigns/human-exp/levelx09h-intro2",        75 __},
{W,2,"../campaigns/human-exp/levelx10h-intro1",        76 __},
{W,2,"../campaigns/human-exp/levelx10h-intro2",        77 __},
{W,2,"../campaigns/human-exp/levelx10h-intro3",        78 __},
{W,2,"../campaigns/human-exp/levelx11h-intro1",        79 __},
{W,2,"../campaigns/human-exp/levelx11h-intro2",        80 __},
{W,2,"../campaigns/human-exp/levelx12h-intro1",        81 __},
{W,2,"../campaigns/human-exp/levelx12h-intro2",        82 __},
{W,2,"../campaigns/orc-exp/levelx01o-intro1",          83 __},
{W,2,"../campaigns/orc-exp/levelx01o-intro2",          84 __},
{W,2,"../campaigns/orc-exp/levelx01o-intro3",          85 __},
{W,2,"../campaigns/orc-exp/levelx02o-intro1",          86 __},
{W,2,"../campaigns/orc-exp/levelx02o-intro2",          87 __},
{W,2,"../campaigns/orc-exp/levelx03o-intro1",          88 __},
{W,2,"../campaigns/orc-exp/levelx03o-intro2",          89 __},
{W,2,"../campaigns/orc-exp/levelx04o-intro1",          90 __},
{W,2,"../campaigns/orc-exp/levelx04o-intro2",          91 __},
{W,2,"../campaigns/orc-exp/levelx05o-intro1",          92 __},
{W,2,"../campaigns/orc-exp/levelx05o-intro2",          93 __},
{W,2,"../campaigns/orc-exp/levelx06o-intro1",          94 __},
{W,2,"../campaigns/orc-exp/levelx06o-intro2",          95 __},
{W,2,"../campaigns/orc-exp/levelx07o-intro1",          96 __},
{W,2,"../campaigns/orc-exp/levelx07o-intro2",          97 __},
{W,2,"../campaigns/orc-exp/levelx07o-intro3",          98 __},
{W,2,"../campaigns/orc-exp/levelx08o-intro1",          99 __},
{W,2,"../campaigns/orc-exp/levelx09o-intro1",          100 __},
{W,2,"../campaigns/orc-exp/levelx09o-intro2",          101 __},
{W,2,"../campaigns/orc-exp/levelx10o-intro1",          102 __},
{W,2,"../campaigns/orc-exp/levelx10o-intro2",          103 __},
{W,2,"../campaigns/orc-exp/levelx10o-intro3",          104 __},
{W,2,"../campaigns/orc-exp/levelx11o-intro1",          105 __},
{W,2,"../campaigns/orc-exp/levelx11o-intro2",          106 __},
{W,2,"../campaigns/orc-exp/levelx12o-intro1",          107 __},
{W,2,"../campaigns/orc-exp/levelx12o-intro2",          108 __},
{W,2,"../campaigns/orc-exp/levelx12o-intro3",          109 __},

///////////////////////////////////////////////////////////////////////////////
//		INTERFACE
///////////////////////////////////////////////////////////////////////////////

{F,0,"muddat.cud",                                     6000 __},

{V,0,"videos/orc-1",                                   0 __},
{V,0,"videos/orc-3",                                   2 __},
//{V,0,"videos/human-4",                                 3 __}, //Fixme: 3 and 4 same?
{V,0,"videos/human-4",                                 4 __},
//{V,0,"videos/gameintro",                               5 __}, //Fixme: 5 and 6 same?
{V,0,"videos/gameintro",                               6 __},
//{V,0,"videos/orc-4",                                   8 __}, //Fixme: 8 and 9 same?
{V,0,"videos/orc-4",                                   9 __},
{V,0,"videos/human-3",                                 10 __},
{V,0,"videos/human-1",                                 11 __},
{V,0,"videos/orc-2",                                   12 __},
{V,0,"videos/human-2",                                 14 __},
//{V,2,"videos/exp-1",                                   15 __}, //Fixme: 15 and 16 same?
{V,2,"videos/exp-1",                                   16 __},
{V,2,"videos/human-exp-2",                             17 __},
{V,2,"videos/orc-exp-2",                               18 __},

#undef __
#undef _2
};

// puds that are in their own file
static char *OriginalPuds[] = {
	"../alamo.pud",
	"../channel.pud",
	"../death.pud",
	"../dragon.pud",
	"../icebrdge.pud",
	"../islands.pud",
	"../land_sea.pud",
	"../mutton.pud",
	""
};

static char *ExpansionPuds[] = {
	"../puds/multi/3vs3.pud",
	"../puds/multi/3vs5.pud",
	"../puds/multi/arena.pud",
	"../puds/multi/atols.pud",
	"../puds/multi/battle_1.pud",
	"../puds/multi/battle_2.pud",
	"../puds/multi/blackgld.pud",
	"../puds/multi/collapse.pud",
	"../puds/multi/crowded.pud",
	"../puds/multi/diamond.pud",
	"../puds/multi/dup.pud",
	"../puds/multi/ears.pud",
	"../puds/multi/friends.pud",
	"../puds/multi/funfor3.pud",
	"../puds/multi/gauntlet.pud",
	"../puds/multi/hell.pud",
	"../puds/multi/hourglas.pud",
	"../puds/multi/icewall.pud",
	"../puds/multi/ironcros.pud",
	"../puds/multi/isles.pud",
	"../puds/multi/jimland.pud",
	"../puds/multi/kanthar.pud",
	"../puds/multi/king.pud",
	"../puds/multi/mntnpass.pud",
	"../puds/multi/passes.pud",
	"../puds/multi/plots.pud",
	"../puds/multi/raiders.pud",
	"../puds/multi/ring.pud",
	"../puds/multi/riverx.pud",
	"../puds/multi/rockmaze.pud",
	"../puds/multi/shared.pud",
	"../puds/multi/siege.pud",
	"../puds/multi/tandalos.pud",
	"../puds/multi/theriver.pud",
	"../puds/multi/tourney.pud",
	"../puds/multi/twinhrbr.pud",
	"../puds/multi/up4grabs.pud",
	"../puds/multi/us.pud",
	"../puds/multi/waratsea.pud",
	"../puds/multi/web.pud",
	"../puds/multi/wizard.pud",
	"../puds/single/4_step.pud",
	"../puds/single/anarchy.pud",
	"../puds/single/burn_it.pud",
	"../puds/single/deadmeat.pud",
	"../puds/single/falsie.pud",
	"../puds/single/firering.pud",
	"../puds/single/fortress.pud",
	"../puds/single/grtwall.pud",
	"../puds/single/heroes1.pud",
	"../puds/single/heroes2.pud",
	"../puds/single/invasion.pud",
	"../puds/single/magisle.pud",
	"../puds/single/massacre.pud",
	"../puds/single/midland.pud",
	"../puds/single/minastir.pud",
	"../puds/single/onslaugh.pud",
	"../puds/single/rescue.pud",
	"../puds/single/sacrific.pud",
	"../puds/single/sparta.pud",
	"../puds/single/s_stone.pud",
	"../puds/single/trench.pud",
	"../puds/single/tym.pud",
	"../puds/single/waterres.pud",
	"../puds/single/wish.pud",
	"../puds/strange/chess.pud",
	"../puds/strange/football.pud",
	"../puds/strange/jail.pud",
	"../puds/strange/suicide.pud",
	""
};

typedef struct _grouped_graphic_ {
	int X;               // X offset
	int Y;               // Y offset
	int Width;           // width of image
	int Height;          // height of image
	char Name[100];      // name
} GroupedGraphic;

static GroupedGraphic GroupedGraphicsList[][60] = {
	// group 0 (widgets)
	{
		// 0 and 1 are the same
		{ 0, 0 * 144, 106, 28, "button-grayscale-grayed" },
		{ 0, 1 * 144, 106, 28, "button-grayscale-normal" },
		{ 0, 2 * 144, 106, 28, "button-grayscale-pressed" },
		// 3 and 4 are the same
		{ 0, 3 * 144, 128, 20, "button-thin-medium-grayed" },
		{ 0, 4 * 144, 128, 20, "button-thin-medium-normal" },
		{ 0, 5 * 144, 128, 20, "button-thin-medium-pressed" },
		// 6 and 7 are the same
		{ 0, 6 * 144, 80, 20, "button-thin-small-grayed" },
		{ 0, 7 * 144, 80, 20, "button-thin-small-normal" },
		{ 0, 8 * 144, 80, 20, "button-thin-small-pressed" },
		{ 0, 9 * 144, 106, 28, "button-small-grayed" },
		{ 0, 10 * 144, 106, 28, "button-small-normal" },
		{ 0, 11 * 144, 106, 28, "button-small-pressed" },
		{ 0, 12 * 144, 164, 28, "button-medium-grayed" },
		{ 0, 13 * 144, 164, 28, "button-medium-normal" },
		{ 0, 14 * 144, 164, 28, "button-medium-pressed" },
		{ 0, 15 * 144, 224, 28, "button-large-grayed" },
		{ 0, 16 * 144, 224, 28, "button-large-normal" },
		{ 0, 17 * 144, 224, 28, "button-large-pressed" },
		{ 0, 18 * 144, 19, 19, "radio-grayed" },
		{ 0, 19 * 144, 19, 19, "radio-normal-unselected" },
		{ 0, 20 * 144, 19, 19, "radio-pressed-unselected" },
		{ 0, 21 * 144, 19, 19, "radio-normal-selected" },
		{ 0, 22 * 144, 19, 19, "radio-pressed-selected" },
		{ 0, 23 * 144, 17, 17, "checkbox-grayed" },
		{ 0, 24 * 144, 17, 17, "checkbox-normal-unselected" },
		{ 0, 25 * 144, 17, 17, "checkbox-pressed-unselected" },
		{ 0, 26 * 144, 17, 20, "checkbox-normal-selected" },
		{ 0, 27 * 144, 17, 20, "checkbox-pressed-selected" },
		{ 0, 28 * 144, 19, 20, "up-arrow-grayed" },
		{ 0, 29 * 144, 19, 20, "up-arrow-normal" },
		{ 0, 30 * 144, 19, 20, "up-arrow-pressed" },
		{ 0, 31 * 144, 19, 20, "down-arrow-grayed" },
		{ 0, 32 * 144, 19, 20, "down-arrow-normal" },
		{ 0, 33 * 144, 19, 20, "down-arrow-pressed" },
		{ 0, 34 * 144, 20, 19, "left-arrow-grayed" },
		{ 0, 35 * 144, 20, 19, "left-arrow-normal" },
		{ 0, 36 * 144, 20, 19, "left-arrow-pressed" },
		{ 0, 37 * 144, 20, 19, "right-arrow-grayed" },
		{ 0, 38 * 144, 20, 19, "right-arrow-normal" },
		{ 0, 39 * 144, 20, 19, "right-arrow-pressed" },
		{ 0, 40 * 144, 17, 17, "slider-knob" },
		{ 0, 41 * 144 + 20, 19, 124, "vslider-bar-grayed" },
		{ 0, 42 * 144 + 20, 19, 124, "vslider-bar-normal" },
		{ 20, 43 * 144, 172, 19, "hslider-bar-grayed" },
		{ 20, 44 * 144, 172, 19, "hslider-bar-normal" },
		{ 0, 45 * 144, 300, 18, "pulldown-bar-grayed" },
		{ 0, 46 * 144, 300, 18, "pulldown-bar-normal" },
		// 47 and 48 are the same
		{ 0, 47 * 144, 80, 15, "button-verythin-grayed" },
		{ 0, 48 * 144, 80, 15, "button-verythin-normal" },
		{ 0, 49 * 144, 80, 15, "button-verythin-pressed" },
		// NB: the following 3 sizes are incorrect for human
		{ 0, 50 * 144, 37, 24, "folder-up-grayed" },
		{ 0, 51 * 144, 37, 24, "folder-up-normal" },
		{ 0, 52 * 144, 37, 24, "folder-up-pressed" },
		{ 0, 0, 0, 0, "" },
	}
};

/**
**  File names.
*/
static char* UnitNames[110];
static int UnitNamesLast = 0;

//----------------------------------------------------------------------------
//  TOOLS
//----------------------------------------------------------------------------

/**
**  Check if path exists, if not make all directories.
*/
void CheckPath(const char* path)
{
	char* cp;
	char* s;

	cp = strdup(path);
	s = strrchr(cp, '/');
	if (s) {
		*s = '\0';  // remove file
		s = cp;
		for (;;) {  // make each path element
			s = strchr(s, '/');
			if (s) {
				*s = '\0';
			}
#if defined(_MSC_VER) || defined(WIN32)
			mkdir(cp);
#else
			mkdir(cp, 0777);
#endif
			if (s) {
				*s++ = '/';
			} else {
				break;
			}
		}
	}
	free(cp);
}

/**
**  Given a file name that would appear in a PC archive convert it to what
**  would appear on the Mac.
*/
void ConvertToMac(char* filename)
{
	if (!strcmp(filename, "rezdat.war")) {
		strcpy(filename, "War Resources");
		return;
	}
	if (!strcmp(filename, "strdat.war")) {
		strcpy(filename, "War Strings");
		return;
	}
	if (!strcmp(filename, "maindat.war")) {
		strcpy(filename, "War Data");
		return;
	}
	if (!strcmp(filename, "snddat.war")) {
		strcpy(filename, "War Music");
		return;
	}
	if (!strcmp(filename, "muddat.cud")) {
		strcpy(filename, "War Movies");
		return;
	}
	if (!strcmp(filename, "sfxdat.sud")) {
		strcpy(filename, "War Sounds");
		return;
	}
}

//----------------------------------------------------------------------------
//  PNG
//----------------------------------------------------------------------------

/**
**  Save a png file.
**
**  @param name         File name
**  @param image        Graphic data
**  @param w            Graphic width
**  @param h            Graphic height
**  @param pal          Palette
**  @param transparent  Image uses transparency
*/
int SavePNG(const char* name, unsigned char* image, int x, int y, int w,
	int h, int pitch, unsigned char* pal, int transparent)
{
	FILE* fp;
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned char** lines;
	int i;

	if (!(fp = fopen(name, "wb"))) {
		printf("%s:", name);
		perror("Can't open file");
		fflush(stdout); fflush(stderr);
		return 1;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		fclose(fp);
		return 1;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, NULL);
		fclose(fp);
		return 1;
	}

	if (setjmp(png_ptr->jmpbuf)) {
		// FIXME: must free buffers!!
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return 1;
	}
	png_init_io(png_ptr, fp);

	// zlib parameters
	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

	// prepare the file information
	info_ptr->width = w;
	info_ptr->height = h;
	info_ptr->bit_depth = 8;
	info_ptr->color_type = PNG_COLOR_TYPE_PALETTE;
	info_ptr->interlace_type = 0;
	info_ptr->valid |= PNG_INFO_PLTE;
	info_ptr->palette = (png_colorp)pal;
	info_ptr->num_palette = 256;

	if (transparent) {
		unsigned char* p;
		unsigned char* end;
		png_byte trans[256];

		p = image + y * pitch + x;
		end = image + (y + h) * pitch + x;
		while (p < end) {
			if (!*p) {
				*p = 0xFF;
			}
			++p;
		}

		memset(trans, 0xFF, sizeof(trans));
		trans[255] = 0x0;
		png_set_tRNS(png_ptr, info_ptr, trans, 256, 0);
	}

	// write the file header information
	png_write_info(png_ptr, info_ptr);

	// set transformation

	// prepare image
	lines = (unsigned char **)malloc(h * sizeof(*lines));
	if (!lines) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return 1;
	}

	for (i = 0; i < h; ++i) {
		lines[i] = image + (i + y) * pitch + x;
	}

	png_write_image(png_ptr, lines);
	png_write_end(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);

	free(lines);

	return 0;
}

//----------------------------------------------------------------------------
//  Archive
//----------------------------------------------------------------------------

/**
**  Open the archive file.
**
**  @param file  Archive file name
**  @param type  Archive type requested
*/
int OpenArchive(const char* file, int type)
{
	int f;
	struct stat stat_buf;
	unsigned char* buf;
	unsigned char* cp;
	unsigned char** op;
	int entries;
	int i;

	//
	//  Open the archive file
	//
	f = open(file, O_RDONLY | O_BINARY, 0);
	if (f == -1) {
		printf("Can't open %s\n", file);
		exit(-1);
	}
	if (fstat(f, &stat_buf)) {
		printf("Can't fstat %s\n", file);
		exit(-1);
	}
//	printf("Filesize %ld %ldk\n",
//		(long)stat_buf.st_size, stat_buf.st_size / 1024);

	//
	//  Read in the archive
	//
	buf = (unsigned char *)malloc(stat_buf.st_size);
	if (!buf) {
		printf("Can't malloc %ld\n", (long)stat_buf.st_size);
		exit(-1);
	}
	if (read(f, buf, stat_buf.st_size) != stat_buf.st_size) {
		printf("Can't read %ld\n", (long)stat_buf.st_size);
		exit(-1);
	}
	close(f);

	cp = buf;
	i = FetchLE32(cp);
//	printf("Magic\t%08X\t", i);
	if (i != 0x19) {
		printf("Wrong magic %08x, expected %08x\n", i, 0x00000019);
		exit(-1);
	}
	entries = FetchLE16(cp);
//	printf("Entries\t%5d\t", entries);
	i = FetchLE16(cp);
//	printf("ID\t%d\n", i);
	if (i != type) {
		printf("Wrong type %08x, expected %08x\n", i, type);
		exit(-1);
	}

	//
	//  Read offsets.
	//
	op = (unsigned char **)malloc((entries + 1) * sizeof(unsigned char*));
	if (!op) {
		printf("Can't malloc %d entries\n", entries);
		exit(-1);
	}
	for (i = 0; i < entries; ++i) {
		op[i] = buf + FetchLE32(cp);
//		printf("Offset\t%d\n", op[i]);
	}
	op[i] = buf + stat_buf.st_size;

	ArchiveOffsets = op;
	ArchiveBuffer = buf;

	return 0;
}

/**
**  Extract/uncompress entry.
**
**  @param cp    Pointer to compressed entry
**  @param lenp  Return pointer of length of the entry
**
**  @return      Pointer to uncompressed entry
*/
unsigned char* ExtractEntry(unsigned char* cp, size_t* lenp)
{
	unsigned char* dp;
	unsigned char* dest;
	size_t uncompressed_length;
	int flags;

	uncompressed_length = FetchLE32(cp);
	flags = uncompressed_length >> 24;
	uncompressed_length &= 0x00FFFFFF;
//	printf("Entry length %8d flags %02x\t", uncompressed_length, flags);

	dp = dest = (unsigned char *)malloc(uncompressed_length);
	if (!dest) {
		printf("Can't malloc %d\n", (int)uncompressed_length);
		exit(-1);
	}

	if (flags == 0x20) {
		unsigned char buf[4096];
		unsigned char* ep;
		int bi;

//		printf("Compressed entry\n");

		bi = 0;
		memset(buf, 0, sizeof(buf));
		ep = dp + uncompressed_length;

		// FIXME: If the decompression is too slow, optimise this loop :->
		while (dp < ep) {
			int i;
			int bflags;

			bflags = FetchByte(cp);
//			printf("Ctrl %02x ", bflags);
			for (i = 0; i < 8; ++i) {
				int j;
				int o;

				if (bflags & 1) {
					j = FetchByte(cp);
					*dp++ = j;
					buf[bi++ & 0xFFF] = j;
//					printf("=%02x", j);
				} else {
					o = FetchLE16(cp);
//					printf("*%d,%d", o >> 12, o & 0xFFF);
					j = (o >> 12) + 3;
					o &= 0xFFF;
					while (j--) {
						buf[bi++ & 0xFFF] = *dp++ = buf[o++ & 0xFFF];
						if (dp == ep) {
							break;
						}
					}
				}
				if (dp == ep) {
					break;
				}
				bflags >>= 1;
			}
//			printf("\n");
		}
		//if (dp != ep) printf("%p,%p %d\n", dp, ep, dp - dest);
	} else if (flags == 0x00) {
//		printf("Uncompressed entry\n");
		memcpy(dest, cp, uncompressed_length);
	} else {
		printf("Unknown flags %x\n", flags);
		exit(-1);
	}

	// return resulting length
	if (lenp) {
		*lenp = uncompressed_length;
	}

	return dest;
}

/**
**  Close the archive file.
*/
int CloseArchive(void)
{
	free(ArchiveBuffer);
	free(ArchiveOffsets);
	ArchiveBuffer = 0;
	ArchiveOffsets = 0;

	return 0;
}

//----------------------------------------------------------------------------
//  Palette
//----------------------------------------------------------------------------

/**
**  Convert palette.
**
**  @param pal  Pointer to palette
**
**  @return     Pointer to palette
*/
unsigned char* ConvertPalette(unsigned char* pal)
{
	int i;

	// PNG needs 0-256
	for (i = 0; i < 768; ++i) {
		pal[i] <<= 2;
	}

	return pal;
}

/**
**  Convert rgb to my format.
*/
int ConvertRgb(char* file, int rgbe)
{
	unsigned char* rgbp;
	char buf[1024];
	FILE* f;
	int i;
	size_t l;

	rgbp = ExtractEntry(ArchiveOffsets[rgbe], &l);
	ConvertPalette(rgbp);

	//
	//  Generate RGB File.
	//
	sprintf(buf, "%s/%s/%s.rgb", Dir, TILESET_PATH, file);
	CheckPath(buf);
	f = fopen(buf, "wb");
	if (!f) {
		perror("");
		printf("Can't open %s\n", buf);
		exit(-1);
	}
	if (l != fwrite(rgbp, 1, l, f)) {
		printf("Can't write %d bytes\n", (int)l);
		fflush(stdout);
	}

	fclose(f);

	//
	//  Generate GIMP palette
	//
	sprintf(buf, "%s/%s/%s.gimp", Dir, TILESET_PATH, file);
	CheckPath(buf);
	f = fopen(buf, "wb");
	if (!f) {
		perror("");
		printf("Can't open %s\n", buf);
		exit(-1);
	}
	fprintf(f, "GIMP Palette\n# Stratagus %c%s -- GIMP Palette file\n",
		toupper(*file), file + 1);

	for (i = 0; i < 256; ++i) {
		// FIXME: insert nice names!
		fprintf(f, "%d %d %d\t#%d\n",
			rgbp[i * 3], rgbp[i * 3 + 1], rgbp[i * 3 + 2], i);
	}

	fclose(f);
	free(rgbp);

	return 0;
}

//----------------------------------------------------------------------------
//  Tileset
//----------------------------------------------------------------------------

/**
**  Count used mega tiles for map.
*/
int CountUsedTiles(const unsigned char* map, const unsigned char* mega,
	int* map2tile)
{
	int i;
	int j;
	int used;
	const unsigned char* tp;
	int img2tile[0x9E0];

	memset(map2tile, 0, sizeof(map2tile));

	//
	//  Build conversion table.
	//
	for (i = 0; i < 0x9E; ++i) {
		tp = map + i * 42;
//		printf("%02X:", i);
		for (j = 0; j < 0x10; ++j) {
//			printf("%04X ", AccessLE16(tp + j * 2));
			map2tile[(i << 4) | j] = AccessLE16(tp + j * 2);
		}
//		printf("\n");
	}

	//
	//  Mark all used mega tiles.
	//
	used = 0;
	for (i = 0; i < 0x9E0; ++i) {
		if (!map2tile[i]) {
			continue;
		}
		for (j = 0; j < used; ++j) {
			if (img2tile[j] == map2tile[i]) {
				break;
			}
		}
		if (j == used) {
			//
			//  Check unique mega tiles.
			//
			for (j = 0; j < used; ++j) {
				if (!memcmp(mega + img2tile[j] * 32, mega + map2tile[i] * 32, 32)) {
					break;
				}
			}
			if (j == used) {
				img2tile[used++] = map2tile[i];
			}
		}
	}
//	printf("Used mega tiles %d\n", used);
#if 0
	for (i = 0; i < used; ++i) {
		if (!(i % 16)) {
			printf("\n");
		}
		printf("%3d ",img2tile[i]);
	}
	printf("\n");
#endif

	return used;
}

/**
**  Convert for ccl.
*/
static void SaveCCL(char* name, unsigned char* map __attribute__((unused)),
	const int* map2tile)
{
	int i;
	char* cp;
	FILE* f;
	char file[1024];
	char tileset[1024];

	f = stdout;
	// FIXME: open file!

	// remove leading path
	if ((cp = strrchr(name, '/'))) {
		++cp;
	} else {
		cp = (char*)name;
	}
	strcpy(file, cp);
	strcpy(tileset, cp);
	// remove suffix
	if ((cp = strrchr(tileset, '.'))) {
		*cp = '\0';
	}

	fprintf(f, "(tileset Tileset%c%s \"%s\" \"%s\"\n",
		toupper(*tileset), tileset + 1, tileset, file);

	fprintf(f, "  #(");
	for (i = 0; i < 0x9E0; ++i) {
		if (i & 15) {
			fprintf(f, " ");
		} else if (i) {
			fprintf(f, "\t; %03X\n	", i - 16);
		}
		fprintf(f, "%3d", map2tile[i]);
	}

	fprintf(f, "  ))\n");

	// fclose(f);
}

/**
**  Decode a minitile into the image.
*/
void DecodeMiniTile(unsigned char* image, int ix, int iy, int iadd,
	unsigned char* mini, int index, int flipx, int flipy)
{
	static const int flip[] = {
		7, 6, 5, 4, 3, 2, 1, 0, 8
	};
	int x;
	int y;

	for (y = 0; y < 8; ++y) {
		for (x = 0; x < 8; ++x) {
			image[(y + iy * 8) * iadd + ix * 8 + x] = mini[index +
				(flipy ? flip[y] : y) * 8 + (flipx ? flip[x] : x)];
		}
	}
}

/**
**  Convert tiles into image.
*/
unsigned char* ConvertTile(unsigned char* mini, const unsigned char* mega, int msize,
	const unsigned char* map __attribute__((unused)), int *wp, int *hp)
{
	unsigned char* image;
	const unsigned short* mp;
	int height;
	int width;
	int i;
	int x;
	int y;
	int offset;
	int numtiles;

//	printf("Tiles in mega %d\t", msize / 32);
	numtiles = msize / 32;

	width = TILE_PER_ROW * 32;
	height = ((numtiles + TILE_PER_ROW - 1) / TILE_PER_ROW) * 32;
//	printf("Image %dx%d\n", width, height);
	image = (unsigned char *)malloc(height * width);
	memset(image, 0, height * width);

	for (i = 0; i < numtiles; ++i) {
		//mp = (const unsigned short*)(mega + img2tile[i] * 32);
		mp = (const unsigned short*)(mega + i * 32);
		if (i < 16) {  // fog of war
			for (y = 0; y < 32; ++y) {
				offset = i * 32 * 32 + y * 32;
				memcpy(image + (i % TILE_PER_ROW) * 32 +
					(((i / TILE_PER_ROW) * 32) + y) * width,
					mini + offset, 32);
			}
		} else {  // normal tile
			for (y = 0; y < 4; ++y) {
				for (x = 0; x < 4; ++x) {
					offset = ConvertLE16(mp[x + y * 4]);
					DecodeMiniTile(image,
						x + ((i % TILE_PER_ROW) * 4), y + (i / TILE_PER_ROW) * 4, width,
						mini, (offset & 0xFFFC) * 16, offset & 2, offset & 1);
				}
			}
		}
	}

	*wp = width;
	*hp = height;

	return image;
}

/**
**  Convert a tileset to my format.
*/
int ConvertTileset(char* file, int pale, int mege, int mine, int mape)
{
	unsigned char* palp;
	unsigned char* megp;
	unsigned char* minp;
	unsigned char* mapp;
	unsigned char* image;
	int w;
	int h;
	size_t megl;
	char buf[1024];

	palp = ExtractEntry(ArchiveOffsets[pale], NULL);
	megp = ExtractEntry(ArchiveOffsets[mege], &megl);
	minp = ExtractEntry(ArchiveOffsets[mine], NULL);
	mapp = ExtractEntry(ArchiveOffsets[mape], NULL);

//	printf("%s:\t", file);
	image = ConvertTile(minp, megp, megl, mapp, &w, &h);

	free(megp);
	free(minp);
	free(mapp);

	ConvertPalette(palp);

	sprintf(buf, "%s/%s/%s.png", Dir, TILESET_PATH, file);
	CheckPath(buf);
	SavePNG(buf, image, 0, 0, w, h, w, palp, 1);

	free(image);
	free(palp);

	return 0;
}

//----------------------------------------------------------------------------
//  Graphics
//----------------------------------------------------------------------------

/**
**  Decode a entry(frame) into image.
*/
void DecodeGfxEntry(int index, unsigned char* start,
	unsigned char* image, int ix, int iy, int iadd)
{
	unsigned char* bp;
	unsigned char* sp;
	unsigned char* dp;
	int xoff;
	int yoff;
	int width;
	int height;
	int offset;
	unsigned char* rows;
	int h;
	int w;
	int ctrl;

	bp = start + index * 8;
	xoff = FetchByte(bp);
	yoff = FetchByte(bp);
	width = FetchByte(bp);
	height = FetchByte(bp);
	offset = FetchLE32(bp);

//	printf("%2d: +x %2d +y %2d width %2d height %2d offset %d\n",
//		index, xoff, yoff, width, height, offset);

	rows = start + offset - 6;
	dp = image + xoff - ix + (yoff - iy) * iadd;

	for (h = 0; h < height; ++h) {
//		printf("%2d: row-offset %2d\t", index, AccessLE16(rows + h * 2));
		sp = rows + AccessLE16(rows + h * 2);
		for (w = 0; w < width; ) {
			ctrl = *sp++;
//			printf("%02X", ctrl);
			if (ctrl & 0x80) {  // transparent
				ctrl &= 0x7F;
//				printf("-%d,", ctrl);
				memset(dp+h*iadd+w,255,ctrl);
				w+=ctrl;
			} else if (ctrl & 0x40) {  // repeat
				ctrl &= 0x3F;
//				printf("*%d,", ctrl);
				memset(dp + h * iadd + w, *sp++, ctrl);
				w += ctrl;
			} else {						// set pixels
				ctrl &= 0x3F;
//				printf("=%d,", ctrl);
				memcpy(dp + h * iadd + w, sp, ctrl);
				sp += ctrl;
				w += ctrl;
			}
		}
		//dp[h * iadd + width - 1] = 0;
//		printf("\n");
	}
}

/**
**  Decode a entry(frame) into image.
*/
void DecodeGfuEntry(int index, unsigned char* start,
	unsigned char* image, int ix, int iy, int iadd)
{
	unsigned char* bp;
	unsigned char* sp;
	unsigned char* dp;
	int i;
	int xoff;
	int yoff;
	int width;
	int height;
	int offset;

	bp = start + index * 8;
	xoff = FetchByte(bp);
	yoff = FetchByte(bp);
	width = FetchByte(bp);
	height = FetchByte(bp);
	offset = FetchLE32(bp);
	// High bit of width
	if (offset < 0) {
		offset &= 0x7FFFFFFF;
		width += 256;
	}

//	printf("%2d: +x %2d +y %2d width %2d height %2d offset %d\n",
//		index, xoff, yoff, width, height, offset);

	sp = start + offset - 6;
	dp = image + xoff - ix + (yoff - iy) * iadd;
	for (i = 0; i < height; ++i) {
		memcpy(dp, sp, width);
		dp += iadd;
		sp += width;
	}
}

/**
**  Convert graphics into image.
*/
unsigned char* ConvertGraphic(int gfx, unsigned char* bp, int *wp, int *hp,
	unsigned char* bp2, int start2)
{
	int i;
	int count;
	int length;
	int max_width;
	int max_height;
	int minx;
	int miny;
	int best_width;
	int best_height;
	unsigned char* image;
	int IPR;

	// Init pointer to 2nd animation
	if (bp2) {
		count = FetchLE16(bp2);
		max_width = FetchLE16(bp2);
		max_height = FetchLE16(bp2);
	}
	count = FetchLE16(bp);
	max_width = FetchLE16(bp);
	max_height = FetchLE16(bp);


//	printf("Entries %2d Max width %3d height %3d, ", count,
//		max_width, max_height);

	// Find best image size
	minx = 999;
	miny = 999;
	best_width = 0;
	best_height = 0;
	for (i = 0; i < count; ++i) {
		unsigned char* p;
		int xoff;
		int yoff;
		int width;
		int height;

		p = bp + i * 8;
		xoff = FetchByte(p);
		yoff = FetchByte(p);
		width = FetchByte(p);
		height = FetchByte(p);
		// high bit of width
		if (AccessLE32(p) & 0x80000000) {
			width += 256;
		}
		// increase p by 32bits, as AccessLE32 cannot do it above.
		p += 4;
		if (xoff < minx) {
			minx = xoff;
		}
		if (yoff < miny) {
			miny = yoff;
		}
		if (xoff + width > best_width) {
			best_width = xoff + width;
		}
		if (yoff + height > best_height) {
			best_height = yoff + height;
		}
	}
	// FIXME: the image isn't centered!!

#if 0
	// Taken out, must be rewritten.
	if (max_width - best_width < minx) {
		minx = max_width - best_width;
		best_width -= minx;
	} else {
		best_width = max_width - minx;
	}
	if (max_height - best_height < miny) {
		miny = max_height - best_height;
		best_height -= miny;
	} else {
		best_height = max_width - miny;
	}

	//best_width -= minx;
	//best_height -= miny;
#endif

//	printf("Best image size %3d, %3d\n", best_width, best_height);

	minx = 0;
	miny = 0;

	if (gfx) {
		best_width = max_width;
		best_height = max_height;
		IPR = 5;  // st*rcr*ft 17!
		if (count < IPR) {  // images per row !!
			IPR = 1;
			length = count;
		} else {
			length = ((count + IPR - 1) / IPR) * IPR;
		}
	} else {
		max_width = best_width;
		max_height = best_height;
		IPR = 1;
		length = count;
	}

	image = (unsigned char *)malloc(best_width * best_height * length);

	//  Image: 0, 1, 2, 3, 4,
	//         5, 6, 7, 8, 9, ...
	if (!image) {
		printf("Can't allocate image\n");
		exit(-1);
	}
	// Set all to transparent.
	memset(image, 255, best_width * best_height * length);

	if (gfx) {
		for (i = 0; i < count; ++i) {
			// Hardcoded support for worker with resource repairing
			if (i >= start2 && bp2) {
				DecodeGfxEntry(i, bp2,
					image + best_width * (i % IPR) + best_height * best_width * IPR * (i / IPR),
					minx, miny, best_width * IPR);
			} else {
				DecodeGfxEntry(i, bp,
					image + best_width * (i % IPR) + best_height * best_width * IPR * (i / IPR),
					minx, miny, best_width * IPR);
			}
		}
	} else {
		for (i = 0; i < count; ++i) {
			DecodeGfuEntry(i, bp,
				image + best_width * (i % IPR) + best_height * best_width * IPR * (i / IPR),
				minx, miny, best_width * IPR);
		}
	}

	*wp = best_width * IPR;
	*hp = best_height * (length / IPR);

	return image;
}

/**
**  Convert a graphic to my format.
*/
int ConvertGfx(char* file, int pale, int gfxe, int gfxe2, int start2)
{
	unsigned char* palp;
	unsigned char* gfxp;
	unsigned char* gfxp2;
	unsigned char* image;
	int w;
	int h;
	char buf[1024];

	palp = ExtractEntry(ArchiveOffsets[pale], NULL);
	gfxp = ExtractEntry(ArchiveOffsets[gfxe], NULL);
	if (gfxe2) {
		gfxp2 = ExtractEntry(ArchiveOffsets[gfxe2], NULL);
	} else {
		gfxp2 = NULL;
	}

	image = ConvertGraphic(1, gfxp, &w, &h, gfxp2, start2);

	if (gfxp2) {
		free(gfxp2);
	}

	free(gfxp);
	ConvertPalette(palp);

	sprintf(buf, "%s/%s/%s.png", Dir, GRAPHICS_PATH, file);
	CheckPath(buf);
	SavePNG(buf, image, 0, 0, w, h, w, palp, 1);

	free(image);
	free(palp);

	return 0;
}

/**
**  Convert a uncompressed graphic to my format.
*/
int ConvertGfu(char* file,int pale,int gfue)
{
	unsigned char* palp;
	unsigned char* gfup;
	unsigned char* image;
	int w;
	int h;
	char buf[1024];

	palp = ExtractEntry(ArchiveOffsets[pale], NULL);
	gfup = ExtractEntry(ArchiveOffsets[gfue], NULL);

	image = ConvertGraphic(0, gfup, &w, &h, NULL, 0);

	free(gfup);
	ConvertPalette(palp);

	sprintf(buf, "%s/%s/%s.png", Dir, GRAPHICS_PATH, file);
	CheckPath(buf);
	SavePNG(buf, image, 0, 0, w, h, w, palp, 1);

	free(image);
	free(palp);

	return 0;
}

/**
**  Split up and convert uncompressed graphics to seperate PNGs
*/
int ConvertGroupedGfu(char *path, int pale, int gfue, int glist)
{
	unsigned char* palp;
	unsigned char* gfup;
	unsigned char* image;
	int w;
	int h;
	char buf[1024];
	int i;
	GroupedGraphic *gg;

	palp = ExtractEntry(ArchiveOffsets[pale], NULL);
	gfup = ExtractEntry(ArchiveOffsets[gfue], NULL);

	image = ConvertGraphic(0, gfup, &w, &h, NULL, 0);

	free(gfup);
	ConvertPalette(palp);

	for (i = 0; GroupedGraphicsList[glist][i].Name[0]; ++i) {
		gg = &GroupedGraphicsList[glist][i];

		// hack for expansion/original difference
		if (gg->Y + gg->Height > h) {
			break;
		}

		// hack for multiple palettes
		if (i == 3 && strstr(path, "widgets")) {
			free(palp);
			palp = ExtractEntry(ArchiveOffsets[14], NULL);
			ConvertPalette(palp);
		}

		sprintf(buf, "%s/%s/%s/%s.png", Dir, GRAPHICS_PATH, path, gg->Name);
		CheckPath(buf);
		SavePNG(buf, image, gg->X, gg->Y, gg->Width, gg->Height, w, palp, 1);
	}

	free(image);
	free(palp);

	return 0;
}

//----------------------------------------------------------------------------
//  Puds
//----------------------------------------------------------------------------

/**
**  Convert pud to my format.
*/
void ConvertPud(char* file, int pude)
{
	unsigned char* pudp;
	char buf[1024];
	size_t l;

	pudp = ExtractEntry(ArchiveOffsets[pude], &l);

	sprintf(buf, "%s/%s/%s", Dir, PUD_PATH, file);
	CheckPath(buf);

	*strrchr(buf, '/') = '\0';
	PudToStratagus(pudp, l, strrchr(file, '/') + 1, buf);

	free(pudp);
}

/**
**	Convert puds that are in their own file
*/
void ConvertFilePuds(char **pudlist)
{
	char pudname[1024];
	char base[1024];
	char outdir[1024];
	unsigned char *puddata;
	struct stat sb;
	FILE *f;
	int i;

	for (i = 0; pudlist[i][0] != '\0'; ++i) {
		char origname[1024];
		if (CDType & CD_UPPER) {
			char filename[1024];
			int j = 0;
			strcpy(filename, pudlist[i]);
			strcpy(origname, pudlist[i]);
			pudlist[i] = filename;
			while (pudlist[i][j]) {
				pudlist[i][j] = toupper(pudlist[i][j]);
				++j;
			}
		}
		sprintf(pudname, "%s/%s", ArchiveDir, pudlist[i]);
		if (stat(pudname, &sb)) {
			continue;
		}
		if (!(f = fopen(pudname, "rb"))) {
			return;
		}
		puddata = (unsigned char *)malloc(sb.st_size);
		if (!puddata) {
			return;
		}
		if (!fread(puddata, 1, sb.st_size, f)) {
			return;
		}
		fclose(f);

		strcpy(base, strrchr(pudlist[i], '/') + 1);

		if (CDType & CD_UPPER) {
			*strstr(base, ".PUD") = '\0';
			pudlist[i] = origname;
		} else {
			*strstr(base, ".pud") = '\0';
		}

		if (strstr(pudlist[i], "puds/")) {
			sprintf(outdir, "%s/maps/%s", Dir, strstr(pudlist[i], "puds/") + 5);
			*strrchr(outdir, '/') = '\0';
		} else {
			sprintf(outdir, "%s/maps", Dir);
		}

		strcpy(pudname, outdir);
		strcat(pudname, "/dummy");
		CheckPath(pudname);

		PudToStratagus(puddata, (size_t)(sb.st_size), base, outdir);
		free(puddata);
	}
}

//----------------------------------------------------------------------------
//  Font
//----------------------------------------------------------------------------

/**
**  Convert font into image.
*/
unsigned char* ConvertFnt(unsigned char* start, int *wp, int *hp)
{
	int i;
	int count;
	int max_width;
	int max_height;
	int width;
	int height;
	int w;
	int h;
	int xoff;
	int yoff;
	unsigned char* bp;
	unsigned char* dp;
	unsigned char* image;
	unsigned* offsets;
	int image_width;
	int image_height;
	int IPR;

	bp = start + 5;  // skip "FONT "
	count = FetchByte(bp);
	count -= 32;
	max_width = FetchByte(bp);
	max_height = FetchByte(bp);

	IPR = 15;  // 15 characters per row
	image_width = max_width * IPR;
	image_height = (count + IPR - 1) / IPR * max_height;

//	printf("Font: count %d max-width %2d max-height %2d\n",
//		count, max_width, max_height);

	offsets = (unsigned *)malloc(count * sizeof(uint32_t));
	for (i = 0; i < count; ++i) {
		offsets[i] = FetchLE32(bp);
//		printf("%03d: offset %d\n", i, offsets[i]);
	}

	image = (unsigned char *)malloc(image_width * image_height);
	if (!image) {
		printf("Can't allocate image\n");
		exit(-1);
	}
	memset(image, 255, image_width * image_height);

	for (i = 0; i < count; ++i) {
		if (!offsets[i]) {
//			printf("%03d: unused\n", i);
			continue;
		}
		bp = start + offsets[i];
		width = FetchByte(bp);
		height = FetchByte(bp);
		xoff = FetchByte(bp);
		yoff = FetchByte(bp);

//		printf("%03d: width %d height %d xoff %d yoff %d\n",
//			i, width, height, xoff, yoff);

		dp = image + (i % IPR) * max_width + (i / IPR) * image_width * max_height;
		dp += xoff + yoff * image_width;
		h = w = 0;
		for (;;) {
			int ctrl;

			ctrl = FetchByte(bp);
//			printf("%d,%d ", ctrl >> 3, ctrl & 7);
			w += (ctrl >> 3) & 0x1F;
			if (w >= width) {
//				printf("\n");
				w -= width;
				++h;
				if (h >= height) {
					break;
				}
			}
			dp[h * image_width + w] = ctrl & 0x07;
			++w;
			if (w >= width) {
//				printf("\n");
				w -= width;
				++h;
				if (h >= height) {
					break;
				}
			}
		}
	}

	free(offsets);

	*wp = image_width;
	*hp = image_height;

	return image;
}

/**
**  Convert a font to my format.
*/
int ConvertFont(char* file, int pale, int fnte)
{
	unsigned char* palp;
	unsigned char* fntp;
	unsigned char* image;
	int w;
	int h;
	char buf[1024];

	palp = ExtractEntry(ArchiveOffsets[pale], NULL);
	fntp = ExtractEntry(ArchiveOffsets[fnte], NULL);

	image = ConvertFnt(fntp, &w, &h);

	free(fntp);
	ConvertPalette(palp);

	sprintf(buf, "%s/%s/%s.png", Dir, FONT_PATH, file);
	CheckPath(buf);
	SavePNG(buf, image, 0, 0, w, h, w, palp, 1);

	if (!strcmp(file, "game")) {
		game_font_width = w / 15;
	}

	free(image);
	free(palp);

	return 0;
}

//----------------------------------------------------------------------------
//  Image
//----------------------------------------------------------------------------

/**
**  Convert image into image.
*/
unsigned char* ConvertImg(unsigned char* bp, int* wp, int* hp)
{
	int width;
	int height;
	unsigned char* image;

	width = FetchLE16(bp);
	height = FetchLE16(bp);

//	printf("Image: width %3d height %3d\n", width, height);

	image = (unsigned char *)malloc(width * height);
	if (!image) {
		printf("Can't allocate image\n");
		exit(-1);
	}
	memcpy(image, bp, width * height);

	*wp = width;
	*hp = height;

	if (!*wp || !*hp) {
		return NULL;
	} else {
		return image;
	}
}

/**
**  Resize an image
**
**  @param image  image data to be converted
**  @param ow     old image width
**  @param oh     old image height
**  @param nw     new image width
**  @param nh     new image height
*/
void ResizeImage(unsigned char** image, int ow, int oh, int nw, int nh)
{
	int i;
	int j;
	unsigned char *data;
	int x;

	if (ow == nw && nh == oh) {
		return;
	}

	data = (unsigned char *)malloc(nw * nh);
	x = 0;
	for (i = 0; i < nh; ++i) {
		for (j = 0; j < nw; ++j) {
			data[x] = ((unsigned char*) * image)[i * oh / nh * ow + j * ow / nw];
			++x;
		}
	}

	free(*image);
	*image = data;
}

/**
**  Convert an image to my format.
*/
int ConvertImage(char* file, int pale, int imge, int nw, int nh)
{
	unsigned char* palp;
	unsigned char* imgp;
	unsigned char* image;
	int w;
	int h;
	char buf[1024];

	// Workaround for MAC expansion CD
	if (CDType & CD_MAC) {
		if (imge >= 94 && imge <= 103) {
			imge += 7;
		}
		if (pale == 93) {
			pale += 7;
		}
	}

	palp = ExtractEntry(ArchiveOffsets[pale], NULL);
	if (pale == 27 && imge == 28) {
		Pal27 = palp;
	}
	imgp = ExtractEntry(ArchiveOffsets[imge], NULL);

	image = ConvertImg(imgp, &w, &h);

	if (!image) {
		printf("Please report this bug, could not extract image: file=%s pale=%d imge=%d nw=%d nh=%d mac=%d\n",
			file, pale, imge, nw, nh, CDType & CD_MAC);
		exit(-1);
	}
	free(imgp);
	ConvertPalette(palp);

	sprintf(buf, "%s/%s/%s.png", Dir, GRAPHIC_PATH, file);
	CheckPath(buf);

	// Only resize if parameters 3 and 4 are non-zero
	if (nw && nh) {
		ResizeImage(&image, w, h, nw, nh);
		w = nw;
		h = nh;
	}
	SavePNG(buf, image, 0, 0, w, h, w, palp, 0);

	free(image);
	if (pale != 27 || imge != 28) {
		free(palp);
	}

	return 0;
}

//----------------------------------------------------------------------------
//  Cursor
//----------------------------------------------------------------------------

/**
**  Convert cursor into image.
*/
unsigned char* ConvertCur(unsigned char* bp, int* wp, int* hp)
{
	int i;
	int hotx;
	int hoty;
	int width;
	int height;
	unsigned char* image;

	hotx = FetchLE16(bp);
	hoty = FetchLE16(bp);
	width = FetchLE16(bp);
	height = FetchLE16(bp);

//	printf("Cursor: hotx %d hoty %d width %d height %d\n",
//		hotx, hoty, width, height);

	image = (unsigned char *)malloc(width * height);
	if (!image) {
		printf("Can't allocate image\n");
		exit(-1);
	}
	for (i = 0; i < width * height; ++i) {
		image[i] = bp[i] ? bp[i] : 255;
	}

	*wp = width;
	*hp = height;

	return image;
}

/**
**  Convert a cursor to my format.
*/
int ConvertCursor(char* file, int pale, int cure)
{
	unsigned char* palp;
	unsigned char* curp;
	unsigned char* image;
	int w;
	int h;
	char buf[1024];

	if (pale == 27 && cure == 314 && Pal27 ) { // Credits arrow (Blue arrow NW)
		palp = Pal27;
	} else {
		palp = ExtractEntry(ArchiveOffsets[pale], NULL);
	}
	curp = ExtractEntry(ArchiveOffsets[cure], NULL);

	image = ConvertCur(curp, &w, &h);

	free(curp);
	ConvertPalette(palp);

	sprintf(buf, "%s/%s/%s.png", Dir, CURSOR_PATH, file);
	CheckPath(buf);
	SavePNG(buf, image, 0, 0, w, h, w, palp, 1);

	free(image);
	if (pale != 27 || cure != 314 || !Pal27) {
		free(palp);
	}

	return 0;
}

//----------------------------------------------------------------------------
//  Wav
//----------------------------------------------------------------------------

/**
**  Extract Wav
*/
int ConvertWav(char* file, int wave)
{
	unsigned char* wavp;
	char buf[1024];
	gzFile gf;
	size_t l;

	wavp = ExtractEntry(ArchiveOffsets[wave], &l);

	sprintf(buf, "%s/%s/%s.wav.gz", Dir, SOUND_PATH, file);
	CheckPath(buf);
	gf = gzopen(buf, "wb9");
	if (!gf) {
		perror("");
		printf("Can't open %s\n", buf);
		exit(-1);
	}
	if (l != (size_t)gzwrite(gf, wavp, l)) {
		printf("Can't write %d bytes\n", (int)l);
		fflush(stdout);
	}

	free(wavp);

	gzclose(gf);
	return 0;
}

//----------------------------------------------------------------------------
//  XMI Midi
//----------------------------------------------------------------------------

/**
**  Convert XMI Midi sound to OGG
*/

int ConvertXmi(char* file, int xmi)
{
	unsigned char* xmip;
	unsigned char* midp;
	unsigned char* oggp;
	char buf[1024];
	char* cmd;
	gzFile gf;
	FILE* f;
	size_t xmil;
	size_t midl;
	size_t oggl;
	int ret;

	xmip = ExtractEntry(ArchiveOffsets[xmi], &xmil);
	midp = TranscodeXmiToMid(xmip, xmil, &midl);
	free(xmip);

	sprintf(buf, "%s/%s/%s.mid", Dir, MUSIC_PATH, file);
	CheckPath(buf);
	f = fopen(buf, "wb");
	if (!f) {
		perror("");
		printf("Can't open %s\n", buf);
		exit(-1);
	}
	if (midl != fwrite(midp, 1, midl, f)) {
		printf("Can't write %d bytes\n", (int)midl);
		fflush(stdout);
	}

	free(midp);
	fclose(f);

	cmd = (char*) calloc(strlen("timidity -Ow \"") + strlen(buf) + strlen("\" -o \"") + strlen(buf) + strlen("\"") + 1, 1);
	if (!cmd) {
		fprintf(stderr, "Memory error\n");
		exit(-1);
	}

	sprintf(cmd, "timidity -Ow \"%s/%s/%s.mid\" -o \"%s/%s/%s.wav\"", Dir, MUSIC_PATH, file, Dir, MUSIC_PATH, file);

	ret = system(cmd);

	free(cmd);
	remove(buf);

	if (ret != 0) {
		printf("Can't convert midi sound %s to wav format. Is timidity installed in PATH?\n", file);
		fflush(stdout);
		return ret;
	}

	sprintf(buf, "%s/%s/%s.wav", Dir, MUSIC_PATH, file);
	CheckPath(buf);

	cmd = (char*) calloc(strlen("ffmpeg2theora --optimize \"") + strlen(buf) + strlen("\" -o \"") + strlen(buf) + strlen("\"") + 1, 1);
	if (!cmd) {
		fprintf(stderr, "Memory error\n");
		exit(-1);
	}

	sprintf(cmd, "ffmpeg2theora --optimize \"%s/%s/%s.wav\" -o \"%s/%s/%s.ogg\"", Dir, MUSIC_PATH, file, Dir, MUSIC_PATH, file);

	ret = system(cmd);

	free(cmd);
	remove(buf);

	if (ret != 0) {
		printf("Can't convert wav sound %s to ogv format. Is ffmpeg2theora installed in PATH?\n", file);
		fflush(stdout);
		return ret;
	}

	sprintf(buf, "%s/%s/%s.ogg", Dir, MUSIC_PATH, file);
	CheckPath(buf);
	f = fopen(buf, "rb");
	if (!f) {
		perror("");
		printf("Can't open %s\n", buf);
		exit(-1);
	}

	fseek(f, 0, SEEK_END);
	oggl = ftell(f);
	rewind(f);

	oggp = (unsigned char*) malloc(oggl);
	if (!oggp) {
		fprintf(stderr, "Memory error\n");
		exit(-1);
	}

	if (oggl != (size_t)fread(oggp, 1, oggl, f)) {
		printf("Can't read %d bytes\n", (int)oggl);
		fflush(stdout);
	}

	fclose(f);
	remove(buf);

	sprintf(buf, "%s/%s/%s.ogg.gz", Dir, MUSIC_PATH, file);
	CheckPath(buf);
	gf = gzopen(buf, "wb9");
	if (!gf) {
		perror("");
		printf("Can't open %s\n", buf);
		exit(-1);
	}

	if (oggl != (size_t)gzwrite(gf, oggp, oggl)) {
		printf("Can't write %d bytes\n", (int)oggl);
		fflush(stdout);
	}

	gzclose(gf);
	free(oggp);

	return 0;
}

//----------------------------------------------------------------------------
//  Ripped music
//----------------------------------------------------------------------------


/**
**  Copy file
*/

#if ! defined(_MSC_VER) && ! defined(WIN32)
int CopyFile(char *from, char *to, int overwrite)
{
	struct stat st;
	char *cmd;
	int ret;

	if (!overwrite && !stat(to, &st))
		return 0;

	cmd = calloc(strlen("cp \"") + strlen(from) + strlen("\" \"") + strlen(to) + strlen("\""), 1);
	if (!cmd) {
		fprintf(stderr, "Memory error\n");
		exit(-1);
	}

	sprintf(cmd, "cp \"%s\" \"%s\"", from, to);
	ret = system(cmd);
	free(cmd);

	if (ret != 0)
		return 0;
	else
		return 1;
}
#endif

/**
**  Copy ripped music
*/

int CopyMusic(void)
{
	struct stat st;
	char buf1[1024];
	char buf2[1024];
	char ext[4];
	int i;
	int count = 0;

	for (i = 0; MusicNames[i]; ++i) {
		strcpy(ext, "wav");
		sprintf(buf1, "%s/music/%s.%s", ArchiveDir, MusicNames[i], ext);
		if (stat(buf1, &st)) {
			strcpy(ext, "ogg");
			sprintf(buf1, "%s/music/%s.%s", ArchiveDir, MusicNames[i], ext);
			if (stat(buf1, &st))
				continue;
		}

		printf("Found ripped music file \"%s\"\n", buf1);
		fflush(stdout);

		sprintf(buf2, "%s/%s/%s.%s", Dir, MUSIC_PATH, MusicNames[i], ext);
		CheckPath(buf2);
		if (CopyFile(buf1, buf2, 0))
			++count;
	}

	if (count == 0)
		return 1;
	else
		return 0;
}

/**
**  Convert Ripped WAV music files to OGG
*/

int ConvertMusic(void)
{
	struct stat st;
	char buf[1024];
	char *cmd;
	int ret, i;
	int count = 0;

	for ( i = 0; MusicNames[i]; ++i ) {
		sprintf(buf, "%s/%s/%s.wav", Dir, MUSIC_PATH, MusicNames[i]);
		CheckPath(buf);

		if (stat(buf, &st))
			continue;

		cmd = (char*) calloc(strlen("ffmpeg2theora --optimize \"") + strlen(buf) + strlen("\" -o \"") + strlen(buf) + strlen("\"") + 1, 1);
		if (!cmd) {
			fprintf(stderr, "Memory error\n");
			exit(-1);
		}

		sprintf(cmd, "ffmpeg2theora --optimize \"%s\" -o \"%s/%s/%s.ogg\"", buf, Dir, MUSIC_PATH, MusicNames[i]);

		ret = system(cmd);

		free(cmd);
		remove(buf);

		if (ret != 0) {
			printf("Can't convert wav sound %s to ogv format. Is ffmpeg2theora installed in PATH?\n", MusicNames[i]);
			fflush(stdout);
		}

		++count;
	}

	if (count == 0)
		return 1;
	else
		return 0;
}

//----------------------------------------------------------------------------
//  Video
//----------------------------------------------------------------------------

/**
**  Convert SMK video to OGV
*/
int ConvertVideo(char* file, int video)
{
	unsigned char* vidp;
	char buf[1024];
	char* cmd;
	FILE* f;
	size_t l;
	int ret;

	vidp = ExtractEntry(ArchiveOffsets[video], &l);

	sprintf(buf,"%s/%s.smk", Dir, file);
	CheckPath(buf);
	f = fopen(buf, "wb");
	if (!f) {
		perror("");
		printf("Can't open %s\n", buf);
		exit(-1);
	}
	if (l != fwrite(vidp, 1, l, f)) {
		printf("Can't write %d bytes\n", (int)l);
		fflush(stdout);
	}

	free(vidp);
	fclose(f);

	cmd = (char*) calloc(strlen("ffmpeg2theora --optimize \"") + strlen(buf) + strlen("\" -o \"") + strlen(buf) + strlen("\"") + 1, 1);
	if (!cmd) {
		fprintf(stderr, "Memory error\n");
		exit(-1);
	}

	sprintf(cmd, "ffmpeg2theora --optimize \"%s/%s.smk\" -o \"%s/%s.ogv\"", Dir, file, Dir, file);

	ret = system(cmd);

	free(cmd);
	remove(buf);

	if (ret != 0) {
		printf("Can't convert video %s to ogv format. Is ffmpeg2theora installed in PATH?\n", file);
		fflush(stdout);
		return ret;
	}

	return 0;
}

//----------------------------------------------------------------------------
//  Text
//----------------------------------------------------------------------------

/**
**  Convert a string to utf8 format
**  Note: this isn't a true conversion, buf could be any character set
*/
unsigned char *ConvertString(unsigned char *buf, size_t len)
{
	unsigned char *str;
	unsigned char *p;
	size_t i;

	if (len == 0) {
		len = strlen((char *)buf);
	}

	str = (unsigned char *)malloc(2 * len + 1);
	p = str;

	for (i = 0; i < len; ++i, ++buf) {
		if (*buf > 0x7f) {
			*p++ = (0xc0 | (*buf >> 6));
			*p++ = (0x80 | (*buf & 0x1f));
		} else {
			*p++ = *buf;
		}
	}
	*p = '\0';
	return str;
}

/**
**  Convert text to my format.
*/
int ConvertText(char* file, int txte, int ofs)
{
	unsigned char* txtp;
	char buf[1024];
	gzFile gf;
	size_t l;
	size_t l2;
	unsigned char *str;

	// workaround for German/UK/Australian CD's
	if (!(CDType & CD_EXPANSION) && (CDType & (CD_GERMAN | CD_UK))) {
		--txte;
	}

	// workaround for MAC expansion CD
	if ((CDType & CD_MAC) && txte >= 99) {
		txte += 6;
	}

	txtp = ExtractEntry(ArchiveOffsets[txte], &l);

	sprintf(buf, "%s/%s/%s.txt.gz", Dir, TEXT_PATH, file);
	CheckPath(buf);
	gf = gzopen(buf, "wb9");
	if (!gf) {
		perror("");
		printf("Can't open %s\n", buf);
		exit(-1);
	}
	str = ConvertString(txtp + ofs, l - ofs);
	l2 = strlen((char *)str) + 1;

	if (l2 != (size_t)gzwrite(gf, str, l2)) {
		printf("Can't write %d bytes\n", (int)l2);
		fflush(stdout);
	}

	free(txtp);
	free(str);

	gzclose(gf);
	return 0;
}

/**
**  Names for localized versions.
*/
unsigned char Names[]={
0xDF,0x01,0xC0,0x03,0xC1,0x03,0xC9,0x03,0xCF,0x03,0xD7,0x03,0xDC,0x03,0xE5,0x03,
0xEE,0x03,0xF5,0x03,0xFA,0x03,0x07,0x04,0x18,0x04,0x1D,0x04,0x2A,0x04,0x32,0x04,
0x3C,0x04,0x55,0x04,0x64,0x04,0x73,0x04,0x7F,0x04,0x86,0x04,0x90,0x04,0x98,0x04,
0xA8,0x04,0xBC,0x04,0xC4,0x04,0xCC,0x04,0xDC,0x04,0xED,0x04,0xFC,0x04,0x0C,0x05,
0x1A,0x05,0x2A,0x05,0x3A,0x05,0x45,0x05,0x57,0x05,0x58,0x05,0x62,0x05,0x63,0x05,
0x64,0x05,0x76,0x05,0x83,0x05,0x9A,0x05,0xAA,0x05,0xB8,0x05,0xBF,0x05,0xC8,0x05,
0xD7,0x05,0xDE,0x05,0xF0,0x05,0xF1,0x05,0xFA,0x05,0x01,0x06,0x09,0x06,0x1C,0x06,
0x23,0x06,0x24,0x06,0x2D,0x06,0x34,0x06,0x3C,0x06,0x41,0x06,0x4A,0x06,0x59,0x06,
0x66,0x06,0x6D,0x06,0x7D,0x06,0x89,0x06,0x95,0x06,0x9D,0x06,0xA8,0x06,0xB9,0x06,
0xCA,0x06,0xD9,0x06,0xE6,0x06,0xF5,0x06,0x02,0x07,0x0C,0x07,0x17,0x07,0x29,0x07,
0x3B,0x07,0x49,0x07,0x55,0x07,0x60,0x07,0x75,0x07,0x86,0x07,0x95,0x07,0xA4,0x07,
0xB1,0x07,0xC4,0x07,0xD5,0x07,0xDA,0x07,0xE5,0x07,0xEC,0x07,0xF5,0x07,0xFF,0x07,
0x09,0x08,0x1E,0x08,0x31,0x08,0x43,0x08,0x53,0x08,0x66,0x08,0x77,0x08,0x87,0x08,
0x93,0x08,0x9D,0x08,0xA2,0x08,0xA3,0x08,0xBC,0x08,0xD5,0x08,0xEC,0x08,0x03,0x09,
0x1C,0x09,0x35,0x09,0x4E,0x09,0x67,0x09,0x87,0x09,0xA7,0x09,0xC5,0x09,0xE3,0x09,
0xFF,0x09,0x1B,0x0A,0x35,0x0A,0x4F,0x0A,0x6A,0x0A,0x85,0x0A,0x9E,0x0A,0xB7,0x0A,
0xD3,0x0A,0xEF,0x0A,0x0B,0x0B,0x27,0x0B,0x3D,0x0B,0x4E,0x0B,0x5E,0x0B,0x72,0x0B,
0x8B,0x0B,0xA1,0x0B,0xB4,0x0B,0xCB,0x0B,0xE7,0x0B,0x03,0x0C,0x17,0x0C,0x27,0x0C,
0x38,0x0C,0x4D,0x0C,0x5E,0x0C,0x6B,0x0C,0x80,0x0C,0x92,0x0C,0xA3,0x0C,0xBA,0x0C,
0xCC,0x0C,0xDF,0x0C,0xF2,0x0C,0x04,0x0D,0x12,0x0D,0x27,0x0D,0x35,0x0D,0x4D,0x0D,
0x4E,0x0D,0x5A,0x0D,0x62,0x0D,0x69,0x0D,0x72,0x0D,0x79,0x0D,0x82,0x0D,0x89,0x0D,
0x92,0x0D,0x99,0x0D,0xA2,0x0D,0xA9,0x0D,0xB2,0x0D,0xB9,0x0D,0xC2,0x0D,0xC9,0x0D,
0xD3,0x0D,0xDB,0x0D,0xE5,0x0D,0xED,0x0D,0xF7,0x0D,0xFF,0x0D,0x09,0x0E,0x11,0x0E,
0x24,0x0E,0x33,0x0E,0x3E,0x0E,0x49,0x0E,0x59,0x0E,0x6B,0x0E,0x7D,0x0E,0x8D,0x0E,
0x9D,0x0E,0xA9,0x0E,0xB5,0x0E,0xC2,0x0E,0xCE,0x0E,0xDB,0x0E,0xE8,0x0E,0xF5,0x0E,
0x02,0x0F,0x10,0x0F,0x1E,0x0F,0x31,0x0F,0x46,0x0F,0x5C,0x0F,0x72,0x0F,0x85,0x0F,
0x9B,0x0F,0xB0,0x0F,0xC3,0x0F,0xD8,0x0F,0xED,0x0F,0x03,0x10,0x19,0x10,0x2F,0x10,
0x44,0x10,0x59,0x10,0x6F,0x10,0x82,0x10,0x97,0x10,0xAD,0x10,0xC3,0x10,0xD8,0x10,
0xEB,0x10,0x00,0x11,0x15,0x11,0x2B,0x11,0x40,0x11,0x55,0x11,0x62,0x11,0x6F,0x11,
0x7C,0x11,0x88,0x11,0x94,0x11,0xA0,0x11,0xAC,0x11,0xB9,0x11,0xC6,0x11,0xD3,0x11,
0xE0,0x11,0xEB,0x11,0xF7,0x11,0x02,0x12,0x03,0x12,0x16,0x12,0x21,0x12,0x2C,0x12,
0x36,0x12,0x40,0x12,0x49,0x12,0x52,0x12,0x5B,0x12,0x64,0x12,0x6C,0x12,0x74,0x12,
0x7F,0x12,0x8A,0x12,0x93,0x12,0x9C,0x12,0xA9,0x12,0xB6,0x12,0xC1,0x12,0xD6,0x12,
0xE2,0x12,0xEE,0x12,0xFB,0x12,0x08,0x13,0x17,0x13,0x1B,0x13,0x2A,0x13,0x3C,0x13,
0x51,0x13,0x62,0x13,0x73,0x13,0x87,0x13,0x98,0x13,0xB2,0x13,0xC4,0x13,0xD8,0x13,
0xEF,0x13,0x00,0x14,0x0F,0x14,0x1F,0x14,0x31,0x14,0x44,0x14,0x57,0x14,0x6C,0x14,
0x80,0x14,0x94,0x14,0xA9,0x14,0xC0,0x14,0xDC,0x14,0xF3,0x14,0x0D,0x15,0x26,0x15,
0x3E,0x15,0x4F,0x15,0x72,0x15,0x8B,0x15,0x9E,0x15,0xB4,0x15,0xC6,0x15,0xDB,0x15,
0xEB,0x15,0xFC,0x15,0x16,0x16,0x25,0x16,0x3B,0x16,0x52,0x16,0x69,0x16,0x80,0x16,
0x92,0x16,0xA1,0x16,0xB4,0x16,0xC9,0x16,0xDF,0x16,0xF7,0x16,0x0B,0x17,0x25,0x17,
0x40,0x17,0x5C,0x17,0x77,0x17,0x92,0x17,0xAB,0x17,0xC1,0x17,0xD6,0x17,0xF5,0x17,
0x0D,0x18,0x25,0x18,0x44,0x18,0x6A,0x18,0x8A,0x18,0xA9,0x18,0xC8,0x18,0xE7,0x18,
0x07,0x19,0x29,0x19,0x40,0x19,0x5A,0x19,0x7A,0x19,0x98,0x19,0xBC,0x19,0xD9,0x19,
0xFE,0x19,0x1F,0x1A,0x3A,0x1A,0x5D,0x1A,0x80,0x1A,0x95,0x1A,0xA2,0x1A,0xB3,0x1A,
0xBC,0x1A,0xCD,0x1A,0xDB,0x1A,0xE8,0x1A,0xFE,0x1A,0x12,0x1B,0x21,0x1B,0x2B,0x1B,
0x3C,0x1B,0x46,0x1B,0x54,0x1B,0x64,0x1B,0x7B,0x1B,0x8F,0x1B,0x9C,0x1B,0xAF,0x1B,
0xBD,0x1B,0xCD,0x1B,0xDC,0x1B,0xFC,0x1B,0x17,0x1C,0x2D,0x1C,0x47,0x1C,0x59,0x1C,
0x73,0x1C,0x8A,0x1C,0xA0,0x1C,0xBD,0x1C,0xD5,0x1C,0xE8,0x1C,0x02,0x1D,0x15,0x1D,
0x2C,0x1D,0x45,0x1D,0x5A,0x1D,0x77,0x1D,0x8D,0x1D,0xA9,0x1D,0xC0,0x1D,0xD9,0x1D,
0xF1,0x1D,0xFA,0x1D,0x03,0x1E,0x0E,0x1E,0x19,0x1E,0x36,0x1E,0x43,0x1E,0x5D,0x1E,
0x7A,0x1E,0x8D,0x1E,0xA3,0x1E,0xB5,0x1E,0xC6,0x1E,0xD1,0x1E,0xE6,0x1E,0xF3,0x1E,
0x04,0x1F,0x22,0x1F,0x41,0x1F,0x5A,0x1F,0x60,0x1F,0x67,0x1F,0x6F,0x1F,0x76,0x1F,
0x7D,0x1F,0x84,0x1F,0x8B,0x1F,0x95,0x1F,0xA1,0x1F,0xA8,0x1F,0xAE,0x1F,0xB8,0x1F,
0xC3,0x1F,0xCD,0x1F,0xDA,0x1F,0xE5,0x1F,0xF1,0x1F,0xF7,0x1F,0xFF,0x1F,0x04,0x20,
0x0E,0x20,0x1D,0x20,0x29,0x20,0x34,0x20,0x53,0x20,0x79,0x20,0x93,0x20,0xB7,0x20,
0xD9,0x20,0xFE,0x20,0x1E,0x21,0x3C,0x21,0x54,0x21,0x7F,0x21,0x9D,0x21,0xD1,0x21,
0x02,0x22,0x2C,0x22,0x31,0x22,0x37,0x22,0x3D,0x22,0x42,0x22,0x49,0x22,0x50,0x22,
0x5C,0x22,0x68,0x22,0x74,0x22,0x7B,0x22,0x81,0x22,0x8E,0x22,0x9B,0x22,0xA1,0x22,
0xA9,0x22,0xB0,0x22,0xB6,0x22,0xBC,0x22,0xC3,0x22,0xCA,0x22,0xCF,0x22,0xD6,0x22,
0xDB,0x22,0xE1,0x22,0xEB,0x22,0xF1,0x22,0xFB,0x22,0x00,0x23,0x09,0x23,0x14,0x23,
0x00,0x46,0x6F,0x6F,0x74,0x6D,0x61,0x6E,0x00,0x47,0x72,0x75,0x6E,0x74,0x00,0x50,
0x65,0x61,0x73,0x61,0x6E,0x74,0x00,0x50,0x65,0x6F,0x6E,0x00,0x42,0x61,0x6C,0x6C,
0x69,0x73,0x74,0x61,0x00,0x43,0x61,0x74,0x61,0x70,0x75,0x6C,0x74,0x00,0x4B,0x6E,
0x69,0x67,0x68,0x74,0x00,0x4F,0x67,0x72,0x65,0x00,0x45,0x6C,0x76,0x65,0x6E,0x20,
0x41,0x72,0x63,0x68,0x65,0x72,0x00,0x54,0x72,0x6F,0x6C,0x6C,0x20,0x41,0x78,0x65,
0x74,0x68,0x72,0x6F,0x77,0x65,0x72,0x00,0x4D,0x61,0x67,0x65,0x00,0x44,0x65,0x61,
0x74,0x68,0x20,0x4B,0x6E,0x69,0x67,0x68,0x74,0x00,0x50,0x61,0x6C,0x61,0x64,0x69,
0x6E,0x00,0x4F,0x67,0x72,0x65,0x2D,0x4D,0x61,0x67,0x65,0x00,0x44,0x77,0x61,0x72,
0x76,0x65,0x6E,0x20,0x44,0x65,0x6D,0x6F,0x6C,0x69,0x74,0x69,0x6F,0x6E,0x20,0x53,
0x71,0x75,0x61,0x64,0x00,0x47,0x6F,0x62,0x6C,0x69,0x6E,0x20,0x53,0x61,0x70,0x70,
0x65,0x72,0x73,0x00,0x41,0x74,0x74,0x61,0x63,0x6B,0x20,0x50,0x65,0x61,0x73,0x61,
0x6E,0x74,0x00,0x41,0x74,0x74,0x61,0x63,0x6B,0x20,0x50,0x65,0x6F,0x6E,0x00,0x52,
0x61,0x6E,0x67,0x65,0x72,0x00,0x42,0x65,0x72,0x73,0x65,0x72,0x6B,0x65,0x72,0x00,
0x41,0x6C,0x6C,0x65,0x72,0x69,0x61,0x00,0x54,0x65,0x72,0x6F,0x6E,0x20,0x47,0x6F,
0x72,0x65,0x66,0x69,0x65,0x6E,0x64,0x00,0x4B,0x75,0x72,0x64,0x72,0x61,0x6E,0x20,
0x61,0x6E,0x64,0x20,0x53,0x6B,0x79,0x27,0x72,0x65,0x65,0x00,0x44,0x65,0x6E,0x74,
0x61,0x72,0x67,0x00,0x4B,0x68,0x61,0x64,0x67,0x61,0x72,0x00,0x47,0x72,0x6F,0x6D,
0x20,0x48,0x65,0x6C,0x6C,0x73,0x63,0x72,0x65,0x61,0x6D,0x00,0x48,0x75,0x6D,0x61,
0x6E,0x20,0x4F,0x69,0x6C,0x20,0x54,0x61,0x6E,0x6B,0x65,0x72,0x00,0x4F,0x72,0x63,
0x20,0x4F,0x69,0x6C,0x20,0x54,0x61,0x6E,0x6B,0x65,0x72,0x00,0x48,0x75,0x6D,0x61,
0x6E,0x20,0x54,0x72,0x61,0x6E,0x73,0x70,0x6F,0x72,0x74,0x00,0x4F,0x72,0x63,0x20,
0x54,0x72,0x61,0x6E,0x73,0x70,0x6F,0x72,0x74,0x00,0x45,0x6C,0x76,0x65,0x6E,0x20,
0x44,0x65,0x73,0x74,0x72,0x6F,0x79,0x65,0x72,0x00,0x54,0x72,0x6F,0x6C,0x6C,0x20,
0x44,0x65,0x73,0x74,0x72,0x6F,0x79,0x65,0x72,0x00,0x42,0x61,0x74,0x74,0x6C,0x65,
0x73,0x68,0x69,0x70,0x00,0x4F,0x67,0x72,0x65,0x20,0x4A,0x75,0x67,0x67,0x65,0x72,
0x6E,0x61,0x75,0x67,0x68,0x74,0x00,0x00,0x44,0x65,0x61,0x74,0x68,0x77,0x69,0x6E,
0x67,0x00,0x00,0x00,0x47,0x6E,0x6F,0x6D,0x69,0x73,0x68,0x20,0x53,0x75,0x62,0x6D,
0x61,0x72,0x69,0x6E,0x65,0x00,0x47,0x69,0x61,0x6E,0x74,0x20,0x54,0x75,0x72,0x74,
0x6C,0x65,0x00,0x47,0x6E,0x6F,0x6D,0x69,0x73,0x68,0x20,0x46,0x6C,0x79,0x69,0x6E,
0x67,0x20,0x4D,0x61,0x63,0x68,0x69,0x6E,0x65,0x00,0x47,0x6F,0x62,0x6C,0x69,0x6E,
0x20,0x5A,0x65,0x70,0x70,0x65,0x6C,0x69,0x6E,0x00,0x47,0x72,0x79,0x70,0x68,0x6F,
0x6E,0x20,0x52,0x69,0x64,0x65,0x72,0x00,0x44,0x72,0x61,0x67,0x6F,0x6E,0x00,0x54,
0x75,0x72,0x61,0x6C,0x79,0x6F,0x6E,0x00,0x45,0x79,0x65,0x20,0x6F,0x66,0x20,0x4B,
0x69,0x6C,0x72,0x6F,0x67,0x67,0x00,0x44,0x61,0x6E,0x61,0x74,0x68,0x00,0x4B,0x6F,
0x72,0x67,0x61,0x74,0x68,0x20,0x42,0x6C,0x61,0x64,0x65,0x66,0x69,0x73,0x74,0x00,
0x00,0x43,0x68,0x6F,0x27,0x67,0x61,0x6C,0x6C,0x00,0x4C,0x6F,0x74,0x68,0x61,0x72,
0x00,0x47,0x75,0x6C,0x27,0x64,0x61,0x6E,0x00,0x55,0x74,0x68,0x65,0x72,0x20,0x4C,
0x69,0x67,0x68,0x74,0x62,0x72,0x69,0x6E,0x67,0x65,0x72,0x00,0x5A,0x75,0x6C,0x6A,
0x69,0x6E,0x00,0x00,0x53,0x6B,0x65,0x6C,0x65,0x74,0x6F,0x6E,0x00,0x44,0x61,0x65,
0x6D,0x6F,0x6E,0x00,0x43,0x72,0x69,0x74,0x74,0x65,0x72,0x00,0x46,0x61,0x72,0x6D,
0x00,0x50,0x69,0x67,0x20,0x46,0x61,0x72,0x6D,0x00,0x48,0x75,0x6D,0x61,0x6E,0x20,
0x42,0x61,0x72,0x72,0x61,0x63,0x6B,0x73,0x00,0x4F,0x72,0x63,0x20,0x42,0x61,0x72,
0x72,0x61,0x63,0x6B,0x73,0x00,0x43,0x68,0x75,0x72,0x63,0x68,0x00,0x41,0x6C,0x74,
0x61,0x72,0x20,0x6F,0x66,0x20,0x53,0x74,0x6F,0x72,0x6D,0x73,0x00,0x53,0x63,0x6F,
0x75,0x74,0x20,0x54,0x6F,0x77,0x65,0x72,0x00,0x57,0x61,0x74,0x63,0x68,0x20,0x54,
0x6F,0x77,0x65,0x72,0x00,0x53,0x74,0x61,0x62,0x6C,0x65,0x73,0x00,0x4F,0x67,0x72,
0x65,0x20,0x4D,0x6F,0x75,0x6E,0x64,0x00,0x47,0x6E,0x6F,0x6D,0x69,0x73,0x68,0x20,
0x49,0x6E,0x76,0x65,0x6E,0x74,0x6F,0x72,0x00,0x47,0x6F,0x62,0x6C,0x69,0x6E,0x20,
0x41,0x6C,0x63,0x68,0x65,0x6D,0x69,0x73,0x74,0x00,0x47,0x72,0x79,0x70,0x68,0x6F,
0x6E,0x20,0x41,0x76,0x69,0x61,0x72,0x79,0x00,0x44,0x72,0x61,0x67,0x6F,0x6E,0x20,
0x52,0x6F,0x6F,0x73,0x74,0x00,0x48,0x75,0x6D,0x61,0x6E,0x20,0x53,0x68,0x69,0x70,
0x79,0x61,0x72,0x64,0x00,0x4F,0x72,0x63,0x20,0x53,0x68,0x69,0x70,0x79,0x61,0x72,
0x64,0x00,0x54,0x6F,0x77,0x6E,0x20,0x48,0x61,0x6C,0x6C,0x00,0x47,0x72,0x65,0x61,
0x74,0x20,0x48,0x61,0x6C,0x6C,0x00,0x45,0x6C,0x76,0x65,0x6E,0x20,0x4C,0x75,0x6D,
0x62,0x65,0x72,0x20,0x4D,0x69,0x6C,0x6C,0x00,0x54,0x72,0x6F,0x6C,0x6C,0x20,0x4C,
0x75,0x6D,0x62,0x65,0x72,0x20,0x4D,0x69,0x6C,0x6C,0x00,0x48,0x75,0x6D,0x61,0x6E,
0x20,0x46,0x6F,0x75,0x6E,0x64,0x72,0x79,0x00,0x4F,0x72,0x63,0x20,0x46,0x6F,0x75,
0x6E,0x64,0x72,0x79,0x00,0x4D,0x61,0x67,0x65,0x20,0x54,0x6F,0x77,0x65,0x72,0x00,
0x54,0x65,0x6D,0x70,0x6C,0x65,0x20,0x6F,0x66,0x20,0x74,0x68,0x65,0x20,0x44,0x61,
0x6D,0x6E,0x65,0x64,0x00,0x48,0x75,0x6D,0x61,0x6E,0x20,0x42,0x6C,0x61,0x63,0x6B,
0x73,0x6D,0x69,0x74,0x68,0x00,0x4F,0x72,0x63,0x20,0x42,0x6C,0x61,0x63,0x6B,0x73,
0x6D,0x69,0x74,0x68,0x00,0x48,0x75,0x6D,0x61,0x6E,0x20,0x52,0x65,0x66,0x69,0x6E,
0x65,0x72,0x79,0x00,0x4F,0x72,0x63,0x20,0x52,0x65,0x66,0x69,0x6E,0x65,0x72,0x79,
0x00,0x48,0x75,0x6D,0x61,0x6E,0x20,0x4F,0x69,0x6C,0x20,0x50,0x6C,0x61,0x74,0x66,
0x6F,0x72,0x6D,0x00,0x4F,0x72,0x63,0x20,0x4F,0x69,0x6C,0x20,0x50,0x6C,0x61,0x74,
0x66,0x6F,0x72,0x6D,0x00,0x4B,0x65,0x65,0x70,0x00,0x53,0x74,0x72,0x6F,0x6E,0x67,
0x68,0x6F,0x6C,0x64,0x00,0x43,0x61,0x73,0x74,0x6C,0x65,0x00,0x46,0x6F,0x72,0x74,
0x72,0x65,0x73,0x73,0x00,0x47,0x6F,0x6C,0x64,0x20,0x4D,0x69,0x6E,0x65,0x00,0x4F,
0x69,0x6C,0x20,0x50,0x61,0x74,0x63,0x68,0x00,0x48,0x75,0x6D,0x61,0x6E,0x20,0x53,
0x74,0x61,0x72,0x74,0x20,0x4C,0x6F,0x63,0x61,0x74,0x69,0x6F,0x6E,0x00,0x4F,0x72,
0x63,0x20,0x53,0x74,0x61,0x72,0x74,0x20,0x4C,0x6F,0x63,0x61,0x74,0x69,0x6F,0x6E,
0x00,0x48,0x75,0x6D,0x61,0x6E,0x20,0x47,0x75,0x61,0x72,0x64,0x20,0x54,0x6F,0x77,
0x65,0x72,0x00,0x4F,0x72,0x63,0x20,0x47,0x75,0x61,0x72,0x64,0x20,0x54,0x6F,0x77,
0x65,0x72,0x00,0x48,0x75,0x6D,0x61,0x6E,0x20,0x43,0x61,0x6E,0x6E,0x6F,0x6E,0x20,
0x54,0x6F,0x77,0x65,0x72,0x00,0x4F,0x72,0x63,0x20,0x43,0x61,0x6E,0x6E,0x6F,0x6E,
0x20,0x54,0x6F,0x77,0x65,0x72,0x00,0x43,0x69,0x72,0x63,0x6C,0x65,0x20,0x6F,0x66,
0x20,0x50,0x6F,0x77,0x65,0x72,0x00,0x44,0x61,0x72,0x6B,0x20,0x50,0x6F,0x72,0x74,
0x61,0x6C,0x00,0x52,0x75,0x6E,0x65,0x73,0x74,0x6F,0x6E,0x65,0x00,0x57,0x61,0x6C,
0x6C,0x00,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x53,0x77,0x6F,0x72,0x64,
0x0A,0x53,0x74,0x72,0x65,0x6E,0x67,0x74,0x68,0x20,0x31,0x00,0x55,0x70,0x67,0x72,
0x61,0x64,0x65,0x0A,0x53,0x77,0x6F,0x72,0x64,0x0A,0x53,0x74,0x72,0x65,0x6E,0x67,
0x74,0x68,0x20,0x32,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x41,0x78,0x65,
0x0A,0x53,0x74,0x72,0x65,0x6E,0x67,0x74,0x68,0x20,0x31,0x00,0x55,0x70,0x67,0x72,
0x61,0x64,0x65,0x0A,0x41,0x78,0x65,0x0A,0x53,0x74,0x72,0x65,0x6E,0x67,0x74,0x68,
0x20,0x32,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x41,0x72,0x72,0x6F,0x77,
0x0A,0x53,0x74,0x72,0x65,0x6E,0x67,0x74,0x68,0x20,0x31,0x00,0x55,0x70,0x67,0x72,
0x61,0x64,0x65,0x0A,0x41,0x72,0x72,0x6F,0x77,0x0A,0x53,0x74,0x72,0x65,0x6E,0x67,
0x74,0x68,0x20,0x32,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x53,0x70,0x65,
0x61,0x72,0x0A,0x53,0x74,0x72,0x65,0x6E,0x67,0x74,0x68,0x20,0x31,0x00,0x55,0x70,
0x67,0x72,0x61,0x64,0x65,0x0A,0x53,0x70,0x65,0x61,0x72,0x0A,0x53,0x74,0x72,0x65,
0x6E,0x67,0x74,0x68,0x20,0x32,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x48,
0x75,0x6D,0x61,0x6E,0x20,0x53,0x68,0x69,0x65,0x6C,0x64,0x0A,0x53,0x74,0x72,0x65,
0x6E,0x67,0x74,0x68,0x20,0x31,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x48,
0x75,0x6D,0x61,0x6E,0x20,0x53,0x68,0x69,0x65,0x6C,0x64,0x0A,0x53,0x74,0x72,0x65,
0x6E,0x67,0x74,0x68,0x20,0x32,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x4F,
0x72,0x63,0x20,0x53,0x68,0x69,0x65,0x6C,0x64,0x0A,0x53,0x74,0x72,0x65,0x6E,0x67,
0x74,0x68,0x20,0x31,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x4F,0x72,0x63,
0x20,0x53,0x68,0x69,0x65,0x6C,0x64,0x0A,0x53,0x74,0x72,0x65,0x6E,0x67,0x74,0x68,
0x20,0x32,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x48,0x75,0x6D,0x61,0x6E,
0x20,0x53,0x68,0x69,0x70,0x0A,0x41,0x74,0x74,0x61,0x63,0x6B,0x20,0x31,0x00,0x55,
0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x48,0x75,0x6D,0x61,0x6E,0x20,0x53,0x68,0x69,
0x70,0x0A,0x41,0x74,0x74,0x61,0x63,0x6B,0x20,0x32,0x00,0x55,0x70,0x67,0x72,0x61,
0x64,0x65,0x0A,0x4F,0x72,0x63,0x20,0x53,0x68,0x69,0x70,0x0A,0x41,0x74,0x74,0x61,
0x63,0x6B,0x20,0x31,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x4F,0x72,0x63,
0x20,0x53,0x68,0x69,0x70,0x0A,0x41,0x74,0x74,0x61,0x63,0x6B,0x20,0x32,0x00,0x55,
0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x48,0x75,0x6D,0x61,0x6E,0x20,0x53,0x68,0x69,
0x70,0x0A,0x41,0x72,0x6D,0x6F,0x72,0x20,0x31,0x00,0x55,0x70,0x67,0x72,0x61,0x64,
0x65,0x0A,0x48,0x75,0x6D,0x61,0x6E,0x20,0x53,0x68,0x69,0x70,0x0A,0x41,0x72,0x6D,
0x6F,0x72,0x20,0x32,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x4F,0x72,0x63,
0x20,0x53,0x68,0x69,0x70,0x0A,0x41,0x72,0x6D,0x6F,0x72,0x20,0x31,0x00,0x55,0x70,
0x67,0x72,0x61,0x64,0x65,0x0A,0x4F,0x72,0x63,0x20,0x53,0x68,0x69,0x70,0x0A,0x41,
0x72,0x6D,0x6F,0x72,0x20,0x32,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x43,
0x61,0x74,0x61,0x70,0x75,0x6C,0x74,0x20,0x53,0x74,0x72,0x65,0x6E,0x67,0x74,0x68,
0x20,0x31,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x43,0x61,0x74,0x61,0x70,
0x75,0x6C,0x74,0x20,0x53,0x74,0x72,0x65,0x6E,0x67,0x74,0x68,0x20,0x32,0x00,0x55,
0x70,0x67,0x72,0x61,0x64,0x65,0x0A,0x42,0x61,0x6C,0x6C,0x69,0x73,0x74,0x61,0x20,
0x53,0x74,0x72,0x65,0x6E,0x67,0x74,0x68,0x20,0x31,0x00,0x55,0x70,0x67,0x72,0x61,
0x64,0x65,0x0A,0x42,0x61,0x6C,0x6C,0x69,0x73,0x74,0x61,0x20,0x53,0x74,0x72,0x65,
0x6E,0x67,0x74,0x68,0x20,0x32,0x00,0x45,0x6C,0x76,0x65,0x6E,0x20,0x52,0x61,0x6E,
0x67,0x65,0x72,0x20,0x54,0x72,0x61,0x69,0x6E,0x69,0x6E,0x67,0x00,0x52,0x65,0x73,
0x65,0x61,0x72,0x63,0x68,0x20,0x4C,0x6F,0x6E,0x67,0x62,0x6F,0x77,0x00,0x52,0x61,
0x6E,0x67,0x65,0x72,0x20,0x53,0x63,0x6F,0x75,0x74,0x69,0x6E,0x67,0x00,0x52,0x61,
0x6E,0x67,0x65,0x72,0x20,0x4D,0x61,0x72,0x6B,0x73,0x6D,0x61,0x6E,0x73,0x68,0x69,
0x70,0x00,0x54,0x72,0x6F,0x6C,0x6C,0x20,0x42,0x65,0x72,0x73,0x65,0x72,0x6B,0x65,
0x72,0x20,0x54,0x72,0x61,0x69,0x6E,0x69,0x6E,0x67,0x00,0x52,0x65,0x73,0x65,0x61,
0x72,0x63,0x68,0x20,0x4C,0x69,0x67,0x68,0x74,0x65,0x72,0x20,0x41,0x78,0x65,0x73,
0x00,0x42,0x65,0x72,0x73,0x65,0x72,0x6B,0x65,0x72,0x20,0x53,0x63,0x6F,0x75,0x74,
0x69,0x6E,0x67,0x00,0x42,0x65,0x72,0x73,0x65,0x72,0x6B,0x65,0x72,0x20,0x52,0x65,
0x67,0x65,0x6E,0x65,0x72,0x61,0x74,0x69,0x6F,0x6E,0x00,0x55,0x70,0x67,0x72,0x61,
0x64,0x65,0x20,0x4F,0x67,0x72,0x65,0x73,0x20,0x74,0x6F,0x20,0x4F,0x67,0x72,0x65,
0x2D,0x4D,0x61,0x67,0x65,0x73,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x65,0x20,0x4B,
0x6E,0x69,0x67,0x68,0x74,0x73,0x20,0x74,0x6F,0x20,0x50,0x61,0x6C,0x61,0x64,0x69,
0x6E,0x73,0x00,0x53,0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x48,0x6F,0x6C,0x79,0x20,
0x56,0x69,0x73,0x69,0x6F,0x6E,0x00,0x53,0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x48,
0x65,0x61,0x6C,0x69,0x6E,0x67,0x00,0x53,0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x45,
0x78,0x6F,0x72,0x63,0x69,0x73,0x6D,0x00,0x53,0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,
0x46,0x6C,0x61,0x6D,0x65,0x20,0x53,0x68,0x69,0x65,0x6C,0x64,0x00,0x53,0x70,0x65,
0x6C,0x6C,0x20,0x2D,0x0A,0x46,0x69,0x72,0x65,0x62,0x61,0x6C,0x6C,0x00,0x53,0x70,
0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x53,0x6C,0x6F,0x77,0x00,0x53,0x70,0x65,0x6C,0x6C,
0x20,0x2D,0x0A,0x49,0x6E,0x76,0x69,0x73,0x69,0x62,0x69,0x6C,0x69,0x74,0x79,0x00,
0x53,0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x50,0x6F,0x6C,0x79,0x6D,0x6F,0x72,0x70,
0x68,0x00,0x53,0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x42,0x6C,0x69,0x7A,0x7A,0x61,
0x72,0x64,0x00,0x53,0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x45,0x79,0x65,0x20,0x6F,
0x66,0x20,0x4B,0x69,0x6C,0x72,0x6F,0x67,0x67,0x00,0x53,0x70,0x65,0x6C,0x6C,0x20,
0x2D,0x0A,0x42,0x6C,0x6F,0x6F,0x64,0x6C,0x75,0x73,0x74,0x00,0x53,0x70,0x65,0x6C,
0x6C,0x20,0x2D,0x0A,0x52,0x61,0x69,0x73,0x65,0x20,0x44,0x65,0x61,0x64,0x00,0x53,
0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x44,0x65,0x61,0x74,0x68,0x20,0x43,0x6F,0x69,
0x6C,0x00,0x53,0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x57,0x68,0x69,0x72,0x6C,0x77,
0x69,0x6E,0x64,0x00,0x53,0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x48,0x61,0x73,0x74,
0x65,0x00,0x53,0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x55,0x6E,0x68,0x6F,0x6C,0x79,
0x20,0x41,0x72,0x6D,0x6F,0x72,0x00,0x53,0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x52,
0x75,0x6E,0x65,0x73,0x00,0x53,0x70,0x65,0x6C,0x6C,0x20,0x2D,0x0A,0x44,0x65,0x61,
0x74,0x68,0x20,0x61,0x6E,0x64,0x20,0x44,0x65,0x63,0x61,0x79,0x00,0x00,0x4C,0x61,
0x6E,0x64,0x20,0x41,0x74,0x74,0x61,0x63,0x6B,0x00,0x50,0x61,0x73,0x73,0x69,0x76,
0x65,0x00,0x5F,0x4F,0x72,0x63,0x20,0x33,0x00,0x5F,0x48,0x75,0x6D,0x61,0x6E,0x20,
0x34,0x00,0x5F,0x4F,0x72,0x63,0x20,0x34,0x00,0x5F,0x48,0x75,0x6D,0x61,0x6E,0x20,
0x35,0x00,0x5F,0x4F,0x72,0x63,0x20,0x35,0x00,0x5F,0x48,0x75,0x6D,0x61,0x6E,0x20,
0x36,0x00,0x5F,0x4F,0x72,0x63,0x20,0x36,0x00,0x5F,0x48,0x75,0x6D,0x61,0x6E,0x20,
0x37,0x00,0x5F,0x4F,0x72,0x63,0x20,0x37,0x00,0x5F,0x48,0x75,0x6D,0x61,0x6E,0x20,
0x38,0x00,0x5F,0x4F,0x72,0x63,0x20,0x38,0x00,0x5F,0x48,0x75,0x6D,0x61,0x6E,0x20,
0x39,0x00,0x5F,0x4F,0x72,0x63,0x20,0x39,0x00,0x5F,0x48,0x75,0x6D,0x61,0x6E,0x20,
0x31,0x30,0x00,0x5F,0x4F,0x72,0x63,0x20,0x31,0x30,0x00,0x5F,0x48,0x75,0x6D,0x61,
0x6E,0x20,0x31,0x31,0x00,0x5F,0x4F,0x72,0x63,0x20,0x31,0x31,0x00,0x5F,0x48,0x75,
0x6D,0x61,0x6E,0x20,0x31,0x32,0x00,0x5F,0x4F,0x72,0x63,0x20,0x31,0x32,0x00,0x5F,
0x48,0x75,0x6D,0x61,0x6E,0x20,0x31,0x33,0x00,0x5F,0x4F,0x72,0x63,0x20,0x31,0x33,
0x00,0x5F,0x48,0x75,0x6D,0x61,0x6E,0x20,0x31,0x34,0x20,0x28,0x4F,0x72,0x61,0x6E,
0x67,0x65,0x29,0x00,0x5F,0x4F,0x72,0x63,0x20,0x31,0x34,0x20,0x28,0x42,0x6C,0x75,
0x65,0x29,0x00,0x53,0x65,0x61,0x20,0x41,0x74,0x74,0x61,0x63,0x6B,0x00,0x41,0x69,
0x72,0x20,0x41,0x74,0x74,0x61,0x63,0x6B,0x00,0x5F,0x48,0x75,0x6D,0x61,0x6E,0x20,
0x31,0x34,0x20,0x28,0x52,0x65,0x64,0x29,0x00,0x5F,0x48,0x75,0x6D,0x61,0x6E,0x20,
0x31,0x34,0x20,0x28,0x57,0x68,0x69,0x74,0x65,0x29,0x00,0x5F,0x48,0x75,0x6D,0x61,
0x6E,0x20,0x31,0x34,0x20,0x28,0x42,0x6C,0x61,0x63,0x6B,0x29,0x00,0x5F,0x4F,0x72,
0x63,0x20,0x31,0x34,0x20,0x28,0x47,0x72,0x65,0x65,0x6E,0x29,0x00,0x5F,0x4F,0x72,
0x63,0x20,0x31,0x34,0x20,0x28,0x57,0x68,0x69,0x74,0x65,0x29,0x00,0x5F,0x4F,0x72,
0x63,0x20,0x45,0x78,0x70,0x2E,0x20,0x34,0x00,0x5F,0x4F,0x72,0x63,0x20,0x45,0x78,
0x70,0x2E,0x20,0x35,0x00,0x5F,0x4F,0x72,0x63,0x20,0x45,0x78,0x70,0x2E,0x20,0x37,
0x61,0x00,0x5F,0x4F,0x72,0x63,0x20,0x45,0x78,0x70,0x2E,0x20,0x39,0x00,0x5F,0x4F,
0x72,0x63,0x20,0x45,0x78,0x70,0x2E,0x20,0x31,0x30,0x00,0x5F,0x4F,0x72,0x63,0x20,
0x45,0x78,0x70,0x2E,0x20,0x31,0x32,0x00,0x5F,0x4F,0x72,0x63,0x20,0x45,0x78,0x70,
0x2E,0x20,0x36,0x61,0x00,0x5F,0x4F,0x72,0x63,0x20,0x45,0x78,0x70,0x2E,0x20,0x36,
0x62,0x00,0x5F,0x4F,0x72,0x63,0x20,0x45,0x78,0x70,0x2E,0x20,0x31,0x31,0x61,0x00,
0x5F,0x4F,0x72,0x63,0x20,0x45,0x78,0x70,0x2E,0x20,0x31,0x31,0x62,0x00,0x5F,0x48,
0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x32,0x61,0x20,0x28,0x52,0x65,0x64,0x29,
0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x32,0x62,0x20,0x28,0x42,
0x6C,0x61,0x63,0x6B,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,
0x32,0x63,0x20,0x28,0x59,0x65,0x6C,0x6C,0x6F,0x77,0x29,0x00,0x5F,0x48,0x75,0x6D,
0x20,0x45,0x78,0x70,0x2E,0x20,0x33,0x61,0x20,0x28,0x4F,0x72,0x61,0x6E,0x67,0x65,
0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x33,0x62,0x20,0x28,
0x52,0x65,0x64,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x33,
0x63,0x20,0x28,0x56,0x69,0x6F,0x6C,0x65,0x74,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,
0x45,0x78,0x70,0x2E,0x20,0x34,0x61,0x20,0x28,0x42,0x6C,0x61,0x63,0x6B,0x29,0x00,
0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x34,0x62,0x20,0x28,0x52,0x65,
0x64,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x34,0x63,0x20,
0x28,0x57,0x68,0x69,0x74,0x65,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,
0x2E,0x20,0x35,0x61,0x20,0x28,0x47,0x72,0x65,0x65,0x6E,0x29,0x00,0x5F,0x48,0x75,
0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x35,0x62,0x20,0x28,0x4F,0x72,0x61,0x6E,0x67,
0x65,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x35,0x63,0x20,
0x28,0x56,0x69,0x6F,0x6C,0x65,0x74,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,
0x70,0x2E,0x20,0x35,0x64,0x20,0x28,0x59,0x65,0x6C,0x6C,0x6F,0x77,0x29,0x00,0x5F,
0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x36,0x61,0x20,0x28,0x47,0x72,0x65,
0x65,0x6E,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x36,0x62,
0x20,0x28,0x42,0x6C,0x61,0x63,0x6B,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,
0x70,0x2E,0x20,0x36,0x63,0x20,0x28,0x4F,0x72,0x61,0x6E,0x67,0x65,0x29,0x00,0x5F,
0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x36,0x64,0x20,0x28,0x52,0x65,0x64,
0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x38,0x61,0x20,0x28,
0x57,0x68,0x69,0x74,0x65,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,
0x20,0x38,0x62,0x20,0x28,0x59,0x65,0x6C,0x6C,0x6F,0x77,0x29,0x00,0x5F,0x48,0x75,
0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x38,0x63,0x20,0x28,0x56,0x69,0x6F,0x6C,0x65,
0x74,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x39,0x61,0x20,
0x28,0x42,0x6C,0x61,0x63,0x6B,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,
0x2E,0x20,0x39,0x62,0x20,0x28,0x52,0x65,0x64,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,
0x45,0x78,0x70,0x2E,0x20,0x39,0x63,0x20,0x28,0x47,0x72,0x65,0x65,0x6E,0x29,0x00,
0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x20,0x39,0x64,0x20,0x28,0x57,0x68,
0x69,0x74,0x65,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x31,0x30,
0x61,0x20,0x28,0x56,0x69,0x6F,0x6C,0x65,0x74,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,
0x45,0x78,0x70,0x2E,0x31,0x30,0x62,0x20,0x28,0x47,0x72,0x65,0x65,0x6E,0x29,0x00,
0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x31,0x30,0x63,0x20,0x28,0x42,0x6C,
0x61,0x63,0x6B,0x29,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x31,0x31,
0x61,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x31,0x31,0x62,0x00,0x5F,
0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x31,0x32,0x61,0x00,0x5F,0x4F,0x72,0x63,
0x20,0x45,0x78,0x70,0x2E,0x35,0x62,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,
0x2E,0x37,0x61,0x00,0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x37,0x62,0x00,
0x5F,0x48,0x75,0x6D,0x20,0x45,0x78,0x70,0x2E,0x37,0x63,0x00,0x5F,0x4F,0x72,0x63,
0x20,0x45,0x78,0x70,0x2E,0x31,0x32,0x61,0x00,0x5F,0x4F,0x72,0x63,0x20,0x45,0x78,
0x70,0x2E,0x31,0x32,0x62,0x00,0x5F,0x4F,0x72,0x63,0x20,0x45,0x78,0x70,0x2E,0x31,
0x32,0x63,0x00,0x5F,0x4F,0x72,0x63,0x20,0x45,0x78,0x70,0x2E,0x31,0x32,0x64,0x00,
0x5F,0x4F,0x72,0x63,0x20,0x45,0x78,0x70,0x2E,0x32,0x00,0x5F,0x4F,0x72,0x63,0x20,
0x45,0x78,0x70,0x2E,0x37,0x62,0x00,0x5F,0x4F,0x72,0x63,0x20,0x45,0x78,0x70,0x2E,
0x33,0x00,0x00,0x44,0x77,0x61,0x72,0x76,0x65,0x6E,0x20,0x44,0x65,0x6D,0x6F,0x20,
0x53,0x71,0x75,0x61,0x64,0x00,0x4F,0x69,0x6C,0x20,0x54,0x61,0x6E,0x6B,0x65,0x72,
0x00,0x4F,0x69,0x6C,0x20,0x54,0x61,0x6E,0x6B,0x65,0x72,0x00,0x54,0x72,0x61,0x6E,
0x73,0x70,0x6F,0x72,0x74,0x00,0x54,0x72,0x61,0x6E,0x73,0x70,0x6F,0x72,0x74,0x00,
0x42,0x61,0x72,0x72,0x61,0x63,0x6B,0x73,0x00,0x42,0x61,0x72,0x72,0x61,0x63,0x6B,
0x73,0x00,0x53,0x68,0x69,0x70,0x79,0x61,0x72,0x64,0x00,0x53,0x68,0x69,0x70,0x79,
0x61,0x72,0x64,0x00,0x46,0x6F,0x75,0x6E,0x64,0x72,0x79,0x00,0x46,0x6F,0x75,0x6E,
0x64,0x72,0x79,0x00,0x42,0x6C,0x61,0x63,0x6B,0x73,0x6D,0x69,0x74,0x68,0x00,0x42,
0x6C,0x61,0x63,0x6B,0x73,0x6D,0x69,0x74,0x68,0x00,0x52,0x65,0x66,0x69,0x6E,0x65,
0x72,0x79,0x00,0x52,0x65,0x66,0x69,0x6E,0x65,0x72,0x79,0x00,0x4F,0x69,0x6C,0x20,
0x50,0x6C,0x61,0x74,0x66,0x6F,0x72,0x6D,0x00,0x4F,0x69,0x6C,0x20,0x50,0x6C,0x61,
0x74,0x66,0x6F,0x72,0x6D,0x00,0x4D,0x61,0x67,0x65,0x20,0x54,0x6F,0x77,0x65,0x72,
0x00,0x54,0x65,0x6D,0x70,0x6C,0x65,0x20,0x6F,0x66,0x20,0x74,0x68,0x65,0x20,0x44,
0x61,0x6D,0x6E,0x65,0x64,0x00,0x47,0x75,0x61,0x72,0x64,0x20,0x54,0x6F,0x77,0x65,
0x72,0x00,0x47,0x75,0x61,0x72,0x64,0x20,0x54,0x6F,0x77,0x65,0x72,0x00,0x43,0x61,
0x6E,0x6E,0x6F,0x6E,0x20,0x54,0x6F,0x77,0x65,0x72,0x00,0x43,0x61,0x6E,0x6E,0x6F,
0x6E,0x20,0x54,0x6F,0x77,0x65,0x72,0x00,0x45,0x79,0x65,0x20,0x6F,0x66,0x20,0x4B,
0x69,0x6C,0x72,0x6F,0x67,0x67,0x00,0x45,0x53,0x43,0x00,0x70,0x01,0x54,0x52,0x41,
0x49,0x4E,0x20,0x04,0x50,0x01,0x45,0x4F,0x4E,0x00,0x70,0x01,0x54,0x52,0x41,0x49,
0x4E,0x20,0x04,0x50,0x01,0x45,0x41,0x53,0x41,0x4E,0x54,0x00,0x61,0x01,0x54,0x52,
0x41,0x49,0x4E,0x20,0x04,0x41,0x01,0x58,0x45,0x54,0x48,0x52,0x4F,0x57,0x45,0x52,
0x00,0x61,0x01,0x54,0x52,0x41,0x49,0x4E,0x20,0x04,0x41,0x01,0x52,0x43,0x48,0x45,
0x52,0x00,0x61,0x01,0x54,0x52,0x41,0x49,0x4E,0x20,0x52,0x04,0x41,0x01,0x4E,0x47,
0x45,0x52,0x00,0x61,0x01,0x54,0x52,0x04,0x41,0x01,0x49,0x4E,0x20,0x42,0x45,0x52,
0x53,0x45,0x52,0x4B,0x45,0x52,0x00,0x6B,0x01,0x54,0x52,0x41,0x49,0x4E,0x20,0x04,
0x4B,0x01,0x4E,0x49,0x47,0x48,0x54,0x00,0x6F,0x01,0x54,0x52,0x41,0x49,0x4E,0x20,
0x54,0x57,0x4F,0x2D,0x48,0x45,0x41,0x44,0x45,0x44,0x20,0x04,0x4F,0x01,0x47,0x52,
0x45,0x00,0x70,0x01,0x54,0x52,0x41,0x49,0x4E,0x20,0x04,0x50,0x01,0x41,0x4C,0x41,
0x44,0x49,0x4E,0x00,0x6F,0x01,0x54,0x52,0x41,0x49,0x4E,0x20,0x04,0x4F,0x01,0x47,
0x52,0x45,0x2D,0x4D,0x41,0x47,0x45,0x00,0x74,0x01,0x04,0x54,0x01,0x52,0x41,0x49,
0x4E,0x20,0x44,0x45,0x41,0x54,0x48,0x20,0x4B,0x4E,0x49,0x47,0x48,0x54,0x00,0x74,
0x01,0x04,0x54,0x01,0x52,0x41,0x49,0x4E,0x20,0x43,0x4C,0x45,0x52,0x49,0x43,0x00,
0x74,0x01,0x04,0x54,0x01,0x52,0x41,0x49,0x4E,0x20,0x4D,0x41,0x47,0x45,0x00,0x67,
0x01,0x54,0x52,0x41,0x49,0x4E,0x20,0x04,0x47,0x01,0x52,0x55,0x4E,0x54,0x00,0x66,
0x01,0x54,0x52,0x41,0x49,0x4E,0x20,0x04,0x46,0x01,0x4F,0x4F,0x54,0x4D,0x41,0x4E,
0x00,0x62,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x42,0x01,0x41,0x4C,0x4C,0x49,
0x53,0x54,0x41,0x00,0x63,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x43,0x01,0x41,
0x54,0x41,0x50,0x55,0x4C,0x54,0x00,0x6F,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,
0x4F,0x01,0x49,0x4C,0x20,0x54,0x41,0x4E,0x4B,0x45,0x52,0x00,0x64,0x01,0x42,0x55,
0x49,0x4C,0x44,0x20,0x04,0x44,0x01,0x45,0x53,0x54,0x52,0x4F,0x59,0x45,0x52,0x00,
0x74,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x54,0x01,0x52,0x41,0x4E,0x53,0x50,
0x4F,0x52,0x54,0x00,0x62,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x42,0x01,0x41,
0x54,0x54,0x4C,0x45,0x53,0x48,0x49,0x50,0x00,0x6A,0x01,0x42,0x55,0x49,0x4C,0x44,
0x20,0x04,0x4A,0x01,0x55,0x47,0x47,0x45,0x52,0x4E,0x41,0x55,0x47,0x48,0x54,0x00,
0x73,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x47,0x4E,0x4F,0x4D,0x49,0x53,0x48,0x20,
0x04,0x53,0x01,0x55,0x42,0x4D,0x41,0x52,0x49,0x4E,0x45,0x00,0x67,0x01,0x54,0x52,
0x41,0x49,0x4E,0x20,0x04,0x47,0x01,0x49,0x41,0x4E,0x54,0x20,0x54,0x55,0x52,0x54,
0x4C,0x45,0x00,0x7A,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x47,0x4F,0x42,0x4C,0x49,
0x4E,0x20,0x04,0x5A,0x01,0x45,0x50,0x50,0x45,0x4C,0x49,0x4E,0x00,0x66,0x01,0x42,
0x55,0x49,0x4C,0x44,0x20,0x04,0x46,0x01,0x4C,0x59,0x49,0x4E,0x47,0x20,0x4D,0x41,
0x43,0x48,0x49,0x4E,0x45,0x00,0x67,0x01,0x54,0x52,0x41,0x49,0x4E,0x20,0x04,0x47,
0x01,0x52,0x59,0x50,0x48,0x4F,0x4E,0x20,0x52,0x49,0x44,0x45,0x52,0x00,0x64,0x01,
0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x44,0x01,0x52,0x41,0x47,0x4F,0x4E,0x00,0x64,
0x01,0x54,0x52,0x41,0x49,0x4E,0x20,0x04,0x44,0x01,0x57,0x41,0x52,0x56,0x45,0x4E,
0x20,0x44,0x45,0x4D,0x4F,0x4C,0x49,0x54,0x49,0x4F,0x4E,0x20,0x53,0x51,0x55,0x41,
0x44,0x00,0x73,0x01,0x54,0x52,0x41,0x49,0x4E,0x20,0x47,0x4F,0x42,0x4C,0x49,0x4E,
0x20,0x04,0x53,0x01,0x41,0x50,0x50,0x45,0x52,0x53,0x00,0x62,0x01,0x42,0x55,0x49,
0x4C,0x44,0x20,0x04,0x42,0x01,0x41,0x52,0x52,0x41,0x43,0x4B,0x53,0x00,0x6C,0x01,
0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x4C,0x01,0x55,0x4D,0x42,0x45,0x52,0x20,0x4D,
0x49,0x4C,0x4C,0x00,0x61,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x53,0x54,0x04,0x41,
0x01,0x42,0x4C,0x45,0x53,0x00,0x6F,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x4F,
0x01,0x47,0x52,0x45,0x20,0x4D,0x4F,0x55,0x4E,0x44,0x00,0x74,0x01,0x42,0x55,0x49,
0x4C,0x44,0x20,0x04,0x54,0x01,0x4F,0x57,0x45,0x52,0x00,0x63,0x01,0x42,0x55,0x49,
0x4C,0x44,0x20,0x04,0x43,0x01,0x48,0x55,0x52,0x43,0x48,0x00,0x6C,0x01,0x42,0x55,
0x49,0x4C,0x44,0x20,0x41,0x04,0x4C,0x01,0x54,0x41,0x52,0x20,0x4F,0x46,0x20,0x53,
0x54,0x4F,0x52,0x4D,0x53,0x00,0x66,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x46,
0x01,0x41,0x52,0x4D,0x00,0x68,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x41,0x20,0x54,
0x4F,0x57,0x4E,0x20,0x04,0x48,0x01,0x41,0x4C,0x4C,0x00,0x68,0x01,0x42,0x55,0x49,
0x4C,0x44,0x20,0x41,0x20,0x47,0x52,0x45,0x41,0x54,0x20,0x04,0x48,0x01,0x41,0x4C,
0x4C,0x00,0x62,0x01,0x04,0x42,0x01,0x55,0x49,0x4C,0x44,0x20,0x4F,0x49,0x4C,0x20,
0x50,0x4C,0x41,0x54,0x46,0x4F,0x52,0x4D,0x00,0x72,0x01,0x42,0x55,0x49,0x4C,0x44,
0x20,0x4F,0x49,0x4C,0x20,0x04,0x52,0x01,0x45,0x46,0x49,0x4E,0x45,0x52,0x59,0x00,
0x66,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x46,0x01,0x4F,0x55,0x4E,0x44,0x52,
0x59,0x00,0x77,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x57,0x01,0x41,0x4C,0x4C,
0x00,0x73,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x53,0x01,0x48,0x49,0x50,0x59,
0x41,0x52,0x44,0x00,0x73,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x42,0x4C,0x41,0x43,
0x4B,0x04,0x53,0x01,0x4D,0x49,0x54,0x48,0x00,0x63,0x01,0x55,0x50,0x47,0x52,0x41,
0x44,0x45,0x20,0x54,0x4F,0x20,0x04,0x43,0x01,0x41,0x53,0x54,0x4C,0x45,0x00,0x66,
0x01,0x55,0x50,0x47,0x52,0x41,0x44,0x45,0x20,0x54,0x4F,0x20,0x04,0x46,0x01,0x4F,
0x52,0x54,0x52,0x45,0x53,0x53,0x00,0x6B,0x01,0x55,0x50,0x47,0x52,0x41,0x44,0x45,
0x20,0x54,0x4F,0x20,0x04,0x4B,0x01,0x45,0x45,0x50,0x00,0x73,0x01,0x55,0x50,0x47,
0x52,0x41,0x44,0x45,0x20,0x54,0x4F,0x20,0x04,0x53,0x01,0x54,0x52,0x4F,0x4E,0x47,
0x48,0x4F,0x4C,0x44,0x00,0x67,0x01,0x55,0x50,0x47,0x52,0x41,0x44,0x45,0x20,0x54,
0x4F,0x20,0x04,0x47,0x01,0x55,0x41,0x52,0x44,0x20,0x54,0x4F,0x57,0x45,0x52,0x00,
0x63,0x01,0x55,0x50,0x47,0x52,0x41,0x44,0x45,0x20,0x54,0x4F,0x20,0x04,0x43,0x01,
0x41,0x4E,0x4E,0x4F,0x4E,0x20,0x54,0x4F,0x57,0x45,0x52,0x00,0x69,0x01,0x42,0x55,
0x49,0x4C,0x44,0x20,0x47,0x4E,0x4F,0x4D,0x49,0x53,0x48,0x20,0x04,0x49,0x01,0x4E,
0x56,0x45,0x4E,0x54,0x4F,0x52,0x00,0x61,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x47,
0x4F,0x42,0x4C,0x49,0x4E,0x20,0x04,0x41,0x01,0x4C,0x43,0x48,0x45,0x4D,0x49,0x53,
0x54,0x00,0x67,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x47,0x01,0x52,0x59,0x50,
0x48,0x4F,0x4E,0x20,0x41,0x56,0x49,0x41,0x52,0x59,0x00,0x64,0x01,0x4D,0x41,0x4B,
0x45,0x20,0x04,0x44,0x01,0x52,0x41,0x47,0x4F,0x4E,0x20,0x52,0x4F,0x4F,0x53,0x54,
0x00,0x6D,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x4D,0x01,0x41,0x47,0x45,0x20,
0x54,0x4F,0x57,0x45,0x52,0x00,0x74,0x01,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x54,
0x01,0x45,0x4D,0x50,0x4C,0x45,0x20,0x4F,0x46,0x20,0x54,0x48,0x45,0x20,0x44,0x41,
0x4D,0x4E,0x45,0x44,0x00,0x62,0x02,0x04,0x42,0x01,0x52,0x45,0x45,0x44,0x20,0x46,
0x41,0x53,0x54,0x45,0x52,0x20,0x48,0x4F,0x52,0x53,0x45,0x53,0x00,0x62,0x02,0x04,
0x42,0x01,0x52,0x45,0x45,0x44,0x20,0x46,0x41,0x53,0x54,0x45,0x52,0x20,0x57,0x4F,
0x4C,0x56,0x45,0x53,0x00,0x77,0x02,0x55,0x50,0x47,0x52,0x41,0x44,0x45,0x20,0x53,
0x04,0x57,0x01,0x4F,0x52,0x44,0x53,0x20,0x28,0x44,0x61,0x6D,0x61,0x67,0x65,0x20,
0x2B,0x32,0x29,0x00,0x75,0x02,0x04,0x55,0x01,0x50,0x47,0x52,0x41,0x44,0x45,0x20,
0x54,0x48,0x52,0x4F,0x57,0x49,0x4E,0x47,0x20,0x41,0x58,0x45,0x53,0x20,0x28,0x44,
0x61,0x6D,0x61,0x67,0x65,0x20,0x2B,0x31,0x29,0x00,0x77,0x02,0x55,0x50,0x47,0x52,
0x41,0x44,0x45,0x20,0x04,0x57,0x01,0x45,0x41,0x50,0x4F,0x4E,0x53,0x20,0x28,0x44,
0x61,0x6D,0x61,0x67,0x65,0x20,0x2B,0x32,0x29,0x00,0x75,0x02,0x04,0x55,0x01,0x50,
0x47,0x52,0x41,0x44,0x45,0x20,0x41,0x52,0x52,0x4F,0x57,0x53,0x20,0x28,0x44,0x61,
0x6D,0x61,0x67,0x65,0x20,0x2B,0x31,0x29,0x00,0x68,0x02,0x55,0x50,0x47,0x52,0x41,
0x44,0x45,0x20,0x53,0x04,0x48,0x01,0x49,0x45,0x4C,0x44,0x53,0x20,0x28,0x41,0x72,
0x6D,0x6F,0x72,0x20,0x2B,0x32,0x29,0x00,0x68,0x02,0x55,0x50,0x47,0x52,0x41,0x44,
0x45,0x20,0x53,0x04,0x48,0x01,0x49,0x45,0x4C,0x44,0x53,0x20,0x28,0x41,0x72,0x6D,
0x6F,0x72,0x20,0x2B,0x32,0x29,0x00,0x63,0x02,0x55,0x50,0x47,0x52,0x41,0x44,0x45,
0x20,0x04,0x43,0x01,0x41,0x4E,0x4E,0x4F,0x4E,0x53,0x20,0x28,0x44,0x61,0x6D,0x61,
0x67,0x65,0x20,0x2B,0x35,0x29,0x00,0x61,0x02,0x55,0x50,0x47,0x52,0x41,0x44,0x45,
0x20,0x53,0x48,0x49,0x50,0x20,0x04,0x41,0x01,0x52,0x4D,0x4F,0x52,0x20,0x28,0x41,
0x72,0x6D,0x6F,0x72,0x20,0x2B,0x35,0x29,0x00,0x73,0x02,0x55,0x50,0x47,0x52,0x41,
0x44,0x45,0x20,0x53,0x48,0x49,0x50,0x20,0x04,0x53,0x01,0x50,0x45,0x45,0x44,0x00,
0x72,0x02,0x45,0x4C,0x56,0x45,0x4E,0x20,0x04,0x52,0x01,0x41,0x4E,0x47,0x45,0x52,
0x20,0x54,0x52,0x41,0x49,0x4E,0x49,0x4E,0x47,0x00,0x6C,0x02,0x52,0x45,0x53,0x45,
0x41,0x52,0x43,0x48,0x20,0x04,0x4C,0x01,0x4F,0x4E,0x47,0x42,0x4F,0x57,0x20,0x28,
0x52,0x61,0x6E,0x67,0x65,0x20,0x2B,0x31,0x29,0x00,0x73,0x02,0x52,0x41,0x4E,0x47,
0x45,0x52,0x20,0x04,0x53,0x01,0x43,0x4F,0x55,0x54,0x49,0x4E,0x47,0x20,0x28,0x53,
0x69,0x67,0x68,0x74,0x3A,0x39,0x29,0x00,0x6D,0x02,0x52,0x41,0x4E,0x47,0x45,0x52,
0x20,0x04,0x4D,0x01,0x41,0x52,0x4B,0x53,0x4D,0x41,0x4E,0x53,0x48,0x49,0x50,0x20,
0x28,0x44,0x61,0x6D,0x61,0x67,0x65,0x20,0x2B,0x33,0x29,0x00,0x62,0x02,0x54,0x52,
0x4F,0x4C,0x4C,0x20,0x04,0x42,0x01,0x45,0x52,0x53,0x45,0x52,0x4B,0x45,0x52,0x20,
0x54,0x52,0x41,0x49,0x4E,0x49,0x4E,0x47,0x00,0x61,0x02,0x52,0x45,0x53,0x45,0x41,
0x52,0x43,0x48,0x20,0x4C,0x49,0x47,0x48,0x54,0x45,0x52,0x20,0x04,0x41,0x01,0x58,
0x45,0x53,0x20,0x28,0x52,0x61,0x6E,0x67,0x65,0x20,0x2B,0x31,0x29,0x00,0x73,0x02,
0x42,0x45,0x52,0x53,0x45,0x52,0x4B,0x45,0x52,0x20,0x04,0x53,0x01,0x43,0x4F,0x55,
0x54,0x49,0x4E,0x47,0x20,0x28,0x53,0x69,0x67,0x68,0x74,0x3A,0x39,0x29,0x00,0x72,
0x02,0x42,0x45,0x52,0x53,0x45,0x52,0x4B,0x45,0x52,0x20,0x04,0x52,0x01,0x45,0x47,
0x45,0x4E,0x45,0x52,0x41,0x54,0x49,0x4F,0x4E,0x00,0x63,0x02,0x55,0x50,0x47,0x52,
0x41,0x44,0x45,0x20,0x04,0x43,0x01,0x41,0x54,0x41,0x50,0x55,0x4C,0x54,0x53,0x20,
0x28,0x44,0x61,0x6D,0x61,0x67,0x65,0x20,0x2B,0x31,0x35,0x29,0x00,0x62,0x02,0x55,
0x50,0x47,0x52,0x41,0x44,0x45,0x20,0x04,0x42,0x01,0x41,0x4C,0x4C,0x49,0x53,0x54,
0x41,0x53,0x20,0x28,0x44,0x61,0x6D,0x61,0x67,0x65,0x20,0x2B,0x31,0x35,0x29,0x00,
0x61,0x00,0x4C,0x49,0x47,0x48,0x54,0x4E,0x49,0x4E,0x47,0x20,0x04,0x41,0x01,0x54,
0x54,0x41,0x43,0x4B,0x00,0x66,0x03,0x04,0x46,0x01,0x49,0x52,0x45,0x42,0x41,0x4C,
0x4C,0x00,0x6C,0x03,0x46,0x04,0x4C,0x01,0x41,0x4D,0x45,0x20,0x53,0x48,0x49,0x45,
0x4C,0x44,0x00,0x6F,0x03,0x53,0x4C,0x04,0x4F,0x01,0x57,0x00,0x69,0x03,0x04,0x49,
0x01,0x4E,0x56,0x49,0x53,0x49,0x42,0x49,0x4C,0x49,0x54,0x59,0x00,0x70,0x03,0x04,
0x50,0x01,0x4F,0x4C,0x59,0x4D,0x4F,0x52,0x50,0x48,0x00,0x62,0x03,0x04,0x42,0x01,
0x4C,0x49,0x5A,0x5A,0x41,0x52,0x44,0x00,0x61,0x00,0x54,0x4F,0x55,0x43,0x48,0x20,
0x4F,0x46,0x20,0x44,0x04,0x41,0x01,0x52,0x4B,0x4E,0x45,0x53,0x53,0x00,0x64,0x03,
0x04,0x44,0x01,0x45,0x41,0x54,0x48,0x20,0x41,0x4E,0x44,0x20,0x44,0x45,0x43,0x41,
0x59,0x00,0x63,0x03,0x44,0x45,0x41,0x54,0x48,0x20,0x04,0x43,0x01,0x4F,0x49,0x4C,
0x00,0x68,0x03,0x04,0x48,0x01,0x41,0x53,0x54,0x45,0x00,0x75,0x03,0x04,0x55,0x01,
0x4E,0x48,0x4F,0x4C,0x59,0x20,0x41,0x52,0x4D,0x4F,0x52,0x00,0x72,0x03,0x04,0x52,
0x01,0x55,0x4E,0x45,0x53,0x00,0x77,0x03,0x04,0x57,0x01,0x48,0x49,0x52,0x4C,0x57,
0x49,0x4E,0x44,0x00,0x76,0x03,0x48,0x4F,0x4C,0x59,0x20,0x04,0x56,0x01,0x49,0x53,
0x49,0x4F,0x4E,0x00,0x68,0x03,0x04,0x48,0x01,0x45,0x41,0x4C,0x49,0x4E,0x47,0x20,
0x28,0x70,0x65,0x72,0x20,0x31,0x20,0x48,0x50,0x29,0x00,0x67,0x03,0x04,0x47,0x01,
0x52,0x45,0x41,0x54,0x45,0x52,0x20,0x48,0x45,0x41,0x4C,0x49,0x4E,0x47,0x00,0x65,
0x03,0x04,0x45,0x01,0x58,0x4F,0x52,0x43,0x49,0x53,0x4D,0x00,0x65,0x03,0x04,0x45,
0x01,0x59,0x45,0x20,0x4F,0x46,0x20,0x4B,0x49,0x4C,0x52,0x4F,0x47,0x47,0x00,0x62,
0x03,0x04,0x42,0x01,0x4C,0x4F,0x4F,0x44,0x4C,0x55,0x53,0x54,0x00,0x68,0x03,0x04,
0x48,0x01,0x41,0x4C,0x4C,0x55,0x43,0x49,0x4E,0x41,0x54,0x45,0x00,0x72,0x03,0x04,
0x52,0x01,0x41,0x49,0x53,0x45,0x20,0x44,0x45,0x41,0x44,0x00,0x70,0x02,0x55,0x50,
0x47,0x52,0x41,0x44,0x45,0x20,0x4B,0x4E,0x49,0x47,0x48,0x54,0x53,0x20,0x54,0x4F,
0x20,0x04,0x50,0x01,0x41,0x4C,0x41,0x44,0x49,0x4E,0x53,0x00,0x6D,0x02,0x55,0x50,
0x47,0x52,0x41,0x44,0x45,0x20,0x4F,0x47,0x52,0x45,0x53,0x20,0x54,0x4F,0x20,0x04,
0x4D,0x01,0x41,0x47,0x45,0x53,0x00,0x66,0x02,0x52,0x45,0x53,0x45,0x41,0x52,0x43,
0x48,0x20,0x04,0x46,0x01,0x49,0x52,0x45,0x42,0x41,0x4C,0x4C,0x00,0x6C,0x02,0x52,
0x45,0x53,0x45,0x41,0x52,0x43,0x48,0x20,0x46,0x04,0x4C,0x01,0x41,0x4D,0x45,0x20,
0x53,0x48,0x49,0x45,0x4C,0x44,0x00,0x6F,0x02,0x52,0x45,0x53,0x45,0x41,0x52,0x43,
0x48,0x20,0x53,0x4C,0x04,0x4F,0x01,0x57,0x00,0x69,0x02,0x52,0x45,0x53,0x45,0x41,
0x52,0x43,0x48,0x20,0x04,0x49,0x01,0x4E,0x56,0x49,0x53,0x49,0x42,0x49,0x4C,0x49,
0x54,0x59,0x00,0x70,0x02,0x52,0x45,0x53,0x45,0x41,0x52,0x43,0x48,0x20,0x04,0x50,
0x01,0x4F,0x4C,0x59,0x4D,0x4F,0x52,0x50,0x48,0x00,0x62,0x02,0x52,0x45,0x53,0x45,
0x41,0x52,0x43,0x48,0x20,0x04,0x42,0x01,0x4C,0x49,0x5A,0x5A,0x41,0x52,0x44,0x00,
0x64,0x02,0x52,0x45,0x53,0x45,0x41,0x52,0x43,0x48,0x20,0x04,0x44,0x01,0x45,0x41,
0x54,0x48,0x20,0x41,0x4E,0x44,0x20,0x44,0x45,0x43,0x41,0x59,0x00,0x63,0x02,0x52,
0x45,0x53,0x45,0x41,0x52,0x43,0x48,0x20,0x44,0x45,0x41,0x54,0x48,0x20,0x04,0x43,
0x01,0x4F,0x49,0x4C,0x00,0x68,0x02,0x52,0x45,0x53,0x45,0x41,0x52,0x43,0x48,0x20,
0x04,0x48,0x01,0x41,0x53,0x54,0x45,0x00,0x75,0x02,0x52,0x45,0x53,0x45,0x41,0x52,
0x43,0x48,0x20,0x04,0x55,0x01,0x4E,0x48,0x4F,0x4C,0x59,0x20,0x41,0x52,0x4D,0x4F,
0x52,0x00,0x72,0x02,0x52,0x45,0x53,0x45,0x41,0x52,0x43,0x48,0x20,0x04,0x52,0x01,
0x55,0x4E,0x45,0x53,0x00,0x77,0x02,0x52,0x45,0x53,0x45,0x41,0x52,0x43,0x48,0x20,
0x04,0x57,0x01,0x48,0x49,0x52,0x4C,0x57,0x49,0x4E,0x44,0x00,0x76,0x02,0x52,0x45,
0x53,0x45,0x41,0x52,0x43,0x48,0x20,0x48,0x4F,0x4C,0x59,0x20,0x04,0x56,0x01,0x49,
0x53,0x49,0x4F,0x4E,0x00,0x68,0x02,0x52,0x45,0x53,0x45,0x41,0x52,0x43,0x48,0x20,
0x04,0x48,0x01,0x45,0x41,0x4C,0x49,0x4E,0x47,0x00,0x67,0x02,0x52,0x45,0x53,0x45,
0x41,0x52,0x43,0x48,0x20,0x04,0x47,0x01,0x52,0x45,0x41,0x54,0x45,0x52,0x20,0x48,
0x45,0x41,0x4C,0x49,0x4E,0x47,0x00,0x65,0x02,0x52,0x45,0x53,0x45,0x41,0x52,0x43,
0x48,0x20,0x04,0x45,0x01,0x58,0x4F,0x52,0x43,0x49,0x53,0x4D,0x00,0x65,0x02,0x52,
0x45,0x53,0x45,0x41,0x52,0x43,0x48,0x20,0x04,0x45,0x01,0x59,0x45,0x20,0x4F,0x46,
0x20,0x4B,0x49,0x4C,0x52,0x4F,0x47,0x47,0x00,0x62,0x02,0x52,0x45,0x53,0x45,0x41,
0x52,0x43,0x48,0x20,0x04,0x42,0x01,0x4C,0x4F,0x4F,0x44,0x4C,0x55,0x53,0x54,0x00,
0x68,0x02,0x52,0x45,0x53,0x45,0x41,0x52,0x43,0x48,0x20,0x04,0x48,0x01,0x41,0x4C,
0x4C,0x55,0x43,0x49,0x4E,0x41,0x54,0x45,0x00,0x72,0x02,0x52,0x45,0x53,0x45,0x41,
0x52,0x43,0x48,0x20,0x04,0x52,0x01,0x41,0x49,0x53,0x45,0x20,0x44,0x45,0x41,0x44,
0x00,0x6D,0x00,0x04,0x4D,0x01,0x4F,0x56,0x45,0x00,0x73,0x00,0x04,0x53,0x01,0x54,
0x4F,0x50,0x00,0x61,0x00,0x04,0x41,0x01,0x54,0x54,0x41,0x43,0x4B,0x00,0x72,0x00,
0x04,0x52,0x01,0x45,0x50,0x41,0x49,0x52,0x00,0x68,0x00,0x04,0x48,0x01,0x41,0x52,
0x56,0x45,0x53,0x54,0x20,0x4C,0x55,0x4D,0x42,0x45,0x52,0x2F,0x4D,0x49,0x4E,0x45,
0x20,0x47,0x4F,0x4C,0x44,0x00,0x68,0x00,0x04,0x48,0x01,0x41,0x55,0x4C,0x20,0x4F,
0x49,0x4C,0x00,0x62,0x00,0x42,0x55,0x49,0x4C,0x44,0x20,0x04,0x42,0x01,0x41,0x53,
0x49,0x43,0x20,0x53,0x54,0x52,0x55,0x43,0x54,0x55,0x52,0x45,0x00,0x76,0x00,0x42,
0x55,0x49,0x4C,0x44,0x20,0x41,0x44,0x04,0x56,0x01,0x41,0x4E,0x43,0x45,0x44,0x20,
0x53,0x54,0x52,0x55,0x43,0x54,0x55,0x52,0x45,0x00,0x6F,0x00,0x53,0x45,0x41,0x52,
0x43,0x48,0x20,0x46,0x4F,0x52,0x20,0x04,0x4F,0x01,0x49,0x4C,0x00,0x67,0x00,0x52,
0x45,0x54,0x55,0x52,0x4E,0x20,0x57,0x49,0x54,0x48,0x20,0x04,0x47,0x01,0x4F,0x4F,
0x44,0x53,0x00,0x67,0x00,0x41,0x54,0x54,0x41,0x43,0x4B,0x20,0x04,0x47,0x01,0x52,
0x4F,0x55,0x4E,0x44,0x00,0x74,0x00,0x53,0x04,0x54,0x01,0x41,0x4E,0x44,0x20,0x47,
0x52,0x4F,0x55,0x4E,0x44,0x00,0x70,0x00,0x04,0x50,0x01,0x41,0x54,0x52,0x4F,0x4C,
0x00,0x75,0x00,0x04,0x55,0x01,0x4E,0x4C,0x4F,0x41,0x44,0x20,0x54,0x52,0x41,0x4E,
0x53,0x50,0x4F,0x52,0x54,0x00,0x64,0x00,0x04,0x44,0x01,0x45,0x4D,0x4F,0x4C,0x49,
0x53,0x48,0x00,0x10,0x00,0x04,0x45,0x53,0x43,0x01,0x20,0x2D,0x20,0x43,0x41,0x4E,
0x43,0x45,0x4C,0x00,0x10,0x00,0x04,0x45,0x53,0x43,0x01,0x20,0x2D,0x20,0x43,0x41,
0x4E,0x43,0x45,0x4C,0x20,0x43,0x4F,0x4E,0x53,0x54,0x52,0x55,0x43,0x54,0x49,0x4F,
0x4E,0x00,0x10,0x00,0x04,0x45,0x53,0x43,0x01,0x20,0x2D,0x20,0x43,0x41,0x4E,0x43,
0x45,0x4C,0x20,0x55,0x4E,0x49,0x54,0x20,0x54,0x52,0x41,0x49,0x4E,0x49,0x4E,0x47,
0x00,0x10,0x00,0x04,0x45,0x53,0x43,0x01,0x20,0x2D,0x20,0x43,0x41,0x4E,0x43,0x45,
0x4C,0x20,0x55,0x50,0x47,0x52,0x41,0x44,0x45,0x00,0x4C,0x65,0x76,0x65,0x6C,0x00,
0x41,0x72,0x6D,0x6F,0x72,0x3A,0x00,0x44,0x61,0x6D,0x61,0x67,0x65,0x3A,0x00,0x52,
0x61,0x6E,0x67,0x65,0x3A,0x00,0x53,0x69,0x67,0x68,0x74,0x3A,0x00,0x53,0x70,0x65,
0x65,0x64,0x3A,0x00,0x4D,0x61,0x67,0x69,0x63,0x3A,0x00,0x4F,0x69,0x6C,0x20,0x4C,
0x65,0x66,0x74,0x3A,0x00,0x46,0x6F,0x6F,0x64,0x20,0x55,0x73,0x61,0x67,0x65,0x20,
0x00,0x47,0x72,0x6F,0x77,0x6E,0x3A,0x00,0x55,0x73,0x65,0x64,0x3A,0x00,0x54,0x72,
0x61,0x69,0x6E,0x69,0x6E,0x67,0x3A,0x00,0x55,0x70,0x67,0x72,0x61,0x64,0x69,0x6E,
0x67,0x3A,0x00,0x42,0x75,0x69,0x6C,0x64,0x69,0x6E,0x67,0x3A,0x00,0x52,0x65,0x73,
0x65,0x61,0x72,0x63,0x68,0x69,0x6E,0x67,0x3A,0x00,0x47,0x6F,0x6C,0x64,0x20,0x4C,
0x65,0x66,0x74,0x3A,0x00,0x50,0x72,0x6F,0x64,0x75,0x63,0x74,0x69,0x6F,0x6E,0x20,
0x00,0x47,0x6F,0x6C,0x64,0x3A,0x00,0x4C,0x75,0x6D,0x62,0x65,0x72,0x3A,0x00,0x4F,
0x69,0x6C,0x3A,0x00,0x55,0x6E,0x69,0x74,0x20,0x50,0x74,0x72,0x3A,0x00,0x43,0x75,
0x72,0x72,0x65,0x6E,0x74,0x20,0x4F,0x72,0x64,0x65,0x72,0x3A,0x00,0x4E,0x65,0x78,
0x74,0x20,0x4F,0x72,0x64,0x65,0x72,0x3A,0x00,0x25,0x20,0x43,0x6F,0x6D,0x70,0x6C,
0x65,0x74,0x65,0x00,0x4E,0x6F,0x74,0x20,0x65,0x6E,0x6F,0x75,0x67,0x68,0x20,0x6D,
0x61,0x6E,0x61,0x20,0x74,0x6F,0x20,0x63,0x61,0x73,0x74,0x20,0x73,0x70,0x65,0x6C,
0x6C,0x2E,0x00,0x4E,0x6F,0x77,0x68,0x65,0x72,0x65,0x20,0x74,0x6F,0x20,0x72,0x65,
0x74,0x75,0x72,0x6E,0x20,0x74,0x6F,0x2E,0x2E,0x2E,0x63,0x61,0x6E,0x6E,0x6F,0x74,
0x20,0x72,0x65,0x74,0x75,0x72,0x6E,0x2E,0x00,0x43,0x61,0x6E,0x6E,0x6F,0x74,0x20,
0x63,0x61,0x73,0x74,0x20,0x6F,0x6E,0x20,0x62,0x75,0x69,0x6C,0x64,0x69,0x6E,0x67,
0x73,0x2E,0x00,0x4E,0x6F,0x74,0x20,0x65,0x6E,0x6F,0x75,0x67,0x68,0x20,0x66,0x6F,
0x6F,0x64,0x2E,0x2E,0x2E,0x62,0x75,0x69,0x6C,0x64,0x20,0x6D,0x6F,0x72,0x65,0x20,
0x66,0x61,0x72,0x6D,0x73,0x2E,0x00,0x4E,0x6F,0x74,0x20,0x65,0x6E,0x6F,0x75,0x67,
0x68,0x20,0x67,0x6F,0x6C,0x64,0x2E,0x2E,0x2E,0x6D,0x69,0x6E,0x65,0x20,0x6D,0x6F,
0x72,0x65,0x20,0x67,0x6F,0x6C,0x64,0x2E,0x00,0x4E,0x6F,0x74,0x20,0x65,0x6E,0x6F,
0x75,0x67,0x68,0x20,0x6C,0x75,0x6D,0x62,0x65,0x72,0x2E,0x2E,0x2E,0x63,0x68,0x6F,
0x70,0x20,0x6D,0x6F,0x72,0x65,0x20,0x74,0x72,0x65,0x65,0x73,0x2E,0x00,0x4E,0x6F,
0x74,0x20,0x65,0x6E,0x6F,0x75,0x67,0x68,0x20,0x6F,0x69,0x6C,0x2E,0x2E,0x2E,0x64,
0x72,0x69,0x6C,0x6C,0x20,0x66,0x6F,0x72,0x20,0x6F,0x69,0x6C,0x2E,0x00,0x59,0x6F,
0x75,0x20,0x63,0x61,0x6E,0x6E,0x6F,0x74,0x20,0x62,0x75,0x69,0x6C,0x64,0x20,0x6F,
0x66,0x66,0x20,0x74,0x68,0x65,0x20,0x6D,0x61,0x70,0x2E,0x00,0x59,0x6F,0x75,0x20,
0x63,0x61,0x6E,0x6E,0x6F,0x74,0x20,0x62,0x75,0x69,0x6C,0x64,0x20,0x74,0x68,0x65,
0x72,0x65,0x2E,0x00,0x59,0x6F,0x75,0x20,0x6D,0x75,0x73,0x74,0x20,0x62,0x75,0x69,
0x6C,0x64,0x20,0x74,0x68,0x69,0x73,0x20,0x62,0x75,0x69,0x6C,0x64,0x69,0x6E,0x67,
0x20,0x6F,0x6E,0x20,0x74,0x68,0x65,0x20,0x63,0x6F,0x61,0x73,0x74,0x2E,0x00,0x59,
0x6F,0x75,0x20,0x6D,0x75,0x73,0x74,0x20,0x65,0x78,0x70,0x6C,0x6F,0x72,0x65,0x20,
0x74,0x68,0x65,0x72,0x65,0x20,0x66,0x69,0x72,0x73,0x74,0x2E,0x00,0x59,0x6F,0x75,
0x20,0x6D,0x75,0x73,0x74,0x20,0x62,0x75,0x69,0x6C,0x64,0x20,0x61,0x6E,0x20,0x6F,
0x69,0x6C,0x20,0x70,0x6C,0x61,0x74,0x66,0x6F,0x72,0x6D,0x20,0x6F,0x76,0x65,0x72,
0x20,0x61,0x20,0x70,0x61,0x74,0x63,0x68,0x20,0x6F,0x66,0x20,0x6F,0x69,0x6C,0x2E,
0x00,0x59,0x6F,0x75,0x20,0x63,0x61,0x6E,0x6E,0x6F,0x74,0x20,0x62,0x75,0x69,0x6C,
0x64,0x20,0x61,0x20,0x74,0x6F,0x77,0x6E,0x68,0x61,0x6C,0x6C,0x20,0x74,0x6F,0x6F,
0x20,0x6E,0x65,0x61,0x72,0x20,0x61,0x20,0x67,0x6F,0x6C,0x64,0x6D,0x69,0x6E,0x65,
0x2E,0x00,0x59,0x6F,0x75,0x20,0x63,0x61,0x6E,0x6E,0x6F,0x74,0x20,0x62,0x75,0x69,
0x6C,0x64,0x20,0x74,0x6F,0x6F,0x20,0x6E,0x65,0x61,0x72,0x20,0x61,0x20,0x70,0x61,
0x74,0x63,0x68,0x20,0x6F,0x66,0x20,0x6F,0x69,0x6C,0x2E,0x00,0x44,0x65,0x61,0x64,
0x00,0x44,0x79,0x69,0x6E,0x67,0x00,0x47,0x75,0x61,0x72,0x64,0x00,0x4D,0x6F,0x76,
0x65,0x00,0x50,0x61,0x74,0x72,0x6F,0x6C,0x00,0x41,0x74,0x74,0x61,0x63,0x6B,0x00,
0x41,0x74,0x74,0x61,0x63,0x6B,0x20,0x54,0x61,0x72,0x67,0x00,0x41,0x74,0x74,0x61,
0x63,0x6B,0x20,0x41,0x72,0x65,0x61,0x00,0x41,0x74,0x74,0x61,0x63,0x6B,0x20,0x57,
0x61,0x6C,0x6C,0x00,0x44,0x65,0x66,0x65,0x6E,0x64,0x00,0x53,0x74,0x61,0x6E,0x64,
0x00,0x53,0x74,0x61,0x6E,0x64,0x20,0x41,0x74,0x74,0x61,0x63,0x6B,0x00,0x41,0x74,
0x74,0x61,0x63,0x6B,0x20,0x50,0x6C,0x61,0x63,0x65,0x00,0x42,0x75,0x69,0x6C,0x64,
0x00,0x48,0x61,0x72,0x76,0x65,0x73,0x74,0x00,0x52,0x65,0x74,0x75,0x72,0x6E,0x00,
0x45,0x6E,0x74,0x65,0x72,0x00,0x4C,0x65,0x61,0x76,0x65,0x00,0x52,0x65,0x70,0x61,
0x69,0x72,0x00,0x43,0x72,0x65,0x61,0x74,0x65,0x00,0x44,0x6F,0x63,0x6B,0x00,0x4C,
0x61,0x75,0x6E,0x63,0x68,0x00,0x57,0x61,0x69,0x74,0x00,0x42,0x6F,0x61,0x72,0x64,
0x00,0x44,0x69,0x73,0x65,0x6D,0x62,0x61,0x72,0x6B,0x00,0x54,0x72,0x61,0x69,0x6E,
0x00,0x52,0x65,0x73,0x75,0x72,0x72,0x65,0x63,0x74,0x00,0x57,0x61,0x69,0x74,0x00,
0x4D,0x6F,0x76,0x65,0x55,0x6E,0x69,0x74,0x00,0x50,0x61,0x74,0x72,0x6F,0x6C,0x55,
0x6E,0x69,0x74,0x00,0x4E,0x6F,0x6E,0x65,0x00,
};

/**
**  Convert text to my format.
*/
int SetupNames(char* file __attribute__((unused)), int txte __attribute__((unused)))
{
	unsigned char* txtp;
	const unsigned short* mp;
	size_t l;
	unsigned u;
	unsigned n;

	//txtp = ExtractEntry(ArchiveOffsets[txte], &l);
	txtp = Names;
	l = sizeof(Names);
	mp = (const unsigned short*)txtp;

	n = ConvertLE16(mp[0]);
	for (u = 1; u < n; ++u) {
//		printf("%d %x ", u, ConvertLE16(mp[u]));
//		printf("%s\n", txtp + ConvertLE16(mp[u]));
		if (u < sizeof(UnitNames) / sizeof(*UnitNames)) {
			UnitNames[u] = strdup((char*)txtp + ConvertLE16(mp[u]));
			UnitNamesLast = u;
		}
	}

	if (txtp != Names) {
		free(txtp);
	}
	return 0;
}

/**
**  Parse string.
*/
char* ParseString(char* input)
{
	static char buf[1024];
	char* dp;
	char* sp;
	char* tp;
	int i;
	int f;

//	printf("%s -> ", input);

	for (sp = input, dp = buf; *sp;) {
		if (*sp == '%') {
			f = 0;
			if (*++sp == '-') {
				f = 1;
				++sp;
			}
			i = strtol(sp, &sp, 0);
			tp = UnitNames[i];
			if (f) {
				tp = strchr(tp, ' ') + 1;
			}
			while (*tp) {  // make them readabler
				if (*tp == '-') {
					*dp++ = '_';
					++tp;
				} else if (*tp == ' ') {
					*dp++ = '_';
					++tp;
				} else {
					*dp++ = tolower(*tp++);
				}
			}
			continue;
		}
		*dp++ = *sp++;
	}
	*dp = '\0';

//	printf("%s\n", buf);
	return buf;
}

//----------------------------------------------------------------------------
//  Import the campaigns
//----------------------------------------------------------------------------

/**
**  FIXME: docu
*/
int CampaignsCreate(char* file __attribute__((unused)), int txte, int ofs)
{
	unsigned char* objectives;
	char buf[1024];
	unsigned char* CampaignData[2][26][10];
	unsigned char* current;
	unsigned char* next;
	unsigned char* nextobj;
	unsigned char* currentobj;
	FILE* outlevel;
	size_t l;
	int levelno;
	int noobjs;
	int race;
	int expansion;

	// Campaign data is in different spots for different CD's
	if (CDType & CD_EXPANSION) {
		expansion = 1;
		ofs = 236;
		txte = 54;
	} else {
		expansion = 0;
		// 53 for UK and German CD, else 54
		if (CDType & (CD_UK | CD_GERMAN)) {
			txte = 53;
		} else {
			txte = 54;
		}
		// 172 for Spanish CD, 140 for anything else
		if (CDType & CD_SPANISH) {
			ofs = 172;
		} else {
			ofs = 140;
		}
	}

	objectives = ExtractEntry(ArchiveOffsets[txte], &l);
	if (!objectives) {
		printf("Objectives allocation failed\n");
		exit(-1);
	}
	objectives = (unsigned char *)realloc(objectives, l + 1);
	if (!objectives) {
		printf("Objectives allocation failed\n");
		exit(-1);
	}
	objectives[l] = '\0';

	//Now Search from start of objective data
	levelno = 0;
	race = 0;

	//Extract all the values for objectives
	if (expansion) {
		expansion = 52;
	} else {
		expansion = 28;
	}
	current = objectives + ofs;
	for (l = 0; l < (size_t)expansion; ++l) {
		next = current + strlen((char*)current) + 1;

		noobjs = 1;  // Number of objectives is zero.
		currentobj = current;
		while ((nextobj = (unsigned char*)strchr((char*)currentobj, '\n')) != NULL) {
			*nextobj = '\0';
			++nextobj;
			CampaignData[race][levelno][noobjs] = currentobj;
			currentobj = nextobj;
			++noobjs;
		}
		// Get the final one.
		CampaignData[race][levelno][noobjs] = currentobj;
		for (++noobjs; noobjs < 10; ++noobjs) {
			CampaignData[race][levelno][noobjs] = NULL;
		}
		current = next;
		if (race == 0) {
			race = 1;
		} else if (race == 1) {
			race = 0;
			++levelno;
		};
	}

	// Extract the Level titles now.
	race = 0;
	levelno = 0;
	// Find the start of the Levels
	while (current[0] && current[0] != 'I' && current[1] != '.') {
		current = current + strlen((char*)current) + 1;
	}
	for (l = 0; l < (size_t)expansion; ++l) {
		next = current + strlen((char*)current) + 1;
		CampaignData[race][levelno][0] = current;
		current = next;
		if (race == 0) {
			race = 1;
		} else {
			if (race == 1) {
				race = 0;
				++levelno;
			}
		}
	}

	for (levelno = 0; levelno < expansion / 2; ++levelno) {
		for (race = 0; race < 2; ++race) {
			//Open Relevant file, to write stuff too.
			sprintf(buf, "%s/%s/%s_c2.sms", Dir, TEXT_PATH,
				Todo[2 * levelno + 1 + race + 5].File);
			CheckPath(buf);
			if (!(outlevel = fopen(buf, "wb"))) {
				printf("Cannot Write File (Skipping Level: %s)\n", buf);
				fflush(stdout);
				continue;
			}
			unsigned char *str = ConvertString(CampaignData[race][levelno][0], 0);
			sprintf(buf, "title = \"%s\"\n", str);
			fputs(buf, outlevel);
			free(str);
			fputs("objectives = {", outlevel);
			for (noobjs = 1; noobjs < 10; ++noobjs) {
				if (CampaignData[race][levelno][noobjs] != NULL) {
					unsigned char *str = ConvertString(CampaignData[race][levelno][noobjs], 0);
					sprintf(buf, "%s\"%s\"", (noobjs > 1 ? "," : ""), str);
					fputs(buf, outlevel);
					free(str);
				}
			}
			fputs("}\n", outlevel);
			// Close levels and move on.
			fclose(outlevel);
		}
	}

	free(objectives);
	return 0;
}


//----------------------------------------------------------------------------
//  Main loop
//----------------------------------------------------------------------------

/**
**  Display the usage.
*/
void Usage(const char* name)
{
	printf("%s\n\
Usage: %s [-e|-n] [-m] [-v] [-r] [-V] [-h] archive-directory [destination-directory]\n\
\t-e\tThe archive is expansion compatible (default: autodetect)\n\
\t-n\tThe archive is not expansion compatible (default: autodetect)\n\
\t-m\tExtract and convert midi sound files\n\
\t-v\tExtract and convert videos\n\
\t-r\tRip sound tracks from CD-ROM (needs original CD, no image/emulation)\n\
\t-V\tShow version\n\
\t-h\tShow usage\n\
archive-directory\tDirectory which includes the archives maindat.war...\n\
destination-directory\tDirectory where the extracted files are placed.\n"
	,NameLine, name);
	fflush(stdout);
}

/**
**		Main
*/
#undef main
int main(int argc, char** argv)
{
	unsigned u;
	char buf[1024];
	struct stat st;
	int expansion_cd;
	int video;
	int midi;
	int rip;
	int a;
	char filename[1024];
	FILE* f;

	a = 1;
	video = rip = midi = expansion_cd = CDType = 0;
	while (argc >= 2) {
		if (!strcmp(argv[a], "-v")) {
			video = 1;
			++a;
			--argc;
			continue;
		}
		if (!strcmp(argv[a], "-m")) {
			midi = 1;
			++a;
			--argc;
			continue;
		}
		if (!strcmp(argv[a], "-r")) {
			rip = 1;
			++a;
			--argc;
			continue;
		}
		if (!strcmp(argv[a], "-e")) {
			expansion_cd = 1;
			++a;
			--argc;
			continue;
		}
		if (!strcmp(argv[a], "-n")) {
			expansion_cd = -1;
			++a;
			--argc;
			continue;
		}
		if (!strcmp(argv[a], "-V")) {
			printf(VERSION "\n");
			++a;
			--argc;
			exit(0);
		}
		if (!strcmp(argv[a], "-h")) {
			Usage(argv[0]);
			++a;
			--argc;
			exit(0);
		}
		break;
	}

	if (argc != 2 && argc != 3) {
		Usage(argv[0]);
		exit(-1);
	}

	ArchiveDir = argv[a];
	if (argc == 3) {
		Dir = argv[a + 1];
	} else {
		Dir = "data";
	}

	sprintf(buf, "%s/extracted", Dir);
	f = fopen(buf, "r");
	if (f) {
		char version[20];
		int len = 0;
		if (fgets(version, 20, f))
			len = 1;
		fclose(f);
		if (len != 0 && strcmp(version, VERSION) == 0) {
			printf("Note: Data is already extracted in Dir \"%s\" with this version of wartool\n", Dir);
			fflush(stdout);
		}
	}

	// Detect if CD is Mac/Dos, Expansion/Original, and language
	sprintf(buf, "%s/rezdat.war", ArchiveDir);
	sprintf(filename, "%s/strdat.war", ArchiveDir);
	if (stat(buf, &st)) {
		sprintf(buf, "%s/REZDAT.WAR", ArchiveDir);
		sprintf(filename, "%s/STRDAT.WAR", ArchiveDir);
		CDType |= CD_UPPER;
	}
	if (stat(buf, &st)) {
		CDType |= CD_MAC | CD_US;
		sprintf(buf, "%s/War Resources", ArchiveDir);
		if (stat(buf, &st)) {
			printf("Could not find Warcraft 2 Data\n");
			exit(-1);
		}
		if (expansion_cd == -1 || (expansion_cd != 1 && st.st_size != 2876978)) {
			printf("Detected original MAC CD\n");
			fflush(stdout);
		} else {
			printf("Detected expansion MAC CD\n");
			fflush(stdout);
			CDType |= CD_EXPANSION;
		}
	} else {
		if (st.st_size != 2811086) {
			expansion_cd = 0;
			stat(filename, &st);
			switch (st.st_size) {
				case 51550:
					printf("Detected US original DOS CD\n");
					fflush(stdout);
					CDType |= CD_US;
					break;
				case 53874:
					printf("Detected Italian original DOS CD\n");
					fflush(stdout);
					CDType |= CD_ITALIAN;
					break;
				case 55014:
					printf("Detected Spanish original DOS CD\n");
					fflush(stdout);
					CDType |= CD_SPANISH;
					break;
				case 55724:
					printf("Detected German original DOS CD\n");
					fflush(stdout);
					CDType |= CD_GERMAN;
					break;
				case 51451:
					printf("Detected UK/Australian original DOS CD\n");
					fflush(stdout);
					CDType |= CD_UK;
					break;
				case 52883:
					printf("Detected Portuguese original DOS CD\n");
					fflush(stdout);
					CDType |= CD_PORTUGUESE;
					break;
				case 55079:
					printf("Detected French original DOS CD\n");
					fflush(stdout);
					CDType |= CD_FRENCH;
					break;

				default:
					printf("Could not detect CD version:\n");
					printf("Defaulting to German original DOS CD\n");
					fflush(stdout);
					CDType |= CD_GERMAN;
					break;
			}
		} else {
			printf("Detected expansion DOS CD\n");
			fflush(stdout);
			CDType |= CD_EXPANSION | CD_US;
		}
	}

	if (expansion_cd == -1 || (expansion_cd != 1 && !(CDType & CD_EXPANSION))) {
		expansion_cd = 0;
	} else {
		expansion_cd = 1;
	}

	printf("Extract from \"%s\" to \"%s\"\n", ArchiveDir, Dir);
	printf("Please be patient, the data may take a couple of minutes to extract...\n");
	fflush(stdout);

	for (u = 0; u < sizeof(Todo) / sizeof(*Todo); ++u) {
		if (CDType & CD_MAC) {
			strcpy(filename, Todo[u].File);
			Todo[u].File = filename;
			ConvertToMac(Todo[u].File);
		}
		// Should only be on the expansion cd
#ifdef DEBUG
		printf("%s:\n", ParseString(Todo[u].File));
		fflush(stdout);
#endif
		if (!expansion_cd && Todo[u].Version==2 ) {
			continue;
		}
		// Extract dummy expansion files
		if (expansion_cd && Todo[u].Version == 3) {
			continue;
		}
		switch (Todo[u].Type) {

			case F:
				if (CDType & CD_UPPER) {
					int i = 0;
					strcpy(filename, Todo[u].File);
					Todo[u].File = filename;
					while (Todo[u].File[i]) {
						Todo[u].File[i] = toupper(Todo[u].File[i]);
						++i;
					}
				}
				sprintf(buf, "%s/%s", ArchiveDir, Todo[u].File);
				printf("Archive \"%s\"\n", buf);
				fflush(stdout);
				if (ArchiveBuffer) {
					CloseArchive();
				}
				OpenArchive(buf, Todo[u].Arg1);
				break;
			case R:
				ConvertRgb(Todo[u].File, Todo[u].Arg1);
				break;
			case T:
				ConvertTileset(Todo[u].File, Todo[u].Arg1, Todo[u].Arg2,
					Todo[u].Arg3, Todo[u].Arg4);
				break;
			case G:
				ConvertGfx(ParseString(Todo[u].File), Todo[u].Arg1, Todo[u].Arg2,
					Todo[u].Arg3, Todo[u].Arg4);
				break;
			case U:
				ConvertGfu(Todo[u].File, Todo[u].Arg1, Todo[u].Arg2);
				break;
			case D:
				ConvertGroupedGfu(Todo[u].File, Todo[u].Arg1, Todo[u].Arg2,
					Todo[u].Arg3);
				break;
			case P:
				ConvertPud(Todo[u].File, Todo[u].Arg1);
				break;
			case N:
				ConvertFont(Todo[u].File, 2, Todo[u].Arg1);
				break;
			case I:
				ConvertImage(Todo[u].File, Todo[u].Arg1, Todo[u].Arg2,
					Todo[u].Arg3, Todo[u].Arg4);
				break;
			case C:
				ConvertCursor(Todo[u].File, Todo[u].Arg1, Todo[u].Arg2);
				break;
			case M:
				if (midi) {
					ConvertXmi(Todo[u].File, Todo[u].Arg1);
				}
				break;
			case W:
				ConvertWav(Todo[u].File, Todo[u].Arg1);
				break;
			case X:
				ConvertText(Todo[u].File, Todo[u].Arg1, Todo[u].Arg2);
				break;
			case S:
				SetupNames(Todo[u].File, Todo[u].Arg1);
				break;
			case V:
				if (video) {
					ConvertVideo(Todo[u].File, Todo[u].Arg1);
				}
				break;
			case L:
				CampaignsCreate(Todo[u].File, Todo[u].Arg1, Todo[u].Arg2);
				break;
			default:
				break;
		}
	}

	ConvertFilePuds(OriginalPuds);
	ConvertFilePuds(ExpansionPuds);

	CopyMusic();

	if (rip) {
		sprintf(buf, "%s/%s/", Dir, MUSIC_PATH);
		CheckPath(buf);
		RipMusic(expansion_cd, ArchiveDir, buf);
	}

	ConvertMusic();

	if (ArchiveBuffer) {
		CloseArchive();
	}
	if (Pal27) {
		free(Pal27);
	}

	while (UnitNamesLast > 0) {
		free(UnitNames[UnitNamesLast]);
		--UnitNamesLast;
	}

	sprintf(buf, "%s/scripts/wc2-config.lua", Dir);
	CheckPath(buf);
	f = fopen(buf, "w");

	if (!f) {
		perror("");
		printf("Can't open %s\n", buf);
		exit(-1);
	}

	if (expansion_cd) {
		fprintf(f, "wargus.expansion = true\n");
	} else {
		fprintf(f, "wargus.expansion = false\n");
	}
	fprintf(f, "wargus.game_font_width = %d\n", game_font_width);
	fclose(f);

	sprintf(buf, "%s/extracted", Dir);
	f = fopen(buf, "w");

	if (!f) {
		perror("");
		printf("Can't open %s\n", buf);
		exit(-1);
	}

	fputs(VERSION, f);
	fclose(f);

	printf("Done.\n");

	return 0;
}

//@}
