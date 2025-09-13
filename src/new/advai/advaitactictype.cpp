/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera (Dawn of the Tiberium Age Build)
 *
 *  @file          ADVAITACTICTYPE.CPP
 *
 *  @authors       Rampastring
 *
 *  @brief         Tactic class type for Advanced AI.
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
#include "advaitactictype.h"
#include "ccini.h"
#include "vinifera_globals.h"
#include "tibsun_inline.h"
#include "tibsun_globals.h"
#include "tibsun_functions.h"
#include "asserthandler.h"
#include "house.h"
#include "houseext.h"
#include "findmake.h"
#include "rules.h"
#include "rulesext.h"
#include "script.h"
#include "scripttype.h"
#include "taskforce.h"
#include "team.h"
#include "teamext.h"
#include "teamtype.h"
#include "teamtypeext.h"
#include "vinifera_saveload.h"


AdvancedAITacticTypeClass::AdvancedAITacticTypeClass()
{
    AdvancedAITacticTypes.Add(this);
}


AdvancedAITacticTypeClass::AdvancedAITacticTypeClass(const char* name) :
    IniName(""),
    GroundTacticType(AdvAITacticType::TACTIC_NONE),
    AirTacticType(AdvAIAirTacticType::AIRTACTIC_NONE),
    NavalTacticType(AdvAINavalTacticType::NAVALTACTIC_NONE),
    MinimumExpectedDuration(-1),
    MaximumExpectedDuration(-1),
    ScaleDurationByBuildTime(false),
    House(HOUSE_NONE),
    SetTacticalValueIndex(-1),
    SetTacticalValueMin(-1),
    SetTacticalValueMax(-1),
    SetTacticalValueChance(100),
    FrameBasedDurationIncrease(0),
    FrameBasedDurationIncreaseMax(0),
    FrameBasedDurationTimeStep(0),
    IsAir(false),
    IsNaval(false)
{
    ASSERT_FATAL_PRINT(name != nullptr, "Invalid name for AdvAITacticType!");

    std::strncpy(IniName, name, sizeof(IniName));

    Conditions.Clear();
    AdvAITeamTypes.Clear();
    
    AdvancedAITacticTypes.Add(this);
}

AdvancedAITacticTypeClass::~AdvancedAITacticTypeClass()
{
    Conditions.Clear();
    AdvAITeamTypes.Clear();
    AdvancedAITacticTypes.Delete(this);
}


/**
 *  Retrieves pointers to the supported interfaces on an object.
 *
 *  @author: CCHyper, tomsons26
 */
LONG AdvancedAITacticTypeClass::QueryInterface(REFIID riid, LPVOID* ppv)
{
    /**
     *  Always set out parameter to NULL, validating it first.
     */
    if (ppv == nullptr) {
        return E_POINTER;
    }
    *ppv = nullptr;

    if (riid == __uuidof(IUnknown)) {
        *ppv = reinterpret_cast<IUnknown*>(this);
    }

    if (riid == __uuidof(IStream)) {
        *ppv = reinterpret_cast<IStream*>(this);
    }

    if (riid == __uuidof(IPersistStream)) {
        *ppv = static_cast<IPersistStream*>(this);
    }

    if (*ppv == nullptr) {
        return E_NOINTERFACE;
    }

    /**
     *  Increment the reference count and return the pointer.
     */
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();

    return S_OK;
}


/**
 *  Increments the reference count for an interface pointer to a COM object.
 *
 *  @author: CCHyper
 */
ULONG AdvancedAITacticTypeClass::AddRef()
{
    //EXT_DEBUG_TRACE("AdvancedAITacticTypeClass::AddRef - 0x%08X\n", (uintptr_t)(this));

    return 1;
}


/**
 *  Decrements the reference count for an interface on a COM object.
 *
 *  @author: CCHyper
 */
ULONG AdvancedAITacticTypeClass::Release()
{
    //EXT_DEBUG_TRACE("AdvancedAITacticTypeClass::Release - 0x%08X\n", (uintptr_t)(this));

    return 1;
}


/**
 *  Retrieves the class identifier (CLSID) of the object.
 *
 *  @author: CCHyper
 */
HRESULT AdvancedAITacticTypeClass::GetClassID(CLSID* lpClassID)
{
    //EXT_DEBUG_TRACE("AdvancedAITacticTypeClass::GetClassID - Name: %s (0x%08X)\n", Name(), (uintptr_t)(this));

    if (lpClassID == nullptr) {
        return E_POINTER;
    }

    *lpClassID = __uuidof(this);

    return S_OK;
}


/**
 *  Determines whether an object has changed since it was last saved to its stream.
 *
 *  @author: CCHyper
 */
HRESULT AdvancedAITacticTypeClass::IsDirty()
{
    //EXT_DEBUG_TRACE("AdvancedAITacticTypeClass::IsDirty - 0x%08X\n", (uintptr_t)(this));

    return S_OK;
}


/**
 *  Loads the object from the stream.
 *
 *  @author: Rampastring, CCHyper, tomsons26
 */
HRESULT AdvancedAITacticTypeClass::Load(IStream* pStm)
{
    //EXT_DEBUG_TRACE("AdvancedAITacticTypeClass::Load - 0x%08X\n", (uintptr_t)(this));

    if (!pStm) {
        return E_POINTER;
    }

    Conditions.Clear();
    AdvAITeamTypes.Clear();

    /**
     *  Load the unique id for this class.
     */
    LONG id = 0;
    HRESULT hr = pStm->Read(&id, sizeof(LONG), nullptr);
    if (FAILED(hr)) {
        return hr;
    }

    /**
     *  Register this instance to be available for remapping references to.
     */
    VINIFERA_SWIZZLE_REGISTER_POINTER(id, this, IniName);

    /**
     *  Read this class's binary blob data directly into this instance.
     */
    hr = pStm->Read(this, sizeof(*this), nullptr);
    if (FAILED(hr)) {
        return hr;
    }

    new (this) AdvancedAITacticTypeClass(NoInitClass());

    hr = Load_Primitive_Vector(pStm, Conditions, "AdvancedAITacticTypeClass.Conditions");
    if (FAILED(hr)) {
        return hr;
    }
    
    hr = Load_Primitive_Vector(pStm, AdvAITeamTypes, "AdvancedAITacticTypeClass.AdvAITeamTypes");
    if (FAILED(hr)) {
        return hr;
    }
    
    return hr;
}


/**
 *  Saves the object to the stream.
 *
 *  @author: CCHyper, tomsons26
 */
HRESULT AdvancedAITacticTypeClass::Save(IStream* pStm, BOOL fClearDirty)
{
    //EXT_DEBUG_TRACE("AdvancedAITacticTypeClass::Internal_Save - 0x%08X\n", (uintptr_t)(this));

    if (!pStm) {
        return E_POINTER;
    }

    /**
     *  Fetch the save id for this instance.
     */
    const LONG id = reinterpret_cast<LONG>(this);

    //DEV_DEBUG_INFO("Writing id = 0x%08X.\n", id);

    HRESULT hr = pStm->Write(&id, sizeof(id), nullptr);
    if (FAILED(hr)) {
        return hr;
    }

    /**
     *  Write this class instance as a binary blob.
     */
    hr = pStm->Write(this, sizeof(*this), nullptr);
    if (FAILED(hr)) {
        return hr;
    }
    
    hr = Save_Primitive_Vector(pStm, Conditions, "AdvancedAITacticTypeClass.Conditions");
    if (FAILED(hr)) {
        return hr;
    }
    
    hr = Save_Primitive_Vector(pStm, AdvAITeamTypes, "AdvancedAITacticTypeClass.AdvAITeamTypes");
    if (FAILED(hr)) {
        return hr;
    }

    return hr;
}


/**
 *  Retrieves the size of the stream needed to save the object.
 *
 *  @author: CCHyper, tomsons26
 */
LONG AdvancedAITacticTypeClass::GetSizeMax(ULARGE_INTEGER* pcbSize)
{
    //EXT_DEBUG_TRACE("AdvancedAITacticTypeClass::GetSizeMax - 0x%08X\n", (uintptr_t)(this));

    if (!pcbSize) {
        return E_POINTER;
    }
    
    pcbSize->LowPart = sizeof(*this) + sizeof(uint32_t); // Add size of swizzle "id".
    pcbSize->HighPart = 0;
    
    return S_OK;
}


