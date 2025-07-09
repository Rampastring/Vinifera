/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera
 *
 *  @file          VINIFERAGAMEOPTIONS.CPP
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

#include "viniferaloadoptions.h"
#include "viniferamsgbox.h"
#include "tibsun_globals.h"
#include "tibsun_defines.h"
#include "tibsun_functions.h"
#include "tibsun_util.h"
#include "tspp.h"
#include "dsurface.h"
#include "xsurface.h"
#include "edit.h"
#include "event.h"
#include "gscreen.h"
#include "house.h"
#include "housetype.h"
#include "textbtn.h"
#include "textprint.h"
#include "session.h"
#include "wwmouse.h"
#include "mouse.h"
#include "wwfont.h"
#include "map.h"
#include "miscutil.h"
#include "saveload.h"
#include "savever.h"
#include "theme.h"
#include "hooker.h"
#include "hooker_macros.h"
#include "vinifera_globals.h"
#include "vinifera_saveload.h"
#include "vinifera_savever.h"
#include "drawshape.h"


#define TPF_BUTTON	(TPF_CENTER|TPF_6PT_GRAD|TPF_NOSHADOW)
#define	BUTTON_1		1
#define	BUTTON_2		2
#define	BUTTON_3		3
#define	BUTTON_FLAG	0x8000


ViniferaLoadOptionsClass::ViniferaLoadOptionsClass(void) :
    Files(0, 0),
    Style(NONE),
    Description(NULL)
{
    Style = NONE;
    Description = NULL;
    Extension = "SAV";
    MinSpaceRequired = 2048;
    Files.Clear();
}


ViniferaLoadOptionsClass::~ViniferaLoadOptionsClass()
{
    for (int i = 0; i < Files.Count(); i++) {
        delete Files[i];
    }
    Files.Clear();
}


bool ViniferaLoadOptionsClass::Load(void)
{
    Style = LOAD;
    Description = NULL;
    return Process();
}


bool ViniferaLoadOptionsClass::Save(char* name)
{
    Style = SAVE;
    Description = name;
    return Process();
}


bool ViniferaLoadOptionsClass::Delete(void)
{
    Style = WWDELETE;
    Description = NULL;
    return Process();
}


