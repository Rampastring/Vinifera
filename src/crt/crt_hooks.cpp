/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera
 *
 *  @file          CRT_HOOKS.CPP
 *
 *  @author        CCHyper
 *
 *  @brief         Setup all the hooks to take control of the basic CRT.
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

#include "always.h"

#include "crt_hooks.h"

#include "debughandler.h"
#include "hooker.h"
#include "hooker_macros.h"
#include "session.h"
#include "tibsun_globals.h"

#include <crtdbg.h>
#include <cstring>
#include <fenv.h>


static unsigned int precision = 0;
static unsigned int rounding = 0;


static void Set_Vinifera_FP_Mode()
{
    precision = _PC_24;

    if (Session.Type == GAME_IPX)
    {
        // Multiplayer - prioritize stability
        // Based on extensive testing in DTA, _RC_NEAR seems to significantly reduce frequency of desyncs.
        rounding = _RC_NEAR;
    }
    else 
    {
        // Singleplayer - prioritize functionality
        rounding = _RC_CHOP;
    }

    _set_controlfp(precision, _MCW_PC);
    _set_controlfp(rounding, _MCW_RC); // _RC_NEAR in SupCom code

    // Call the game's function to store the FPU mode.
    _asm { mov edx, 0x006B2314 }
    _asm { call edx }

    /**
     *  And this is required for the std c++ lib.
     */
    fesetround(rounding == _RC_NEAR ? FE_TONEAREST : FE_TOWARDZERO);
}


/**
 *  Set the FPU mode to match the game (rounding towards zero [chop mode]).
 */
DECLARE_PATCH(_set_fp_mode)
{
    // Call to "WWDebug_Printf"
    _asm { mov edx, 0x004082D0 }
    _asm { call edx }

    /**
     *  Set the FPU mode.
     *  According to a Supreme Commander developer, this mode is
     *  necessary for determinism.
     *  https://gafferongames.com/post/floating_point_determinism/
     */
    Set_Vinifera_FP_Mode();

    JMP(0x005FFDB0);
}


static void Check_Vinifera_FP_Mode()
{
    // Fetch FP control value
    if (Session.Type == GAME_IPX) {
        int fpcontrol = _controlfp(0, 0);
        if ((fpcontrol & _MCW_PC) != precision)
        {
            DEBUG_FATAL("FPU precision mode change detected. Value: 0x%08x\n", fpcontrol);
            Emergency_Exit(255);
        }

        if ((fpcontrol & _MCW_RC) != rounding) // _RC_NEAR for SupCom mode
        {
            DEBUG_FATAL("FPU rounding mode change detected. Value: 0x%08x\n", fpcontrol);
            Emergency_Exit(255);
        }
    }
}

DECLARE_PATCH(_LogicClass_AI_Beginning_Set_FP_Mode)
{
    _asm { push  ecx }

    Set_Vinifera_FP_Mode();

    // Stolen bytes / code
    _asm { mov  edx, dword ptr ds:0x00804D28 } // mov edx, LogicInit
    _asm { pop  ecx }
    JMP(0x00506AB9);
}


DECLARE_PATCH(_LogicClass_AI_End_Check_FP_Mode)
{
    Check_Vinifera_FP_Mode();

    // Rebuild function epilogue
    _asm { add  esp, 28h }
    _asm { retn }
}


/**
 *  Main function for patching the hooks.
 */
void CRT_Hooks()
{
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif

    /**
     *  Call the games fpmath to make sure we init 
     */
    Patch_Jump(0x005FFD97, &_set_fp_mode);

    // Set the FP mode in the beginning of LogicClass::AI
    Patch_Jump(0x00506AB3, &_LogicClass_AI_Beginning_Set_FP_Mode);
    Patch_Jump(0x00507205, &_LogicClass_AI_End_Check_FP_Mode);

    /**
     *  Standard functions.
     */
    Hook_Function(0x006B602A, &std::strtok);
    Hook_Function(0x006BE766, &strdup);

    /**
     *  C memory functions.
     */
    Hook_Function(0x006B72CC, &std::malloc);
    Hook_Function(0x006BCA26, &std::calloc);
    Hook_Function(0x006B7F72, &std::realloc);
    Hook_Function(0x006B67E4, &std::free);
    Hook_Function(0x006B80AA, &_msize);

    /**
     *  C++ new and delete.
     */
    Hook_Function(0x006B51D7, &std::malloc);
    Hook_Function(0x006B51CC, &std::free);
}
