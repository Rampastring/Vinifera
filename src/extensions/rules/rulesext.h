/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera
 *
 *  @file          RULESEXT.H
 *
 *  @author        CCHyper
 *
 *  @brief         Extended RulesClass class.
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
#pragma once

#include "always.h"
#include "extension.h"
#include "point.h"
#include "rules.h"
#include "tibsun_defines.h"


class CCINIClass;


class RulesClassExtension final : public GlobalExtensionClass<RulesClass>
{
public:
    IFACEMETHOD(Load)(IStream* pStm);
    IFACEMETHOD(Save)(IStream* pStm, BOOL fClearDirty);

public:
    RulesClassExtension(const RulesClass* this_ptr);
    RulesClassExtension(const NoInitClass& noinit);
    virtual ~RulesClassExtension();

    virtual int Get_Object_Size() const override;
    virtual void Detach(AbstractClass* target, bool all = true) override;
    virtual void Object_CRC(CRCEngine& crc) const override;

    virtual const char* Name() const override { return "Rule"; }
    virtual const char* Full_Name() const override { return "Rule"; }

    void Process(CCINIClass& ini);
    void Initialize(CCINIClass& ini);

    bool Objects(CCINIClass& ini);

    bool AI(CCINIClass & ini);
    bool General(CCINIClass& ini);
    bool MPlayer(CCINIClass& ini);
    bool AudioVisual(CCINIClass& ini);
    bool CombatDamage(CCINIClass& ini);
    bool Weapons(CCINIClass& ini);
    bool Armors(CCINIClass& ini);
    bool Rockets(CCINIClass& ini);
    bool Tiberiums(CCINIClass& ini);
    bool PrerequisiteGroups(CCINIClass& ini);
    bool AdvancedAIGroundTactics(CCINIClass& ini);
    bool AdvancedAIAirTactics(CCINIClass& ini);
    bool AdvancedAINavalTactics(CCINIClass& ini);

    static bool Set_Voxel_Light_Angle(float azimuth, float elevation, float offset);

private:
    void Check();
    void Fixups(CCINIClass& ini);

public:
    /**
     *  Should the MCV unit auto deploy on game start?
     */
    bool IsMPAutoDeployMCV;

    /**
     *  Are construction yards pre-placed on the map rather than a MCV given to the player?
     */
    bool IsMPPrePlacedConYards;

    /**
     *  Can players build their own structures adjacent to structures owned by their allies?
     */
    bool IsBuildOffAlly;

    /**
     *  Should active super weapons show their recharge timer display
     *  on the tactical view?
     */
    bool IsShowSuperWeaponTimers;

    /**
     *  Defines the strength of ice. Higher values make ice less likely
     *  to break from a shot.
     */
    int IceStrength;

    /**
     *  Storage pip used for weeds.
     */
    int WeedPipIndex;

    /**
     *  Customizable maximum counts for drawing different pips.
     */
    TypeList<int> MaxPips;

    /**
     *  When looking for refineries, harvesters will prefer a distant free
     *  refinery over a closer occupied refinery if the refineries' distance
     *  difference in cells is less than this.
     */
    int MaxFreeRefineryDistanceBias;

    /**
     *  Should prerequisites be rechecked when buildings are lost, making the player lose access to units/buildings?
     */
    bool IsRecheckPrerequisites;

    /**
     *  Should the game assume there is more than one MCV (that factions don't share their MCV?)
     */
    bool IsMultiMCV;

    /**
     *  The distance in cells the computer player can place their Naval Yard from their Construction Yard.
     */
    int AINavalYardAdjacency;

    /**
     *  The "double penalty" or "half penalty". Multiply this by the power
     *  units you are short of to get the actual penalty to the build speed.
     */
    float LowPowerPenaltyModifier;

    /**
     *  The maximum number of factories that can be considered when calculating
     *  the multiple factory bonus on an object's build time.
     */
    int MultipleFactoryCap;

    /**
     *  Horizontal direction of the light source.
     */
    float VoxelLightAzimuth;

    /**
     *  Vertical angle of the light source.
     */
    float VoxelLightElevation;

    /**
     *  How much the shadow is offset from the unit.
     */
    float VoxelShadowOffset;

    /**
     *  Determines whether the Tiberium storage logic is enabled.
     */
    bool IsTiberiumStorage;

    /**
     *  Sounds played when a unit is promoted.
     */
    VocType UpgradeVeteranSound;
    VocType UpgradeEliteSound;

    /**
     *  EVA announcement when a unit is promoted.
     */
    VoxType VoxUnitPromoted;