int AdvancedAITacticTypeClass::From_Name(const char* name)
{
    ASSERT(name != nullptr);

    if (name != nullptr) {
        for (int i = 0; i < AdvancedAITacticTypes.Count(); i++) {
            if (std::strncmp(AdvancedAITacticTypes[i]->IniName, name, sizeof(IniName)) == 0) {
                return i;
            }
        }
    }

    return -1;
}


const AdvancedAITacticTypeClass* AdvancedAITacticTypeClass::Find_Or_Make(const char* name, bool air, bool naval)
{
    ASSERT(name != nullptr);
    ASSERT(!(air && naval));

    for (int i = 0; i < AdvancedAITacticTypes.Count(); i++) {
        if (std::strncmp(AdvancedAITacticTypes[i]->IniName, name, sizeof(IniName)) == 0) {

            ASSERT(AdvancedAITacticTypes[i]->IsAir == air);
            ASSERT(AdvancedAITacticTypes[i]->IsNaval == naval);

            return AdvancedAITacticTypes[i];
        }
    }

    AdvancedAITacticTypeClass* ptr = new AdvancedAITacticTypeClass(name);
    ASSERT(ptr != nullptr);

    if (ptr)
    {
        ptr->IsAir = air;
        ptr->IsNaval = naval;
    }
    
    return ptr;
}

TaskForceClass* Find_Or_Make_Dummy_TaskForce()
{
    const char* taskforcename = "TASKFORCE_DUMMY";

    TaskForceType taskforceid = TaskForceClass::From_Name(taskforcename);
    if (taskforceid != SCRIPT_NONE) {
        return TaskForces[taskforceid];
    }

    TaskForceClass* newtaskforce = new TaskForceClass();

    if (newtaskforce == nullptr)
        return nullptr;

    newtaskforce->IniName.assign(taskforcename);

    newtaskforce->ClassCount = 0;
    return newtaskforce;
}

ScriptTypeClass* Find_Or_Make_Hunt_Script()
{
    const char* scriptname = "SCRIPT_HUNT";

    ScriptType scriptid = ScriptTypeClass::From_Name(scriptname);
    if (scriptid != SCRIPT_NONE) {
        return ScriptTypes[scriptid];
    }

    ScriptTypeClass* newscripttype = new ScriptTypeClass();

    if (newscripttype == nullptr)
        return nullptr;

    newscripttype->IniName.assign(scriptname);

    newscripttype->MissionCount = 1;
    newscripttype->MissionList[0].Mission = SMISSION_DO;
    newscripttype->MissionList[0].Data.Mission = MISSION_HUNT;
    return newscripttype;
}

ScriptTypeClass* Find_Or_Make_Attack_Harvesters_Script()
{
    const char* scriptname = "SCRIPT_ATTACK_HARVESTERS";

    ScriptType scriptid = ScriptTypeClass::From_Name(scriptname);
    if (scriptid != SCRIPT_NONE) {
        return ScriptTypes[scriptid];
    }

    ScriptTypeClass* newscripttype = new ScriptTypeClass();

    if (newscripttype == nullptr)
        return nullptr;

    newscripttype->IniName.assign(scriptname);

    newscripttype->MissionCount = 4;
    newscripttype->MissionList[0].Mission = SMISSION_ATTACK;
    newscripttype->MissionList[0].Data.Quarry = (QuarryType)VINIFERA_QUARRY_HARVESTERS;
    newscripttype->MissionList[1].Mission = SMISSION_ATTACK;
    newscripttype->MissionList[1].Data.Quarry = QUARRY_VEHICLES;
    newscripttype->MissionList[2].Mission = SMISSION_ATTACK;
    newscripttype->MissionList[2].Data.Quarry = QUARRY_ANYTHING;

    return newscripttype;
}

ScriptTypeClass* Find_Or_Make_Attack_ConYard_Script()
{
    const char* scriptname = "SCRIPT_ATTACK_CONYARDS";

    ScriptType scriptid = ScriptTypeClass::From_Name(scriptname);
    if (scriptid != SCRIPT_NONE) {
        return ScriptTypes[scriptid];
    }

    ScriptTypeClass* newscripttype = new ScriptTypeClass();

    if (newscripttype == nullptr)
        return nullptr;

    newscripttype->IniName.assign(scriptname);

    // Gather a list of all the construction yards that we should try to attack.
    StructType conyards[49];
    int conyardcount = 0;

    for (StructType i = STRUCT_FIRST; i < BuildingTypes.Count() && conyardcount < std::size(conyards); i++) {
        if (BuildingTypes[i]->IsConstructionYard) {
            conyards[conyardcount] = i;
            conyardcount++;
        }
    }

    newscripttype->MissionCount = conyardcount + 1;
    for (int i = 0; i < conyardcount; i++) {
        newscripttype->MissionList[i].Mission = SMISSION_ATTACK_ENEMY_BUILDING;
        newscripttype->MissionList[i].Data.Value = conyards[i];
    }
    newscripttype->MissionList[conyardcount].Mission = SMISSION_DO;
    newscripttype->MissionList[conyardcount].Data.Mission = MISSION_HUNT;

    return newscripttype;
}

ScriptTypeClass* Find_Or_Make_Attack_Refineries_Script()
{
    const char* scriptname = "SCRIPT_ATTACK_REFINERIES";

    ScriptType scriptid = ScriptTypeClass::From_Name(scriptname);
    if (scriptid != SCRIPT_NONE) {
        return ScriptTypes[scriptid];
    }

    ScriptTypeClass* newscripttype = new ScriptTypeClass();

    if (newscripttype == nullptr)
        return nullptr;

    newscripttype->IniName.assign(scriptname);

    // Gather a list of all the refineries that we should try to attack.
    StructType refineries[49];
    int refinerycount = 0;

    for (StructType i = STRUCT_FIRST; i < BuildingTypes.Count() && refinerycount < std::size(refineries); i++) {
        if (BuildingTypes[i]->IsRefinery) {
            refineries[refinerycount] = i;
            refinerycount++;
        }
    }

    newscripttype->MissionCount = refinerycount + 1;
    for (int i = 0; i < refinerycount; i++) {
        newscripttype->MissionList[i].Mission = SMISSION_ATTACK_ENEMY_BUILDING;
        newscripttype->MissionList[i].Data.Value = refineries[i];
    }
    newscripttype->MissionList[refinerycount].Mission = SMISSION_ATTACK;
    newscripttype->MissionList[refinerycount].Data.Quarry = QUARRY_ANYTHING;

    return newscripttype;
}

ScriptTypeClass* Find_Or_Make_Attack_Factories_Script()
{
    const char* scriptname = "SCRIPT_ATTACK_FACTORIES";

    ScriptType scriptid = ScriptTypeClass::From_Name(scriptname);
    if (scriptid != SCRIPT_NONE) {
        return ScriptTypes[scriptid];
    }

    ScriptTypeClass* newscripttype = new ScriptTypeClass();

    if (newscripttype == nullptr)
        return nullptr;

    newscripttype->IniName.assign(scriptname);

    // Gather a list of all the refineries that we should try to attack.
    StructType factories[49];
    int factorycount = 0;

    for (int i = 0; i < Rule->BuildWeapons.Count() && factorycount < std::size(factories); i++) {
        factories[factorycount] = Rule->BuildWeapons[i]->HeapID;
        factorycount++;
    }

    for (int i = 0; i < Rule->BuildConst.Count() && factorycount < std::size(factories); i++) {
        factories[factorycount] = Rule->BuildConst[i]->HeapID;
        factorycount++;
    }

    for (int i = 0; i < Rule->BuildBarracks.Count() && factorycount < std::size(factories); i++) {
        factories[factorycount] = Rule->BuildBarracks[i]->HeapID;
        factorycount++;
    }

    for (int i = 0; i < Rule->BuildHelipad.Count() && factorycount < std::size(factories); i++) {
        factories[factorycount] = Rule->BuildHelipad[i]->HeapID;
        factorycount++;
    }

    newscripttype->MissionCount = factorycount + 1;
    for (int i = 0; i < factorycount; i++) {
        newscripttype->MissionList[i].Mission = SMISSION_ATTACK_ENEMY_BUILDING;
        newscripttype->MissionList[i].Data.Value = factories[i];
    }
    newscripttype->MissionList[factorycount].Mission = SMISSION_ATTACK;
    newscripttype->MissionList[factorycount].Data.Quarry = QUARRY_ANYTHING;

    return newscripttype;
}

