/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera
 *
 *  @file          HOUSEEXT_HOOKS.CPP
 *
 *  @author        CCHyper
 *
 *  @brief         Contains the hooks for the extended HouseClass.
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
#include "houseext_hooks.h"
#include "houseext_init.h"
#include "vinifera_globals.h"
#include "tibsun_globals.h"
#include "tibsun_inline.h"
#include "building.h"
#include "house.h"
#include "housetype.h"
#include "houseext.h"
#include "houseext.h"
#include "building.h"
#include "buildingext.h"
#include "buildingtype.h"
#include "buildingtypeext.h"
#include "unit.h"
#include "infantry.h"
#include "technotype.h"
#include "super.h"
#include "factory.h"
#include "techno.h"
#include "terrain.h"
#include "terraintype.h"
#include "unittype.h"
#include "unittypeext.h"
#include "extension.h"
#include "techno.h"
#include "super.h"
#include "rules.h"
#include "rulesext.h"
#include "scenario.h"
#include "scenarioext.h"
#include "session.h"
#include "mouse.h"
#include "fatal.h"
#include "debughandler.h"
#include "asserthandler.h"
#include "extension_globals.h"
#include "sidebarext.h"

#include "hooker.h"
#include "hooker_macros.h"
#include "tibsun_functions.h"


/**
  *  A fake class for implementing new member functions which allow
  *  access to the "this" pointer of the intended class.
  *
  *  @note: This must not contain a constructor or deconstructor!
  *  @note: All functions must be prefixed with "_" to prevent accidental virtualization.
  */
static class HouseClassExt final : public HouseClass
{
public:
    ProdFailType _Begin_Production(RTTIType type, int id, bool resume);
    ProdFailType _Abandon_Production(RTTIType type, int id);
    bool _AI_Target_MultiMissile(SuperClass* super);
    int _AI_Building_Replacement(void);
};


/**
 *  Reimplements the entire HouseClass::Begin_Production function.
 *
 *  @author: ZivDero
 */
ProdFailType HouseClassExt::_Begin_Production(RTTIType type, int id, bool resume)
{
    bool has_suspended = false;
    bool suspend = false;
    FactoryClass* fptr;
    TechnoTypeClass const* tech = Fetch_Techno_Type(type, id);

    if (!tech->Who_Can_Build_Me(false, true, true, this))
    {
        if (!resume || !tech->Who_Can_Build_Me(true, false, true, this))
        {
            DEBUG_INFO("Request to Begin_Production of '%s' was rejected. No-one can build.\n", tech->FullName);
            return PROD_CANT;
        }
        suspend = true;
    }

    fptr = Fetch_Factory(type);

    /*
    **	If no factory exists, create one.
    */
    if (fptr == nullptr)
    {
        fptr = new FactoryClass();
        if (!fptr)
        {
            DEBUG_INFO("Request to Begin_Production of '%s' was rejected. Unable to create factory\n", tech->FullName);
            return PROD_CANT;
        }
        Set_Factory(type, fptr);
    }

    /*
    **	If the house is already busy producing a building, then
    **	return with this failure code.
    */
    if (fptr->Is_Building() && type == RTTI_BUILDINGTYPE)
    {
        DEBUG_INFO("Request to Begin_Production of '%s' was rejected. Cannot queue buildings.\n", tech->FullName);
        return PROD_CANT;
    }

    /*
    **	Check if we have an object of this type currently suspended in production.
    */
    if (fptr->IsSuspended)
    {
        ObjectClass* obj = fptr->Get_Object();
        if (obj != nullptr && obj->Techno_Type_Class() == tech)
        {
            has_suspended = true;
        }
    }

    if (has_suspended || fptr->Set(*tech, *this, resume))
    {
        if (has_suspended || resume || fptr->Queued_Object_Count() == 0)
        {
            fptr->Start(suspend);

            /*
            **	Link this factory to the sidebar so that proper graphic feedback
            **	can take place.
            */
            if (PlayerPtr == this)
                Map.Factory_Link(fptr, type, id);

            return PROD_OK;
        }
        else
        {
            SidebarExtension->Flag_Strip_To_Redraw(type);
            return PROD_OK;
        }
    }

    DEBUG_INFO("Request to Begin_Production of '%s' was rejected. Factory was unable to create the requested object\n", tech->Full_Name());

    /*
    **	If the factory has queued objects or is currently
    **  building an object, reject production.
    */
    if (fptr->Queued_Object_Count() > 0 || fptr->Object)
        return PROD_CANT;


    /*
    **	Output debug information if production failed.
    */
    DEBUG_INFO("type=%d\n", type);
    DEBUG_INFO("Frame == %d\n", Frame);
    DEBUG_INFO("fptr->QueuedObjects.Count() == %d\n", fptr->QueuedObjects.Count());
    if (fptr->Get_Object())
    {
        DEBUG_INFO("Object->RTTI == %d\n", fptr->Object->Kind_Of());
        DEBUG_INFO("Object->HeapID == %d\n", fptr->Object->Get_Heap_ID());
    }
    DEBUG_INFO("IsSuspended\t= %d\n", fptr->IsSuspended);

    delete fptr;
    Set_Factory(type, nullptr);

    return PROD_CANT;
}


/**
 *  Reimplements the entire HouseClass::Abandon_Production function.
 *
 *  @author: ZivDero
 */
ProdFailType HouseClassExt::_Abandon_Production(RTTIType type, int id)
{
    FactoryClass* fptr = Fetch_Factory(type);

    /*
    **	If there is no factory to abandon, then return with a failure code.
    */
    if (fptr == nullptr)
        return PROD_CANT;

    /*
    **	If we're just dequeuing a unit, redraw the strip.
    */
    if (fptr->Queued_Object_Count() > 0 && id >= 0)
    {
        const TechnoTypeClass* technotype = Fetch_Techno_Type(type, id);
        if (fptr->Remove_From_Queue(*technotype))
        {
            SidebarExtension->Flag_Strip_To_Redraw(type);
            return PROD_OK;
        }
    }

    if (id != -1)
    {
        ObjectClass* obj = fptr->Get_Object();
        if (obj == nullptr)
            return PROD_OK;

        ObjectTypeClass* cls = obj->Class_Of();
        if (id != cls->Get_Heap_ID())
            return PROD_OK;
    }

    /*
    **	Tell the sidebar that it needs to be redrawn because of this.
    */
    if (PlayerPtr == this)
    {
        Map.Abandon_Production(type, fptr);
        if (type == RTTI_BUILDINGTYPE || type == RTTI_BUILDING)
        {
            Map.PendingObjectPtr = nullptr;
            Map.PendingObject = nullptr;
            Map.PendingHouse = HOUSE_NONE;
            Map.Set_Cursor_Shape(nullptr);
        }
    }

    /*
    **	Abandon production of the object.
    */
    fptr->Abandon();
    if (fptr->Queued_Object_Count() > 0)
    {
        fptr->Resume_Queue();
        return PROD_OK;
    }

    Set_Factory(type, nullptr);
    delete fptr;

    return PROD_OK;
}