    /**
     *  The number of frames that a newly elite unit will flash for.
     */
    int EliteFlashTimer;

    /**
     *  Controls for beacons.
     */
    bool IsBeaconsEnabled;
    bool IsSPBeacons;
    int MaxBeacons;
    VocType PlaceBeaconSound;
    VoxType PlaceBeaconVoice;
    VoxType DetectBeaconVoice;

    /**
     *  Defines for how many frames buildings do not get flames spawned on them on
     *  damage state change after once catching fire.
     */
    int BuildingFlameSpawnBlockFrames;

    /**
     *  How much value (in credits) a house needs to destroy to strengthen their objects by one percentage.
     */
    int StrengthenDestroyedValueThreshold;

    /**
     *  A multiplier for value of buildings when a house destroys objects.
     *  Allows making buildings more valuable than units for purposes of the Strengthening mechanic.
     */
    int StrengthenBuildingValueMultiplier;

    /**
     *  Is the strengthening mechanic enabled?
     */
    bool IsStrengtheningEnabled;

    /**
     *  Is DTA's custom advanced AI logic enabled?
     */
    bool IsUseAdvancedAI;

    /**
     *  Is the advanced AI allowed to own multiple Construction Yards at a time?
     */
    bool IsAdvancedAIMultiConYard;

    /**
     *  Specifies the maximum distance that the advanced AI is allowed to expand at.
     */
    int AdvancedAIMaxExpansionDistance;

    /**
     *  Specifies the minimum number of refineries that the Advanced AI thinks it should build.
     */
    int AdvancedAIMinimumRefineryCount;

    /**
     *  List of naval yards for the AI to build.
     */
    TypeList<BuildingTypeClass*> BuildNavalYard;

    /**
     *  Specifies the distance to a Tiberium tree that is considered "close enough" for building a refinery near the tree.
     */
    int AdvancedAIExpansionCloseEnough;

    /**
     *  Specifies the maximum distance that a refinery without an assigned expansion point 
     *  can have to a Tiberium tree for the tree to be considered occupied.
     */
    int AdvancedAIFieldOccupyMaximumDistance;

    /**
     *  Percent chance that the AI will kite at any given level.
     */
    TypeList<int> AIKiteChance;

    TypeList<int> AdvancedAITacticSelectionDelay;

    TypeList<int> AdvancedAIIonCannonRandomizationFactors;

    int AdvancedAIExpansionDistanceComparisonRandomness;

    /**
     *  Maximum threat value that a building's cell can have for it to be considered viable for sneak attacks.
     */
    int AdvancedAIVulnerableBuildingMaxThreat;

    /**
     *  Maximum threat value that a building's cell can have for it to be considered viable for attacks that can handle only light resistance.
     */
    int AdvancedAILightlyDefendedBuildingMaxThreat;

    /**
     *  Percentual value of how much the AI lowers the preference for building a unit if
     *  a unit of the same type already exists in the team. Cumulative.
     */
    double AdvancedAISameUnitAntiBias;


    double AdvancedAIOverweightedArmorTypeAntiBias;

    /**
     *  Multiplier to value of each weapon. Determines the balance between preferring offensive and defensive value of units.
     */
    int AdvancedAIOffensiveWeaponValueMultiplier;

    /**
     *  If the Advanced AI has a war factory, it will skip building infantry if the highest-scoring infantry is not valued
     *  above this threshold.
     */
    int AdvancedAISkipInfantryProductionValueThreshold;

    /**
     *  If the Advanced AI has a war factory, it has a chance of skipping building 
     *  infantry for some time if the highest-scoring infantry is not valued above this threshold.
     */
    int AdvancedAIConditionalSkipInfantryProductionValueThreshold;

    /**
     *  If lacking this many or more anti-ground defenses, the AI should prioritize building base defenses instead of economy and production.
     */
    int AdvancedAICriticalBaseDefenseDeficiencyThreshold;

    bool AdvancedAIIonCannonTargeting;

    bool AdvancedAINukeTargeting;

    bool AdvancedAIBaseBuilding;

    bool AdvancedAIUnitProduction;

    bool AdvancedAIAreaGuard;

    bool AdvancedAISmartHunt;

    bool AdvancedAIAircraftTargeting;

    bool AdvancedAIAircraftReuse;

    int AdvancedAINoTechCenterBeforeFrame;

    /**
     *  Is LandType Beach considered as "requires crushing" for passability purposes, as opposed to water?
     */
    bool IsBeachIsCrush;
};
