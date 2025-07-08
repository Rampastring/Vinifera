/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera
 *
 *  @file          VINIFERAGAMEOPTIONS.H
 *
 *  @authors       Rampastring
 *
 *  @brief         GameOptionsClass reimplementation using DirectDraw rather
 *                 than Windows Forms.
 *
 *  @license       Vinifera is free software: you can redistribute it and/or
 *                 modify it under the terms of the GNU General Public License
 *                 as published by the Free Software Foundation, either version
 *                 3 of the License, or (at your option) any later version.
 *
 *                 Vinifera is distributed in the hope that it will be
 *                 useful, but WITHOUT ANY WARRANTY; without even the implied
 *                 warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *                 PURPOSE. See the GNU General Public License for more details.
 *
 *                 You should have received a copy of the GNU General Public
 *                 License along with this program.
 *                 If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/
#pragma once

#ifndef VINIFERAGAMEOPTIONS_H
#define VINIFERAGAMEOPTIONS_H

#include "tibsun_defines.h"
#include "options.h"
#include "language.h"

class ViniferaGameOptionsClass : public OptionsClass
{
	enum GameOptionsButtonEnum {
		BUTTON_GAME = 1,
		BUTTON_RESTATE,
		BUTTON_LOAD,
		BUTTON_SAVE,
		BUTTON_DELETE,
		BUTTON_QUIT,
		BUTTON_RESUME,

		BUTTON_COUNT
	};

	enum GameOptionsEnum {
		OPTION_WIDTH = (432 + 16),
		OPTION_HEIGHT = 200,
		OPTION_X = ((640 - (432 + 16)) / 2),
		OPTION_Y = ((400 - 200) / 2),
		BUTTON_WIDTH = 260,
		CAPTION_Y_POS = 10,
		BUTTON_Y = 42,
		BUTTON_HEIGHT = 20,
		BORDER1_LEN = 144,
		BORDER2_LEN = 32,
		BUTTON_RESUME_Y = (200 - 30)
	};

public:
	ViniferaGameOptionsClass(void): OptionsClass () { }

	void Process();

private:
};

void ViniferaGameOptionsClass_Hooks();

#endif