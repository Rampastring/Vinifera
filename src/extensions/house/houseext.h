/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Extended HouseClass class.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#pragma once

#include "abstractext.h"
#include "advaitactictype.h"
#include "detach_listener.h"
#include "house.h"
#include "housetype.h"
#include "technotypeext.h"

template <typename T> struct AdvAITacticInfo
{
    AdvAITacticInfo() { }

    AdvAITacticInfo(T tactic, int selectionframe, int duration)
    {
        Tactic = tactic;
        SelectionFrame = selectionframe;
        Duration = duration;
    }

    T Tactic;
    int SelectionFrame;
    int Duration;

    int EndFrame()
    {
        return SelectionFrame + Duration;
    }
};

struct AttemptedBuildingCaptureStruct
{
    AttemptedBuildingCaptureStruct() { }
    
    AttemptedBuildingCaptureStruct(Cell buildingloc, int frame)
    {
        BuildingLocation = buildingloc;
        Frame = frame;
    }

    int operator==(const AttemptedBuildingCaptureStruct& q) const { return std::memcmp(this, &q, sizeof(q)) == 0; }
    int operator!=(const AttemptedBuildingCaptureStruct& q) const { return std::memcmp(this, &q, sizeof(q)) != 0; }

    Cell BuildingLocation;
    int Frame;
};

struct EnemyStrengthStruct
{
    EnemyStrengthStruct() { }

    int None;
    int Light;
    int Heavy;

    int Total() const { return None + Light + Heavy; }

    void Clear()
    {
        None = 0;
        Light = 0;
        Heavy = 0;
    }

    void Add_Techno_Type(TechnoTypeClassExtension* technotypeext)
    {
        None += technotypeext->AntiNoneArmorValue();
        Light += technotypeext->AntiLightArmorValue();
        Heavy += technotypeext->AntiHeavyArmorValue();
    }

    void Add_Techno_Type_Half_Weight(TechnoTypeClassExtension* technotypeext)
    {
        None += technotypeext->AntiNoneArmorValue() / 2;
        Light += technotypeext->AntiLightArmorValue() / 2;
        Heavy += technotypeext->AntiHeavyArmorValue() / 2;
    }

    void Add_Techno_Type_AntiAir(TechnoTypeClassExtension* technotypeext)
    {
        None += technotypeext->AntiNoneArmorValueAA();
        Light += technotypeext->AntiLightArmorValueAA();
        Heavy += technotypeext->AntiHeavyArmorValueAA();
    }
};

enum class AdvancedAINavalOnlyState
{
    NOT_CHECKED = 0,
    NORMAL = 1,
    NAVAL_ONLY = 2
};


class FactoryClass;


