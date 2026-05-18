/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Contains the hooks for the extended TeamClass.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#include "always.h"

#include "teamext_hooks.h"

#include "asserthandler.h"
#include "building.h"
#include "cell.h"
#include "extension.h"
#include "foot.h"
#include "footext.h"
#include "hooker.h"
#include "house.h"
#include "iomap.h"
#include "rulesext.h"
#include "scripttype.h"
#include "syringe.h"
#include "tag.h"
#include "taskforce.h"
#include "team.h"
#include "teamext.h"
#include "teamext_init.h"
#include "teamtype.h"
#include "technotypeext.h"
#include "vinifera_defines.h"
#include "weapontype.h"


/**
 *  A fake class for implementing new member functions which allow
 *  access to the "this" pointer of the intended class.
 *
 *  @note: This must not contain a constructor or destructor!
 *  @note: All functions must be prefixed with "_" to prevent accidental virtualization.
 */
DECLARE_EXTENDING_CLASS_AND_PAIR(TeamClass)
{
public:
    bool _Add(FootClass * obj);
    bool _Remove(FootClass * obj, int typeindex);
    bool _Recalc_Strength();
    void _TMission_ATTACK(ScriptMissionClass * mission, bool a1);
};


/**
 *  #issue-196
 * 
 *  Fixes incorrect cell calculation for the MOVECELL script.
 * 
 *  The original code used outdated code from Red Alert to calculate
 *  the cell position on the map.
 * 
 *  @author: CCHyper (based on research by E1Elite)
 */
DEFINE_HOOK(0x00622B2C, _TeamClass_AI_MoveCell_FixCellCalc_Patch, 0)
{
    GET_STACK(unsigned, argument, 0x24);

    /**
     *  Get the cell X and Y position from the script argument.
     */
    Cell tmpcell;
    if (NewINIFormat < 4) {
        tmpcell.X = argument % 256;
        tmpcell.Y = argument / 256;
    } else {
        tmpcell.X = argument % 1000;
        tmpcell.Y = argument / 1000;
    }

    /**
     *  Fetch the map cell. Added pointer check to make sure the
     *  script didn't have an invalid position.
     */
    CellClass* cell = &Map[tmpcell];
    if (!cell) {
        goto coordinate_move;
    }

    /**
     *  The Assign_Mission_Target call pushes EAX into the stack
     *  for the cell argument.
     */
    R->EAX(cell);

assign_mission_target:
    return 0x00622B5F;

coordinate_move:
    return 0x00622B19;
}


/**
 *  #issue-71
 *
 *  Increases the amount of available waypoints (see ScenarioClassExtension for implementation).
 *
 *  @author: ZivDero
 */
DEFINE_HOOK(0x00625886, _TeamClass_TMission_PATROL_WaypointMax, 0)
{
    GET(ScriptMissionClass*, mission, EAX);

    if (mission->Data.Value < NEW_WAYPOINT_COUNT) {
        return 0x0062588C;
    }

    return 0x00625894;
}


int Get_Group(TeamTypeClass* teamtype)
{
    if (teamtype->Group != -2 && teamtype->Group == -1) {
        if (teamtype->TaskForce != nullptr) {
            return(teamtype->TaskForce->Group);
        }
    }
    return(teamtype->Group);
}


/**
 *	TeamClass::Add reimplementation for Advanced AI to be able to form teams out of
 *  a non-predefined list of units.
 *
 *  Author: Original implementation by tomsons26/ZivDero, AdvAI adjustments by Rampastring
 */