/**
 *  Patch for InstantSuperRechargeCommandClass
 * 
 *  @author: CCHyper
 */
DECLARE_PATCH(_HouseClass_Super_Weapon_Handler_InstantRecharge_Patch)
{
    GET_REGISTER_STATIC(HouseClass *, this_ptr, edi);
    GET_REGISTER_STATIC(SuperClass *, special, esi);
    static bool is_player;

    is_player = false;
    if (this_ptr == PlayerPtr) {
        is_player = true;
    }

    if (Vinifera_DeveloperMode) {

        if (!special->IsReady) {

            /**
             *  If AIInstantBuild is toggled on, make sure this is a non-human AI house.
             */
            if (Vinifera_Developer_AIInstantSuperRecharge
                && !this_ptr->Is_Human_Control() && this_ptr != PlayerPtr) {

                special->Forced_Charge(is_player);

            /**
             *  If InstantBuild is toggled on, make sure the local player is a human house.
             */
            } else if (Vinifera_Developer_InstantSuperRecharge
                && this_ptr->Is_Human_Control() && this_ptr == PlayerPtr) {
                
                special->Forced_Charge(is_player);

            /**
             *  If the AI has taken control of the player house, it needs a special
             *  case to handle the "player" instant recharge mode.
             */
            } else if (Vinifera_Developer_InstantSuperRecharge) {
                if (Vinifera_Developer_AIControl && this_ptr == PlayerPtr) {
                    
                    special->Forced_Charge(is_player);
                }
            }

        }

    }

    /**
     *  Stolen bytes/code.
     */
    if (!special->AI(is_player)) {
        goto continue_function;
    }

add_to_sidebar:
    JMP(0x004BD320);

continue_function:
    JMP(0x004BD332);
}


/**
 *  Patch for BuildCheatCommandClass
 * 
 *  @author: CCHyper
 */
DECLARE_PATCH(_HouseClass_Can_Build_BuildCheat_Patch)
{
    GET_REGISTER_STATIC(HouseClass *, this_ptr, ebp);
    GET_REGISTER_STATIC(int, vector_count, ecx);
    GET_STACK_STATIC(TechnoTypeClass *, objecttype, esp, 0x30);

    if (Vinifera_DeveloperMode && Vinifera_Developer_BuildCheat) {

        /**
         *  AI houses have access to everything, so we can just
         *  filter to the human houses only.
         */
        if (this_ptr->IsHuman && this_ptr->IsPlayerControl) {

            /**
             *  Check that the object has this house set as one of its owners.
             *  if true, force this 
             */
            if (((1 << this_ptr->Class->ID) & objecttype->Get_Ownable()) != 0) {
                //DEBUG_INFO("Forcing \"%s\" available.\n", objecttype->IniName);
                goto return_true;
            }
        }
    }

    /**
     *  Stolen bytes/code.
     */
original_code:
    _asm { xor eax, eax }
    _asm { mov [esp+0x34], eax }

    _asm { mov ecx, vector_count }
    _asm { test ecx, ecx }

    JMP_REG(ecx, 0x004BBD2E); // Need to use ECX as EAX is used later on.

return_true:
    JMP(0x004BBD17);
}


/**
 *  #issue-611, #issue-715
 *
 *  Gets the number of queued objects when determining whether a cameo
 *  should be disabled.
 *
 *  Author: Rampastring
 */
int _HouseClass_ShouldDisableCameo_Get_Queued_Count(FactoryClass* factory, TechnoTypeClass* technotype)
{
    int count = factory->Total_Queued(*technotype);
    TechnoClass* factoryobject = factory->Get_Object();

    if (factoryobject == nullptr || count == 0) {
        return 0;
    }

    /**
     *  Check that the factory is trying to create the object that the player is trying to queue
     *  If not, we don't need to mess with the count
     */
    if (factoryobject->Techno_Type_Class() != technotype) {
        return count;
    }

    /**
     *  #issue-611
     *
     *  If the object has a build limit, then reduce count by 1.
     *  In this state, the object is taken into account twice: in the object trackers
     *  and in the factory, resulting in the player being able to queue one object less
     *  than BuildLimit allows.
     */
    if (technotype->BuildLimit > 0) {
        count--;
    }

    /**
    *  #issue-715
    *
    *  If the object can transform into another object through our special logic,
    *  then check that doing so doesn't allow circumventing build limits
    */
    if (technotype->What_Am_I() == RTTI_UNITTYPE) {
        UnitTypeClass* unittype = reinterpret_cast<UnitTypeClass*>(technotype);
        UnitTypeClassExtension* unittypeext = Extension::Fetch<UnitTypeClassExtension>(unittype);

        if (unittype->DeploysInto == nullptr && unittypeext->TransformsInto != nullptr) {
            count += factory->House->UQuantity.Count_Of((UnitType)(unittypeext->TransformsInto->Get_Heap_ID()));
        }
    }

    return count;
}


/**
 *  #issue-611 #issue-715
 *
 *  Fixes the game allowing the player to queue one unit too few
 *  when a unit has BuildLimit > 1.
 *
 *  Also updates the build limit logic with unit queuing to
 *  take our unit transformation logic into account.
 */
DECLARE_PATCH(_HouseClass_ShouldDisableCameo_BuildLimit_Fix)
{
    GET_REGISTER_STATIC(FactoryClass*, factory, ecx);
    GET_REGISTER_STATIC(TechnoTypeClass*, technotype, esi);
    static int queuedcount;

    queuedcount = _HouseClass_ShouldDisableCameo_Get_Queued_Count(factory, technotype);

    _asm { mov eax, [queuedcount] }
    JMP_REG(ecx, 0x004CB77D);
}


/**
 *  #issue-715
 *
 *  Take vehicles that can transform into other vehicles into acccount when
 *  determining whether a build limit has been met/exceeded.
 *  Otherwise these kinds of units could be used to bypass build limits
 *  (build a limited vehicle, transform it, now you can build another vehicle).
 *
 *  Author: Rampastring
 */
DECLARE_PATCH(_HouseClass_Can_Build_BuildLimit_Handle_Vehicle_Transform)
{
    GET_REGISTER_STATIC(UnitTypeClass*, unittype, edi);
    GET_REGISTER_STATIC(HouseClass*, house, ebp);
    static UnitTypeClassExtension* unittypeext;
    static int objectcount;

    unittypeext = Extension::Fetch<UnitTypeClassExtension>(unittype);

    /**
     *  Stolen bytes / code.
     */
    objectcount = house->UQuantity.Count_Of((UnitType)unittype->Get_Heap_ID());

    /**
     *  Check whether this unit can deploy into a building.
     *  If it can, increment the object count by the number of buildings.
     */
    if (unittype->DeploysInto != nullptr) {
        objectcount += house->BQuantity.Count_Of((BuildingType)unittype->DeploysInto->Get_Heap_ID());
    }
    else if (unittypeext->TransformsInto != nullptr) {

        /**
         *  This unit can transform into another unit, increment the object count
         *  by the number of transformed units.
         */
        objectcount += house->UQuantity.Count_Of((UnitType)(unittypeext->TransformsInto->Get_Heap_ID()));
    }

    _asm { mov esi, objectcount }

continue_function:
    JMP(0x004BC1B9);
}