ScriptTypeClass* Find_Or_Make_APC_Vs_Refineries_Script()
{
    const char* scriptname = "SCRIPT_APC_VS_REFINERIES";

    ScriptType scriptid = ScriptTypeClass::From_Name(scriptname);
    if (scriptid != SCRIPT_NONE) {
        return ScriptTypes[scriptid];
    }

    ScriptTypeClass* newscripttype = new ScriptTypeClass();

    if (newscripttype == nullptr)
        return nullptr;

    newscripttype->IniName.assign(scriptname);

    // Gather a list of all the refineries that we should try to attack.
    StructType refineries[40];
    int refinerycount = 0;

    for (StructType i = STRUCT_FIRST; i < BuildingTypes.Count() && refinerycount < std::size(refineries); i++) {
        if (BuildingTypes[i]->IsRefinery) {
            refineries[refinerycount] = i;
            refinerycount++;
        }
    }

    newscripttype->MissionCount = refinerycount + 4;
    newscripttype->MissionList[0].Mission = SMISSION_LOAD;
    newscripttype->MissionList[1].Mission = SMISSION_WAIT_TILL_FULLY_LOADED;

    for (int i = 0; i < refinerycount; i++) {
        newscripttype->MissionList[i + 2].Mission = SMISSION_MOVETO_ENEMY_BUILDING;
        newscripttype->MissionList[i + 2].Data.Value = refineries[i];
    }

    newscripttype->MissionList[refinerycount + 2].Mission = SMISSION_UNLOAD;
    newscripttype->MissionList[refinerycount + 2].Data.Value = 0; // Keep Transports, Keep Units
    newscripttype->MissionList[refinerycount + 3].Mission = SMISSION_DO;
    newscripttype->MissionList[refinerycount + 3].Data.Mission = MISSION_HUNT;

    return newscripttype;
}

ScriptTypeClass* Find_Or_Make_APC_Vs_ConYards_Script()
{
    const char* scriptname = "SCRIPT_APC_VS_CONYARDS";

    ScriptType scriptid = ScriptTypeClass::From_Name(scriptname);
    if (scriptid != SCRIPT_NONE) {
        return ScriptTypes[scriptid];
    }

    ScriptTypeClass* newscripttype = new ScriptTypeClass();

    if (newscripttype == nullptr)
        return nullptr;

    newscripttype->IniName.assign(scriptname);

    // Gather a list of all the construction yards that we should try to attack.
    StructType conyards[40];
    int conyardcount = 0;

    for (StructType i = STRUCT_FIRST; i < BuildingTypes.Count() && conyardcount < std::size(conyards); i++) {
        if (BuildingTypes[i]->IsConstructionYard) {
            conyards[conyardcount] = i;
            conyardcount++;
        }
    }

    newscripttype->MissionCount = conyardcount + 4;
    newscripttype->MissionList[0].Mission = SMISSION_LOAD;
    newscripttype->MissionList[1].Mission = SMISSION_WAIT_TILL_FULLY_LOADED;

    for (int i = 0; i < conyardcount; i++) {
        newscripttype->MissionList[i + 2].Mission = SMISSION_MOVETO_ENEMY_BUILDING;
        newscripttype->MissionList[i + 2].Data.Value = conyards[i];
    }

    newscripttype->MissionList[conyardcount + 2].Mission = SMISSION_UNLOAD;
    newscripttype->MissionList[conyardcount + 2].Data.Value = 0; // Keep Transports, Keep Units
    newscripttype->MissionList[conyardcount + 3].Mission = SMISSION_ATTACK;
    newscripttype->MissionList[conyardcount + 3].Data.Quarry = QUARRY_ANYTHING;

    return newscripttype;
}

ScriptTypeClass* Find_Or_Make_Attack_Vehicles_Script()
{
    const char* scriptname = "SCRIPT_ATTACK_VEHICLES";

    ScriptType scriptid = ScriptTypeClass::From_Name(scriptname);
    if (scriptid != SCRIPT_NONE) {
        return ScriptTypes[scriptid];
    }

    ScriptTypeClass* newscripttype = new ScriptTypeClass();

    if (newscripttype == nullptr)
        return nullptr;

    newscripttype->IniName.assign(scriptname);

    newscripttype->MissionCount = 2;
    newscripttype->MissionList[0].Mission = SMISSION_ATTACK;
    newscripttype->MissionList[0].Data.Quarry = QUARRY_VEHICLES;
    newscripttype->MissionList[1].Mission = SMISSION_ATTACK;
    newscripttype->MissionList[1].Data.Quarry = QUARRY_ANYTHING;

    return newscripttype;
}

ScriptTypeClass* Find_Or_Make_Attack_Infantry_Script()
{
    const char* scriptname = "SCRIPT_ATTACK_INFANTRY";

    ScriptType scriptid = ScriptTypeClass::From_Name(scriptname);
    if (scriptid != SCRIPT_NONE) {
        return ScriptTypes[scriptid];
    }

    ScriptTypeClass* newscripttype = new ScriptTypeClass();

    if (newscripttype == nullptr)
        return nullptr;

    newscripttype->IniName.assign(scriptname);

    newscripttype->MissionCount = 2;
    newscripttype->MissionList[0].Mission = SMISSION_ATTACK;
    newscripttype->MissionList[0].Data.Quarry = QUARRY_INFANTRY;
    newscripttype->MissionList[1].Mission = SMISSION_ATTACK;
    newscripttype->MissionList[1].Data.Quarry = QUARRY_ANYTHING;

    return newscripttype;
}

ScriptTypeClass* Find_Or_Make_Empty_Script()
{
    const char* scriptname = "SCRIPT_EMPTY";

    ScriptType scriptid = ScriptTypeClass::From_Name(scriptname);
    if (scriptid != SCRIPT_NONE) {
        return ScriptTypes[scriptid];
    }

    ScriptTypeClass* newscripttype = new ScriptTypeClass();

    if (newscripttype == nullptr)
        return nullptr;

    newscripttype->IniName.assign(scriptname);

    newscripttype->MissionCount = 1;
    newscripttype->MissionList[0].Mission = SMISSION_SCATTER;
    newscripttype->MissionList[0].Data.Value = 0;

    return newscripttype;
}

TeamTypeClass* Find_Or_Make_Defense_TeamType(HouseClass* house)
{
    char buffer[24];
    sprintf(buffer, "TT_H%d_DEFEND", house->HeapID);
    TeamType teamtypeid = TeamTypeClass::From_Name(buffer);

    if (teamtypeid != TEAM_NONE) {
        return TeamTypes[teamtypeid];
    }

    TeamTypeClass* teamtype = TeamTypeClass::Find_Or_Make(buffer);

    teamtype->MaxAllowed = -1; // allow an infinite number of these
    teamtype->House = house;

    TeamTypeClassExtension* teamtypeext = Extension::Fetch(teamtype);
    teamtypeext->SmartHunt = true;

    teamtype->Script = Find_Or_Make_Empty_Script();

    teamtype->TaskForce = Find_Or_Make_Dummy_TaskForce();
    return teamtype;
}

TeamTypeClass* Find_Or_Make_Scouting_TeamType(HouseClass* house)
{
    bool usehunt = Percent_Chance(50);

    char buffer[24];
    sprintf(buffer, "TT_H%d_SCOUT_%d", house->HeapID, usehunt);
    TeamType teamtypeid = TeamTypeClass::From_Name(buffer);

    if (teamtypeid != TEAM_NONE) {
        return TeamTypes[teamtypeid];
    }

    TeamTypeClass* teamtype = TeamTypeClass::Find_Or_Make(buffer);

    teamtype->MaxAllowed = -1; // allow an infinite number of these
    teamtype->House = house;

    TeamTypeClassExtension* teamtypeext = Extension::Fetch(teamtype);
    teamtypeext->SmartHunt = true;

    // For scouting, either use hunt teams, or anti-ConYard teams with AvoidThreats=true
    if (usehunt) {
        teamtype->Script = Find_Or_Make_Hunt_Script();
    }
    else {
        teamtype->Script = Find_Or_Make_Attack_ConYard_Script();
        teamtype->AvoidThreats = true;
    }

    teamtype->TaskForce = Find_Or_Make_Dummy_TaskForce();
    return teamtype;
}

