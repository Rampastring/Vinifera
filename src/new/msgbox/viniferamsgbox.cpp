/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera
 *
 *  @file          VINIFERAMSGBOX.CPP
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

#include "viniferamsgbox.h"
#include "tibsun_defines.h"
#include "tibsun_functions.h"
#include "dsurface.h"
#include "xsurface.h"
#include "gscreen.h"
#include "textbtn.h"
#include "textprint.h"
#include "wwmouse.h"
#include "wwfont.h"
#include "drawshape.h"

ViniferaMessageBox::ViniferaMessageBox(int caption) { Caption = caption; }

void Dialog_Box(Rect rect)
{
	LogicSurface->Fill_Rect(rect, 100);

	// Calculate dialog center points.
	int cx = rect.X + rect.Width / 2;
	int cy = rect.Y + rect.Height / 2;

	void const* shapedata = MixFileClass::Retrieve("DD-BKGND.SHP");
	if (shapedata != nullptr) {
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 0, Point2D(cx - 312, cy - 192), LogicSurface->Get_Rect());
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 1, Point2D(cx, cy - 192), LogicSurface->Get_Rect());
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 2, Point2D(cx - 312, cy), LogicSurface->Get_Rect());
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 3, Point2D(cx, cy), LogicSurface->Get_Rect());
	}

	/*
    **	Draw the side strips.
    */
	shapedata = MixFileClass::Retrieve("DD-EDGE.SHP");
	if (shapedata != nullptr) {
		for (int yy = 0; yy < rect.Height; yy += 6) {
			Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 0, Point2D(rect.X + 14, yy), LogicSurface->Get_Rect());
			Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 1, Point2D(rect.X + rect.Width - (14 + 16), yy), LogicSurface->Get_Rect());
		}
	}

	/*
    **	Draw the border bars.
    */
	shapedata = MixFileClass::Retrieve("DD-LEFT.SHP");
	if (shapedata != nullptr) {
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 0, Point2D(rect.X, cy - 200), LogicSurface->Get_Rect());
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 0, Point2D(rect.X, cy), LogicSurface->Get_Rect());
	}

	int rightx = rect.X + rect.Width - 14;
	shapedata = MixFileClass::Retrieve("DD-RIGHT.SHP");
	if (shapedata != nullptr) {
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 0, Point2D(rightx, cy - 200), LogicSurface->Get_Rect());
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 0, Point2D(rightx, cy), LogicSurface->Get_Rect());
	}

	shapedata = MixFileClass::Retrieve("DD-BOTM.SHP");
	if (shapedata != nullptr) {
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 0, Point2D(cx - 320, rect.Y + rect.Height - 16), LogicSurface->Get_Rect());
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 0, Point2D(cx, rect.Y + rect.Height - 16), LogicSurface->Get_Rect());
	}

	shapedata = MixFileClass::Retrieve("DD-TOP.SHP");
	if (shapedata != nullptr) {
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 0, Point2D(cx - 320, rect.Y), LogicSurface->Get_Rect());
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 0, Point2D(cx, rect.Y), LogicSurface->Get_Rect());
	}

	/*
	**	Draw the corner caps.
	*/
	shapedata = MixFileClass::Retrieve("DD-CRNR.SHP");
	if (shapedata != nullptr) {
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 0, Point2D(rect.X, rect.Y), LogicSurface->Get_Rect());
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 1, Point2D(rect.X + rect.Width - (24 - 1), rect.Y), LogicSurface->Get_Rect());
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 2, Point2D(rect.X, rect.Y + rect.Height - 24), LogicSurface->Get_Rect());
		Draw_Shape(*LogicSurface, *SidebarDrawer, (ShapeSet*)shapedata, 3, Point2D(rect.X + rect.Width - (24 - 1), rect.Y + rect.Height - 24), LogicSurface->Get_Rect());
	}
}