bool TeamClassExt::_Add(FootClass* obj)
{
    if (!obj) return(false);

    TeamClassExtension* teamext = Extension::Fetch(this);

    // Advanced AI can add members to the group regardless of what the TeamType consists of.
    // Though, we need to reimplement some checks of the original Can_Add function.
    if (RuleExtension->AdvancedAIUnitProduction && teamext->IsAdvAITeam) {
        if (obj->Team == this || !Is_It_Breathing(obj) || obj->In_Radio_Contact() || obj->House != House) {
            return false;
        }

        if (obj->Mission != MISSION_NONE && !MissionClass::Is_Recruitable_Mission(obj->Mission)) {
            return(false);
        }

        if (obj->Team != nullptr) {
            obj->Team->Remove(obj);
        }

		teamext->AntiNoneStrength += Extension::Fetch(obj->TClass)->AntiNoneArmorValue();
		teamext->AntiLightStrength += Extension::Fetch(obj->TClass)->AntiLightArmorValue();
		teamext->AntiHeavyStrength += Extension::Fetch(obj->TClass)->AntiHeavyArmorValue();
		teamext->ArtilleryStrength += Extension::Fetch(obj->TClass)->ArtilleryValue();
		teamext->CurrentCost += obj->TClass->Cost;
		if (obj->RTTI == RTTI_INFANTRY) {
			teamext->InfantryCost += obj->TClass->Cost;
		}
		else if (obj->RTTI == RTTI_UNIT) {
			teamext->VehicleCost += obj->TClass->Cost;
		}

		if (obj->TClass->Armor == ARMOR_NONE) {
			teamext->NoneArmorStrength += obj->TClass->MaxStrength;
		} else if (Extension::Fetch(obj->TClass)->CategorizedAsLightlyArmored()) {
			teamext->LightArmorStrength += obj->TClass->MaxStrength;
		} else if (obj->TClass->Armor == ARMOR_STEEL) {
			teamext->HeavyArmorStrength += obj->TClass->MaxStrength;
		}

    } else {
        int typeindex;
        if (!Can_Add(obj, typeindex)) return(false);

        /*
        **	All is ok to add the object to the team, but if the object is already part of
        **	another team, then it must be removed from that team first.
        */
        if (obj->Team != nullptr) {
            obj->Team->Remove(obj);
        }

        /*
        **	Actually add the object to the team.
        */
        Quantity[typeindex]++;
    }

    obj->IsInitiated = (Member == nullptr);
    obj->Member = Member;
    Member = obj;
    obj->Team = this;
    obj->Group = Get_Group(Class);

    /*
    **	If a common trigger is designated for this team type, then attach the
    **	trigger to this team member.
    */
    if (Tag != nullptr && (!Class->OnTransOnly || obj->TClass->MaxPassengers > 0)) {
        obj->Attach_Tag(Tag);
    }

    Total++;
    Risk += obj->Risk();
    if (Zone == nullptr) {
        Calc_Center(Zone, ClosestMember);
    }

    /*
    **	Return with success, since the object was added to the team.
    */
    IsAltered = JustAltered = true;
    obj->field_206 = Class->AreTeamMembersRecruitable;
    return(true);
}


/**
 *	TeamClass::Remove reimplementation for Advanced AI.
 *
 *  Author: Original implementation by tomsons26/ZivDero, adjustments by Rampastring
 */