/**
 *  #issue-994
 *
 *  Fixes a bug where a superweapon was enabled in non-suspended mode
 *  when the scenario was started with a pre-placed powered-down superweapon
 *  building on the map.
 *
 *  Author: Rampastring
 */
DECLARE_PATCH(_HouseClass_Enable_SWs_Check_For_Building_Power)
{
    GET_REGISTER_STATIC(int, quiet, eax);
    GET_REGISTER_STATIC(BuildingClass*, building, esi);

    if (!building->IsPowerOn)
    {
        /**
         *  Enable the superweapon in suspended mode.
         */
        _asm { mov eax, 1 }
    }
    else
    {
        /**
         *  Enable the superweapon in non-suspended mode.
         */
        _asm {xor eax, eax }
    }

    /**
     *  Stolen bytes/code.
     */
    _asm { mov  esi, [PlayerPtr] }

    /**
     *  Continue the SW enablement process.
     */
    JMP_REG(ecx, 0x004CB6C7);
}


/**
 *  Helper function. Returns the value of an object for super-weapon targeting.
 */
int SuperTargeting_Evaluate_Object(HouseClass* house, HouseClass* enemy, TechnoClass* techno)
{
    int threat = -1;

    if (techno->Owning_House() == enemy) {

        if (techno->Cloak == CLOAKED || (techno->What_Am_I() == RTTI_BUILDING && reinterpret_cast<BuildingClass*>(techno)->TranslucencyLevel == 0xF)) {
            threat = Scen->RandomNumber(0, 100);
        }
        else {
            Cell targetcell = Coord_Cell(techno->Center_Coord());
            threat = Map.Cell_Threat(targetcell, house);
        }
    }

    return threat;
}


/**
 *  #issue-700
 *
 *  Custom implementation of the multi-missile super weapon AI targeting.
 *  This is functionally identical to the original game's function
 *  at 0x004CA4A0 aside from also considering vehicles as potential targets.
 *  The original function only evaluated buildings.
 *
 *  Author: Rampastring
 */
bool Vinifera_HouseClass_AI_Target_MultiMissile(HouseClass* this_ptr, SuperClass* super)
{
    if (this_ptr->Enemy == HOUSE_NONE) {
        return false;
    }

    ObjectClass* besttarget = nullptr;
    int          highestthreat = -1;
    HouseClass* enemyhouse = Houses[this_ptr->Enemy];

    for (int i = 0; i < Buildings.Count(); i++) {
        BuildingClass* target = Buildings[i];
        int threat = SuperTargeting_Evaluate_Object(this_ptr, enemyhouse, target);

        if (threat > highestthreat) {
            highestthreat = threat;
            besttarget = target;
        }
    }

    // AI improvement: also go through enemy units and infantry
    for (int i = 0; i < Units.Count(); i++) {
        UnitClass* target = Units[i];
        int threat = SuperTargeting_Evaluate_Object(this_ptr, enemyhouse, target);

        if (threat > highestthreat) {
            highestthreat = threat;
            besttarget = target;
        }
    }

    for (int i = 0; i < Infantry.Count(); i++) {
        InfantryClass* target = Infantry[i];
        int threat = SuperTargeting_Evaluate_Object(this_ptr, enemyhouse, target);

        if (threat > highestthreat) {
            highestthreat = threat;
            besttarget = target;
        }
    }

    if (besttarget) {
        Coordinate center = besttarget->Center_Coord();
        Cell targetcell = Coord_Cell(center);
        int superid = this_ptr->SuperWeapon.ID(super);
        bool result = this_ptr->SuperWeapon[superid]->Discharged(this_ptr == PlayerPtr, targetcell);
        return result;
    }

    return false;
}


bool HouseClassExt::_AI_Target_MultiMissile(SuperClass* super)
{
    return Vinifera_HouseClass_AI_Target_MultiMissile(this, super);
}


bool AdvAI_House_Search_For_Next_Expansion_Point(HouseClass* house)
{
    HouseClassExtension* ext = Extension::Fetch<HouseClassExtension>(house);

    if (ext->NextExpansionPointLocation.X != 0 && ext->NextExpansionPointLocation.Y != 0) {
        return false;
    }

    // Fetch our first ConYard.
    BuildingClass* firstbuilding = nullptr;
    for (int i = 0; i < Buildings.Count(); i++) {
        BuildingClass* building = Buildings[i];
        if (building->IsActive && !building->IsInLimbo && building->House == house && building->Class->ToBuild == RTTI_BUILDINGTYPE) {
            firstbuilding = Buildings[i];
            break;
        }
    }

    if (firstbuilding == nullptr) {
        return false;
    }

    // Scan through terrain objects that spawn Tiberium, pick the closest one that does not have a refinery near it yet
    int nearestdistance = INT_MAX;
    Cell target = Cell();

    for (int i = 0; i < Terrains.Count(); i++) {
        TerrainClass* terrain = Terrains[i];
        if (terrain->IsActive && !terrain->IsInLimbo && terrain->Class->IsSpawnsTiberium) {

            Cell terraincell = terrain->Get_Cell();

            // Fetch the cell of the terrain. If the cell has overlay on it,
            // we should not expand towards it. This allows a way for mappers to mark
            // that the AI should not expand towards specific Tiberium trees.
            CellClass& cell = Map[terraincell];
            if (cell.Overlay != OVERLAY_NONE) {
                continue;
            }

            bool found = false;
            for (int j = 0; j < Buildings.Count(); j++) {
                BuildingClass* building = Buildings[j];

                if (!building->IsActive || building->IsInLimbo || !building->Class->IsRefinery) {
                    continue;
                }

                // Check if any existing AI refinery has been assigned for this expansion point yet.
                // If yes, consider it occupied, but only if it is ours.
                BuildingClassExtension* buildingext = Extension::Fetch<BuildingClassExtension>(building);
                if (building->House == house && buildingext->AssignedExpansionPoint == terraincell) {
                    found = true;
                    break;
                }

                // Not all refineries have an assigned expansion point. For example, initial
                // base refineries and human players' refineries do not.
                // For these refineries, we rely on a distance check.
                int dist = ::Distance(building->Get_Cell(), terraincell);
                if (dist < 15) {
                    found = true;
                    break;
                }
            }

            if (found)
                continue; // Someone is already occupying this Tiberium tree

            int distance = ::Distance(firstbuilding->Center_Coord(), terrain->Center_Coord());
            if (distance < nearestdistance) {
                // Don't expand super far.
                if (distance / CELL_LEPTON_W < RuleExtension->AdvancedAIMaxExpansionDistance) {
                    nearestdistance = distance;
                    target = terrain->Get_Cell();
                }
            }
        }
    }

    if (target.X == 0 || target.Y == 0) {
        // We couldn't find anywhere to expand towards
        return false;
    }

    ext->NextExpansionPointLocation = target;

    return true;
}