#define TPF_BUTTON	(TPF_CENTER|TPF_6PT_GRAD|TPF_NOSHADOW)
#define	BUTTON_1		1
#define	BUTTON_2		2
#define	BUTTON_3		3
#define	BUTTON_FLAG	0x8000
int ViniferaMessageBox::Process(const char* msg, const char* b1txt, const char* b2txt, const char* b3txt, bool preserve)
{
	char buffer[1024];
	int retval = 0;
	bool process = true;								// loop while true
	int selection;
	bool pressed;
	int curbutton;
	TextButtonClass* buttons[3];
	void* back;
	bool display = true;									// display level
	int  realval[5];
	WWFontClass* font = Metal12FontPtr;

	DSurface seen_buff_save(PrimarySurface->Get_Width(), PrimarySurface->Get_Height(), true);

	if (b1txt != NULL && *b1txt == '\0') b1txt = NULL;
	if (b2txt != NULL && *b2txt == '\0') b2txt = NULL;
	if (b3txt != NULL && *b3txt == '\0') b3txt = NULL;

	/*
	**	Examine the optional button parameters. Fetch the width and starting
	**	characters for each.
	*/
	int bwidth, bheight;				// button width and height
	int numbuttons = 0;
	if (b1txt != NULL) {

		/*
		**	Build the button list.
		*/
		bheight = font->Get_Font_Height() + font->Get_Y_Spacing() + 4;
		bwidth = std::max(font->String_Pixel_Width(b1txt) + 16, 60);

		if (b2txt != NULL) {
			numbuttons = 2;
			bwidth = std::max(font->String_Pixel_Width(b2txt) + 16, bwidth);

			if (b3txt != NULL) {
				numbuttons = 3;
			}

		}
		else {
			numbuttons = 1;
		}
	}

	/*
	**	Determine the dimensions of the text to be used for the dialog box.
	**	These dimensions will control how the dialog box looks.
	*/
	buffer[ARRAY_SIZE(buffer) - 1] = 0;
	strncpy(buffer, msg, ARRAY_SIZE(buffer) - 1);

	int width;
	int height;
	int lines = Format_Window_String(buffer, font, 510, width, height);
	TextPrintType tpf = TPF_METAL12 | TPF_NOSHADOW;

	width = std::max(width, 180);
	width += 80;
	height += (numbuttons == 0) ? 80 : 120;

	int x = (AlternateSurface->Get_Width() - width) / 2;
	int y = (AlternateSurface->Get_Height() - height) / 2;
	int printx = x + 40;

	/*
	**	Special hack to center a one line dialog box text.
	*/
	if (lines == 1) {
		printx = x + width / 2;
		tpf = tpf | TPF_CENTER;
	}

	/*
	**	Initialize the button structures. All are initialized, even though one (or none) may
	**	actually be added to the button list.
	*/
	TextButtonClass button1(BUTTON_1, b1txt, TPF_BUTTON,
		x + ((numbuttons == 1) ? ((width - bwidth) >> 1) : 40), y + height - (bheight + 30), bwidth, -1, false, false);

	/*
	**	Center button.
	*/
	TextButtonClass button2(BUTTON_2, b2txt, TPF_BUTTON,
		x + width - (bwidth + 40), y + height - (bheight + 30), bwidth, -1, false, false);

	/*
	**	Right button.
	*/
	TextButtonClass button3(BUTTON_3, b3txt, TPF_BUTTON, 0, y + height - (bheight + 30), -1, -1, false, false);
	button3.X = x + ((width - button3.Width) >> 1);

	TextButtonClass* buttonlist = 0;
	curbutton = 0;

	/*
	**	Add and initialize the buttons to the button list.
	*/
	memset(buttons, '\0', sizeof(buttons));
	if (numbuttons > 0) {
		buttonlist = &button1;
		buttons[0] = &button1;
		realval[0] = BUTTON_1;
		if (numbuttons > 2) {
			button3.Add(*buttonlist);
			buttons[1] = &button3;
			realval[1] = BUTTON_3;
			button2.Add(*buttonlist);
			buttons[2] = &button2;
			realval[2] = BUTTON_2;
			buttons[curbutton]->Turn_On();
		}
		else {
			if (numbuttons == 2) {
				button2.Add(*buttonlist);
				buttons[1] = &button2;
				realval[1] = BUTTON_2;
				buttons[curbutton]->Turn_On();
			}
		}
	}

	Rect rect = Rect(x, y, width, height);

	/*
	**	Main Processing Loop.
	*/
	if (buttonlist) {
		process = true;
		pressed = false;
		while (process) {

			/*
			** If we have just received input focus again after running in the background then
			** we need to redraw.
			*/
			// if (AllSurfaces.SurfacesRestored) {
			// 	AllSurfaces.SurfacesRestored = false;
			// 	seen_buff_save. Blit(VisiblePage);
			// 	display = true;
			// }

			if (display) {
				display = false;

				WWMouse->Hide_Mouse();

				Dialog_Box(rect);				
				//Draw_Caption(Caption, x, y, width);

				/*
				**	Draw the body of the message.
				*/
				Fancy_Text_Print(buffer, LogicSurface, &LogicSurface->Get_Rect(), &Point2D(printx, y + 40), ColorSchemes[GadgetClass::Get_Color_Scheme()], COLOR_TBLACK, tpf);

				/*
				**	Redraw the buttons.
				*/
				if (buttonlist) {
					buttonlist->Draw_All();
				}				
			}

			/*
			**	Invoke game callback.
			*/
			Call_Back();

			/*
			**	Fetch and process input.
			*/
			KeyNumType input = buttonlist->Input();

			switch (input) {
			case (KN_ESC):
				selection = realval[numbuttons - 1];
				pressed = true;
				break;

			case (BUTTON_1 | BUTTON_FLAG):
				selection = realval[0];
				pressed = true;
				break;

			case (BUTTON_2 | BUTTON_FLAG):
				if (numbuttons > 2) {
					selection = realval[2];
				}
				else {
					selection = realval[1];
				}
				pressed = true;
				break;

			case (BUTTON_3 | BUTTON_FLAG):
				selection = realval[1];
				pressed = true;
				break;

			case (KN_LEFT):
				if (numbuttons > 1) {
					buttons[curbutton]->Turn_Off();
					buttons[curbutton]->Flag_To_Redraw();

					curbutton--;
					if (curbutton < 0) {
						curbutton = numbuttons - 1;
					}

					buttons[curbutton]->Turn_On();
					buttons[curbutton]->Flag_To_Redraw();
				}
				break;

			case (KN_RIGHT):
				if (numbuttons > 1) {
					buttons[curbutton]->Turn_Off();
					buttons[curbutton]->Flag_To_Redraw();

					curbutton++;
					if (curbutton > (numbuttons - 1)) {
						curbutton = 0;
					}

					buttons[curbutton]->Turn_On();
					buttons[curbutton]->Flag_To_Redraw();
				}
				break;

			case (KN_RETURN):
				selection = realval[curbutton];
				pressed = true;
				break;

				/*
				**	Check 'input' to see if it's the 1st char of button text
				*/
			default:
				break;
			}

			if (pressed) {

				TextButtonClass* toggle;
				/*
				**	Turn all the buttons off.
				*/
				toggle = (TextButtonClass*)buttonlist->Extract_Gadget(BUTTON_1);
				if (toggle != NULL) {
					toggle->Turn_Off();
					toggle->IsPressed = false;
				}
				toggle = (TextButtonClass*)buttonlist->Extract_Gadget(BUTTON_2);
				if (toggle != NULL) {
					toggle->Turn_Off();
					toggle->IsPressed = false;
				}
				toggle = (TextButtonClass*)buttonlist->Extract_Gadget(BUTTON_3);
				if (toggle != NULL) {
					toggle->Turn_Off();
					toggle->IsPressed = false;
				}

				/*
				**	Turn on and depress the button that was selected.
				*/
				if (selection == BUTTON_1 || selection == BUTTON_2 || selection == BUTTON_3) {
					TextButtonClass* toggle = (TextButtonClass*)buttonlist->Extract_Gadget(selection);
					if (toggle != NULL) {
						toggle->Turn_On();
						toggle->IsPressed = true;
					}
				}
				
				WWMouse->Hide_Mouse();
				buttonlist->Draw_All(true);
				WWMouse->Show_Mouse();

				switch (selection) {
				case (BUTTON_1):
					retval = 0;
					process = false;
					break;

				case (BUTTON_2):
					retval = 1;
					process = false;
					break;

				case BUTTON_3:
					retval = 2;
					process = false;
					break;
				}

				pressed = false;
			}

			CompositeSurface->Copy_From(rect, *LogicSurface, rect);
			WWMouse->Show_Mouse();
			GScreenClass::Blit(true, CompositeSurface);
		}

	}
	else {

		WWKeyboard->Clear();
	}

	/*
	**	Restore the screen if necessary.
	*/
	// if (preserve) {
	// 	Hide_Mouse();
	// 	if (SeenBuff.Lock()) {
	// 		Buffer_To_Page(x, y, width, height, back, &SeenBuff);
	// 	}
	// 	SeenBuff.Unlock();
	// 
	// 	delete[] back;
	// 	back = NULL;
	// 	Show_Mouse();
	// }
	return(retval);
}