bool TeamClassExt::_Remove(FootClass* obj, int typeindex)
{
	obj->field_34F = true;

	/*
	**	Make sure that the object is in fact a member of this team. If not, then it can't
	**	be removed. Return success because the end result is the same.
	*/
	if (this != obj->Team) {
		return(true);
	}

	TeamClassExtension* teamext = Extension::Fetch(this);

	// Decrement the Advanced AI team strength values.
	if (RuleExtension->AdvancedAIUnitProduction && teamext->IsAdvAITeam)
	{
		teamext->AntiNoneStrength -= Extension::Fetch(obj->TClass)->AntiNoneArmorValue();
		teamext->AntiLightStrength -= Extension::Fetch(obj->TClass)->AntiLightArmorValue();
		teamext->AntiHeavyStrength -= Extension::Fetch(obj->TClass)->AntiHeavyArmorValue();
		teamext->ArtilleryStrength -= Extension::Fetch(obj->TClass)->ArtilleryValue();

		teamext->CurrentCost -= obj->TClass->Cost;
		if (obj->RTTI == RTTI_INFANTRY) {
			teamext->InfantryCost -= obj->TClass->Cost;
		}
		else if (obj->RTTI == RTTI_UNIT) {
			teamext->VehicleCost -= obj->TClass->Cost;
		}

		if (obj->TClass->Armor == ARMOR_NONE) {
			teamext->NoneArmorStrength -= obj->TClass->MaxStrength;
		}
		else if (Extension::Fetch(obj->TClass)->CategorizedAsLightlyArmored()) {
			teamext->LightArmorStrength -= obj->TClass->MaxStrength;
		}
		else if (obj->TClass->Armor == ARMOR_STEEL) {
			teamext->HeavyArmorStrength -= obj->TClass->MaxStrength;
		}
	}

	/*
	**	Detach the common trigger for this team type. Only current and active members of the
	**	team have that trigger attached. The exception is for player team members that
	**	get removed from a reinforcement team.
	*/
	if (obj->Tag == Tag) {
		HouseClass* hptr = obj->House;
		if (hptr != nullptr && !hptr->Is_Human_Player()) {
			obj->Attach_Tag(nullptr);
		}
	}

	/*
	**	If the proper team index was not provided, then find it in the type type class. The
	**	team type class will not be set if the appropriate type could not be found
	**	for this object. This indicates that the object was illegally added. Continue to
	**	process however, since removing this object from the team is a good idea.
	*/
	if (typeindex == -1 && Class != nullptr && Class->TaskForce != nullptr) {
		for (typeindex = 0; typeindex < Class->TaskForce->ClassCount; typeindex++) {
			if (Class->TaskForce->Members[typeindex].Class == obj->Class_Of()) {
				break;
			}
		}
	}

	/*
	**	Decrement the counter for the team class. There is now one less of this object type.
	*/
	if (typeindex > -1 && Class != nullptr && Class->TaskForce != nullptr && (unsigned)typeindex < Class->TaskForce->ClassCount) {
		Quantity[typeindex]--;
	}

	/*
	**	Actually remove the object from the team. Scan through the team members
	**	looking for the one that matches the one specified. If it is found, it
	**	is unlinked from the member chain. During this scan, a check is made to
	**	ensure that at least one remaining member is still initiated. If not, then
	**	a new team captain must be chosen.
	*/
	bool initiated = false;
	FootClass* prev = 0;
	FootClass* curr = Member;
	bool found = false;
	while (curr != nullptr && (!found || !initiated)) {
		if (curr == obj) {
			if (prev != nullptr) {
				prev->Member = curr->Member;
			}
			else {
				Member = curr->Member;
			}
			FootClass* temp = curr->Member;
			curr->Member = 0;
			curr->Team = 0;
			curr->SuspendedMission = MISSION_NONE;
			curr->SuspendedNavCom = nullptr;
			curr->SuspendedTarCom = nullptr;
			curr = temp;
			Total--;
			found = true;
			Risk -= obj->Risk();
			continue;
		}

		/*
		**	If this (remaining) member is initiated, then keep a record of this.
		*/
		//initiated |= curr->IsInitiated;
		if (curr->IsInitiated) {
			initiated = true;
		}

		prev = curr;
		curr = curr->Member;
	}

	if (obj->Team != nullptr) {
		obj->Team = nullptr;
	}

	/*
	**	A unit that breaks off of a team will enter idle mode.
	*/
	if (GameActive && obj->IsActive && !obj->IsInLimbo) {
		obj->Enter_Idle_Mode();
	}

	/*
	**	If, after removing the team member, there are no initiated members left
	**	in the team, then just make the first remaining member of the team the
	**	team captain. Mark the center location of the team as invalid so that
	**	it will be centered around the captain.
	*/
	if (!initiated && Member != nullptr) {
		Member->IsInitiated = true;
		Zone = nullptr;
	}

	/*
	**	Must record that the team composition has changed. At the next opportunity,
	**	the team members will be counted and appropriate AI adjustments made.
	*/
	IsAltered = JustAltered = true;
	return(true);
}


/**
 *  TeamClass::Recalc_Strength reimplementation for Advanced AI.
 *
 *  Author: Original implementation by tomsons26/ZivDero, adjustments by Rampastring
 */
bool TeamClassExt::_Recalc_Strength()
{
	TeamClassExtension* teamext = Extension::Fetch(this);

	bool old_under = IsUnderStrength;
	int desired = 0;
	
	if (RuleExtension->AdvancedAIUnitProduction && teamext->IsAdvAITeam) {
		desired = teamext->OriginalTeamMemberCount;
	}
	else {
		desired = Class->TaskForce->Required_Object_Count();
	}

	if (Total > 0) {

		if (!RuleExtension->AdvancedAIUnitProduction || !teamext->IsAdvAITeam) {
			IsFullStrength = (Total == desired);
			if (IsFullStrength) {
				IsHasBeen = true;
			}
		}

		/*
		**	Reinforceable teams will revert (or snap out of) the under strength
		**	mode when the members transition the magic 1/3 strength threshold.
		*/
		if (Class->IsReinforcable) {
			if (desired > 2) {
				IsUnderStrength = (Total <= desired / 3);
			}
			else {
				IsUnderStrength = (Total < desired);
			}
		}
		else {

			/*
			**	Teams that are not flagged as reinforceable are never considered under
			**	strength if the team has already started its main mission. This
			**	ensures that once the team has started, it won't dally to pick up
			**	new members.
			*/
			IsUnderStrength = !IsHasBeen;
		}

		if (Class->IsGuardSlower) {
			if (IsUnderStrength) {
				field_66 = false;
			}
			else {
				field_66 = true;
			}
		}

		IsAltered = JustAltered = false;
	}
	else {
		field_66 = false;
		IsUnderStrength = true;
		IsFullStrength = false;
		Zone = NULL;

		/*
		**	A team that exists on the player's side is automatically destroyed
		**	when there are no team members left. This team was created as a
		**	result of reinforcement logic and no longer needs to exist when there
		**	are no more team members.
		*/
		if (IsHasBeen) {

			/*
			**	If this team had no members (i.e., the team object wasn't terminated by some
			**	outside means), then pass through the logic triggers to see if one that
			**	depends on this team leaving the map should be sprung.
			*/
			if (IsLeaveMap) {
				for (int index = LogicTriggers.Count() - 1; index >= 0; index--) {
					TagClass* tag = LogicTriggers[index];
					if (tag->Spring(TEVENT_LEAVES_MAP)) {
						if (LogicTriggers.Count() == 0) break;
					}
				}
			}
			delete this;
			return(false);
		}
	}

	/*
	** If the team has gone from under strength to no longer under
	** strength than the team needs to reform.
	*/
	if (old_under != IsUnderStrength) {
		IsReforming = true;
	}
	return(true);
}


