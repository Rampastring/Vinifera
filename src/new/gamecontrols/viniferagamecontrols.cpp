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

#include "viniferagamecontrols.h"
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
#include "slider.h"
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


 /***********************************************************************************************
  * OptionsClass::Process -- Handles all the options graphic interface.                         *
  *                                                                                             *
  *    This routine is the main control for the visual representation of the options            *
  *    screen. It handles the visual overlay and the player input.                              *
  *                                                                                             *
  * INPUT:   none                                                                               *
  * OUTPUT:  none                                                                               *
  * WARNINGS:   none                                                                            *
  * HISTORY:                                                                                    *
  *   12/31/1994 MML : Created.                                                                 *
  *=============================================================================================*/
void ViniferaGameControlsClass::Process(void)
{
	/*
	**	Dialog & button dimensions
	*/
	int	d_dialog_w = 464;											// dialog width
	int	d_dialog_h = 282;											// dialog height
	int	d_dialog_x = ((PrimarySurface->Get_Width() - d_dialog_w) / 2);				// dialog x-coord
	int	d_dialog_y = ((PrimarySurface->Get_Height() - d_dialog_h) / 2);				// centered y-coord
	int	d_dialog_cx = d_dialog_x + (d_dialog_w / 2);		// center x-coord
	int	d_top_margin = 50;

	int	d_txt6_h = 12 + 1;												// ht of 6-pt text
	int	d_margin1 = 10;												// large margin
	int	d_margin2 = 4;												// small margin

	int	d_speed_w = d_dialog_w - 68;
	int	d_speed_h = 12;
	int	d_speed_x = d_dialog_x + 34;
	int	d_speed_y = d_dialog_y + d_top_margin + d_margin1 + d_txt6_h;

	int	d_scroll_w = d_dialog_w - 68;
	int	d_scroll_h = 12;
	int	d_scroll_x = d_dialog_x + 34;
	int	d_scroll_y = d_speed_y + d_speed_h + d_txt6_h + (d_margin1 * 2) + d_txt6_h;

	int	d_visual_w = d_dialog_w - 80;
	int	d_visual_h = 18;
	int	d_visual_x = d_dialog_x + 40;
	int	d_visual_y = d_scroll_y + d_scroll_h + d_txt6_h + (d_margin1 * 2);

	int	d_sound_w = d_dialog_w - 80;
	int	d_sound_h = 18;
	int	d_sound_x = d_dialog_x + 40;
	int	d_sound_y = d_visual_y + d_visual_h + d_margin1;

	int	d_ok_w = 40;
	int	d_ok_h = 18;
	int	d_ok_x = d_dialog_cx - (d_ok_w / 2);
	int	d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - d_margin1 - 8;


	enum {
		BUTTON_SPEED = 100,
		BUTTON_SCROLLRATE,
		BUTTON_VISUAL,
		BUTTON_SOUND,
		BUTTON_OK,
		BUTTON_COUNT,
		BUTTON_FIRST = BUTTON_SPEED,
	};

	/*
	**	Dialog variables
	*/
	KeyNumType input;

	int gamespeed = Options.GameSpeed;
	int scrollrate = Options.ScrollRate;
	int selection;
	bool pressed = false;
	int curbutton = 0;
	TextButtonClass* buttons[BUTTON_COUNT - BUTTON_FIRST];
	TextPrintType style;

	ColorScheme* scheme = ColorSchemes[GadgetClass::Get_Color_Scheme()];

	/*
	**	Buttons
	*/
	GadgetClass* commands;										// button list

	SliderClass gspeed_btn(BUTTON_SPEED, d_speed_x, d_speed_y, d_speed_w, d_speed_h, true);
	SliderClass scrate_btn(BUTTON_SCROLLRATE, d_scroll_x, d_scroll_y, d_scroll_w, d_scroll_h, true);
	TextButtonClass visual_btn(BUTTON_VISUAL, "Visual Controls", TPF_BUTTON, d_visual_x, d_visual_y, d_visual_w, d_visual_h, false, false);
	TextButtonClass sound_btn(BUTTON_SOUND, "Sound Controls", TPF_BUTTON, d_sound_x, d_sound_y, d_sound_w, d_sound_h, false, false);
	TextButtonClass okbtn(BUTTON_OK, TXT_OPTIONS_MENU, TPF_BUTTON, d_ok_x, d_ok_y, -1, -1, false, false);
	okbtn.X = (PrimarySurface->Get_Width() - okbtn.Width) / 2;

#ifdef WOLAPI_INTEGRATION
	TextButtonClass wol_btn(BUTTON_WOLAPI, TXT_WOL_OPTTITLE, TPF_BUTTON, d_wol_x, d_wol_y, d_wol_w, d_wol_h);
#endif

	/*
	**	Various Inits.
	*/
	LogicSurface = HiddenSurface;

	/*
	**	Build button list
	*/
	commands = &okbtn;
	gspeed_btn.Add_Tail(*commands);
	scrate_btn.Add_Tail(*commands);
	visual_btn.Add_Tail(*commands);
	sound_btn.Add_Tail(*commands);
#ifdef WOLAPI_INTEGRATION
	if (bShowWolapi)
		wol_btn.Add_Tail(*commands);
#endif
	/*
	**	Init button states
	**	For sliders, the thumb ranges from 0 - (maxval-1), so to convert the
	**	thumb value to a real-world value:
	**		val = (MAX - slider.Get_Value()) - 1;
	**	and,
	**		slider.Set_Value(-(val + 1 - MAX));
	*/
	const int MAX_SPEED_SETTING = 7;
	const int MAX_SCROLL_SETTING = 7;
	gspeed_btn.Set_Maximum(MAX_SPEED_SETTING);	// varies from 0 - 7
	gspeed_btn.Set_Thumb_Size(1);
	gspeed_btn.Set_Value((MAX_SPEED_SETTING - 1) - gamespeed);

	scrate_btn.Set_Maximum(MAX_SCROLL_SETTING);	// varies from 0 - 7
	scrate_btn.Set_Thumb_Size(1);
	scrate_btn.Set_Value((MAX_SCROLL_SETTING - 1) - scrollrate);

	/*
	**	Fill array of button ptrs.
	*/
	buttons[0] = NULL;
	buttons[1] = NULL;
	buttons[2] = &visual_btn;
	buttons[3] = &sound_btn;
	buttons[4] = &okbtn;

	Rect dialogrect = Rect(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

	/*
	**	Processing loop.
	*/
	bool process = true;
	bool display = true;
	bool refresh = true;
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
		// 	AllSurfaces.SurfacesRestored = FALSE;
		// 	display = true;
		// }

		/*
		**	Refresh display if needed.
		*/
		if (display) {
			WWMouse->Hide_Mouse();

			Map.Flag_To_Redraw(true);
			Map.Render();

			Dialog_Box(dialogrect);
			Draw_Caption("Game Controls", Metal12FontPtr, d_dialog_x, d_dialog_y, d_dialog_w);
			WWMouse->Show_Mouse();
			display = false;
			refresh = true;
		}

#define TPF_TEXT TPF_METAL12 | TPF_NOSHADOW

		if (refresh) {
			WWMouse->Hide_Mouse();

			/*
			**	Label the game speed slider
			*/
			style = TPF_METAL12 | TPF_NOSHADOW;
			if (curbutton == (BUTTON_SPEED - BUTTON_FIRST)) {
				style = (TextPrintType)(style | TPF_BRIGHT_COLOR);
			}
			Fancy_Text_Print("Game Speed", LogicSurface, &LogicSurface->Get_Rect(), &Point2D(d_speed_x, d_speed_y - d_txt6_h), scheme, TBLACK, style);

			Fancy_Text_Print(TXT_SLOWER, LogicSurface, &LogicSurface->Get_Rect(), &Point2D(d_speed_x, d_speed_y + d_speed_h + 2), scheme, TBLACK, style);
			Fancy_Text_Print(TXT_FASTER, LogicSurface, &LogicSurface->Get_Rect(), &Point2D(d_speed_x + d_speed_w, d_speed_y + d_speed_h + 2), scheme, TBLACK, style | TPF_RIGHT);

			/*
			**	Label the scroll rate slider
			*/
			style = TPF_TEXT;
			if (curbutton == (BUTTON_SCROLLRATE - BUTTON_FIRST)) {
				style = (TextPrintType)(style | TPF_BRIGHT_COLOR);
			}
			Fancy_Text_Print("Scroll Rate", LogicSurface, &LogicSurface->Get_Rect(), &Point2D(d_scroll_x, d_scroll_y - d_txt6_h), scheme, TBLACK, style);

			Fancy_Text_Print(TXT_SLOWER, LogicSurface, &LogicSurface->Get_Rect(), &Point2D(d_scroll_x, d_scroll_y + d_scroll_h + 2), scheme, TBLACK, TPF_TEXT);
			Fancy_Text_Print(TXT_FASTER, LogicSurface, &LogicSurface->Get_Rect(), &Point2D(d_scroll_x + d_scroll_w, d_scroll_y + d_scroll_h + 2), scheme, TBLACK, TPF_TEXT | TPF_RIGHT);

			commands->Draw_All();

			WWMouse->Show_Mouse();
			refresh = false;
		}

		/*
		**	Get user input.
		*/
		input = commands->Input();

		/*
		**	Process input.
		*/
		switch (input) {
		case (BUTTON_SPEED | KN_BUTTON):
			curbutton = (BUTTON_SPEED - BUTTON_FIRST);
			refresh = true;
			break;

		case (BUTTON_SCROLLRATE | KN_BUTTON):
			curbutton = (BUTTON_SCROLLRATE - BUTTON_FIRST);
			refresh = true;
			break;

		case (BUTTON_VISUAL | KN_BUTTON):
			selection = BUTTON_VISUAL;
			pressed = true;
			break;

		case (BUTTON_SOUND | KN_BUTTON):
			selection = BUTTON_SOUND;
			pressed = true;
			break;

		case (BUTTON_OK | KN_BUTTON):
			selection = BUTTON_OK;
			pressed = true;
			break;

#ifdef WOLAPI_INTEGRATION
		case (BUTTON_WOLAPI | KN_BUTTON):
			selection = BUTTON_WOLAPI;
			pressed = true;
			break;
#endif

		case (KN_ESC):
			process = false;
			break;

		case (KN_LEFT):
			if (curbutton == (BUTTON_SPEED - BUTTON_FIRST)) {
				gspeed_btn.Bump(1);
			}
			else
				if (curbutton == (BUTTON_SCROLLRATE - BUTTON_FIRST)) {
					scrate_btn.Bump(1);
				}
			break;

		case (KN_RIGHT):
			if (curbutton == (BUTTON_SPEED - BUTTON_FIRST)) {
				gspeed_btn.Bump(0);
			}
			else
				if (curbutton == (BUTTON_SCROLLRATE - BUTTON_FIRST)) {
					scrate_btn.Bump(0);
				}
			break;

		case (KN_UP):
			if (buttons[curbutton]) {
				buttons[curbutton]->Turn_Off();
				buttons[curbutton]->Flag_To_Redraw();
			}

			curbutton--;
#ifdef WOLAPI_INTEGRATION
			if (!bShowWolapi)
			{
				if (curbutton == BUTTON_WOLAPI - BUTTON_FIRST)
					curbutton--;		//	Skip over missing button.
			}
#endif
			if (curbutton < 0) {
				curbutton = (BUTTON_COUNT - BUTTON_FIRST - 1);
			}

			if (buttons[curbutton]) {
				buttons[curbutton]->Turn_On();
				buttons[curbutton]->Flag_To_Redraw();
			}
			refresh = true;
			break;

		case (KN_DOWN):
			if (buttons[curbutton]) {
				buttons[curbutton]->Turn_Off();
				buttons[curbutton]->Flag_To_Redraw();
			}

			curbutton++;
#ifdef WOLAPI_INTEGRATION
			if (!bShowWolapi)
			{
				if (curbutton == BUTTON_WOLAPI - BUTTON_FIRST)
					curbutton++;		//	Skip over missing button.
			}
#endif
			if (curbutton > (BUTTON_COUNT - BUTTON_FIRST - 1)) {
				curbutton = 0;
			}

			if (buttons[curbutton]) {
				buttons[curbutton]->Turn_On();
				buttons[curbutton]->Flag_To_Redraw();
			}
			refresh = true;
			break;

		case (KN_RETURN):
			selection = curbutton + BUTTON_FIRST;
			pressed = true;
			break;

		default:
			break;
		}

		/*
		**	Perform some action. Either to exit the dialog or bring up another.
		*/
		if (pressed) {

			/*
			**	Record the new options slider settings.
			** The GameSpeed data member MUST NOT BE SET HERE!  It will cause multiplayer
			** games to go out of sync.  It's set by virtue of the event being executed.
			*/
			if (gamespeed != ((MAX_SPEED_SETTING - 1) - gspeed_btn.Get_Value())) {
				gamespeed = (MAX_SPEED_SETTING - 1) - gspeed_btn.Get_Value();
				OutList.Add(EventClass::EventClass(PlayerPtr->ID, EVENT_GAMESPEED, gamespeed));
			}

			if (scrollrate != ((MAX_SCROLL_SETTING - 1) - scrate_btn.Get_Value())) {
				scrollrate = (MAX_SCROLL_SETTING - 1) - scrate_btn.Get_Value();
				Options.ScrollRate = scrollrate;
			}
			process = false;

			/*
			** Save the settings in such a way that the GameSpeed is only set during
			** the save process; restore it when we're done, so multiplayer games don't
			** go out of sync.
			*/
			if (Session.Singleplayer_Game()) {
				Options.GameSpeed = gamespeed;
				Options.Save_Settings();			// save new value
			}
			else {
				int old = Options.GameSpeed;		// save orig value
				Options.GameSpeed = gamespeed;
				Options.Save_Settings();			// save new value
				Options.GameSpeed = old;			// restore old value
			}

			/*
			**	Possibly launch into another dialog if so directed.
			*/
			switch (selection) {
			case (BUTTON_VISUAL):
				// VisualControlsClass().Process();
				// process = true;
				// display = true;
				// refresh = true;
				break;

			case (BUTTON_SOUND):
				// if (!SoundType) {
				// 	ViniferaMessageBox().Process(Text_String(TXT_NO_SOUND_CARD));
				// 	process = true;
				// 	display = true;
				// 	refresh = true;
				// }
				// else {
				// 	SoundControlsClass().Process();
				// 	process = true;
				// 	display = true;
				// 	refresh = true;
				// }
				break;

#ifdef WOLAPI_INTEGRATION
			case BUTTON_WOLAPI:
				if (WOL_Options_Dialog(pWolapi, true))
				{
					//	The game ended while in this dialog.
					process = false;
				}
				else
				{
					process = true;
					display = true;
					refresh = true;
				}
				break;
#endif

			case (BUTTON_OK):
				break;
			}

			pressed = false;
		}

		if (!display && process) {
			CompositeSurface->Copy_From(dialogrect, *LogicSurface, dialogrect);
			WWMouse->Show_Mouse();
			GScreenClass::Blit(true, CompositeSurface);
		}
	}
}