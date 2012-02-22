--       _________ __                 __
--      /   _____//  |_____________ _/  |______     ____  __ __  ______
--      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
--      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ \
--     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
--             \/                  \/          \//_____/            \/
--  ______________________                           ______________________
--                        T H E   W A R   B E G I N S
--         Stratagus - A free fantasy real time strategy game engine
--
--      constructions.lua - Define the constructions.
--
--      (c) Copyright 2001-2004 by Lutz Sammer and Jimmy Salmon
--
--      This program is free software; you can redistribute it and/or modify
--      it under the terms of the GNU General Public License as published by
--      the Free Software Foundation; either version 2 of the License, or
--      (at your option) any later version.
--
--      This program is distributed in the hope that it will be useful,
--      but WITHOUT ANY WARRANTY; without even the implied warranty of
--      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--      GNU General Public License for more details.
--
--      You should have received a copy of the GNU General Public License
--      along with this program; if not, write to the Free Software
--      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
--

if (wargus.tileset == nil) then
  wargus.tileset = "summer"
end

Load("scripts/human/constructions.lua")
Load("scripts/orc/constructions.lua")

local files = {summer = "neutral/buildings/land_construction_site.png",
  winter = "tilesets/winter/neutral/buildings/land_construction_site.png",
  wasteland = "neutral/buildings/land_construction_site.png",
  swamp = "neutral/buildings/land_construction_site.png"}

DefineConstruction("construction-none", {
  Files = {
    File = files[wargus.tileset],
    Size = {64, 64}},
  Constructions = {
   {Percent = 0,
    File = "construction",
    Frame = 0},
   {Percent = 25,
    File = "construction",
    Frame = 1},
   {Percent = 50,
    File = "main",
    Frame = 1}}
})

local files = {summer = "neutral/buildings/land_construction_site.png",
  winter = "tilesets/winter/neutral/buildings/land_construction_site.png",
  wasteland = "neutral/buildings/land_construction_site.png",
  swamp = "neutral/buildings/land_construction_site.png"}

DefineConstruction("construction-land", {
  Files = {
    File = files[wargus.tileset],
    Size = {64, 64}},
  Constructions = {
   {Percent = 0,
    File = "construction",
    Frame = 0},
   {Percent = 25,
    File = "construction",
    Frame = 1},
   {Percent = 50,
    File = "main",
    Frame = 1}}
})

local files = {summer = "neutral/buildings/land_construction_site.png",
  winter = "tilesets/winter/neutral/buildings/land_construction_site.png",
  wasteland = "neutral/buildings/land_construction_site.png",
  swamp = "neutral/buildings/land_construction_site.png"}

DefineConstruction("construction-land2", {
  Files = {
    File = files[wargus.tileset],
    Size = {64, 64}},
  Constructions = {
   {Percent = 0,
    File = "construction",
    Frame = 0},
   {Percent = 25,
    File = "construction",
    Frame = 1}}
})

local files = {summer = "tilesets/summer/neutral/buildings/wall_construction_site.png",
  winter = "tilesets/winter/neutral/buildings/wall_construction_site.png",
  wasteland = "tilesets/wasteland/neutral/buildings/wall_construction_site.png",
  swamp = "tilesets/summer/neutral/buildings/wall_construction_site.png"}

DefineConstruction("construction-wall", {
  Files = {
    File = files[wargus.tileset],
    Size = {32, 32}},
  Constructions = {
   {Percent = 0,
    File = "construction",
    Frame = 0},
   {Percent = 25,
    File = "construction",
    Frame = 1},
   {Percent = 50,
    File = "main",
    Frame = 1}}
})
