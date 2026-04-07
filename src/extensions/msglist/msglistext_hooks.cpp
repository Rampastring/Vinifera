/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera
 *
 *  @file          MSGLISTEXT_HOOKS.CPP
 *
 *  @author        CCHyper
 *
 *  @brief         Contains the hooks for the extended message input function.
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
#include "msglistext_hooks.h"
#include "vinifera_globals.h"
#include "tibsun_globals.h"
#include "session.h"
#include "msglist.h"
#include "house.h"
#include "housetype.h"
#include "uicontrol.h"
#include "rules.h"
#include "fatal.h"
#include "optionsext.h"
#include "gadget.h"
#include "txtlabel.h"
#include "debughandler.h"
#include "asserthandler.h"

#include "hooker.h"

class MessageListClassExt final : public MessageListClass
{
public:
    void MessageListClassExt::Set_Width(int width);
};


/**
 *  Shrinks the width of the message list to accommodate for its moved position.
 *
 *  Author: Rampastring
 */
DECLARE_PATCH(_MessageListClass_Init_Modify_Width_Patch)
{
    GET_REGISTER_STATIC(MessageListClass *, this_ptr, esi);
    GET_REGISTER_STATIC(int, width, eax);
    static int posx;

    posx = 0;
    if (UIControls != nullptr) {
        posx = UIControls->MessageListPositionX;
    }

    width -= posx;
    width -= 5;

    DEBUG_INFO("MessageListClass::Init(Width: %d)\n", width);

    _asm { xor ebx, ebx }
    _asm { mov edi, [esi] }
    JMP(0x00572EC4);
}


/**
 *  Shrinks the width of the message list to accommodate for its moved position.
 *
 *  Author: original RE by tomsons26/ZivDero, modified by Rampastring
 */
void MessageListClassExt::Set_Width(int width)
{
    // It'd be technically cleaner to modify the callsites instead,
    // but we have all of them reimplemented yet
    if (UIControls != nullptr) {
        if (OptionsExtension == nullptr || !OptionsExtension->IsClassicMessagePosition)
        {
            width = width - UIControls->MessageListPositionX;
        }
    }

    GadgetClass* gadg;

    width = width - 8;
    Width = width;
    DEBUG_INFO("MessageListClass::Set_Width(%d)\n", width);

    if (MessageList) {
        gadg = MessageList;
        while (gadg) {
            ((TextLabelClass*)gadg)->PixWidth = width;
            gadg = (GadgetClass*)gadg->Get_Next();
        }
    }

    if (IsEdit) {
        EditLabel->PixWidth = width;
    }
}


/**
 *  Main function for patching the hooks.
 */
void MessageListClassExtension_Hooks()
{
    // Replace the message format to add a space after the semicolon after the message author's name.
    Patch_Dword(0x00573161 + 1, reinterpret_cast<uintptr_t>(&"%s: %s"));
    Patch_Jump(0x00572EAC, &_MessageListClass_Init_Modify_Width_Patch);
    Patch_Jump(0x00573EF0, &MessageListClassExt::Set_Width);
}
