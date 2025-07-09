/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera
 *
 *  @file          VINIFERAMSGBOX.H
 *
 *  @authors       Rampastring
 *
 *  @brief         WWMessageBox reimplementation using DirectDraw rather 
 *                 than WinForms.
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

#ifndef VINIFERAMSGBOX_H
#define VINIFERAMSGBOX_H

#include "tibsun_defines.h"
#include "language.h"

class WWFontClass;

class ViniferaMessageBox
{
public:
	ViniferaMessageBox(int caption = TXT_NONE);

	int Process(const char* msg, const char* b1txt, const char* b2txt = nullptr, const char* b3txt = nullptr, bool preserve = false);
	int Process(int msg, int b1txt = TXT_OK, int b2txt = TXT_NONE, int b3txt = TXT_NONE, bool preserve = false);
	int Process(char const* msg, int b1txt = TXT_OK, int b2txt = TXT_NONE, int b3txt = TXT_NONE, bool preserve = false);

private:
	int Caption;
};

void Draw_Caption(char const* text, WWFontClass* font, int x, int y, int w);
void Draw_Caption(int text, int x, int y, int w);
void Dialog_Box(Rect rect);

#endif