void TeamClassExt::_TMission_ATTACK(ScriptMissionClass* mission, bool)
{
	if (MissionTarget == NULL && Member != NULL) {

		/*
		**	Pick a team leader that has a weapon. Only in the case of no
		**	team members having any weapons, will a member without a weapon
		**	be chosen.
		*/
		FootClass const* candidate = Fetch_A_Leader();

		/*
		**	Have the team leader pick what the next team target will be.
		*/
		switch (mission->Data.Quarry) {
		case QUARRY_ANYTHING:
			Assign_Mission_Target(candidate->Greatest_Threat(THREAT_NORMAL, candidate->PositionCoord, Class->OnlyTargetHouseEnemy));
			break;

		case QUARRY_BUILDINGS:
			Assign_Mission_Target(candidate->Greatest_Threat(THREAT_BUILDINGS, candidate->PositionCoord, Class->OnlyTargetHouseEnemy));
			break;

		case QUARRY_HARVESTERS:
			Assign_Mission_Target(candidate->Greatest_Threat(THREAT_TIBERIUM, candidate->PositionCoord, Class->OnlyTargetHouseEnemy));
			break;

		case QUARRY_INFANTRY:
			Assign_Mission_Target(candidate->Greatest_Threat(THREAT_INFANTRY, candidate->PositionCoord, Class->OnlyTargetHouseEnemy));
			break;

		case QUARRY_VEHICLES:
			Assign_Mission_Target(candidate->Greatest_Threat(THREAT_VEHICLES, candidate->PositionCoord, Class->OnlyTargetHouseEnemy));
			break;

		case QUARRY_FACTORIES:
			Assign_Mission_Target(candidate->Greatest_Threat(THREAT_FACTORIES, candidate->PositionCoord, Class->OnlyTargetHouseEnemy));
			break;

		case QUARRY_DEFENSE:
			Assign_Mission_Target(candidate->Greatest_Threat(THREAT_BASE_DEFENSE, candidate->PositionCoord, Class->OnlyTargetHouseEnemy));
			break;

		case QUARRY_THREAT:
			Assign_Mission_Target(candidate->Greatest_Threat(THREAT_NORMAL, candidate->PositionCoord, Class->OnlyTargetHouseEnemy));
			break;

		case QUARRY_POWER:
			Assign_Mission_Target(candidate->Greatest_Threat(THREAT_POWER, candidate->PositionCoord, Class->OnlyTargetHouseEnemy));
			break;

		case VINIFERA_QUARRY_HARVESTERS:
			Assign_Mission_Target(candidate->Greatest_Threat((ThreatType)EXT_THREAT_HARVESTERS, candidate->PositionCoord, Class->OnlyTargetHouseEnemy));
			break;

		default:
			break;
		}
		if (MissionTarget == NULL || !Ammo_Check()) IsNextMission = true;
	}
	if (MissionTarget == NULL || !Ammo_Check()) IsNextMission = true;

	Coordinate_Attack();
}


enum TargetPropertyType
{
	TPROPERTY_LEAST_THREAT,
	TPROPERTY_GREATEST_THREAT,
	TPROPERTY_NEAREST,
	TPROPERTY_FARTHEST,

	TPROPERTY_COUNT,
};