bool AdvAI_Can_Build_Building(HouseClass* house, BuildingTypeClass* buildingtype, bool check_prereqs)
{
    ASSERT_FATAL(BuildingTypes.ID(buildingtype) == buildingtype->Get_Heap_ID());
    if (buildingtype->What_Am_I() != RTTI_BUILDINGTYPE) {
        DEBUG_ERROR("Invalid BuildingTypeClass pointer in AdvAI_Can_Build_Building!!!");
        Emergency_Exit(0);
    }

    // DEBUG_INFO("Checking if AI %d can build %s. ", house->Get_Heap_ID(), buildingtype->IniName);

    if ((int)buildingtype->TechLevel > house->Control.TechLevel) {
        // DEBUG_INFO("Result: false (TechLevel)\n");
        return false;
    }

    if (!buildingtype->CanAIBuildThis) {
        // DEBUG_INFO("Result: false (AIBuildThis)\n");
        return false;
    }

    if ((buildingtype->Ownable & (1 << house->ActLike)) != (1 << house->ActLike)) {
        // DEBUG_INFO("Result: false (Ownable)\n");
        return false;
    }

    BuildingTypeClassExtension* buildingtypeext = Extension::Fetch<BuildingTypeClassExtension>(buildingtype);

    if (check_prereqs && !buildingtypeext->IsAdvancedAIIgnoresPrerequisites) {
        for (int i = 0; i < buildingtype->Prerequisite.Count(); i++) {
            int buildingtypeid = buildingtype->Prerequisite[i];
            
            if (buildingtypeid < 0) {
                // TODO handle prerequisite groups
            }
            else if (buildingtypeid >= 0 && house->ActiveBQuantity.Count_Of((BuildingType)buildingtypeid) == 0) {
                // DEBUG_INFO("Result: false (Prerequisite %d: %d %s)\n", i, buildingtypeid, BuildingTypes[buildingtypeid]->IniName);
                return false;
            }
        }
    }

    // If this is an upgrade, do we have a building we could upgrade with it?
    if (buildingtype->PowersUpBuilding[0] != '\0') {
        const BuildingTypeClass* base = BuildingTypeClass::Find_Or_Make(buildingtype->PowersUpBuilding);

        if (house->ActiveBQuantity.Count_Of((BuildingType)base->Get_Heap_ID()) == 0) {
            // DEBUG_INFO("Result: false (no upgradeable buildings)\n");
            return false;
        }

        bool found = false;

        // Scan through the buildings...
        for (int i = 0; i < Buildings.Count(); i++) {
            BuildingClass* building = Buildings[i];

            if (!building->IsActive ||
                building->IsInLimbo ||
                building->Class != base ||
                building->House != house) {
                continue;
            }

            if (building->UpgradeLevel >= base->Upgrades) {
                continue;
            }

            found = true;
        }

        if (!found) {
            // DEBUG_INFO("Result: false (no upgradeable building found in scan)\n");
            return false;
        }
    }

    // DEBUG_INFO("Result: true\n");
    return true;
}


