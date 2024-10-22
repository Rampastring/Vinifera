/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera
 *
 *  @file          TECHNOTYPEEXT_HOOKS.CPP
 *
 *  @author        CCHyper
 *
 *  @brief         Contains the hooks for the extended TechnoTypeClass.
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
#include "technotypeext_hooks.h"
#include "technotypeext.h"
#include "objecttypeext.h"
#include "technotype.h"
#include "house.h"
#include "rules.h"
#include "tibsun_defines.h"
#include "extension.h"
#include "fatal.h"
#include "debughandler.h"
#include "asserthandler.h"
#include "extension_globals.h"
#include "hooker.h"
#include "rulesext.h"

#include "hooker.h"
#include "hooker_macros.h"


/**
 *  A fake class for implementing new member functions which allow
 *  access to the "this" pointer of the intended class.
 *
 *  @note: This must not contain a constructor or destructor.
 *
 *  @note: All functions must not be virtual and must also be prefixed
 *         with "_" to prevent accidental virtualization.
 */
class TechnoTypeClassExt : public TechnoTypeClass
{
public:
    int _Max_Pips() const;
    int _Time_To_Build();
};


/**
 *  Reimplements TechnoTypeClass::Max_Pips.
 *
 *  @author: ZivDero
 */
int TechnoTypeClassExt::_Max_Pips() const
{
    int max_pips = 0;
    if (PipScale - 1 < RuleExtension->MaxPips.Count())
        max_pips = RuleExtension->MaxPips[PipScale - 1];

    // Negative values are not allowed
    if (max_pips < 0)
        return 0;

    switch (PipScale)
    {
    case PIP_AMMO:
        return std::clamp(MaxAmmo, 0, max_pips);

    case PIP_PASSENGERS:
        return std::clamp(MaxPassengers, 0, max_pips);

    case PIP_TIBERIUM:
    case PIP_POWER:
    case PIP_CHARGE:
    default:
        return max_pips;
    }
}


/**
 *  #issue-433
 *
 *  Allows overriding the cost value that is used for calculating the build time of a TechnoType.
 *
 *  Author: Rampastring
 */
int TechnoTypeClassExt::_Time_To_Build()
{
    // TechnoClass::Time_To_Build calls TechnoTypeClass::Time_To_Build,
    // so replacing TechnoTypeClass::Time_To_Build is enough for the desired functionality.

    TechnoTypeClassExtension* technotypeext = Extension::Fetch<TechnoTypeClassExtension>(this);

    int cost;

    if (technotypeext->BuildTimeCost != 0)
    {
        cost = technotypeext->BuildTimeCost;
    }
    else
    {
        cost = Cost;
    }

    return (int)(cost * Rule->BuildSpeedBias * 0.9);
}


/**
 *  #issue-90
 *
 *  Disables the bugged bonus range for arcing projectiles.
 *
 *  Author: Rampastring
 */
DECLARE_PATCH(_TechnoTypeClass_In_Range_Disable_Arcing_Bonus_Range_Patch)
{
    // DTA addition: give some bonus extra range for aircraft so they don't waste time+
    // and sometimes ammo when chasing enemy units. This often happens when an aircraft
    // is told to attack a moving unit; the aircraft reaches firing range, stops to fire,
    // the unit moves, the aircraft is suddenly out of range again and must move forward
    // before it even managed to dispatch a single missile.
    // And the process is repeated again and again.
    // TODO make this a variable in Rules
    GET_STACK_STATIC(TechnoTypeClass *, this_ptr, esp, 0x14);
    if (this_ptr->What_Am_I() == RTTI_AIRCRAFTTYPE) {
        _asm { add  edi, 0x200 } // 0x200 = 512 = 2 cells. Range is in leptons
    }

    JMP(0x0063D6AA);
}


/**
 *  Main function for patching the hooks.
 */
void TechnoTypeClassExtension_Hooks()
{
    Patch_Jump(0x0063D460, &TechnoTypeClassExt::_Max_Pips);
    Patch_Jump(0x0063B8B0, &TechnoTypeClassExt::_Time_To_Build);
    Patch_Jump(0x0063D5A7, &_TechnoTypeClass_In_Range_Disable_Arcing_Bonus_Range_Patch);
}