class DECLSPEC_UUID(UUID_HOUSE_EXTENSION)
HouseClassExtension final : public AbstractClassExtension,
                            public Vinifera::Detach::Listener<FactoryClass>
{
public:
    /**
     *  IPersist
     */
    IFACEMETHOD(GetClassID)(CLSID *pClassID);

    /**
     *  IPersistStream
     */
    IFACEMETHOD(Load)(IStream *pStm);
    IFACEMETHOD(Save)(IStream *pStm, BOOL fClearDirty);

public:
    HouseClassExtension(const HouseClass *this_ptr = nullptr);
    HouseClassExtension(const NoInitClass &noinit);
    virtual ~HouseClassExtension();

    virtual int Get_Object_Size() const override;
    virtual void Object_CRC(CRCEngine &crc) const override;

    void On_Detach(FactoryClass *target, bool all) override;

    virtual const char *Name() const override { return reinterpret_cast<const HouseClass *>(This())->Class->Name(); }
    virtual const char *Full_Name() const override { return reinterpret_cast<const HouseClass *>(This())->Class->Full_Name(); }
    
    virtual HouseClass *This() const override { return reinterpret_cast<HouseClass *>(AbstractClassExtension::This()); }
    virtual const HouseClass *This_Const() const override { return reinterpret_cast<const HouseClass *>(AbstractClassExtension::This_Const()); }
    virtual RTTIType Fetch_RTTI() const override { return RTTI_HOUSE; }

    FactoryClass* Fetch_Factory(RTTIType rtti, ProductionFlags flags) const;
    void Set_Factory(RTTIType rtti, FactoryClass* factory, ProductionFlags flags);
    int* Factory_Counter(RTTIType rtti, ProductionFlags flags);
    int Factory_Count(RTTIType rtti, ProductionFlags flags) const;
    ProdFailType Suspend_Production(RTTIType type, ProductionFlags flags);
    ProdFailType Begin_Production(RTTIType type, int id, bool resume, ProductionFlags flags);
    ProdFailType Abandon_Production(RTTIType type, int id, ProductionFlags flags);
    bool Place_Object(RTTIType type, Cell const& cell, ProductionFlags flags);
    void Update_Factories(RTTIType rtti, ProductionFlags flags);
    TechnoTypeClass const* Suggest_New_Object(RTTIType objecttype, ProductionFlags flags) const;

    int AI_Unit();
    int AI_Naval_Unit();

    bool Has_Prerequisite(int prerequisite);
    bool Has_Prerequisite(PrerequisiteGroupType group);
    bool Has_Prerequisite(StructType building);

    bool Required_Forbidden_Houses_Check(TechnoTypeClass const* ttype);

    void Put_Storage_Pointers();
    static void Load_Unit_Trackers(HouseClass* house, IStream* pStm);
    static void Save_Unit_Trackers(HouseClass* house, IStream* pStm);

    void Set_Spawn_Point(const Cell& cell);

    static HouseClass* House_At_Spawn_Point(WAYPOINT waypoint);
    static HouseClass* House_From_HousesType(HousesType house);

    bool AdvAI_Is_Outnumbered() const;
    bool AdvAI_Is_Disadvantaged() const;
    void Assign_AdvAI_Tactic(AdvAITacticType tactic, int expected_duration);
    void Assign_AdvAI_Air_Tactic(AdvAIAirTacticType airtactic, int expected_duration);
    void Assign_AdvAI_Naval_Tactic(AdvAINavalTacticType navaltactic, int expected_duration);
    TeamClass* Get_Team_In_Production(int& id, RTTIType for_type, ProductionFlags prodflags) const;
    void Fill_Owned_Buildings_List(DynamicVectorClass<const BuildingTypeClass*>& owned) const;
    bool _AI_Has_Prerequisites(const TechnoTypeClass* type, DynamicVectorClass<const BuildingTypeClass*>& owned, int ownedcount) const;
    int Get_Building_Capture_Attempt_Index_For(BuildingClass* building) const;
    bool Is_Valid_Building_For_Capturing(BuildingClass* building, Cell zonecell, bool & okintheory) const;
    void Add_Building_Capture_Attempt(BuildingClass* building);
    bool Has_One_Of(DynamicVectorClass<BuildingTypeClass*> buildingtypes) const;
    bool Has_Barracks() const;
    bool Has_War_Factory() const;
    bool Has_Naval_Yard() const;
    bool Has_Helipad() const;
    bool Has_Construction_Yard() const;
    bool Has_Radar() const;
    bool Has_Tech_Center() const;
    bool Enemy_Building_Scan(DynamicVectorClass<BuildingTypeClass*>& list, int threatvalue) const;
    bool AdvAI_Enemy_Has_Vulnerable_Buildings(DynamicVectorClass<BuildingTypeClass*>& list) const;
    bool AdvAI_Enemy_Has_Lightly_Defended_Buildings(DynamicVectorClass<BuildingTypeClass*>& list) const;
    void AdvAI_Bias_Team_Strength_Ratios_By_Enemy_Strength(TeamClass* team) const;
    void AdvAI_Set_Ground_Team_Desired_Ratios(TeamClass* team, AdvAITacticType tacticoverride = AdvAITacticType::TACTIC_NONE) const;
    void AdvAI_Set_Aircraft_Team_Desired_Ratios(TeamClass* team) const;
    void AdvAI_Set_Naval_Team_Desired_Ratios(TeamClass* team) const;
    bool AdvAI_Is_Recently_Attacked() const;

    bool Can_Use_Iron_Curtain() const;
    void Expend_Iron_Curtain();

public:
    /**
     *  Replacement Tiberium storage.
     */
    VectorClass<int> TiberiumStorage;

    /**
     *  Replacement Weed storage.
     */
    VectorClass<int> WeedStorage;

    /**
     *  Record the number of naval factories active.
     */
    int NavalFactories;

    /**
     *  For human controlled houses, only one type of naval unit can be produced
     *  at any one instant. This is the factory object controlling this production.
     */
    FactoryClass* NavalFactory;

    /**
     *  The type of the naval unit the AI is currently scheduled to build.
     */
    UnitType BuildNavalUnit;

    /**
     *  The waypoint at which this house was spawned.
     */
    WAYPOINT SpawnWaypoint;

    /**
     *  Provides a timer for the availability of the Iron Curtain for this house.
     *  Used until we have a proper superweapon based Iron Curtain implementation.
     */
    CDTimerClass<FrameTimerClass> IronCurtainAvailabilityTimer;

    /**
     *  Determines whether repairs are paused instead of stopped when this house has insufficient funds.
     */
    bool IsPauseRepairs;

    /**
     *  Is this house an observer?
     */
    bool IsObserver;

    int StrengthenDestroyedCost;

    /**
     *  If we are currently expanding our base towards a resourceful location,
     *  this records the cell that we are expanding towards.
     */
    Cell NextExpansionPointLocation;

    Cell ArchivedExpansionPointLocation;

    /**
     *  Locations that we should never expand towards.
     *  Basically, locations that are unreachable.
     */
    Cell PermanentlyBlockedExpansionPointLocations[20];

    /**
     *  Records whether the AI has reached its expansion point.
     *  If yes, the AI should build a refinery.
     */
    bool ShouldBuildRefinery;

    /**
     *  The next planned defense placement location.
     */
    Cell DefensePlacementLocation;

    /**
     *  Set when the AI has built its first barracks during the game.
     *  Used to figure out whether the AI should reset its TeamDelay
     *  timer when it has built a barracks.
     */
    bool HasBuiltFirstBarracks;

    /**
     *  Records when the AI last checked for excess refineries.
     */
    int LastExcessRefineryCheckFrame;

    /**
     *  Records when the AI last checked for sleeping harvesters.
     */
    int LastSleepingHarvesterCheckFrame;

    /**
     *  Defines whether the AI has already performed a final "desperate vehicle charge".
     *  If it has been done, then there is no need to do it again.
     */
    bool HasPerformedVehicleCharge;

    /**
     *  Records a value whether the current structure build choice 
     *  was made under threat of getting rushed early in the game.
     */
    bool IsUnderStartRushThreat;

    /**
     *  Next frame where this house should check for producing engineers
     *  for capturing a neutral oil refinery.
     */
    int NextOilRefineryCaptureCheckFrame;

    /**
     *  Next frame where this house should give orders to its engineers.
     */
    int NextEngineerCheckFrame;

    /**
     *  Currently chosen tactic for the Advanced AI.
     */

    AdvAITacticInfo<AdvAITacticType> AdvAIGroundTactic;
    AdvAITacticInfo<AdvAIAirTacticType> AdvAIAirTactic;
    AdvAITacticInfo<AdvAINavalTacticType> AdvAINavalTactic;

    int AdvAILastExecutionFrameForTactic[TACTIC_COUNT];

    /**
     *  Frame when Advanced AI last executed a tactic.
     */
    int AdvAILastTacticExecutionFrame;

    int EnemyNoneStrength;
    int EnemyLightStrength;
    int EnemyHeavyStrength;
    int EnemyArtilleryStrength;
    int EnemyBaseDefenseStrength;
    int EnemyNavalStrength;
    EnemyStrengthStruct EnemyAntiGroundStrength;
    EnemyStrengthStruct EnemyAntiAirStrength;
    EnemyStrengthStruct EnemyAntiNavalStrength;
    int EnemyHarvesterCount;
    int EnemyRefineryCount;
    bool EnemyHasSensors;

    int LastHarvesterBuildFrame;

    int LastUnitValueDebugPrintFrame;
    int LastInfantryValueDebugPrintFrame;
    int LastAircraftValueDebugPrintFrame;
    int LastNavalValueDebugPrintFrame;

    AttemptedBuildingCaptureStruct AttemptedBuildingCaptures[50];
    int AttemptedBuildingCaptureCount;

    UnitType AdvAILastBuiltUnit;
    int AdvAILastBuiltUnitCount;
    InfantryType AdvAILastBuiltInfantry;
    int AdvAILastBuiltInfantryCount;

    int FactionSpecificTacticalValues[10];

    int AdvAIFunValue;

    int AdvAILastUndeployableUnitCheckFrame;

    AdvancedAINavalOnlyState IsNavalOnly;

    int LastNavalOnlyCheckFrame;
};