const BuildingTypeClass* AdvAI_Evaluate_Get_Best_Building(HouseClass* house) 
{
    HouseClassExtension* houseext = Extension::Fetch<HouseClassExtension>(house);

    BuildingType our_refinery = BUILDING_NONE;
    BuildingType our_basic_power = BUILDING_NONE;
    BuildingType our_advanced_power = BUILDING_NONE;

    for (int i = 0; i < Rule->BuildRefinery.Count(); i++) {
        BuildingTypeClass* refinery = Rule->BuildRefinery[i];
        if (AdvAI_Can_Build_Building(house, refinery, true)) {
            our_refinery = (BuildingType)refinery->Get_Heap_ID();
        }
    }

    for (int i = 0; i < Rule->BuildPower.Count(); i++) {
        if (AdvAI_Can_Build_Building(house, Rule->BuildPower[i], true)) {
            if (our_basic_power != BUILDING_NONE) {
                our_advanced_power = (BuildingType)Rule->BuildPower[i]->Get_Heap_ID();
            }
            else {
                our_basic_power = (BuildingType)Rule->BuildPower[i]->Get_Heap_ID();
            }
        }
    }

    // Since we do not currently know how to handle prerequisite groups, our basic and
    // advanced power plants might be reversed.
    // Check if this is the case and if so, reverse them.
    if (our_basic_power != BUILDING_NONE && our_advanced_power != BUILDING_NONE &&
        BuildingTypes[our_basic_power]->Power > BuildingTypes[our_advanced_power]->Power) {
        BuildingType tmp = our_basic_power;
        our_basic_power = our_advanced_power;
        our_advanced_power = tmp;
    }

    // If we have no power plants yet, then build one
    if (house->ActiveBQuantity.Count_Of(our_basic_power) == 0) {
        DEBUG_INFO("AdvAI: Making AI build %s because it has 0 basic power plants\n", BuildingTypes[our_basic_power]->IniName);
        return BuildingTypes[our_basic_power];
    }

    // On Medium and Hard, build a barracks if we do not have any yet
    if (house->Difficulty < DIFF_HARD && house->Credits >= Rule->AIAlternateProductionCreditCutoff) {
        for (int i = 0; i < Rule->BuildBarracks.Count(); i++) {
            BuildingTypeClass* barracks = Rule->BuildBarracks[i];

            if (AdvAI_Can_Build_Building(house, barracks, true)) {
                int barrackscount = house->ActiveBQuantity.Count_Of((BuildingType)barracks->Get_Heap_ID());
                if (barrackscount < 1) {

                    DEBUG_INFO("AdvAI: Making AI build %s because it does not have a Barracks at all.\n", barracks->IniName);

                    return barracks;
                }
            }
        }
    }

    // Build refinery if we're expanding
    if (our_refinery != BUILDING_NONE && houseext->ShouldBuildRefinery) {
        DEBUG_INFO("AdvAI: Making AI build %s because it has reached an expansion point\n", BuildingTypes[our_refinery]->IniName);
        return BuildingTypes[our_refinery];
    }

    // Build a refinery if we have 0 left
    if (our_refinery != BUILDING_NONE && house->ActiveBQuantity.Count_Of(our_refinery) == 0) {
        DEBUG_INFO("AdvAI: Making AI build %s because it has 0 refineries\n", BuildingTypes[our_refinery]->IniName);
        return BuildingTypes[our_refinery];
    }

    // Build power if necessary
    if (Frame > 5000 && house->Power - house->Drain < 100) {
        if (our_advanced_power != BUILDING_NONE) {
            DEBUG_INFO("AdvAI: Making AI build %s because it is out of power and can build an adv. power plant\n", BuildingTypes[our_advanced_power]->IniName);
            return BuildingTypes[our_advanced_power];
        }

        if (our_basic_power != BUILDING_NONE) {
            DEBUG_INFO("AdvAI: Making AI build %s because it is out of power and can only build a basic power plant\n", BuildingTypes[our_basic_power]->IniName);
            return BuildingTypes[our_basic_power];
        }
    }

    // If we don't have enough barracks, then build one
    int optimal_barracks_count = 1 + (house->ActiveBQuantity.Count_Of(our_refinery) / 3);

    for (int i = 0; i < Rule->BuildBarracks.Count(); i++) {
        BuildingTypeClass* barracks = Rule->BuildBarracks[i];

        if (AdvAI_Can_Build_Building(house, barracks, true)) {
            int barrackscount = house->ActiveBQuantity.Count_Of((BuildingType)barracks->Get_Heap_ID());
            if (barrackscount < optimal_barracks_count) {

                DEBUG_INFO("AdvAI: Making AI build %s because it does not have enough Barracks. Wanted: %d, current: %d\n",
                    barracks->IniName, optimal_barracks_count, barrackscount);

                return barracks;
            }
        }
    }

    // If we don't have enough weapons factory, then build one
    int optimal_wf_count = 1 + (house->ActiveBQuantity.Count_Of(our_refinery) / 4);

    for (int i = 0; i < Rule->BuildWeapons.Count(); i++) {
        BuildingTypeClass* weaponsfactory = Rule->BuildWeapons[i];

        if (AdvAI_Can_Build_Building(house, weaponsfactory, true)) {
            int wfcount = house->ActiveBQuantity.Count_Of((BuildingType)weaponsfactory->Get_Heap_ID());
            if (wfcount < optimal_wf_count) {

                DEBUG_INFO("AdvAI: Making AI build %s because it does not have enough Weapons Factories. Wanted: %d, current: %d\n",
                    weaponsfactory->IniName, optimal_wf_count, wfcount);

                return weaponsfactory;
            }
        }
    }

    // If we have too few refineries, build enough to match the minimum.
    // Because this is not for expanding but an emergency situation,
    // cancel any potential expanding.
    if (house->ActiveBQuantity.Count_Of(our_refinery) < RuleExtension->AdvancedAIMinimumRefineryCount) {
        houseext->NextExpansionPointLocation = Cell(0, 0);
        DEBUG_INFO("AdvAI: Making AI build %s because it only has 1 refinery\n", BuildingTypes[our_refinery]->IniName);
        return BuildingTypes[our_refinery];
    }

    // If we don't have enough defenses, then build one
    BuildingType our_anti_infantry_defense = BUILDING_NONE;
    BuildingType our_anti_vehicle_defense = BUILDING_NONE;

    int best_anti_infantry_rating = INT_MIN;
    int best_anti_vehicle_rating = INT_MIN;

    for (int i = 0; i < Rule->BuildDefense.Count(); i++) {

        BuildingTypeClass* buildingtype = Rule->BuildDefense[i];

        if (AdvAI_Can_Build_Building(house, buildingtype, true)) {
            if (buildingtype->AntiInfantryValue > best_anti_infantry_rating) {
                best_anti_infantry_rating = buildingtype->AntiInfantryValue;
                our_anti_infantry_defense = (BuildingType)buildingtype->Get_Heap_ID();
            }

            if (buildingtype->AntiArmorValue > best_anti_vehicle_rating) {
                best_anti_vehicle_rating = buildingtype->AntiArmorValue;
                our_anti_vehicle_defense = (BuildingType)buildingtype->Get_Heap_ID();
            }
        }
    }

    int optimal_defense_count = house->ActiveBQuantity.Count_Of(our_refinery) + (house->ActiveBQuantity.Count_Of(our_basic_power) + house->ActiveBQuantity.Count_Of(our_advanced_power)) / 4;
    if (houseext->NextExpansionPointLocation.X > 0 && houseext->NextExpansionPointLocation.Y > 0) {
        optimal_defense_count++;
    }

    // If we are under attack, prioritize defense.
    if (house->LATime + TICKS_PER_MINUTE > Frame) {
        optimal_defense_count++;
    }

    if (our_anti_infantry_defense != BUILDING_NONE) {
        int defensecount = house->ActiveBQuantity.Count_Of(our_anti_infantry_defense);
        if (defensecount < optimal_defense_count) {
            DEBUG_INFO("AdvAI: Making AI build %s because it does not have enough anti-inf defenses. Wanted: %d, current: %d\n",
                BuildingTypes[our_anti_infantry_defense]->IniName, optimal_defense_count, defensecount);
            return BuildingTypes[our_anti_infantry_defense];
        }
    }

    // If we have no radar, then build one
    for (int i = 0; i < Rule->BuildRadar.Count(); i++) {
        BuildingTypeClass* radar = Rule->BuildRadar[i];

        // Don't check prereqs to hack around TDPROC vs TDPROC_AI difference
        if (AdvAI_Can_Build_Building(house, radar, false)) {
            int radarcount = house->ActiveBQuantity.Count_Of((BuildingType)radar->Get_Heap_ID());

            if (radarcount < 1) {
                DEBUG_INFO("AdvAI: Making AI build %s because it does not have enough radars. Current count: %d\n",
                    radar->IniName, radarcount);

                return radar;
            }
        }
    }

    // Build some anti-vehicle defenses if we should
    if (our_anti_infantry_defense != our_anti_vehicle_defense && our_anti_vehicle_defense != BUILDING_NONE) {
        int defensecount = house->ActiveBQuantity.Count_Of(our_anti_vehicle_defense);

        if (house->ActiveBQuantity.Count_Of(our_anti_vehicle_defense) < optimal_defense_count) {
            DEBUG_INFO("AdvAI: Making AI build %s because it does not have enough anti-vehicle defenses. Wanted: %d, current: %d\n",
                BuildingTypes[our_anti_vehicle_defense]->IniName, optimal_defense_count, defensecount);
            return BuildingTypes[our_anti_vehicle_defense];
        }
    }

    // Build some AA if we should.
    BuildingType our_anti_air_defense = BUILDING_NONE;

    for (int i = 0; i < Rule->BuildAA.Count(); i++) {
        if (AdvAI_Can_Build_Building(house, Rule->BuildAA[i], true)) {
            our_anti_air_defense = (BuildingType)Rule->BuildAA[i]->Get_Heap_ID();
        }
    }

    if (our_anti_air_defense != BUILDING_NONE) {
        int defensecount = house->ActiveBQuantity.Count_Of(our_anti_air_defense);

        if (house->ActiveBQuantity.Count_Of(our_anti_air_defense) < optimal_defense_count) {
            DEBUG_INFO("AdvAI: Making AI build %s because it does not have enough anti-air defenses. Wanted: %d, current: %d\n",
                BuildingTypes[our_anti_air_defense]->IniName, optimal_defense_count, defensecount);
            return BuildingTypes[our_anti_air_defense];
        }
    }

    // If we don't have enough helipads, then build one
    int optimal_helipad_count = 1 + (house->ActiveBQuantity.Count_Of(our_refinery) / 2);

    for (int i = 0; i < Rule->BuildHelipad.Count(); i++) {
        BuildingTypeClass* helipad = Rule->BuildHelipad[i];

        if (AdvAI_Can_Build_Building(house, helipad, true)) {
            int helipadcount = house->ActiveBQuantity.Count_Of((BuildingType)helipad->Get_Heap_ID());

            if (helipadcount < optimal_helipad_count) {
                DEBUG_INFO("AdvAI: Making AI build %s because it does not have enough helipads. Wanted: %d, current: %d\n",
                    helipad->IniName, optimal_helipad_count, helipadcount);

                return helipad;
            }
        }
    }

    // If we have no tech center, then build one
    for (int i = 0; i < Rule->BuildTech.Count(); i++) {
        BuildingTypeClass* techcenter = Rule->BuildTech[i];
        if (AdvAI_Can_Build_Building(house, techcenter, true)) {
            if (house->ActiveBQuantity.Count_Of((BuildingType)techcenter->Get_Heap_ID()) < 1) {
                DEBUG_INFO("AdvAI: Making AI build %s because it does not have a tech center.\n",
                    techcenter->IniName);

                return techcenter;
            }
        }
    }

    // Build some advanced defenses if we do not have enough
    int optimal_adv_defense_count = optimal_defense_count / 2;

    BuildingType our_adv_defense = BUILDING_NONE;

    for (int i = 0; i < Rule->BuildPDefense.Count(); i++) {
        if (AdvAI_Can_Build_Building(house, Rule->BuildPDefense[i], true)) {
            our_adv_defense = (BuildingType)Rule->BuildPDefense[i]->Get_Heap_ID();
        }
    }

    if (our_adv_defense != BUILDING_NONE) {
        int advdefensecount = house->ActiveBQuantity.Count_Of(our_adv_defense);

        if (advdefensecount < optimal_adv_defense_count) {
            DEBUG_INFO("AdvAI: Making AI build %s because it does not have enough. Wanted: %d, current: %d.\n",
                BuildingTypes[our_adv_defense]->IniName, optimal_adv_defense_count, advdefensecount);

            return BuildingTypes[our_adv_defense];
        }
    }

    // Are there other AIBuildThis=yes buildings that we haven't built yet?
    for (int i = 0; i < BuildingTypes.Count(); i++) {
        if (BuildingTypes[i]->CanAIBuildThis) {
            if (AdvAI_Can_Build_Building(house, BuildingTypes[i], false)) {
                if (house->ActiveBQuantity.Count_Of((BuildingType)i) < 1) {
                    DEBUG_INFO("AdvAI: Making AI build %s because it has AIBuildThis=yes and the AI has none.\n",
                        BuildingTypes[i]->IniName);
                    return BuildingTypes[i];
                }
            }
        }
    }

    // Build power by default, but only if we have somewhere to expand towards.
    if (houseext->NextExpansionPointLocation.X != 0 && houseext->NextExpansionPointLocation.Y != 0) {
        if (our_basic_power != BUILDING_NONE) {
            DEBUG_INFO("AdvAI: Making AI build %s because it the AI is expanding.\n",
                BuildingTypes[our_basic_power]->IniName);
            return BuildingTypes[our_basic_power];
        }
    }

    return nullptr;
}