bool ViniferaLoadOptionsClass::Process(void)
{
    /*
    **	Dialog & button dimensions
    */
    int d_dialog_w = 500;											// dialog width
    int d_dialog_h = 312;											// dialog height
    int d_dialog_x = ((PrimarySurface->Width - d_dialog_w) / 2);	// centered x-coord
    int d_dialog_y = ((PrimarySurface->Height - d_dialog_h) / 2);	// centered y-coord
    int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);				// coord of x-center

    int d_txt8_h = 22;												// ht of 8-pt text
    int d_margin = 14;												// margin width/height
    int x_margin = 32;												// margin width/height

    int d_list_w = d_dialog_w - (x_margin * 2);
    int d_list_h = 208;
    int d_list_x = d_dialog_x + x_margin;
    int d_list_y = d_dialog_y + d_margin + d_txt8_h + d_margin;

    int d_edit_w = d_dialog_w - (x_margin * 2);
    int d_edit_h = 26;
    int d_edit_x = d_dialog_x + x_margin;
    int d_edit_y = d_list_y + d_list_h - 60 + d_margin + d_txt8_h;

    int d_button_w = 80;
    int d_button_h = 26;
    int d_button_x = d_dialog_cx - d_button_w - d_margin;
    int d_button_y = d_dialog_y + d_dialog_h - d_button_h - d_margin;

    int d_cancel_w = 80;
    int d_cancel_h = 26;
    int d_cancel_x = d_dialog_cx + d_margin;
    int d_cancel_y = d_dialog_y + d_dialog_h - d_cancel_h - d_margin;

    /*
    **	Button enumerations
    */
    enum {
        BUTTON_LOAD = 100,
        BUTTON_SAVE,
        BUTTON_DELETE,
        BUTTON_CANCEL,
        BUTTON_LIST,
        BUTTON_EDIT,
    };

    /*
    **	Redraw values: in order from "top" to "bottom" layer of the dialog
    */
    typedef enum {
        REDRAW_NONE = 0,
        REDRAW_BUTTONS,
        REDRAW_BACKGROUND,
        REDRAW_ALL = REDRAW_BACKGROUND
    } RedrawType;

    /*
    **	Dialog variables
    */
    bool cancel = false;						// true = user cancels
    int list_ht = d_list_h;					// adjusted list box height

    /*
    **	Other Variables
    */
    char* btn_txt;								// text on the 'OK' button
    int btn_id;									// ID of 'OK' button
    char* caption;								// dialog caption
    int game_idx = 0;							// index of game to save/load/etc
    int game_num = 0;							// file number of game to load/save/etc
    char game_descr[DESCRIP_MAX] = { 0 };		// save-game description
    char fname[PATH_MAX];			            // for generating filename to delete
    int rc;										// return code

    /*
    **	Buttons
    */
    ControlClass* commands = NULL;		// the button list

    switch (Style) {
    case LOAD:
        btn_txt = "Load";
        btn_id = BUTTON_LOAD;
        caption = "Load Game";
        break;

    case SAVE:
        btn_txt = "Save";
        btn_id = BUTTON_SAVE;
        caption = "Save Game";
        list_ht -= 30;
        break;

    default:
        btn_txt = "Delete";
        btn_id = BUTTON_DELETE;
        caption = "Delete Saved Game";
        break;
    }

    TextButtonClass button(btn_id, btn_txt, TPF_BUTTON, d_button_x, d_button_y, d_button_w, -1, false, false);
    TextButtonClass cancelbtn(BUTTON_CANCEL, TXT_CANCEL, TPF_BUTTON, d_cancel_x, d_cancel_y, d_cancel_w, -1, false, false);

    ListClass listbtn(BUTTON_LIST, d_list_x, d_list_y, d_list_w, list_ht,
        TPF_6PT_GRAD | TPF_NOSHADOW,
        MixFileClass::Retrieve("BTN-UP.SHP"),
        MixFileClass::Retrieve("BTN-DN.SHP"));

	if (Description != nullptr) {
		memcpy(&game_descr, Description, strnlen_s(Description, DESCRIP_MAX));
	}

    EditClass editbtn(BUTTON_EDIT, game_descr, sizeof(game_descr) - 4, TPF_6PT_GRAD | TPF_NOSHADOW, d_edit_x, d_edit_y, d_edit_w, -1, EditClass::ALPHANUMERIC);

    Fill_List(&listbtn);

    /*
    **	Do nothing if list is empty.
    */
    if ((Style == LOAD || Style == WWDELETE) && listbtn.Count() == 0) {
        Clear_List(&listbtn);
        ViniferaMessageBox().Process("No saved games available!", TXT_OK);
        return false;
    }

    /*
    **	Create the button list.
    */
    commands = &button;
    cancelbtn.Add_Tail(*commands);
    listbtn.Add_Tail(*commands);
    if (Style == SAVE) {
        editbtn.Add_Tail(*commands);
        editbtn.Set_Focus();
    }

	Rect rect = Rect(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

	/*
	**	Main Processing Loop.
	*/
	WWKeyboard->Clear();
	bool firsttime = true;
	bool display = true;
	bool process = true;
	while (process) {

		/*
		**	Invoke game callback.
		*/
		if (Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH) {
			Call_Back();
		}
		else {
			if (Main_Loop()) {
				process = false;
				cancel = true;
			}
		}

		// /*
		// ** If we have just received input focus again after running in the background then
		// ** we need to redraw.
		// */
		// if (AllSurfaces.SurfacesRestored) {
		// 	AllSurfaces.SurfacesRestored = FALSE;
		// 	display = true;
		// }

		/*
		**	Refresh display if needed.
		*/
		if (display) {

			/*
			**	Display the dialog box.
			*/
			WWMouse->Hide_Mouse();
			if (display) {
				Dialog_Box(rect);
				Draw_Caption(caption, Metal12FontPtr, d_dialog_x, d_dialog_y, d_dialog_w);

				if (Style == SAVE) {
					Fancy_Text_Print("File Name:", LogicSurface, &rect, &Point2D(d_dialog_cx, d_edit_y - d_txt8_h),
						ColorSchemes[GadgetClass::Get_Color_Scheme()], TBLACK, TPF_METAL12 | TPF_NOSHADOW | TPF_CENTER);
				}
			}

			/*
			**	Redraw the buttons.
			*/
			if (display) {
				commands->Flag_List_To_Redraw();
			}
			WWMouse->Show_Mouse();
			display = false;
		}

		/*
		**	Get user input.
		*/
		KeyNumType input = commands->Input();

		/*
		**	The first time through the processing loop, set the edit
		**	gadget to have the focus if this is the save dialog. The
		**	focus must be set here since the gadget list has changed
		**	and this change will cause any previous focus setting to be
		**	cleared by the input processing routine.
		*/
		if (firsttime && Style == SAVE) {
			firsttime = false;
			editbtn.Set_Focus();
			editbtn.Flag_To_Redraw();
		}

		/*
		**	If the <RETURN> key was pressed, then default to the appropriate
		**	action button according to the style of this dialog box.
		*/
		if (input == KN_RETURN || input == (BUTTON_EDIT | KN_BUTTON)) {
			ToggleClass* toggle = NULL;
			switch (Style) {
			case SAVE:
				input = (KeyNumType)(BUTTON_SAVE | KN_BUTTON);
				cancelbtn.Turn_Off();
				//					cancelbtn.IsOn = false;
				toggle = (ToggleClass*)commands->Extract_Gadget(BUTTON_SAVE);
				if (toggle != NULL) {
					toggle->Turn_On();
					//						toggle->IsOn = true;
					toggle->IsPressed = true;
				}
				break;

			case LOAD:
				input = (KeyNumType)(BUTTON_LOAD | KN_BUTTON);
				//					cancelbtn.IsOn = false;
				cancelbtn.Turn_Off();
				toggle = (ToggleClass*)commands->Extract_Gadget(BUTTON_LOAD);
				if (toggle != NULL) {
					toggle->IsOn = true;
					toggle->IsPressed = true;
				}
				break;

			case WWDELETE:
				input = (KeyNumType)(BUTTON_DELETE | KN_BUTTON);
				//					cancelbtn.IsOn = false;
				cancelbtn.Turn_Off();
				toggle = (ToggleClass*)commands->Extract_Gadget(BUTTON_DELETE);
				if (toggle != NULL) {
					toggle->IsOn = true;
					toggle->IsPressed = true;
				}
				break;
			}
			WWMouse->Hide_Mouse();
			commands->Draw_All(true);
			WWMouse->Show_Mouse();
		}

		const char* filename = nullptr;

		/*
		**	Process input.
		*/
		switch (input) {
			/*
			** Load: if load fails, present a message, and stay in the dialog
			** to allow the user to try another game
			*/
		case (BUTTON_LOAD | KN_BUTTON):
			game_idx = listbtn.Current_Index();
			game_num = Files[game_idx]->Num;
			if (Files[game_idx]->Valid) {

				/*
				** Start a timer before we load the game
				*/
				CDTimerClass<SystemTimerClass> timer;
				//					timer.Start();
				timer = TICKS_PER_SECOND * 4;

				//WWMessageBox().Process(TXT_LOADING, TXT_NONE);
				Theme.Fade_Out();
				rc = Load_File(Files[game_idx]->Filename);

				/*
				** Make sure the message says on the screen at least 1 second
				*/
				while (timer > 0) {
					Call_Back();
				}
				WWKeyboard->Clear();

				if (!rc) {
					ViniferaMessageBox().Process(TXT_ERROR_LOADING_GAME, TXT_OK);
				}
				else {
					//Speak(VOX_LOAD1);
					//while (Is_Speaking()) {
					//	Call_Back();
					//}
					WWMouse->Hide_Mouse();
					// SeenPage.Clear();
					// GamePalette.Set();
					//						Set_Palette(GamePalette);
					WWMouse->Show_Mouse();
					process = false;
				}
			}
			else {
				ViniferaMessageBox().Process(TXT_OBSOLETE_SAVEGAME, TXT_OK);
			}
			break;

			/*
			** Save: Save the game & exit the dialog
			*/
		case (BUTTON_EDIT | KN_BUTTON):
			break;

		case (BUTTON_SAVE | KN_BUTTON):
			if (!strlen(game_descr)) {
				ViniferaMessageBox().Process(TXT_MUSTENTER_DESCRIPTION, TXT_OK);
				firsttime = true;
				display = true;
				break;
			}
			game_idx = listbtn.Current_Index();

			if (Files[game_idx]->Valid) {
				filename = Files[game_idx]->Filename;
			} else {
				char test_filename[256];

				{ // the scope is important to make it match - temp_file nedes to be destroyed before assigning the string
					CCFileClass temp_file;
					do {
						sprintf(test_filename, "SAVE%04X.%s", rand(), Extension);
						temp_file.Set_Name(test_filename);
					} while (temp_file.Is_Available() == true);
				}

				filename = test_filename;
			}

			if (filename != NULL) {
				bool allow = true;
				bool exists = RawFileClass(filename).Is_Available() == true;
				if (exists && !ViniferaMessageBox().Process(TXT_CONFIRM_SAVE, TXT_YES, TXT_NO, TXT_NONE))
				{
					allow = false;
				}

				if (allow) {
					if (!Save_File(filename, game_descr)) {
						ViniferaMessageBox().Process(TXT_ERROR_SAVING_GAME, TXT_OK, TXT_NONE, TXT_NONE);
					}
					else {
						ViniferaMessageBox().Process(TXT_GAME_WAS_SAVED, TXT_OK, TXT_NONE, TXT_NONE);
						if (Description) {
							strcpy(Description, game_descr);
						}
					}
				}
			}
			process = false;
			break;

			/*
			** Delete: delete the file & stay in the dialog, to allow the user
			** to delete multiple files.
			*/
		case (BUTTON_DELETE | KN_BUTTON):
			game_idx = listbtn.Current_Index();
			game_num = Files[game_idx]->Num;
			if (ViniferaMessageBox().Process(TXT_DELETE_FILE_QUERY, TXT_YES, TXT_NO) == 0) {
				Delete_File(Files[game_idx]->Filename);
				Clear_List(&listbtn);
				Fill_List(&listbtn);
				if (listbtn.Count() == 0) {
					process = false;
				}
				else {
					ToggleClass* toggle = (ToggleClass*)commands->Extract_Gadget(BUTTON_DELETE);
					if (toggle != NULL) {
						toggle->Turn_Off();
						toggle->IsPressed = false;
						toggle->Flag_To_Redraw();
					}
				}
			}
			display = true;
			break;

			/*
			** If the user clicks on the list, see if the there is a new current
			** item; if so, and if we're in SAVE mode, copy the list item into
			** the save-game description field.
			*/
		case (BUTTON_LIST | KN_BUTTON):
			if (Style != SAVE) {
				break;
			}

			if (listbtn.Count() && listbtn.Current_Index() != game_idx) {
				game_idx = listbtn.Current_Index();

				/*
				** Copy the game's description, UNLESS it's the empty slot; if
				** it is, set the edit buffer to empty.
				*/
				if (game_idx != 0) {
					strcpy(game_descr, listbtn.Get_Item(game_idx));

					/*
					**	Strip any leading parenthesis off of the description.
					*/
					if (game_descr[0] == '(') {
						char* ptr = strchr(game_descr, ')');
						if (ptr != NULL) {
							strcpy(game_descr, ptr + 1);
							// strtrim(game_descr);
						}
					}

				}
				else {
					game_descr[0] = 0;
				}
				editbtn.Set_Text(game_descr, 40);
			}
			break;

			/*
			** ESC/Cancel: break
			*/
		case (KN_ESC):
		case (BUTTON_CANCEL | KN_BUTTON):
			cancel = true;
			process = false;
			break;

		default:
			break;
		}

		if (!display && process) {
			CompositeSurface->Copy_From(rect, *LogicSurface, rect);
			WWMouse->Show_Mouse();
			GScreenClass::Blit(true, CompositeSurface);
		}
	}

	Clear_List(&listbtn);

	if (cancel) return(false);

	return(true);
}


