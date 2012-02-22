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
--      campaign2.ccl - Define the human campaign 2.
--
--      (c) Copyright 2002-2010 by Lutz Sammer, Jimmy Salmon and Pali Rohár
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

--=============================================================================
--	Define the campaign

campaign_steps = {
  CreateVideoStep("videos/exp-1.ogv"),
  CreatePictureStep("campaigns/human-exp/interface/Act_I_-_A_Time_for_Heroes.png", "sounds/human/act.wav", "Act I", "A Time for Heroes"),
  CreateMapStep("campaigns/human-exp/levelx01h.smp"),
  CreateMapStep("campaigns/human-exp/levelx02h.smp"),
  CreateMapStep("campaigns/human-exp/levelx03h.smp"),

  CreatePictureStep("campaigns/human-exp/interface/Act_II_-_Draenor,_the_Red_World.png", "sounds/human/act.wav", "Act II", "Draenor, The Red World"),
  CreateMapStep("campaigns/human-exp/levelx04h.smp"),
  CreateMapStep("campaigns/human-exp/levelx05h.smp"),
  CreateMapStep("campaigns/human-exp/levelx06h.smp"),

  CreatePictureStep("campaigns/human-exp/interface/Act_III_-_War_in_the_Shadows.png", "sounds/human/act.wav", "Act III", "War in the Shadows"),
  CreateMapStep("campaigns/human-exp/levelx07h.smp"),
  CreateMapStep("campaigns/human-exp/levelx08h.smp"),
  CreateMapStep("campaigns/human-exp/levelx09h.smp"),

  CreatePictureStep("campaigns/human-exp/interface/Act_IV_-_The_Measure_of_Valor.png", "sounds/human/act.wav", "Act IV", "The Measure of Valor"),
  CreateMapStep("campaigns/human-exp/levelx10h.smp"),
  CreateMapStep("campaigns/human-exp/levelx11h.smp"),
  CreateMapStep("campaigns/human-exp/levelx12h.smp"),
  CreateVideoStep("videos/human-exp-2.ogv"),
  CreateVictoryStep("graphics/ui/human/The_End.png", "campaigns/human-exp/victory.txt", { "campaigns/human-exp/victory-1.wav", "campaigns/human-exp/victory-2.wav" })
}

campaign_menu = { 1, 4, 5, 6, 8, 9, 10, 12, 13, 14, 16, 17, 18 }