const BuildingTypeClass* AdvAI_Get_Building_To_Build(HouseClass* house) 
{
    const BuildingTypeClass* buildchoice = AdvAI_Evaluate_Get_Best_Building(house);

    if (buildchoice == nullptr) {
        return nullptr;
    }

    // If our power budget couldn't afford the building, then build a power plant first instead.
    // Unless it's a refinery that we're building, those are considered more critical.
    if (buildchoice->Drain > 0 && !buildchoice->IsRefinery && (house->Drain + buildchoice->Drain > house->Power)) {
        BuildingType our_basic_power = BUILDING_NONE;
        BuildingType our_advanced_power = BUILDING_NONE;

        for (int i = 0; i < Rule->BuildPower.Count(); i++) {
            if (AdvAI_Can_Build_Building(house, Rule->BuildPower[i], true)) {
                if (our_basic_power != BUILDING_NONE) {
                    our_advanced_power = (BuildingType)Rule->BuildPower[i]->Get_Heap_ID();
                }
                else {
                    our_basic_power = (BuildingType)Rule->BuildPower[i]->Get_Heap_ID();
                }
            }
        }

        if (our_advanced_power != BUILDING_NONE) {
            return BuildingTypes[our_advanced_power];
        }

        if (our_basic_power != BUILDING_NONE) {
            return BuildingTypes[our_basic_power];
        }
    }

    return buildchoice;
}


/**
 *  Checks if AdvAI should raise money.
 *  If it should, then raises money.
 *
 *  Author: Rampastring
 */
void AdvAI_Raise_Money(HouseClass* house)
{
    // We should raise money if we are low on funds and have zero refineries.

    if (house->Credits > 100) {
        return;
    }

    BuildingType our_refinery = BUILDING_NONE;

    for (int i = 0; i < Rule->BuildRefinery.Count(); i++) {
        BuildingTypeClass* refinery = Rule->BuildRefinery[i];
        if (AdvAI_Can_Build_Building(house, refinery, true)) {
            our_refinery = (BuildingType)refinery->Get_Heap_ID();
        }
    }

    if (our_refinery == BUILDING_NONE) {
        return;
    }

    int refinery_count = house->ActiveBQuantity.Count_Of(our_refinery);

    if (refinery_count > 0) {
        return;
    }

    // Look for buildings to sell.
    DEBUG_INFO("AdvAI: Attempting to raise money.\n");

    BuildingClass* bestbuilding = nullptr;
    int bestcost = INT_MIN;

    for (int i = 0; i < Buildings.Count(); i++) {
        BuildingClass* building = Buildings[i];

        if (!building->IsActive || building->IsInLimbo || building->House != house || building->Class->IsConstructionYard) {
            continue;
        }

        if (building->Mission == MISSION_CONSTRUCTION || building->MissionQueue == MISSION_CONSTRUCTION) {

            // Don't sell something that we've just built.
            continue;
        }

        if (building->Mission == MISSION_DECONSTRUCTION || building->MissionQueue == MISSION_DECONSTRUCTION) {

            // We are already in the process of selling something.
            return;
        }

        // Prefer selling the most expensive stuff first.
        // Give a lower priority to super-weapon buildings, however.
        // They'll be expensive to replace later on.
        int cost = building->Class->Cost;
        if (building->Class->SuperWeapon != SPECIAL_NONE && building->Class->SuperWeapon2 != SPECIAL_NONE) {
            cost = cost / 3;
        }

        if (cost > bestcost) {
            bestbuilding = building;
        }
    }

    // If we found something to sell, then sell it.
    if (bestbuilding != nullptr) {
        DEBUG_INFO("AdvAI: Found a building to sell.\n");
        bestbuilding->Sell_Back(1);
    }
}