/*
**  Fixes a bug where the AI does not ignore buildings in limbo when selecting a BwP target.
**  Also makes BwP scan respect TargetZoneScanType.
**
**  @author: tomsons26/ZivDero for original code, Rampastring for fixing the aforementioned bug
**           and implementing TargetZoneScanType functionality.
*/
BuildingClass* _Pick_Building_With_Property(BuildingTypeClass* type, HouseClass* house, FootClass* unit, TargetPropertyType prop, bool only_enemy)
{
	int best_same_dist = -1;
	BuildingClass* best_same_ptr = nullptr;
	int best_dist = -1;
	BuildingClass* best_ptr = nullptr;

	TargetZoneScanType tzst = Extension::Fetch(unit->TClass)->TargetZoneScan;
	int ourzone = Map.Get_Cell_Zone(unit->Center_Coord().As_Cell(), unit->TClass->MZone, true);

	for (int index = 0; index < Buildings.Count(); index++) {

		BuildingClass* ptr = Buildings[index];

		if (!ptr->IsActive || ptr->IsInLimbo || !ptr->IsDown) {
			continue;
		}

		HouseClass* hptr = ptr->House;

		bool same_house = hptr == house;

		if (ptr->Class == type && (same_house || !unit->House->Is_Ally(ptr->House))) {

			if (tzst == TargetZoneScanType::TZST_SAME) {
				int targetzone = Map.Get_Cell_Zone(ptr->Center_Coord().As_Cell(), unit->TClass->MZone, false);
				if (targetzone != ourzone) {
					continue;
				}
			}
			else if (tzst == TargetZoneScanType::TZST_INRANGE)
			{
				// If the zone is different, only allow targeting if we can reach the target from our zone.

				int targetzone = Map.Get_Cell_Zone(ptr->Center_Coord().As_Cell(), unit->TClass->MZone, false);

				if (ourzone != targetzone) 
				{
					Cell nearbycell = Map.Nearby_Location(ptr->Center_Coord().As_Cell(),
						unit->TClass->Speed,
						/*Phobos has -1 here*/ ourzone,
						unit->TClass->MZone,
						false, Point2D(1, 1), true, false, false, unit->TClass->Speed != SPEED_FLOAT);

					if (nearbycell == CELL_NONE) {
						// We couldn't find a valid cell to reach the target from
						continue;
					}

					int distance = ::Distance(nearbycell, ptr->Center_Coord().As_Cell());

					WeaponSlotType weaponslot = unit->What_Weapon_Should_I_Use(ptr);
					auto weaponinfo = unit->Get_Weapon(weaponslot);
					if (weaponinfo->Weapon == nullptr) {
						continue;
					}

					if ((distance * CELL_LEPTON_W) >= weaponinfo->Weapon->Range) {
						continue;
					}
				}
			}

			int dist = -1;

			switch (prop) {
			case TPROPERTY_LEAST_THREAT:
				dist = INT_MAX - Map.Cell_Threat(ptr->Center_Coord().As_Cell(), unit->House);
				break;

			case TPROPERTY_GREATEST_THREAT:
				dist = Map.Cell_Threat(ptr->Center_Coord().As_Cell(), unit->House);
				break;

			case TPROPERTY_NEAREST:
				dist = INT_MAX - ptr->Get_Coord().Distance_To(unit->Get_Coord());
				break;

			case TPROPERTY_FARTHEST:
				dist = ptr->Get_Coord().Distance_To(unit->Get_Coord());
				break;

			}

			if (dist > best_same_dist && same_house) {
				best_same_ptr = ptr;
				best_same_dist = dist;
			}
			if (dist > best_dist) {
				best_ptr = ptr;
				best_dist = dist;
			}
		}
	}

	if (best_same_ptr) {
		return(best_same_ptr);
	}

	if (!only_enemy) {
		return(best_ptr);
	}

	return(NULL);
}


/**
 *  Main function for patching the hooks.
 */
void TeamClassExtension_Hooks()
{
    TeamClassExtension_Init();

    Patch_Jump(0x00622B2C, &_TeamClass_AI_MoveCell_FixCellCalc_Patch);
    Patch_Jump(0x00625886, &_TeamClass_TMission_PATROL_WaypointMax);
    Patch_Jump(0x00623780, &TeamClassExt::_Add);
	Patch_Jump(0x00623A00, &TeamClassExt::_Remove);
	Patch_Jump(0x00625B90, &TeamClassExt::_TMission_ATTACK);
	Patch_Jump(0x00623680, &TeamClassExt::_Recalc_Strength);
	Patch_Jump(0x006271F0, &_Pick_Building_With_Property);
}
