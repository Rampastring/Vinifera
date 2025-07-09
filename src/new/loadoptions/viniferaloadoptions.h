/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera
 *
 *  @file          VINIFERALOADOPTIONS.H
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


// This file reuses code from the Command & Conquer Remastered Collection.
// License below.
//
// Copyright 2020 Electronic Arts Inc.
//
// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free 
// software: you can redistribute it and/or modify it under the terms of 
// the GNU General Public License as published by the Free Software Foundation, 
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed 
// in the hope that it will be useful, but with permitted additional restrictions 
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT 
// distributed with this program. You should have received a copy of the 
// GNU General Public License along with permitted additional restrictions 
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection

/* $Header: /CounterStrike/LOADDLG.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                         						  *
 *                 Project Name : Command & Conquer                        						  *
 *                                                                         						  *
 *                    File Name : LOADDLG.H 	                              						  *
 *                                                                         						  *
 *                   Programmer : Maria Legg, Joe Bostic, Bill Randolph     						  *
 *                                                                         						  *
 *                   Start Date : March 19, 1995															  *
 *                                                                         						  *
 *                  Last Update : March 19, 1995															  *
 *                                                                         						  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#pragma once

#include	"list.h"
#include	"house.h"
#include	"session.h"
#include	"vector.h"

class ViniferaFileEntryClass {
public:
	char Descr[128];				// save-game description
	char Filename[32];
	unsigned Scenario;			// scenario #
	HousesType House;				// house
	char PlayerName[64];
	int Num;							// save file number (from the extension)
	FILETIME DateTime;		// date/time stamp of file
	bool Valid;						// Is the scenario valid?
	bool Old;
	GameType Type;

	ViniferaFileEntryClass(void) :
		Scenario(0),
		House(HOUSE_NONE),
		Num(-1),
		Valid(true),
		Old(false),
		Type(GAME_NORMAL)
	{
		Descr[0] = '\0';
		Filename[0] = '\0';
		PlayerName[0] = '\0';
		DateTime.dwHighDateTime = 0;
		DateTime.dwLowDateTime = 0;
	}
};

class ViniferaLoadOptionsClass
{
public:
	/*
	** This defines the style of the dialog
	*/
	typedef enum OperationModeEnum {
		NONE = 0,
		LOAD,
		SAVE,
		WWDELETE
	} LoadStyleType;

	enum SaveGameGameVersion { // todo resolve
		GAMEVER_TS = 114465,
		GAMEVER_FS = 114849
	};

	ViniferaLoadOptionsClass(void);
	virtual ~ViniferaLoadOptionsClass(void);
	bool Process(void);

	bool Load(void);
	bool Save(char* file_name);
	bool Delete(void);

	void Pick_Filename(char* file_name);
	bool Read_Save_Files(void);

	virtual bool Load_File(const char* file_name);
	virtual bool Save_File(const char* file_name, const char* descr);
	virtual bool Delete_File(const char* file_name);
	virtual bool Read_File(ViniferaFileEntryClass* entry, WIN32_FIND_DATAA* ff);

protected:
	/*
	** Internal routines
	*/
	void Clear_List(ListClass* list);		// clears the list & game # array
	void Fill_List(ListClass* list);		// fills the list & game # array
	static int __cdecl Compare(const void* p1, const void* p2); // for qsort()

	/*
	** This is the requested style of the dialog
	*/
	LoadStyleType Style;

	char* Extension;
	char* Description;

	int MinSpaceRequired;

	/*
	** This is an array of pointers to FileEntryClass objects.  These objects
	** are allocated on the fly as files are found, and pointers to them are
	** added to the vector list.  Thus, all the objects must be free'd before
	** the vector list is cleared.  This list is used for sorting the files
	** by date/time.
	*/
	DynamicVectorClass<ViniferaFileEntryClass*> Files;
};