TeamTypeClass* Find_Or_Make_Rush_TeamType(HouseClass* house)
{
    bool rushconyard = Percent_Chance(50);

    int num = rushconyard ? 1 : 0;

    char buffer[24];
    sprintf(buffer, "TT_H%d_RUSH_%d", house->HeapID, num);
    TeamType teamtypeid = TeamTypeClass::From_Name(buffer);

    if (teamtypeid != TEAM_NONE) {
        return TeamTypes[teamtypeid];
    }

    TeamTypeClass* teamtype = TeamTypeClass::Find_Or_Make(buffer);

    teamtype->MaxAllowed = -1; // allow an infinite number of these
    teamtype->House = house;

    TeamTypeClassExtension* teamtypeext = Extension::Fetch(teamtype);
    teamtypeext->SmartHunt = true;

    if (rushconyard)
    {
        teamtype->Script = Find_Or_Make_Attack_ConYard_Script();
        teamtype->AvoidThreats = true;
    }
    else // if (rushrefineries) 
    {
        teamtype->Script = Find_Or_Make_Attack_Refineries_Script();
        teamtype->AvoidThreats = true;
    }

    teamtype->TaskForce = Find_Or_Make_Dummy_TaskForce();
    return teamtype;
}

TeamTypeClass* Find_Or_Make_Direct_Attack_TeamType(HouseClass* house)
{
    char buffer[24];
    sprintf(buffer, "TT_H%d_DA", house->HeapID);
    TeamType teamtypeid = TeamTypeClass::From_Name(buffer);

    if (teamtypeid != TEAM_NONE) {
        return TeamTypes[teamtypeid];
    }

    TeamTypeClass* teamtype = TeamTypeClass::Find_Or_Make(buffer);

    teamtype->MaxAllowed = -1; // allow an infinite number of these
    teamtype->House = house;

    TeamTypeClassExtension* teamtypeext = Extension::Fetch(teamtype);
    teamtypeext->SmartHunt = true;

    teamtype->Script = Find_Or_Make_Hunt_Script();

    teamtype->TaskForce = Find_Or_Make_Dummy_TaskForce();
    return teamtype;
}

TeamTypeClass* Find_Or_Make_Suicidal_Anti_Harvester_TeamType(HouseClass* house)
{
    char buffer[24];
    sprintf(buffer, "TT_H%d_AH", house->HeapID);
    TeamType teamtypeid = TeamTypeClass::From_Name(buffer);

    if (teamtypeid != TEAM_NONE) {
        return TeamTypes[teamtypeid];
    }

    TeamTypeClass* teamtype = TeamTypeClass::Find_Or_Make(buffer);

    teamtype->MaxAllowed = -1; // allow an infinite number of these
    teamtype->House = house;
    teamtype->IsSuicide = true;
    teamtype->AvoidThreats = true;

    TeamTypeClassExtension* teamtypeext = Extension::Fetch(teamtype);
    teamtypeext->SmartHunt = true;

    teamtype->Script = Find_Or_Make_Attack_Harvesters_Script();

    teamtype->TaskForce = Find_Or_Make_Dummy_TaskForce();
    return teamtype;
}

TeamTypeClass* Find_Or_Make_APC_Vs_Refineries_TeamType(HouseClass* house)
{
    char buffer[24];
    sprintf(buffer, "TT_H%d_APC_VS_REFS", house->HeapID);
    TeamType teamtypeid = TeamTypeClass::From_Name(buffer);

    if (teamtypeid != TEAM_NONE) {
        return TeamTypes[teamtypeid];
    }

    TeamTypeClass* teamtype = TeamTypeClass::Find_Or_Make(buffer);

    teamtype->MaxAllowed = -1; // allow an infinite number of these
    teamtype->OnlyTargetHouseEnemy = true;
    teamtype->House = house;
    teamtype->IsSuicide = true;
    teamtype->AvoidThreats = true;

    TeamTypeClassExtension* teamtypeext = Extension::Fetch(teamtype);
    teamtypeext->SmartHunt = true;

    teamtype->Script = Find_Or_Make_APC_Vs_Refineries_Script();

    teamtype->TaskForce = Find_Or_Make_Dummy_TaskForce();
    return teamtype;
}

TeamTypeClass* Find_Or_Make_APC_Vs_ConYards_TeamType(HouseClass* house)
{
    char buffer[24];
    sprintf(buffer, "TT_H%d_APC_VS_CYS", house->HeapID);
    TeamType teamtypeid = TeamTypeClass::From_Name(buffer);

    if (teamtypeid != TEAM_NONE) {
        return TeamTypes[teamtypeid];
    }

    TeamTypeClass* teamtype = TeamTypeClass::Find_Or_Make(buffer);

    teamtype->MaxAllowed = -1; // allow an infinite number of these
    teamtype->OnlyTargetHouseEnemy = true;
    teamtype->House = house;
    teamtype->IsSuicide = true;
    teamtype->AvoidThreats = true;

    TeamTypeClassExtension* teamtypeext = Extension::Fetch(teamtype);
    teamtypeext->SmartHunt = true;

    teamtype->Script = Find_Or_Make_APC_Vs_ConYards_Script();

    teamtype->TaskForce = Find_Or_Make_Dummy_TaskForce();
    return teamtype;
}

TeamTypeClass* Find_Or_Make_Anti_Refineries_TeamType(HouseClass* house, bool suicidal)
{
    char buffer[24];
    sprintf(buffer, "TT_H%d_ANTI_REF_%d", house->HeapID, suicidal);
    TeamType teamtypeid = TeamTypeClass::From_Name(buffer);

    if (teamtypeid != TEAM_NONE) {
        return TeamTypes[teamtypeid];
    }

    TeamTypeClass* teamtype = TeamTypeClass::Find_Or_Make(buffer);

    teamtype->MaxAllowed = -1; // allow an infinite number of these
    teamtype->OnlyTargetHouseEnemy = true;
    teamtype->House = house;
    teamtype->IsSuicide = suicidal;
    teamtype->AvoidThreats = true;

    TeamTypeClassExtension* teamtypeext = Extension::Fetch(teamtype);
    teamtypeext->SmartHunt = true;

    teamtype->Script = Find_Or_Make_Attack_Refineries_Script();

    teamtype->TaskForce = Find_Or_Make_Dummy_TaskForce();
    return teamtype;
}

TeamClass* AdvAI_Create_Team(HouseClass* house, TeamTypeClass* teamtype, RTTIType fortype, int maxcost = -1, bool transport = false, AdvAITacticType forcedtactic = TACTIC_NONE)
{
    if (teamtype == nullptr) {
        DEBUG_INFO("AdvAI_Create_Team called with null teamtype\n");
        return nullptr;
    }

    TeamClass* team = teamtype->Create_One_Of(house);

    if (team == nullptr) {
        DEBUG_INFO("AdvAI: Failed to create an instance of team type %s!\n", teamtype->IniName);
        return nullptr;
    }

    TeamClassExtension* teamext = Extension::Fetch(team);
    teamext->IsAdvAITeam = true;
    teamext->AdvAITactic = forcedtactic != TACTIC_NONE ? forcedtactic : Extension::Fetch(house)->AdvAIGroundTactic.Tactic;
    Extension::Fetch(house)->AdvAI_Set_Ground_Team_Desired_Ratios(team);

    if (maxcost > 0)
        teamext->MaxCost = maxcost;

    if (transport) {
        teamext->IsTransportTeam = true;
        teamext->MaxInfantry = 5;
    }

    teamext->NoAircraft = true;

    if (fortype == RTTI_INFANTRY)
    {
        teamext->NoVehicles = true;
        teamext->NoAircraft = true;
    }
    else if (fortype == RTTI_UNIT)
    {
        teamext->NoInfantry = true;
        teamext->NoAircraft = true;
    }
    else if (fortype == RTTI_AIRCRAFT)
    {
        teamext->NoAircraft = false;
        teamext->NoInfantry = true;
        teamext->NoVehicles = true;
    }

    return team;
}

