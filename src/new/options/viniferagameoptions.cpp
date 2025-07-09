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

#include "viniferagameoptions.h"
#include "viniferamsgbox.h"
#include "viniferaloadoptions.h"
#include "tibsun_globals.h"
#include "tibsun_defines.h"
#include "tibsun_functions.h"
#include "dsurface.h"
#include "xsurface.h"
#include "event.h"
#include "gscreen.h"
#include "house.h"
#include "textbtn.h"
#include "textprint.h"
#include "options.h"
#include "session.h"
#include "wwmouse.h"
#include "mouse.h"
#include "wwfont.h"
#include "map.h"
#include "hooker.h"
#include "hooker_macros.h"
#include "drawshape.h"


#define TPF_BUTTON	(TPF_CENTER|TPF_6PT_GRAD|TPF_NOSHADOW)
#define	BUTTON_1		1
#define	BUTTON_2		2
#define	BUTTON_3		3
#define	BUTTON_FLAG	0x8000
void ViniferaGameOptionsClass::Process()
{
	WWFontClass* font = Metal12FontPtr;

	static struct {
		int ID;				// Button ID to use.
		char* Text;			// Text number to use for this button.
		bool Multiplay;	    // Allowed in multiplayer?
	} _constants[] = {
		{BUTTON_GAME,  	"Game Controls",   true},
		{BUTTON_RESTATE, "Restate Mission", false},
		{BUTTON_LOAD,  	"Load Game",    false},
		{BUTTON_SAVE,  	"Save Game",    true},
		{BUTTON_DELETE,	"Delete Saved Game",  false},
		{BUTTON_QUIT,  	"Abort Mission",    true},
		{BUTTON_RESUME,	"Resume Mission",  true},
	};

	/*
	**	Variables.
	*/
	TextButtonClass* buttons = 0;
	int selection;
	bool pressed;
	int curbutton = 6;
	int y;
	TextButtonClass* buttonsel[ARRAY_SIZE(_constants)];
	static int num_buttons = sizeof(_constants) / sizeof(_constants[0]);
	
	/*
    **	Build the button list for all of the buttons for this dialog.
    */
	int maxwidth = 0;
	int numbuttons = 0;

	for (int index = 0; index < num_buttons; index++) {
		char* text = _constants[index].Text;
		buttonsel[index] = NULL;

		if (!Session.Singleplayer_Game() && !_constants[index].Multiplay) {
			continue;
		}

		y = GameOptionsEnum::BUTTON_Y + (GameOptionsEnum::BUTTON_HEIGHT*numbuttons);

		TextButtonClass* g = new TextButtonClass(_constants[index].ID, text, TPF_BUTTON, 0, y, -1, -1, false, false);
		numbuttons++;

		if (g->Width > maxwidth) {
			maxwidth = g->Width;
		}
		if (buttons == NULL) {
			buttons = g;
		}
		else {
			g->Add_Tail(*buttons);
		}

		buttonsel[index] = g;

		if (_constants[index].ID == BUTTON_RESTATE && Session.Type == GAME_SKIRMISH) {
			g->Disable();
		}
	}

	while (!buttonsel[curbutton - 1] || !buttonsel[curbutton - 1]->Is_Enabled())
		curbutton--;

	buttonsel[curbutton - 1]->Turn_On();

	int dialogx = (PrimarySurface->Get_Width() - GameOptionsEnum::OPTION_WIDTH) / 2;
	int dialogheight = GameOptionsEnum::BUTTON_Y + (GameOptionsEnum::BUTTON_HEIGHT * numbuttons) + GameOptionsEnum::BORDER2_LEN;
	int dialogy = (PrimarySurface->Get_Height() - dialogheight) / 2;

	/*
    **	Force all button lengths to match the maximum length of the widest button.
    */
	GadgetClass* g = buttons;
	while (g != NULL) {
		g->Width = std::max(maxwidth, 180);
		g->Height = GameOptionsEnum::BUTTON_HEIGHT - 2;
		g->X = dialogx + (GameOptionsEnum::OPTION_WIDTH - g->Width) / 2;
		g->Y = dialogy + g->Y;
		g = g->Get_Next();
	}

	/*
	**	This causes left mouse button clicking within the confines of the dialog to
	**	be ignored if it wasn't recognized by any other button or slider.
	*/
	(new GadgetClass(dialogx, dialogy, GameOptionsEnum::OPTION_WIDTH, dialogheight, GadgetClass::LEFTPRESS))->Add_Tail(*buttons);

	/*
	**	This causes a right click anywhere or a left click outside the dialog region
	**	to be equivalent to clicking on the return to game button.
	*/
	(new ControlClass(BUTTON_RESUME, 0, 0, CompositeSurface->Get_Width(), CompositeSurface->Get_Height(), GadgetClass::LEFTPRESS | GadgetClass::RIGHTPRESS))->Add_Tail(*buttons);

	WWKeyboard->Clear();

	Rect rect = Rect(dialogx, dialogy, GameOptionsEnum::OPTION_WIDTH, dialogheight);

	/*
    **	Main Processing Loop.
    */
	bool display = true;
	bool process = true;
	pressed = false;
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
			}
		}

		// /*
		// ** If we have just received input focus again after running in the background then
		// ** we need to redraw.
		// */
		// if (AllSurfaces.SurfacesRestored) {
		// 	AllSurfaces.SurfacesRestored = false;
		// 	display = true;
		// }

		/*
		**	Refresh display if needed.
		*/
		if (display /* || RedrawOptionsMenu*/) {

			/*
			**	Redraw the map.
			*/
			// HidPage.Clear();
			Map.Flag_To_Redraw(true);
			Map.Render();

			/*
			**	Reset up the window.  Window x-coords are in bytes not pixels.
			*/
			// Set_Window(WINDOW_EDITOR, OptionX, OptionY, OptionWidth, OptionHeight);
			WWMouse->Hide_Mouse();

			/*
			**	Draw the background.
			*/
			Dialog_Box(rect);

			/*
			**	Draw the arrows border if requested.
			*/
			// Draw_Caption(TXT_OPTIONS, OptionX, OptionY, OptionWidth);

			/*
			**	Display the version number at the bottom of the dialog box.
			*/
#if (0)//PG
			Fancy_Text_Print("%s\rV%s",
				(OptionX + OptionWidth) - (25 * RESFACTOR),
				OptionY + OptionHeight - ((Session.Type == GAME_NORMAL) ? (32 * RESFACTOR) : (24 * RESFACTOR)),
				GadgetClass::Get_Color_Scheme(), TBLACK,
				TPF_EFNT | TPF_NOSHADOW | TPF_RIGHT,
				Scen.ScenarioName,
				Version_Name());
#endif

			buttons->Draw_All();
			// TabClass::Hilite_Tab(0);
			WWMouse->Show_Mouse();
			display = false;
			// RedrawOptionsMenu = false;
		}

		/*
		**	Get user input.
		*/
		KeyNumType input = buttons->Input();

		/*
		**	Process Input.
		*/
		switch (input) {
		case (BUTTON_RESTATE | KN_BUTTON):
			selection = BUTTON_RESTATE;
			pressed = true;
			break;

		case (BUTTON_LOAD | KN_BUTTON):
			selection = BUTTON_LOAD;
			pressed = true;
			break;

		case (BUTTON_SAVE | KN_BUTTON):
			selection = BUTTON_SAVE;
			pressed = true;
			break;

		case (BUTTON_DELETE | KN_BUTTON):
			selection = BUTTON_DELETE;
			pressed = true;
			break;

		case (BUTTON_QUIT | KN_BUTTON):
			selection = BUTTON_QUIT;
			pressed = true;
			break;

		case (BUTTON_GAME | KN_BUTTON):
			selection = BUTTON_GAME;
			pressed = true;
			break;

		case (KN_ESC):
		case (BUTTON_RESUME | KN_BUTTON):
			selection = BUTTON_RESUME;
			pressed = true;
			break;

		case (KN_UP):
			buttonsel[curbutton - 1]->Turn_Off();
			buttonsel[curbutton - 1]->Flag_To_Redraw();
			do {
				curbutton--;
				if (curbutton < 1) curbutton = num_buttons;
			} while (!buttonsel[curbutton - 1]);

			buttonsel[curbutton - 1]->Turn_On();
			buttonsel[curbutton - 1]->Flag_To_Redraw();
			break;

		case (KN_DOWN):
			buttonsel[curbutton - 1]->Turn_Off();
			buttonsel[curbutton - 1]->Flag_To_Redraw();
			do {
				curbutton++;
				if (curbutton > num_buttons) curbutton = 1;
			} while (!buttonsel[curbutton - 1]);

			buttonsel[curbutton - 1]->Turn_On();
			buttonsel[curbutton - 1]->Flag_To_Redraw();
			break;

		case (KN_RETURN):
			buttonsel[curbutton - 1]->IsPressed = true;
			buttonsel[curbutton - 1]->Draw_Me(true);
			selection = curbutton;
			pressed = true;
			WWKeyboard->Clear();
			break;

		default:
			break;
		}

		if (pressed) {

			buttonsel[curbutton - 1]->Turn_Off();
			buttonsel[curbutton - 1]->Flag_To_Redraw();
			curbutton = selection;
			buttonsel[curbutton - 1]->Turn_On();
			buttonsel[curbutton - 1]->Flag_To_Redraw();

			switch (selection) {
			case BUTTON_RESTATE:
				display = true;
				// if (!Restate_Mission(Scen.ScenarioName, TXT_VIDEO, TXT_RESUME_MISSION/*KOTXT_OPTIONS*/)) {
				// 	BreakoutAllowed = true;
				// 	Play_Movie(Scen.BriefMovie);
				// 	BlackPalette.Adjust(0x08, WhitePalette);
				// 	BlackPalette.Set();
				// 	BlackPalette.Adjust(0xFF);
				// 	BlackPalette.Set();
				// 	GamePalette.Set();
				// 
				// 	Map.Flag_To_Redraw(true);
				// 	Theme.Queue_Song(THEME_PICK_ANOTHER);
				// 	process = false;
				// }
				// else {
				// 	BlackPalette.Adjust(0x08, WhitePalette);
				// 	BlackPalette.Set();
				// 	BlackPalette.Adjust(0xFF);
				// 	BlackPalette.Set();
				// 	GamePalette.Set();
				// 	Map.Flag_To_Redraw(true);
				// 	process = false;
				// }
				break;

			case (BUTTON_LOAD):
				display = true;
				if (ViniferaLoadOptionsClass::ViniferaLoadOptionsClass().Load()) {
					process = false;
				}
				break;

			case (BUTTON_SAVE):
				display = true;
				if (Session.Singleplayer_Game()) {
					ViniferaLoadOptionsClass::ViniferaLoadOptionsClass().Save(Scen->Description);
				}
				else {
					OutList.Add(EventClass::EventClass(PlayerPtr->ID, EVENT_SAVEGAME));
					process = false;
				}
				break;

			case (BUTTON_DELETE):
				display = true;
				if (Session.Singleplayer_Game()) {
					ViniferaLoadOptionsClass::ViniferaLoadOptionsClass().Delete();
				}
				break;

			case (BUTTON_QUIT):
				if (Session.Type == GAME_NORMAL) {
					switch (ViniferaMessageBox().Process("Are you sure you want to abort the mission?", "Abort", "Cancel", "Restart")) {
					case 1:
						display = true;
						break;

					case 0:
						process = false;
						Queue_Exit();
						break;

					case 2:
						PlayerRestarts = true;
						process = false;
						break;
					}
				}
				else if (Session.Type == GAME_SKIRMISH) {
					switch (ViniferaMessageBox().Process("Are you sure you want to abort the mission?", "Abort", "Cancel", "Surrender")) {
					case 1:
						display = true;
						break;

					case 0:
						process = false;
						Queue_Exit();
						break;

					case 2:
						PlayerPtr->MPlayer_Defeated();
						process = false;
						break;
					}
				}
				else {
					if (PlayerPtr->IsDefeated) {
						switch (ViniferaMessageBox().Process("Are you sure you want to abort the mission?", "Abort", "Cancel")) {
						case 1:
							display = true;
							break;

						case 0:
							process = false;
							Queue_Exit();
							break;
						}
					}
					else {
						switch (ViniferaMessageBox().Process("Surrender?", "Yes", "Cancel")) {
						case 1:
							OutList.Add(EventClass::EventClass(PlayerPtr->HeapID, EVENT_DESTRUCT));
							display = false;
							break;

						case 0:
							process = false;
							break;
						}
					}
				}
				break;

			case (BUTTON_GAME):
				display = true;
				// GameControlsClass().Process();
				break;

			case (BUTTON_RESUME):
				Save_Settings();
				process = false;
				display = true;
				break;
			}

			pressed = false;
			buttonsel[curbutton - 1]->IsPressed = false;
			buttonsel[curbutton - 1]->Turn_Off();
			buttonsel[curbutton - 1]->Flag_To_Redraw();

			if (process) {
				buttons->Draw_All(true);
			}
		}

		if (!display && process) {
			CompositeSurface->Copy_From(rect, *LogicSurface, rect);
			WWMouse->Show_Mouse();
			GScreenClass::Blit(true, CompositeSurface);
		}
	}

	/*
	**	Clean up and re-enter the game.
	*/
	buttons->Delete_List();

	/*
	**	Redraw the map.
	*/
	WWKeyboard->Clear();
	Map.Flag_To_Redraw(true);
	Map.Render();
}

void Show_Vinifera_Game_Options_Dialog()
{
	ViniferaGameOptionsClass().Process();
}

void ViniferaGameOptionsClass_Hooks()
{
	Patch_Call(0x004626BC, &Show_Vinifera_Game_Options_Dialog);
}