void ViniferaLoadOptionsClass::Pick_Filename(char* name)
{
    CCFileClass file;
    do {
        sprintf(name, "SAVE%04lX.%3s", rand(), Extension);
        file.Set_Name(name);
    } while (file.Is_Available() == true);
}


/***********************************************************************************************
 * LoadOptionsClass::Clear_List -- clears the list box & Files arrays                          *
 *                                                                                             *
 * This step is essential, because it frees all the strings allocated for list items.          *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      none.                                                                                  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      none.                                                                                  *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   02/14/1995 BR : Created.                                                                  *
 *=============================================================================================*/
void ViniferaLoadOptionsClass::Clear_List(ListClass* list)
{
    /*
    ** For every item in the list, free its buffer & remove it from the list.
    */
    int j = list->Count();
    for (int i = 0; i < j; i++) {
        list->Remove_Item(list->Get_Item(0));
    }

    /*
    ** Clear the array of game numbers
    */
    for (int i = 0; i < Files.Count(); i++) {
        delete Files[i];
    }
    Files.Clear();
}


 /***********************************************************************************************
  * LoadOptionsClass::Fill_List -- fills the list box & GameNum arrays                          *
  *                                                                                             *
  * INPUT:                                                                                      *
  *      none.                                                                                  *
  *                                                                                             *
  * OUTPUT:                                                                                     *
  *      none.                                                                                  *
  *                                                                                             *
  * WARNINGS:                                                                                   *
  *      none.                                                                                  *
  *                                                                                             *
  * HISTORY:                                                                                    *
  *   02/14/1995 BR : Created.                                                                  *
  *   06/25/1995 JLB : Shows which saved games are "(old)".                                     *
  *=============================================================================================*/