TeamTypeClass* Find_Or_Make_Aircraft_TeamType(char* namebuf, HouseClass* house, ScriptTypeClass* script, bool suicidal)
{
    TeamType teamtypeid = TeamTypeClass::From_Name(namebuf);

    if (teamtypeid != TEAM_NONE) {
        return TeamTypes[teamtypeid];
    }

    TeamTypeClass* teamtype = TeamTypeClass::Find_Or_Make(namebuf);

    teamtype->MaxAllowed = -1; // allow an infinite number of these
    teamtype->OnlyTargetHouseEnemy = true;
    teamtype->House = house;
    teamtype->IsSuicide = suicidal;
    teamtype->AvoidThreats = true;

    TeamTypeClassExtension* teamtypeext = Extension::Fetch(teamtype);

    teamtype->Script = script;

    teamtype->TaskForce = Find_Or_Make_Dummy_TaskForce();
    return teamtype;
}

TeamTypeClass* Find_Or_Make_Aircraft_Vs_Vehicles_TeamType(HouseClass* house, bool suicidal)
{
    char buffer[24];
    sprintf(buffer, "TT_H%d_AIR_VS_VEH_%d", house->HeapID, suicidal);
    return Find_Or_Make_Aircraft_TeamType(buffer, house, Find_Or_Make_Attack_Vehicles_Script(), suicidal);
}

TeamTypeClass* Find_Or_Make_Aircraft_Vs_Infantry_TeamType(HouseClass* house, bool suicidal)
{
    char buffer[24];
    sprintf(buffer, "TT_H%d_AIR_VS_INF_%d", house->HeapID, suicidal);
    return Find_Or_Make_Aircraft_TeamType(buffer, house, Find_Or_Make_Attack_Infantry_Script(), suicidal);
}

TeamTypeClass* Find_Or_Make_Aircraft_Vs_Harvesters_TeamType(HouseClass* house, bool suicidal)
{
    char buffer[24];
    sprintf(buffer, "TT_H%d_AIR_VS_HARV_%d", house->HeapID, suicidal);
    return Find_Or_Make_Aircraft_TeamType(buffer, house, Find_Or_Make_Attack_Harvesters_Script(), suicidal);
}

TeamTypeClass* Find_Or_Make_Aircraft_Vs_Factories_TeamType(HouseClass* house, bool suicidal)
{
    char buffer[24];
    sprintf(buffer, "TT_H%d_AIR_VS_FAC_%d", house->HeapID, suicidal);
    return Find_Or_Make_Aircraft_TeamType(buffer, house, Find_Or_Make_Attack_Factories_Script(), suicidal);
}

TeamTypeClass* Find_Or_Make_Aircraft_Vs_Refineries_TeamType(HouseClass* house, bool suicidal)
{
    char buffer[24];
    sprintf(buffer, "TT_H%d_AIR_VS_REF_%d", house->HeapID, suicidal);
    return Find_Or_Make_Aircraft_TeamType(buffer, house, Find_Or_Make_Attack_Refineries_Script(), suicidal);
}

TeamClass* AdvAI_Create_Aircraft_Team(HouseClass* house, TeamTypeClass* teamtype, int maxcost = -1)
{
    if (teamtype == nullptr) {
        DEBUG_INFO("AdvAI_Create_Aircraft_Team called with null teamtype\n");
        return nullptr;
    }

    TeamClass* team = teamtype->Create_One_Of(house);

    if (team == nullptr) {
        DEBUG_INFO("AdvAI: Failed to create an instance of team type %s!\n", teamtype->IniName);
        return nullptr;
    }

    TeamClassExtension* teamext = Extension::Fetch(team);
    teamext->IsAdvAITeam = true;
    teamext->IsAircraftTeam = true;
    teamext->NoInfantry = true;
    teamext->NoVehicles = true;
    teamext->AdvAITactic = Extension::Fetch(house)->AdvAIGroundTactic.Tactic;
    Extension::Fetch(house)->AdvAI_Set_Aircraft_Team_Desired_Ratios(team);

    if (maxcost > 0)
        teamext->MaxCost = maxcost;

    return team;
}

TeamClass* AdvAI_Create_Naval_Team(HouseClass* house, TeamTypeClass* teamtype, int maxcost = -1)
{
    if (teamtype == nullptr) {
        DEBUG_INFO("AdvAI_Create_Naval_Team called with null teamtype\n");
        return nullptr;
    }

    TeamClass* team = teamtype->Create_One_Of(house);

    if (team == nullptr) {
        DEBUG_INFO("AdvAI: Failed to create an instance of team type %s!\n", teamtype->IniName);
        return nullptr;
    }

    TeamClassExtension* teamext = Extension::Fetch(team);
    teamext->ProdFlags = PRODFLAG_NAVAL;
    teamext->IsAdvAITeam = true;
    teamext->NoAircraft = true;
    teamext->NoInfantry = true;
    teamext->AdvAITactic = Extension::Fetch(house)->AdvAIGroundTactic.Tactic;

    Extension::Fetch(house)->AdvAI_Set_Naval_Team_Desired_Ratios(team);

    if (maxcost > 0)
        teamext->MaxCost = maxcost;

    return team;
}