/**
 *  Perfoms some general economy maintenance.
 *  Raises money if necessary.
 *
 *  Author: Rampastring
 */
void AdvAI_Economy_Upkeep(HouseClass* house)
{
    AdvAI_Raise_Money(house);

    // Don't sell refineries on Easy mode.
    if (house->Difficulty == DIFF_HARD) {
        return;
    }

    BuildingType our_refinery = BUILDING_NONE;

    for (int i = 0; i < Rule->BuildRefinery.Count(); i++) {
        BuildingTypeClass* refinery = Rule->BuildRefinery[i];
        if (AdvAI_Can_Build_Building(house, refinery, true)) {
            our_refinery = (BuildingType)refinery->Get_Heap_ID();
        }
    }

    if (our_refinery == BUILDING_NONE) {
        return;
    }

    int refinery_count = house->ActiveBQuantity.Count_Of(our_refinery);

    int harvester_count = 0;
    for (int i = 0; i < Rule->HarvesterUnit.Count(); i++) {
        UnitTypeClass* harvtype = Rule->HarvesterUnit[i];
        harvester_count += house->ActiveUQuantity.Count_Of((UnitType)harvtype->Get_Heap_ID());
    }

    // Also take free-unit harvs into account.
    // WARNING: this assumes that free-unit harvs are not listed in HarvesterUnit,
    // as the situation is in DTA at the time of writing this.
    UnitTypeClass* freeunit = BuildingTypes[our_refinery]->FreeUnit;
    harvester_count += house->ActiveUQuantity.Count_Of((UnitType)freeunit->Get_Heap_ID());

    int to_sell_count = refinery_count - harvester_count;
    if (to_sell_count <= 0) {
        return;
    }

    DEBUG_INFO("AdvAI: Looking for a refinery to sell because we have %d excess.\n", to_sell_count);

    // Sell the refinery that is closest to our primary enemy.
    // If we have extra refineries, we have lost harvesters, and harvesters are most likely
    // lost near the expansion that is closest to our primary enemy.
    // If we have no primary enemy, then sell one near our base center.
    // It probably won't go horribly wrong anyway.

    HouseClass* enemy = nullptr;
    if (house->Enemy != HOUSE_NONE) {
        enemy = HouseClass::As_Pointer(house->Enemy);
    }

    Cell centerpoint;

    if (enemy != nullptr) {
        centerpoint = enemy->Base_Center();
    }
    else {
        centerpoint = house->Base_Center();
    }

    BuildingClass* farthest_refinery = nullptr;
    int closest_distance = INT_MAX;

    for (int i = 0; i < Buildings.Count(); i++) {
        BuildingClass* building = Buildings[i];

        if (!building->IsActive || building->IsInLimbo || building->House != house || !building->Class->IsRefinery) {
            continue;
        }

        if (building->Mission == MISSION_CONSTRUCTION || building->MissionQueue == MISSION_CONSTRUCTION) {
            // If a refinery is in process of being constructed, it hasn't got the spawn its FreeUnit
            // harvester yet.
            DEBUG_INFO("AdvAI: We have a refinery in construction phase, skip.\n");
            return;
        }

        if (building->Mission == MISSION_DECONSTRUCTION || building->MissionQueue == MISSION_DECONSTRUCTION) {
            
            // We are already in the process of selling a refinery, don't sell more
            // until it's finished.
            DEBUG_INFO("AdvAI: We are already selling a refinery, skip.\n");
            return;
        }

        int distance = ::Distance(centerpoint, Coord_Cell(building->Center_Coord()));
        if (distance < closest_distance) {
            closest_distance = distance;
            farthest_refinery = building;
        }
    }

    if (farthest_refinery != nullptr) {
        DEBUG_INFO("AdvAI: Found a Refinery to sell.\n");
        farthest_refinery->Sell_Back(1);
    }
}


/**
 *  Checks for sleeping harvesters. If found, puts them to Harvest mode.
 *
 *  Author: Rampastring
 */
void AdvAI_Awaken_Sleeping_Harvesters(HouseClass* house)
{
    for (int i = 0; i < Units.Count(); i++) {
        UnitClass* unit = Units[i];

        if (!unit->IsActive || unit->IsInLimbo || unit->House != house || !unit->Class->IsToHarvest) {
            continue;
        }

        if (unit->Mission == MISSION_SLEEP || unit->Mission == MISSION_GUARD) {
            DEBUG_INFO("AdvAI: Waking up a sleeping harvester.\n");
            unit->Assign_Mission(MISSION_HARVEST);
            unit->Commence();
        }
    }
}


/**
 *  Sells extra construction yards of the specific house until there is one one left.
 *
 *  Author: Rampastring
 */
void AdvAI_Sell_Extra_ConYards(HouseClass* house) 
{
    int to_sell_count = house->ConstructionYards.Count() - 1;

    DEBUG_INFO("AdvAI: AI %d has too many Construction Yards. Selling off %d of them. Frame: %d\n", house->Get_Heap_ID(), to_sell_count, Frame);

    if (to_sell_count < 1) {
        return;
    }

    int sold_count = 0;

    for (int i = 0; i < Buildings.Count(); i++) {
        BuildingClass* building = Buildings[i];

        if (!building->IsActive || building->IsInLimbo || building->House != house || !building->Class->IsConstructionYard) {
            continue;
        }

        if (building->Mission == MISSION_DECONSTRUCTION || building->MissionQueue == MISSION_DECONSTRUCTION) {
            sold_count++;

            if (sold_count >= to_sell_count) {
                break;
            }

            continue;
        }

        DEBUG_INFO("AdvAI: Found a Construction Yard to sell.\n");

        building->Sell_Back(1);
        sold_count++;

        if (sold_count >= to_sell_count) {
            break;
        }
    }
}


/**
 *  Implements DTA's custom AI building selection logic.
 *
 *  Author: Rampastring
 */
