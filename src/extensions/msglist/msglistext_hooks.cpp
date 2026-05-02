/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Contains the hooks for the extended message input function.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#include "always.h"

#include "msglistext_hooks.h"

#include "debughandler.h"
#include "extension_globals.h"
#include "hooker.h"
#include "msglist.h"
#include "optionsext.h"
#include "syringe.h"
#include "txtlabel.h"
#include "uicontrol.h"

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
DEFINE_HOOK(0x00572EAC, _MessageListClass_Init_Modify_Width, 0)
{
    GET(MessageListClass*, this_ptr, ESI);
    GET(int, width, EAX);

    int posx = 0;
    if (UIControls != nullptr) {
        posx = UIControls->MessageListPositionX;
    }

    width -= posx;
    width -= 5;

    this_ptr->Width = width;

    DEBUG_INFO("MessageListClass::Init(Width: %d)\n", width);

    R->EBX(0);
    R->EDI(*reinterpret_cast<DWORD*>(this_ptr));
    return 0x00572EC4;
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
    Patch_Jump(0x00573EF0, &MessageListClassExt::Set_Width);
}