bool AdvancedAITacticTypeClass::Process(HouseClass* house)
{
    if (house->Class->HeapID != House) {
        return false;
    }

    for (int i = 0; i < Conditions.Count(); i++)
    {
        AdvancedAITacticCondition& condition = Conditions[i];
        
        bool pass = false;

        switch (condition.ConditionType)
        {
        case AdvAITacticConditionType::TACTICCOND_FRAME_LESS_THAN:
            pass = Frame < condition.Parameter1;
            break;
        case AdvAITacticConditionType::TACTICCOND_FRAME_MORE_THAN:
            pass = Frame >= condition.Parameter1;
            break;
        case AdvAITacticConditionType::TACTICCOND_ENEMY_IS_HOUSETYPE:
            pass = house->Enemy != HOUSE_NONE && Houses[house->Enemy]->Class->HeapID == (HousesType)condition.Parameter1;
            break;
        case AdvAITacticConditionType::TACTICCOND_ENEMY_IS_NOT_HOUSETYPE:
            pass = house->Enemy != HOUSE_NONE && Houses[house->Enemy]->Class->HeapID != (HousesType)condition.Parameter1;
            break;
        case AdvAITacticConditionType::TACTICCOND_PERCENT_CHANCE:
            pass = Percent_Chance(condition.Parameter1);
            break;
        case AdvAITacticConditionType::TACTICCOND_HAS_WAR_FACTORY:
            pass = Extension::Fetch(house)->Has_War_Factory();
            break;
        case AdvAITacticConditionType::TACTICCOND_HAS_NO_WAR_FACTORY:
            pass = !Extension::Fetch(house)->Has_War_Factory();
            break;
        case AdvAITacticConditionType::TACTICCOND_IS_OUTNUMBERED:
            pass = Extension::Fetch(house)->AdvAI_Is_Outnumbered();
            break;
        case AdvAITacticConditionType::TACTICCOND_IS_NOT_OUTNUMBERED:
            pass = !Extension::Fetch(house)->AdvAI_Is_Outnumbered();
            break;
        case AdvAITacticConditionType::TACTICCOND_IS_DISADVANTAGED:
            pass = Extension::Fetch(house)->AdvAI_Is_Disadvantaged();
            break;
        case AdvAITacticConditionType::TACTICCOND_IS_NOT_DISADVANTAGED:
            pass = !Extension::Fetch(house)->AdvAI_Is_Disadvantaged();
            break;
        case AdvAITacticConditionType::TACTICCOND_ENEMY_LIGHTLY_DEFENDED_REFINERIES:
            pass = Extension::Fetch(house)->AdvAI_Enemy_Has_Lightly_Defended_Buildings(Rule->BuildRefinery);
            break;
        case AdvAITacticConditionType::TACTICCOND_ENEMY_VULNERABLE_REFINERIES:
            pass = Extension::Fetch(house)->AdvAI_Enemy_Has_Vulnerable_Buildings(Rule->BuildRefinery);
            break;
        case AdvAITacticConditionType::TACTICCOND_ENEMY_LIGHTLY_DEFENDED_CONYARDS:
            pass = Extension::Fetch(house)->AdvAI_Enemy_Has_Lightly_Defended_Buildings(Rule->BuildConst);
            break;
        case AdvAITacticConditionType::TACTICCOND_ENEMY_VULNERABLE_CONYARDS:
            pass = Extension::Fetch(house)->AdvAI_Enemy_Has_Vulnerable_Buildings(Rule->BuildConst);
            break;
        case AdvAITacticConditionType::TACTICCOND_ENEMY_HAS_SENSORS:
            pass = Extension::Fetch(house)->EnemyHasSensors;
            break;
        case AdvAITacticConditionType::TACTICCOND_ENEMY_HAS_NO_SENSORS:
            pass = !Extension::Fetch(house)->EnemyHasSensors;
            break;
        case AdvAITacticConditionType::TACTICCOND_ENEMY_HARVESTER_COUNT_EXCEEDS:
            pass = Extension::Fetch(house)->EnemyHarvesterCount > condition.Parameter1;
            break;
        case AdvAITacticConditionType::TACTICCOND_NOT_REPEATED_FOR_FRAMES:
            if (IsAir || IsNaval)
                pass = true;
            else
                pass = Frame > Extension::Fetch(house)->AdvAILastExecutionFrameForTactic[GroundTacticType] + condition.Parameter1;
            break;
        case AdvAITacticConditionType::TACTICCOND_HOUSE_TACTICAL_VALUE_ABOVE:
            pass = Extension::Fetch(house)->FactionSpecificTacticalValues[condition.Parameter1] > condition.Parameter2;
            break;
        case AdvAITacticConditionType::TACTICCOND_HOUSE_TACTICAL_VALUE_BELOW:
            pass = Extension::Fetch(house)->FactionSpecificTacticalValues[condition.Parameter1] < condition.Parameter2;
            break;
        case AdvAITacticConditionType::TACTICCOND_ENEMY_ARTILLERY_STRENGTH_ABOVE:
            pass = Extension::Fetch(house)->EnemyArtilleryStrength > condition.Parameter1;
            break;
        case AdvAITacticConditionType::TACTICCOND_ENEMY_ANTI_AIR_STRENGTH_BELOW:
            pass = Extension::Fetch(house)->EnemyAntiAirStrength.Total() < condition.Parameter1;
            break;
        default:
            DEBUG_ERROR("Unknown AdvAI tactic condition: %d\n", condition.ConditionType);
            Emergency_Exit(0);
            return false;
        }

        if (!pass) {
            return false;
        }
    }

    // If we get here, either there are no conditions, or they have all passed. Proceed with assigning the tactic and creating teams.
    int duration = MinimumExpectedDuration;
    if (MaximumExpectedDuration > duration) {
        duration = Random_Pick(duration, MaximumExpectedDuration);
    }

    if (FrameBasedDurationIncrease > 0 && FrameBasedDurationTimeStep > 0)
    {
        int framebasedduration = FrameBasedDurationIncrease * (Frame / FrameBasedDurationTimeStep);

        if (FrameBasedDurationIncreaseMax > 0) {
            framebasedduration = std::min(FrameBasedDurationIncreaseMax, framebasedduration);
        }
        
        duration += framebasedduration;
    }

    if (ScaleDurationByBuildTime) {
        duration = (int)(duration * house->BuildSpeedBias);
    }

    if (IsAir)
    {
        Extension::Fetch(house)->Assign_AdvAI_Air_Tactic(AirTacticType, duration);
    }
    else if (IsNaval)
    {
        Extension::Fetch(house)->Assign_AdvAI_Naval_Tactic(NavalTacticType, duration);
    }
    else
    {
        Extension::Fetch(house)->Assign_AdvAI_Tactic(GroundTacticType, duration);
    }

    for (int i = 0; i < AdvAITeamTypes.Count(); i++)
    {
        AdvancedAITacticTeam& advaiteamtype = AdvAITeamTypes[i];

        TeamTypeClass* teamtype = nullptr;

        if (IsAir)
        {
            switch (AirTacticType)
            {
            case AdvAIAirTacticType::AIRTACTIC_ATTACK_VEHICLES:
                teamtype = Find_Or_Make_Aircraft_Vs_Vehicles_TeamType(house, advaiteamtype.IsSuicide);
                break;
            case AdvAIAirTacticType::AIRTACTIC_ATTACK_INFANTRY:
                teamtype = Find_Or_Make_Aircraft_Vs_Infantry_TeamType(house, advaiteamtype.IsSuicide);
                break;
            case AdvAIAirTacticType::AIRTACTIC_ATTACK_HARVESTERS:
                teamtype = Find_Or_Make_Aircraft_Vs_Harvesters_TeamType(house, advaiteamtype.IsSuicide);
                break;
            case AdvAIAirTacticType::AIRTACTIC_ATTACK_REFINERIES:
                teamtype = Find_Or_Make_Aircraft_Vs_Refineries_TeamType(house, advaiteamtype.IsSuicide);
                break;
            case AdvAIAirTacticType::AIRTACTIC_ATTACK_FACTORIES:
                teamtype = Find_Or_Make_Aircraft_Vs_Factories_TeamType(house, advaiteamtype.IsSuicide);
                break;
            default:
                DEBUG_ERROR("Unknown AdvAI aircraft team type. Air tactic type: %d\n", AirTacticType);
                Emergency_Exit(0);
                return false;
            }

            AdvAI_Create_Aircraft_Team(house, teamtype, advaiteamtype.MaxCost);
        }
        else if (IsNaval)
        {
            switch (advaiteamtype.TeamType)
            {
            case AdvAITeamType::ADVAITT_DIRECT_ATTACK:
                teamtype = Find_Or_Make_Direct_Attack_TeamType(house);
                break;
            default:
                DEBUG_ERROR("Unknown AdvAI naval team type: %d\n", advaiteamtype.TeamType);
                Emergency_Exit(0);
                return false;
            }

            AdvAI_Create_Naval_Team(house, teamtype, advaiteamtype.MaxCost);
        }
        else
        {
            switch (advaiteamtype.TeamType)
            {
            case AdvAITeamType::ADVAITT_SCOUT:
                teamtype = Find_Or_Make_Scouting_TeamType(house);
                break;
            case AdvAITeamType::ADVAITT_DIRECT_ATTACK:
                teamtype = Find_Or_Make_Direct_Attack_TeamType(house);
                break;
            case AdvAITeamType::ADVAITT_RUSH:
                teamtype = Find_Or_Make_Rush_TeamType(house);
                break;
            case AdvAITeamType::ADVAITT_ATTACK_REFINERIES:
                teamtype = Find_Or_Make_Anti_Refineries_TeamType(house, advaiteamtype.IsSuicide);
                break;
            case AdvAITeamType::ADVAITT_ATTACK_FACTORIES:
                // TODO currently unimplemented
                teamtype = Find_Or_Make_Direct_Attack_TeamType(house);
                break;
            case AdvAITeamType::ADVAITT_APC_VS_REFINERIES:
                teamtype = Find_Or_Make_APC_Vs_Refineries_TeamType(house);
                break;
            case AdvAITeamType::ADVAITT_APC_VS_FACTORIES:
                teamtype = Find_Or_Make_APC_Vs_ConYards_TeamType(house);
                break;
            case AdvAITeamType::ADVAITT_ATTACK_HARVESTERS:
                teamtype = Find_Or_Make_Suicidal_Anti_Harvester_TeamType(house);
                break;
            case AdvAITeamType::ADVAITT_DEFEND:
                teamtype = Find_Or_Make_Defense_TeamType(house);
                break;
            default:
                DEBUG_ERROR("Unknown AdvAI team type: %d\n", advaiteamtype.TeamType);
                Emergency_Exit(0);
                return false;
            }

            AdvAI_Create_Team(house, teamtype, advaiteamtype.LimitToRTTI, advaiteamtype.MaxCost, advaiteamtype.IsTransportTeam, advaiteamtype.ForcedTacticType);
        }
    }

    if (SetTacticalValueIndex > -1 && (SetTacticalValueChance >= 100 || Percent_Chance(SetTacticalValueChance)))
    {
        int value = SetTacticalValueMin;
        if (SetTacticalValueMax > value)
        {
            value = Random_Pick(value, SetTacticalValueMax);
        }

        Extension::Fetch(house)->FactionSpecificTacticalValues[SetTacticalValueIndex] = value;
    }

    return true;
}

