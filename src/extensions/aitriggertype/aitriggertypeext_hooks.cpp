/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Contains the hooks for the extended BulletClass.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#include "always.h"

#include "aitriggertypeext_hooks.h"
#include "aitrigtype.h"
#include "asserthandler.h"
#include "extension.h"
#include "fastmath.h"
#include "hooker.h"
#include "house.h"
#include "housetype.h"
#include "rules.h"
#include "scenario.h"
#include "syringe.h"
#include "teamtype.h"
#include "tibsun_defines.h"
#include "tibsun_globals.h"


/**
 *  A fake class for implementing new member functions which allow
 *  access to the "this" pointer of the intended class.
 *
 *  @note: This must not contain a constructor or destructor!
 *  @note: All functions must be prefixed with "_" to prevent accidental virtualization.
 */
static DECLARE_EXTENDING_CLASS_AND_PAIR(AITriggerTypeClass)
{
public:
    bool _Process(HouseClass * house, HouseClass * enemy, bool b);
};


bool AITriggerTypeClassExt::_Process(HouseClass* house, HouseClass* enemy, bool b)
{
    bool basedefense;
    if ((TeamTypeOne != nullptr && TeamTypeOne->IsBaseDefense) && (TeamTypeTwo == nullptr || TeamTypeTwo->IsBaseDefense)) {
        basedefense = true;
    } else {
        basedefense = false;
    }

    if (TeamTypeOne == nullptr) {
        return (false);
    }

    if (enemy == nullptr || (Rule->UseMinDefenseRule && house->BaseDefenseTeamCount < Rule->MinimumAIDefensiveTeams[house->Difficulty])) {
        if (!basedefense) {
            return (false);
        }
    }

    if (basedefense && b) {
        return (false);
    }

    if (Scope == SCOPE_GLOBAL && Scen->IsIgnoreGlobalAITriggers == true) {
        return (false);
    }

    if (!IsEnabled) {
        return (false);
    }

    if (Session.Type != GAME_NORMAL && !IsAvailableInSkirmish) {
        return (false);
    }

    int diff;
    // Rampastring: what is this? why use player's difficulty instead of the AI house's??
    // if (Session.Type == GAME_NORMAL) {
    // 
    //     diff = Scen->Difficulty;
    //     if (diff == 0) {
    //         if (!IsEnabledInEasy) {
    //             return (false);
    //         }
    //     } else if (diff == 1) {
    //         if (!IsEnabledInMedium) {
    //             return (false);
    //         }
    //     } else if (diff == 2) {
    //         if (!IsEnabledInHard) {
    //             return (false);
    //         }
    //     }
    // 
    // } else {

        diff = house->Difficulty;
        if (diff == 0) {
            if (!IsEnabledInHard) {
                return (false);
            }
        } else if (diff == 1) {
            if (!IsEnabledInMedium) {
                return (false);
            }
        } else if (diff == 2) {
            if (!IsEnabledInEasy) {
                return (false);
            }
        }
    // }

    if (Session.Type == GAME_NORMAL) {
        if (OwnerHouseType == AITRIG_HOUSE_NONE) {
            return (false);
        }
        if (OwnerHouseType != AITRIG_HOUSE_ALL && OwnerHouseType == AITRIG_HOUSE_INDEX && house->Class->House != House) {
            return (false);
        }
    }

    // Modified to support more than 2 sides.
    // Author: Rampastring
    if (MultiSide > 0) {
        if (house->ActLike != (MultiSide - 1)) {
            return (false);
        }
    }

    if (TechLevel > house->Control.TechLevel) {
        return (false);
    }

    bool res = false;
    if (enemy != nullptr) {
        switch (Type) {
        case AITRIGGEREVENT_NONE:
            res = true;
            break;
        case AITRIGGEREVENT_ENEMY_OWNS:
            res = Check_Enemy_Owns(house, enemy);
            break;
        case AITRIGGEREVENT_HOUSE_OWNS:
            res = Check_House_Owns(house, enemy);
            break;
        case AITRIGGEREVENT_ENEMY_YELLOW_POWER:
            res = Check_Enemy_Yellow_Power(house, enemy);
            break;
        case AITRIGGEREVENT_ENEMY_RED_POWER:
            res = Check_Enemy_Red_Power(house, enemy);
            break;
        case AITRIGGEREVENT_ENEMY_OWNS_N_MONEY:
            res = Check_Enemy_Money(house, enemy);
            break;
        }
    } else {
        if (Type == AITRIGGEREVENT_HOUSE_OWNS) {
            res = Check_House_Owns(house, enemy);
        } else {
            res = Type == AITRIGGEREVENT_NONE;
        }
    }

    if (!res) {
        return (false);
    }

    if (!house->Can_Create_Team(TeamTypeOne)) {
        return (false);
    }
    if (TeamTypeTwo != NULL && !house->Can_Create_Team(TeamTypeTwo)) {
        return (false);
    }

    if (TeamTypeOne && TeamTypeOne->MaxAllowed >= 0 && house->Owned_Team_Count(TeamTypeOne) >= TeamTypeOne->MaxAllowed) {
        return (false);
    }
    if (TeamTypeTwo && TeamTypeTwo->MaxAllowed >= 0 && house->Owned_Team_Count(TeamTypeTwo) >= TeamTypeTwo->MaxAllowed) {
        return (false);
    }

    return (true);
}


/**
 *  Main function for patching the hooks.
 */
void AITriggerTypeClassExtension_Hooks()
{
    Patch_Jump(0x00410840, &AITriggerTypeClassExt::_Process);
}