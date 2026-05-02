/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Contains the hooks for the extended SuperClass.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#include "always.h"

#include "superext_hooks.h"

#include "aircraft.h"
#include "aircraftext.h"
#include "building.h"
#include "extension.h"
#include "hooker.h"
#include "house.h"
#include "housetype.h"
#include "mouse.h"
#include "reinf.h"
#include "scenarioext.h"
#include "scripttype.h"
#include "sideext.h"
#include "superext.h"
#include "superext_init.h"
#include "syringe.h"
#include "taskforce.h"
#include "teamtype.h"
#include "unit.h"


/**
 *  A fake class for implementing new member functions which allow
 *  access to the "this" pointer of the intended class.
 *
 *  @note: This must not contain a constructor or deconstructor!
 *  @note: All functions must be prefixed with "_" to prevent accidental virtualization.
 */
static class SuperClassExt final : public SuperClass
{
public:
	void _Do_DropPods(Cell* cell);
};


/**
 *  Helper function that creates a hunter-seeker for the house's side.
 *
 *  @author: ZivDero
 */
static UnitClass* Make_HunterSeeker(HouseClass* house)
{
    const auto side_ext = Extension::Fetch(Sides[house->Class->Side]);

    if (side_ext->HunterSeeker) {
        return new UnitClass(const_cast<UnitTypeClass*>(side_ext->HunterSeeker), house);
    }

    return nullptr;
}


/**
 *  Patch to use the hunter-seeker for the house's side.
 *
 *  @author: ZivDero
 */
DEFINE_HOOK(0x0060C5DE, _SuperClass_Place_HunterSeeker_Type_Patch, 0)
{
    GET(SuperClass*, this_ptr, ESI);

    /**
     *  Fetch the hunter-seeker for this house's side.
     */
    UnitClass*  hunter_seeker = Make_HunterSeeker(this_ptr->House);
    R->ESI(hunter_seeker);

    /**
     *  If we've successfully created a hunter-seeker, proceed to launching it.
     */
    if (hunter_seeker) {
        return 0x0060C642;
    }

    /**
     *  Otherwise, abort (return).
     */
    else {
        return 0x0060C68F;
    }
}


void SuperClassExt::_Do_DropPods(Cell* cell)
{
	char buffer[24];

	sprintf(buffer, "PARADROPINF_%d", this->House->HeapID);
	
	TeamTypeClass* ttype = nullptr;

	for (int i = 0; i < TeamTypes.Count(); i++)
	{
		if (!strcasecmp(buffer, TeamTypes[i]->IniName.c_str()))
		{
			ttype = TeamTypes[i];
			break;
		}
	}

	if (ttype == nullptr) {
		ttype = new TeamTypeClass();

		if (ttype != nullptr) {
			ttype->IniName.assign(buffer);
			// ttype->IsTransient = true;
			ttype->IsPrebuilt = false;
			ttype->IsReinforcable = false;
			ttype->IsSuicide = true;
			ttype->Origin = WAYPT_SPECIAL;

			ScriptTypeClass* script = nullptr;
			for (int i = 0; i < ScriptTypes.Count(); i++)
			{
				if (!strcasecmp("PARADROPINF_SCRIPT", ScriptTypes[i]->IniName.c_str()))
				{
					script = ScriptTypes[i];
					break;
				}
			}

			if (script == nullptr) {
				script = new ScriptTypeClass();

				if (script == nullptr)
					return;

				script->IniName.assign("PARADROPINF_SCRIPT");
				script->MissionCount = 2;
				script->MissionList[0].Mission = SMISSION_ATT_WAYPT;
				script->MissionList[0].Data.Value = WAYPT_SPECIAL;
				script->MissionList[1].Mission = SMISSION_DO;
				script->MissionList[1].Data.Mission = MISSION_RETREAT;

				ttype->Script = script;
			}

			ttype->Script = script;

			TaskForceClass* taskforce = nullptr;

			for (int i = 0; i < TaskForces.Count(); i++)
			{
				if (!strcasecmp("PARADROPINF_TASKFORCE", TaskForces[i]->IniName.c_str()))
				{
					taskforce = TaskForces[i];
					break;
				}
			}

			if (taskforce == nullptr) {
				taskforce = new TaskForceClass();

				if (taskforce == nullptr)
					return;

				taskforce->IniName.assign("PARADROPINF_TASKFORCE");
				taskforce->ClassCount = 2;
				taskforce->Members[0].Class = InfantryTypeClass::Find_Or_Make("E1");
				taskforce->Members[0].Quantity = AircraftTypeClass::Find_Or_Make("BADGER")->Max_Passengers();
				taskforce->Members[1].Class = AircraftTypeClass::Find_Or_Make("BADGER");
				taskforce->Members[1].Quantity = 1;
			}

			ttype->TaskForce = taskforce;
			ttype->House = this->House;
		}
	}

	if (ttype != NULL) {
		ScenExtension->Waypoint[WAYPT_SPECIAL] = Map.Nearby_Location(*cell, SPEED_FOOT, -1, MZONE_INFANTRY, false, Point2D(1, 1), true, false, false, false, Cell());
		if (Map.In_Local_Radar(ScenExtension->Waypoint[WAYPT_SPECIAL]) && Do_Reinforcements(ttype)) {

			// Mark the aircraft as a loaner so it is able to exit the map
			AircraftClass* spawnedAircraft = Aircrafts[Aircrafts.Count() - 1];
			spawnedAircraft->IsALoaner = true;
			AircraftClassExtension* aircraftext = Extension::Fetch(spawnedAircraft);
			aircraftext->IsParadropReinforcement = true;
		}
		else
		{
			// TODO does not work, SW timer is likely depleted after this function has finished executing
			this->Forced_Charge(false);
		}
	}

	//				Create_Air_Reinforcement(this, AIRCRAFT_BADGER, 1, MISSION_HUNT, ::As_Target(cell), TARGET_NONE, INFANTRY_E1);
	// if (this == PlayerPtr) {
	// 	Map.IsTargettingMode = SPC_NONE;
	// }
}


/**
 *  Patch to use the actual SW HeapID when launching a missile,
 *  instead of the Type= number.
 *
 *  @author: ZivDero
 */
DEFINE_HOOK(0x0060C49E, _SuperClass_Place_NukeType, 0)
{
    GET(SuperClass*, this_ptr, EAX);
    GET(BuildingClass*, launchsite, ESI);

    launchsite->field_298 = this_ptr->Class->HeapID;

    return 0x0060C4AA;
}


/**
 *  Main function for patching the hooks.
 */
void SuperClassExtension_Hooks()
{
    /**
     *  Initialises the extended class.
     */
    SuperClassExtension_Init();
	Patch_Jump(0x0060C880, &SuperClassExt::_Do_DropPods);
}