const AdvAITacticType AdvancedAITacticTypeClass::Get_AdvAI_Ground_Tactic_Type(const char* buffer)
{
    if (!strcasecmp(buffer, "<none>") || !strcasecmp(buffer, "none")) {
        return AdvAITacticType::TACTIC_NONE;
    }

    if (!strcasecmp("TACTIC_SCOUT", buffer)) {
        return AdvAITacticType::TACTIC_SCOUT;
    }

    if (!strcasecmp("TACTIC_DIRECT_ATTACK_REGULAR", buffer)) {
        return AdvAITacticType::TACTIC_DIRECT_ATTACK_REGULAR;
    }

    if (!strcasecmp("TACTIC_DIRECT_ATTACK_FAST", buffer)) {
        return AdvAITacticType::TACTIC_DIRECT_ATTACK_FAST;
    }

    if (!strcasecmp("TACTIC_SIEGE", buffer)) {
        return AdvAITacticType::TACTIC_SIEGE;
    }

    if (!strcasecmp("TACTIC_RUSH_ATTACK", buffer)) {
        return AdvAITacticType::TACTIC_RUSH_ATTACK;
    }

    if (!strcasecmp("TACTIC_ATTACK_HARVESTERS", buffer)) {
        return AdvAITacticType::TACTIC_ATTACK_HARVESTERS;
    }

    if (!strcasecmp("TACTIC_ATTACK_REFINERIES", buffer)) {
        return AdvAITacticType::TACTIC_ATTACK_REFINERIES;
    }

    if (!strcasecmp("TACTIC_APC_ATTACK", buffer)) {
        return AdvAITacticType::TACTIC_APC_ATTACK;
    }

    if (!strcasecmp("TACTIC_CHINOOK_ATTACK", buffer)) {
        return AdvAITacticType::TACTIC_CHINOOK_ATTACK;
    }

    if (!strcasecmp("TACTIC_DEFEND", buffer)) {
        return AdvAITacticType::TACTIC_DEFEND;
    }

    DEBUG_ERROR("Unknown AdvAI ground tactic type %s\n", buffer);
    return AdvAITacticType::TACTIC_NONE;
}

const AdvAIAirTacticType AdvancedAITacticTypeClass::Get_AdvAI_Air_Tactic_Type(const char* buffer)
{
    if (!strcasecmp(buffer, "<none>") || !strcasecmp(buffer, "none")) {
        return AdvAIAirTacticType::AIRTACTIC_NONE;
    }

    if (!strcasecmp("AIRTACTIC_ATTACK_VEHICLES", buffer)) {
        return AdvAIAirTacticType::AIRTACTIC_ATTACK_VEHICLES;
    }

    if (!strcasecmp("AIRTACTIC_ATTACK_INFANTRY", buffer)) {
        return AdvAIAirTacticType::AIRTACTIC_ATTACK_INFANTRY;
    }

    if (!strcasecmp("AIRTACTIC_ATTACK_HARVESTERS", buffer)) {
        return AdvAIAirTacticType::AIRTACTIC_ATTACK_HARVESTERS;
    }

    if (!strcasecmp("AIRTACTIC_ATTACK_REFINERIES", buffer)) {
        return AdvAIAirTacticType::AIRTACTIC_ATTACK_REFINERIES;
    }

    if (!strcasecmp("AIRTACTIC_ATTACK_FACTORIES", buffer)) {
        return AdvAIAirTacticType::AIRTACTIC_ATTACK_FACTORIES;
    }

    DEBUG_ERROR("Unknown AdvAI air tactic type %s\n", buffer);
    return AdvAIAirTacticType::AIRTACTIC_NONE;
}

const AdvAINavalTacticType AdvancedAITacticTypeClass::Get_AdvAI_Naval_Tactic_Type(const char* buffer)
{
    if (!strcasecmp(buffer, "<none>") || !strcasecmp(buffer, "none")) {
        return AdvAINavalTacticType::NAVALTACTIC_NONE;
    }

    if (!strcasecmp("NAVALTACTIC_DIRECT_ATTACK", buffer)) {
        return AdvAINavalTacticType::NAVALTACTIC_DIRECT_ATTACK;
    }

    DEBUG_ERROR("Unknown AdvAI naval tactic type %s\n", buffer);
    return AdvAINavalTacticType::NAVALTACTIC_NONE;
}