void ViniferaLoadOptionsClass::Fill_List(ListClass* list)
{
	ViniferaFileEntryClass* fdata;	// for adding entries to 'Files'
	WIN32_FIND_DATAA ff;		// for FindFirstFile

	/*
	** Make sure the list is empty
	*/
	Clear_List(list);

	/*
	** Add the Empty Slot entry
	*/
	if (Style == SAVE) {
		fdata = new ViniferaFileEntryClass;
		strcpy(fdata->Descr, Text_String(TXT_EMPTY_SLOT));
		if (PlayerPtr != NULL) {
			fdata->Scenario = Scen->Scenario;
			fdata->House = Scen->PlayerHouse;
			fdata->Num = Scen->CampaignID;
			strcpy(fdata->PlayerName, PlayerPtr->Class->FullName);
		}
		else {
			fdata->Scenario = 0;
			fdata->House = (HousesType)Session.House;
			fdata->Num = -1;
			strcpy(fdata->PlayerName, Session.Handle);
		}
		SYSTEMTIME time;
		GetSystemTime(&time);
		SystemTimeToFileTime(&time, &fdata->DateTime);
		fdata->Type = Session.Type;
		fdata->Valid = false;
		Files.Add(fdata);
	}

	char buffer[128];
	sprintf(buffer, "*.%3s", Extension);

	char formatted[128];
	_makepath(formatted, nullptr, Vinifera_SavedGamesDirectory, buffer, nullptr);

	/*
	** Find all savegame files
	*/
	HANDLE hFind = FindFirstFile(formatted, &ff);
	fdata = NULL;

	if (hFind != INVALID_HANDLE_VALUE) {
		while (true) {
			if ((ff.dwFileAttributes & (FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) == 0) {
				if (fdata == NULL) {
					fdata = new ViniferaFileEntryClass;
				}

				/*
				** get the game's info; if success, add it to the list
				*/
				if (Read_File(fdata, &ff) == true) {
					Files.Add(fdata);
					fdata = NULL;
				}
			}

			/*
			** Find the next file
			*/
			if (!FindNextFile(hFind, &ff)) {
				break;
			}
		}

		if (fdata != NULL) {
			delete fdata;
		}

		FindClose(hFind);
	}

	/*
	** Now sort the list in order of Date/Time (newest first, oldest last)
	*/
	qsort((void*)(&Files[0]), Files.Count(), sizeof(class FileEntryClass*), ViniferaLoadOptionsClass::Compare);

	/*
	** Now add every file's name to the list box
	*/
	for (int i = 0; i < Files.Count(); i++) {
		list->Add_Item(Files[i]->Descr);
	}
}


bool ViniferaLoadOptionsClass::Read_Save_Files(void)
{
    bool files_found = false;

    char pattern[64];
    sprintf(pattern, "*.%3s", Extension);

	char formatted[128];
	_makepath(formatted, nullptr, Vinifera_SavedGamesDirectory, pattern, nullptr);

    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFile(formatted, &find_data);

    if (hFind != INVALID_HANDLE_VALUE) {
        while (true) {
            if ((find_data.dwFileAttributes & (FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) == 0) {
                if (_stricmp(find_data.cFileName, NET_SAVE_FILE_NAME) != 0) {
                    ViniferaFileEntryClass entry;
                    if (Read_File(&entry, &find_data) == true) {
                        files_found = true;
                        break;
                    }
                }
            }

            /*
            ** Find the next file
            */
            if (!FindNextFile(hFind, &find_data)) {
                break;
            }
        }
    }

    FindClose(hFind);

    return(files_found);
}


/***********************************************************************************************
 * LoadOptionsClass::Compare -- for qsort                                                      *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      p1,p2      ptrs to elements to compare                                                 *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      0 = same, -1 = (*p1) goes BEFORE (*p2), 1 = (*p1) goes AFTER (*p2)                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   02/14/1995 BR : Created.                                                                  *
 *=============================================================================================*/
int __cdecl ViniferaLoadOptionsClass::Compare(const void* p1, const void* p2)
{
    ViniferaFileEntryClass* fe1, * fe2;

    fe1 = *((ViniferaFileEntryClass**)p1);
    fe2 = *((ViniferaFileEntryClass**)p2);

    int res = CompareFileTime(&fe1->DateTime, &fe2->DateTime);
    return(-res);
}


/**
 *  Opens the "Loading..." window and loads a saved game from the selected file.
 *
 *  @author: ZivDero
 */
bool ViniferaLoadOptionsClass::Load_File(const char* filename)
{
    char formatted_file_name[PATH_MAX];

    //HWND handle = WinDialogClass::Do_Message_Box(Fetch_String(TXT_LOADING), nullptr, nullptr);
    //if (handle) {
    //    WinDialogClass::Display_Dialog(handle);
    //}

    TacticalViewActive = false;
    ScenarioStarted = false;

    _makepath(formatted_file_name, nullptr, Vinifera_SavedGamesDirectory, Filename_From_Path(filename), nullptr);
    const bool result = Load_Game(formatted_file_name);

    //if (handle) {
    //    WinDialogClass::End_Dialog(handle);
    //}

    return result;
}


/**
 *  Opens the "Saving..." window and saves the game to the selected file.
 *
 *  @author: ZivDero
 */
bool ViniferaLoadOptionsClass::Save_File(const char* filename, const char* description)
{
    char formatted_file_name[PATH_MAX];

    //HWND handle = WinDialogClass::Do_Message_Box(Fetch_String(TXT_SAVING_GAME), nullptr, nullptr);
    //if (handle) {
    //    WinDialogClass::Display_Dialog(handle);
    //}

    _makepath(formatted_file_name, nullptr, Vinifera_SavedGamesDirectory, Filename_From_Path(filename), nullptr);
    const bool result = Save_Game(formatted_file_name, description, false);

    //if (handle) {
    //    WinDialogClass::End_Dialog(handle);
    //}

    return result;
}

/**
 *  Deletes the selected saved game.
 *
 *  @author: ZivDero
 */
bool ViniferaLoadOptionsClass::Delete_File(const char* filename)
{
    char formatted_file_name[PATH_MAX];

    _makepath(formatted_file_name, nullptr, Vinifera_SavedGamesDirectory, Filename_From_Path(filename), nullptr);
    return DeleteFileA(formatted_file_name);
}

bool ViniferaLoadOptionsClass::Read_File(ViniferaFileEntryClass* file, WIN32_FIND_DATAA* ff)
{
    char formatted_file_name[PATH_MAX];

	if (file == NULL && ff == NULL) {
		return(false);
	}

    if (std::strcmp(ff->cFileName, NET_SAVE_FILE_NAME) != 0) {

        _makepath(formatted_file_name, nullptr, Vinifera_SavedGamesDirectory, Filename_From_Path(ff->cFileName), nullptr);

        ViniferaSaveVersionInfo saveversion;
        if (Vinifera_Get_Savefile_Info(formatted_file_name, saveversion)) {

            unsigned game_version = saveversion.Get_Internal_Version();
            if (game_version != GameVersion) {
                DEBUG_WARNING("Save file \"%s\" is incompatible! Tiberian Sun: File version 0x%X, Expected version 0x%X.\n", formatted_file_name, game_version, GameVersion);
                return false;
            }

            unsigned vinifera_version = saveversion.Get_Vinifera_Version();
            if (vinifera_version != ViniferaGameVersion) {
                DEBUG_WARNING("Save file \"%s\" is incompatible! Vinifera: File version 0x%X, Expected version 0x%X.\n", formatted_file_name, vinifera_version, ViniferaGameVersion);
                return false;
            }

            wsprintfA(file->Descr, "%s", saveversion.Get_Scenario_Description());
            file->Old = false;
            file->Valid = true;
            file->Scenario = saveversion.Get_Scenario_Number();
            file->Num = saveversion.Get_Campaign_Number();
            file->Type = static_cast<GameEnum>(saveversion.Get_Game_Type());
            std::strncpy(file->Filename, formatted_file_name, std::size(file->Filename));
            std::strncpy(file->PlayerName, saveversion.Get_Player_House(), std::size(file->PlayerName));
            if (std::strlen(file->Filename) == 0) {
                std::strncpy(file->Filename, ff->cAlternateFileName, std::size(file->Filename));
            }
            file->DateTime = ff->ftLastWriteTime;

            return true;
        }
        else {
            DEBUG_WARNING("Failed to read save file \"%s\"!\n", formatted_file_name);
        }
    }
	return(false);
}