int Vinifera_HouseClass_AI_Building(HouseClass* this_ptr)
{
    // First, do some basic maintenance.

    // If we have more than 1 ConYard without Rules allowing it, sell some of them off
    // to avoid the "Extreme AI" syndrome.
    // This would be better done on a higher level (not within building selection),
    // but for the easiness of hacking, we currently have it here.
    if (this_ptr->ConstructionYards.Count() > 1 && !RuleExtension->IsAdvancedAIMultiConYard) {
        AdvAI_Sell_Extra_ConYards(this_ptr);
    }

    HouseClassExtension* houseext = Extension::Fetch<HouseClassExtension>(this_ptr);

    if (Frame > houseext->LastExcessRefineryCheckFrame + 500) {
        houseext->LastExcessRefineryCheckFrame = Frame;
        AdvAI_Economy_Upkeep(this_ptr);
    }

    if (Frame > houseext->LastSleepingHarvesterCheckFrame + 1000) {
        houseext->LastSleepingHarvesterCheckFrame = Frame;
        AdvAI_Awaken_Sleeping_Harvesters(this_ptr);
    }

    // Next, we decide what to build.
    // If we already have something to build, do nothing.
    if (this_ptr->BuildStructure != BUILDING_NONE) return TICKS_PER_SECOND;

    if (this_ptr->ConstructionYards.Count() <= 0) return TICKS_PER_SECOND;

    if (RuleExtension->IsUseAdvancedAI) {

        // If we have nowhere to expand towards, check for a new location to expand to.
        if (houseext->NextExpansionPointLocation.X <= 0 || houseext->NextExpansionPointLocation.Y <= 0) {
            AdvAI_House_Search_For_Next_Expansion_Point(this_ptr);
        }

        const BuildingTypeClass* tobuild = AdvAI_Get_Building_To_Build(this_ptr);

        if (tobuild == nullptr) {
            return TICKS_PER_SECOND * 5;
        }

        DEBUG_INFO("AI %d selected building %s to build. Frame: %d\n", this_ptr->Get_Heap_ID(), tobuild->IniName, Frame);

        this_ptr->BuildStructure = (BuildingType)(tobuild->Get_Heap_ID());

        // Limit the tick rate a bit for better performance and fairness.
        // Also, add some randomization to reduce the "all AIs place buildings at the same time"
        // effect, avoiding a lag spike.
        return TICKS_PER_SECOND * 5 + Random_Pick(0, 3);

    } else {
        BaseNodeClass* node = this_ptr->Base.Next_Buildable();

        if (node != nullptr) {
            this_ptr->BuildStructure = node->Type;
        }
    }

    return TICKS_PER_SECOND;
}


int HouseClassExt::_AI_Building_Replacement() 
{
    return Vinifera_HouseClass_AI_Building(this);
}


/**
 *  Entry point for DTA's custom AI building selection logic.
 *
 *  Author: Rampastring
 */
DECLARE_PATCH(_HouseClass_AI_Building_Intercept)
{
    GET_REGISTER_STATIC(HouseClass*, house, ebx);
    static int retvalue;

    /**
     *  If our custom AI logic is enabled, transfer control to it and return.
     */
    if (RuleExtension->IsUseAdvancedAI) {
        retvalue = Vinifera_HouseClass_AI_Building(house);

        // Rebuild function epilogue
        _asm { pop edi }
        _asm { pop esi }
        _asm { pop ebp }
        _asm { mov eax, dword ptr ds:retvalue }
        _asm { pop ebx }
        _asm { add esp, 20h }
        _asm { retn }
    }

    /**
     *  Stolen bytes / code. Run the game's original building selection AI.
     *  If BuildStructure already has a building, then bail out.
     *  Otherwise continue the function.
     */
    if (house->BuildStructure != BUILDING_NONE) {
        JMP(0x004C1641);
    }

    JMP(0x004C10F8);
}


/**
 *  Fixes an edge case bug where HouseClass::AI_Raise_Money can corrupt
 *  the house's Base Node vector by writing to the vector at index -1.
 *
 *  Author: Rampastring
 */
DECLARE_PATCH(_HouseClass_AI_Raise_Money_Fix_Memory_Corruption)
{
    GET_REGISTER_STATIC(HouseClass*, this_ptr, esi);
    GET_REGISTER_STATIC(BuildingType, buildingtype, eax);
    static int buildable_index;

    buildable_index = this_ptr->Base.Next_Buildable_Index(buildingtype);

    // Stolen bytes / code. Do not insert element to Base Nodes vector
    // if buildable index is 0.
    if (buildable_index == 0) {
        JMP(0x004C10BC);
    }

    // Bugfix: also do not insert element if buildable index is -1. (or below 0)
    if (buildable_index < 0) {
        JMP(0x004C10BC);
    }

    // Apply node index variable and also save it in eax,
    // original game code expects this
    _asm { mov eax, dword ptr buildable_index }
    _asm { mov[esp + 28], eax }
    JMP_REG(ecx, 0x004C0F9F);
}


#if 0
/**
 *  The first base node can sometimes get corrupted for an unknown reason.
 *  Check for it and fix it if it's the case.
 */
void _HouseClass_AI_Building_Check_For_Corrupted_Base_Node(HouseClass* house)
{
    if (house->Base.Nodes.Count() > 0) {
        BuildingType type = house->Base.Nodes[0].Type;
        if (type < BUILDING_FIRST || type >= BuildingTypes.Count()) {
            DEBUG_ERROR("Corrupted base node detected for house %d, fixing it. Frame: %d\n", house->ID, Frame);
            house->Base.Nodes[0].Type = BUILDING_NONE;
            house->Base.Nodes[0].Where = Cell(0, 0);
        }
    }
}
#endif


/**
 *  Main function for patching the hooks.
 */
void HouseClassExtension_Hooks()
{
    /**
     *  Initialises the extended class.
     */
    HouseClassExtension_Init();

    Patch_Jump(0x004BBD26, &_HouseClass_Can_Build_BuildCheat_Patch);
    Patch_Jump(0x004BD30B, &_HouseClass_Super_Weapon_Handler_InstantRecharge_Patch);

    Patch_Jump(0x004BE200, &HouseClassExt::_Begin_Production);
    Patch_Jump(0x004BE6A0, &HouseClassExt::_Abandon_Production);

    Patch_Jump(0x004CB777, &_HouseClass_ShouldDisableCameo_BuildLimit_Fix);
    Patch_Jump(0x004BC187, &_HouseClass_Can_Build_BuildLimit_Handle_Vehicle_Transform);
    Patch_Jump(0x004CB6C1, &_HouseClass_Enable_SWs_Check_For_Building_Power);
    Patch_Jump(0x004CA4A0, &HouseClassExt::_AI_Target_MultiMissile);
    // Patch_Jump(0x004C10E0, &HouseClassFake::_AI_Building_Replacement);
    Patch_Jump(0x004C10F2, &_HouseClass_AI_Building_Intercept);
    Patch_Jump(0x004C0F87, &_HouseClass_AI_Raise_Money_Fix_Memory_Corruption);
    // Patch_Jump(0x004C10E8, &_HouseClass_AI_Building_Intercept);
}