const AdvAITacticConditionType Get_AdvAI_Tactic_Condition_Type(const char* buffer)
{
    if (!strcasecmp(buffer, "<none>") || !strcasecmp(buffer, "none")) {
        return AdvAITacticConditionType::TACTICCOND_NONE;
    }

    if (!strcasecmp("TACTICCOND_NONE", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_NONE;
    }

    if (!strcasecmp("TACTICCOND_FRAME_LESS_THAN", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_FRAME_LESS_THAN;
    }

    if (!strcasecmp("TACTICCOND_FRAME_MORE_THAN", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_FRAME_MORE_THAN;
    }

    if (!strcasecmp("TACTICCOND_ENEMY_IS_HOUSETYPE", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_ENEMY_IS_HOUSETYPE;
    }

    if (!strcasecmp("TACTICCOND_ENEMY_IS_NOT_HOUSETYPE", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_ENEMY_IS_NOT_HOUSETYPE;
    }

    if (!strcasecmp("TACTICCOND_PERCENT_CHANCE", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_PERCENT_CHANCE;
    }

    if (!strcasecmp("TACTICCOND_HAS_WAR_FACTORY", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_HAS_WAR_FACTORY;
    }

    if (!strcasecmp("TACTICCOND_HAS_NO_WAR_FACTORY", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_HAS_NO_WAR_FACTORY;
    }

    if (!strcasecmp("TACTICCOND_IS_OUTNUMBERED", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_IS_OUTNUMBERED;
    }

    if (!strcasecmp("TACTICCOND_IS_NOT_OUTNUMBERED", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_IS_NOT_OUTNUMBERED;
    }

    if (!strcasecmp("TACTICCOND_IS_DISADVANTAGED", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_IS_DISADVANTAGED;
    }

    if (!strcasecmp("TACTICCOND_IS_NOT_DISADVANTAGED", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_IS_NOT_DISADVANTAGED;
    }

    if (!strcasecmp("TACTICCOND_ENEMY_LIGHTLY_DEFENDED_REFINERIES", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_ENEMY_LIGHTLY_DEFENDED_REFINERIES;
    }

    if (!strcasecmp("TACTICCOND_ENEMY_VULNERABLE_REFINERIES", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_ENEMY_VULNERABLE_REFINERIES;
    }

    if (!strcasecmp("TACTICCOND_ENEMY_LIGHTLY_DEFENDED_CONYARDS", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_ENEMY_LIGHTLY_DEFENDED_CONYARDS;
    }

    if (!strcasecmp("TACTICCOND_ENEMY_VULNERABLE_CONYARDS", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_ENEMY_VULNERABLE_CONYARDS;
    }

    if (!strcasecmp("TACTICCOND_ENEMY_HAS_SENSORS", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_ENEMY_HAS_SENSORS;
    }

    if (!strcasecmp("TACTICCOND_ENEMY_HAS_NO_SENSORS", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_ENEMY_HAS_NO_SENSORS;
    }

    if (!strcasecmp("TACTICCOND_ENEMY_HARVESTER_COUNT_EXCEEDS", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_ENEMY_HARVESTER_COUNT_EXCEEDS;
    }

    if (!strcasecmp("TACTICCOND_NOT_REPEATED_FOR_FRAMES", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_NOT_REPEATED_FOR_FRAMES;
    }

    if (!strcasecmp("TACTICCOND_HOUSE_TACTICAL_VALUE_ABOVE", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_HOUSE_TACTICAL_VALUE_ABOVE;
    }

    if (!strcasecmp("TACTICCOND_HOUSE_TACTICAL_VALUE_BELOW", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_HOUSE_TACTICAL_VALUE_BELOW;
    }

    if (!strcasecmp("TACTICCOND_ENEMY_ARTILLERY_STRENGTH_ABOVE", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_ENEMY_ARTILLERY_STRENGTH_ABOVE;
    }

    if (!strcasecmp("TACTICCOND_ENEMY_ANTI_AIR_STRENGTH_BELOW", buffer)) {
        return AdvAITacticConditionType::TACTICCOND_ENEMY_ANTI_AIR_STRENGTH_BELOW;
    }

    DEBUG_ERROR("Unknown AdvAI tactic condition type %s\n", buffer);
    return AdvAITacticConditionType::TACTICCOND_NONE;
}

const AdvAITeamType Get_AdvAI_Team_Type(const char* buffer)
{
    if (!strcasecmp(buffer, "<none>") || !strcasecmp(buffer, "none")) {
        return AdvAITeamType::ADVAITT_NONE;
    }

    if (!strcasecmp("ADVAITT_SCOUT", buffer)) {
        return AdvAITeamType::ADVAITT_SCOUT;
    }

    if (!strcasecmp("ADVAITT_DIRECT_ATTACK", buffer)) {
        return AdvAITeamType::ADVAITT_DIRECT_ATTACK;
    }

    if (!strcasecmp("ADVAITT_RUSH", buffer)) {
        return AdvAITeamType::ADVAITT_RUSH;
    }

    if (!strcasecmp("ADVAITT_ATTACK_REFINERIES", buffer)) {
        return AdvAITeamType::ADVAITT_ATTACK_REFINERIES;
    }

    if (!strcasecmp("ADVAITT_ATTACK_FACTORIES", buffer)) {
        return AdvAITeamType::ADVAITT_ATTACK_FACTORIES;
    }

    if (!strcasecmp("ADVAITT_APC_VS_REFINERIES", buffer)) {
        return AdvAITeamType::ADVAITT_APC_VS_REFINERIES;
    }

    if (!strcasecmp("ADVAITT_APC_VS_FACTORIES", buffer)) {
        return AdvAITeamType::ADVAITT_APC_VS_FACTORIES;
    }

    if (!strcasecmp("ADVAITT_ATTACK_HARVESTERS", buffer)) {
        return AdvAITeamType::ADVAITT_ATTACK_HARVESTERS;
    }

    if (!strcasecmp("ADVAITT_DEFEND", buffer)) {
        return AdvAITeamType::ADVAITT_DEFEND;
    }

    DEBUG_ERROR("Unknown AdvAI team type %s\n", buffer);
    return AdvAITeamType::ADVAITT_NONE;
}

bool AdvancedAITacticTypeClass::Read_INI(CCINIClass& ini)
{
    if (!ini.Is_Present(IniName)) {
        return false;
    }

    char keybuffer[128];
    char outputbuffer[1024];

    char* delimiter = ",";
    int i = 0;

    // Read conditions
    // Conditions.N=ConditionType,parameter
    while (true)
    {
        sprintf(keybuffer, "Conditions.%d", i);

        if (ini.Get_String(IniName, keybuffer, outputbuffer, sizeof(outputbuffer)) > 0)
        {
            char* token = strtok(outputbuffer, delimiter);

            AdvAITacticConditionType conditiontype = Get_AdvAI_Tactic_Condition_Type(token);

            token = strtok(nullptr, delimiter);
            int param1 = atoi(token);
            int param2 = 0;
            if (conditiontype == AdvAITacticConditionType::TACTICCOND_HOUSE_TACTICAL_VALUE_ABOVE || 
                conditiontype == AdvAITacticConditionType::TACTICCOND_HOUSE_TACTICAL_VALUE_BELOW)
            {
                token = strtok(nullptr, delimiter);
                param2 = atoi(token);
            }

            AdvancedAITacticCondition condition = AdvancedAITacticCondition();
            condition.ConditionType = conditiontype;
            condition.Parameter1 = param1;
            condition.Parameter2 = param2;

            Conditions.Add(condition);

            i++;
        }
        else
        {
            break;
        }
    }

    // Read TeamTypes
    // AdvAITeamTypes.N=TeamType,IsSuicide,IsTransportTeam,AvoidThreats,MaxCost,LimitToRTTI,TacticTypeOverride
    i = 0;
    while (true)
    {
        sprintf(keybuffer, "AdvAITeamTypes.%d", i);

        if (ini.Get_String(IniName, keybuffer, outputbuffer, sizeof(outputbuffer)) > 0)
        {
            char* token = strtok(outputbuffer, delimiter);

            AdvAITeamType advaitt = Get_AdvAI_Team_Type(token);

            token = strtok(nullptr, delimiter);
            bool suicide = atoi(token) > 0;
            token = strtok(nullptr, delimiter);
            bool transportteam = atoi(token) > 0;
            token = strtok(nullptr, delimiter);
            bool avoidthreats = atoi(token) > 0;
            token = strtok(nullptr, delimiter);
            int maxcost = atoi(token);
            token = strtok(nullptr, delimiter);
            RTTIType rtti = RTTI_From_Name(token);
            token = strtok(nullptr, delimiter);
            AdvAITacticType tacticoverride = Get_AdvAI_Ground_Tactic_Type(token);

            AdvancedAITacticTeam advaiteamtype = AdvancedAITacticTeam();
            advaiteamtype.TeamType = advaitt;
            advaiteamtype.IsSuicide = suicide;
            advaiteamtype.IsTransportTeam = transportteam;
            advaiteamtype.AvoidThreats = avoidthreats;
            advaiteamtype.MaxCost = maxcost;
            advaiteamtype.LimitToRTTI = rtti;
            advaiteamtype.ForcedTacticType = tacticoverride;

            AdvAITeamTypes.Add(advaiteamtype);

            i++;
        }
        else
        {
            break;
        }
    }

    // Only suicide and max cost are relevant for aircraft team types
    if (IsAir && ini.Get_String(IniName, "AdvAIAircraftTeamType", outputbuffer, sizeof(outputbuffer)) > 0)
    {
        char* token = strtok(outputbuffer, delimiter);
        bool suicide = atoi(token) > 0;
        token = strtok(nullptr, delimiter);
        int maxcost = atoi(token);

        AdvancedAITacticTeam advaiteamtype = AdvancedAITacticTeam();
        advaiteamtype.TeamType = AdvAITeamType::ADVAITT_NONE;
        advaiteamtype.IsSuicide = suicide;
        advaiteamtype.IsTransportTeam = false;
        advaiteamtype.AvoidThreats = true;
        advaiteamtype.MaxCost = maxcost;
        advaiteamtype.LimitToRTTI = RTTI_AIRCRAFT;
        advaiteamtype.ForcedTacticType = TACTIC_NONE;

        AdvAITeamTypes.Add(advaiteamtype);
    }
    
    if (ini.Get_String(IniName, "PrimaryTacticType", outputbuffer, sizeof(outputbuffer)) > 0) 
    {
        if (IsAir)
        {
            AirTacticType = Get_AdvAI_Air_Tactic_Type(outputbuffer);
        }
        else if (IsNaval)
        {
            NavalTacticType = Get_AdvAI_Naval_Tactic_Type(outputbuffer);
        }
        else
        {
            GroundTacticType = Get_AdvAI_Ground_Tactic_Type(outputbuffer);
        }
    }
    
    MinimumExpectedDuration = ini.Get_Int(IniName, "MinimumExpectedDuration", MinimumExpectedDuration);
    MaximumExpectedDuration = ini.Get_Int(IniName, "MaximumExpectedDuration", MinimumExpectedDuration);
    ScaleDurationByBuildTime = ini.Get_Bool(IniName, "ScaleDurationByBuildTime", ScaleDurationByBuildTime);
    House = (HousesType)ini.Get_Int(IniName, "House", (int)House);
    SetTacticalValueIndex = ini.Get_Int(IniName, "SetTacticalValueIndex", SetTacticalValueIndex);
    SetTacticalValueMin = ini.Get_Int(IniName, "SetTacticalValueMin", SetTacticalValueMin);
    SetTacticalValueMax = ini.Get_Int(IniName, "SetTacticalValueMax", SetTacticalValueMax);
    FrameBasedDurationIncrease = ini.Get_Int(IniName, "FrameBasedDurationIncrease", FrameBasedDurationIncrease);
    FrameBasedDurationTimeStep = ini.Get_Int(IniName, "FrameBasedDurationTimeStep", FrameBasedDurationTimeStep);
    FrameBasedDurationIncreaseMax = ini.Get_Int(IniName, "FrameBasedDurationIncreaseMax", FrameBasedDurationIncreaseMax);

    return true;
}
