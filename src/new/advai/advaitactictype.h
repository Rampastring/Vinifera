/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera (Dawn of the Tiberium Age Build)
 *
 *  @file          ADVAITACTICTYPE.H
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
#pragma once

#include "always.h"
#include "macros.h"
#include "vinifera_defines.h"
#include "foot.h"
#include "ttimer.h"

enum AdvAIAirTacticType
{
    AIRTACTIC_NONE = -1,
    AIRTACTIC_ATTACK_VEHICLES = 0,
    AIRTACTIC_ATTACK_INFANTRY = 1,
    AIRTACTIC_ATTACK_HARVESTERS = 2,
    AIRTACTIC_ATTACK_REFINERIES = 3,
    AIRTACTIC_ATTACK_FACTORIES = 4
};
DEFINE_ENUMERATION_OPERATORS(AdvAIAirTacticType);

enum AdvAINavalTacticType
{
    NAVALTACTIC_NONE = -1,
    NAVALTACTIC_DIRECT_ATTACK = 0
};
DEFINE_ENUMERATION_OPERATORS(AdvAINavalTacticType);

enum AdvAITacticType
{
    TACTIC_NONE = -1,
    TACTIC_SCOUT = 0,
    TACTIC_DIRECT_ATTACK_REGULAR,
    TACTIC_DIRECT_ATTACK_FAST,
    TACTIC_SIEGE,
    TACTIC_RUSH_ATTACK,
    TACTIC_ATTACK_HARVESTERS,
    TACTIC_ATTACK_REFINERIES,
    TACTIC_APC_ATTACK,
    TACTIC_CHINOOK_ATTACK,
    TACTIC_DEFEND,
    TACTIC_COUNT
};
DEFINE_ENUMERATION_OPERATORS(AdvAITacticType);

static const char* AdvAITacticType_To_Name(AdvAITacticType tactic)
{
    if (tactic == AdvAITacticType::TACTIC_NONE)
        return "None";

    static const char* _names[] =
    {
        "Scout",
        "Direct Attack (Regular)",
        "Direct Attack (Fast)",
        "Siege",
        "Rush",
        "Attack Harvesters",
        "Attack Refineries",
        "APC Attack",
        "Chinook Attack",
        "Defend"
    };

    return _names[(int)tactic];
}

enum class AdvAITacticConditionType
{
    TACTICCOND_NONE = -1,
    TACTICCOND_FRAME_LESS_THAN = 0,
    TACTICCOND_FRAME_MORE_THAN = 1,
    TACTICCOND_ENEMY_IS_HOUSETYPE = 2,
    TACTICCOND_ENEMY_IS_NOT_HOUSETYPE = 3,
    TACTICCOND_PERCENT_CHANCE = 4,
    TACTICCOND_HAS_WAR_FACTORY = 5,
    TACTICCOND_HAS_NO_WAR_FACTORY = 6,
    TACTICCOND_IS_OUTNUMBERED = 7,
    TACTICCOND_IS_NOT_OUTNUMBERED = 8,
    TACTICCOND_IS_DISADVANTAGED = 9,
    TACTICCOND_IS_NOT_DISADVANTAGED = 10,
    TACTICCOND_ENEMY_LIGHTLY_DEFENDED_REFINERIES = 11,
    TACTICCOND_ENEMY_VULNERABLE_REFINERIES = 12,
    TACTICCOND_ENEMY_LIGHTLY_DEFENDED_CONYARDS = 13,
    TACTICCOND_ENEMY_VULNERABLE_CONYARDS = 14,
    TACTICCOND_ENEMY_HAS_SENSORS = 15,
    TACTICCOND_ENEMY_HAS_NO_SENSORS = 16,
    TACTICCOND_ENEMY_HARVESTER_COUNT_EXCEEDS = 17,
    TACTICCOND_NOT_REPEATED_FOR_FRAMES = 18,
    TACTICCOND_HOUSE_TACTICAL_VALUE_ABOVE = 19,
    TACTICCOND_HOUSE_TACTICAL_VALUE_BELOW = 20,
    TACTICCOND_ENEMY_ARTILLERY_STRENGTH_ABOVE = 21,
    TACTICCOND_ENEMY_ANTI_AIR_STRENGTH_BELOW = 22,
    TACTICCOND_COUNT
};

enum class AdvAITeamType
{
    ADVAITT_NONE = -1,
    ADVAITT_SCOUT = 0,
    ADVAITT_DIRECT_ATTACK = 1,
    ADVAITT_RUSH = 2,
    ADVAITT_ATTACK_REFINERIES = 3,
    ADVAITT_ATTACK_FACTORIES = 4,
    ADVAITT_APC_VS_REFINERIES = 5,
    ADVAITT_APC_VS_FACTORIES = 6,
    ADVAITT_ATTACK_HARVESTERS = 7,
    ADVAITT_DEFEND = 8,
    ADVAITT_COUNT
};

class DECLSPEC_UUID(UUID_ADVANCED_AI_TACTIC)
AdvancedAITacticTypeClass final : IPersistStream
{
public:
    union TacticType
    {
        AdvAITacticType GroundTacticType;
        AdvAIAirTacticType AirTacticType;
        AdvAINavalTacticType NavalTacticType;
    };

    struct AdvancedAITacticCondition
    {
        AdvAITacticConditionType ConditionType;
        int Parameter1;
        int Parameter2;

        bool operator==(const AdvancedAITacticCondition& other) const
        {
            return ConditionType == other.ConditionType &&
                Parameter1 == other.Parameter1 &&
                Parameter2 == other.Parameter2;
        }

        bool operator!=(const AdvancedAITacticCondition& other) const
        {
            return !operator==(other);
        }
    };
    
    struct AdvancedAITacticTeam
    {
        AdvAITacticType ForcedTacticType;
        AdvAITeamType TeamType;
        bool IsSuicide;
        bool IsTransportTeam;
        bool AvoidThreats;
        int MaxCost;
        RTTIType LimitToRTTI;

        bool operator==(const AdvancedAITacticTeam& other) const
        {
            return ForcedTacticType == other.ForcedTacticType &&
                TeamType == other.TeamType &&
                IsSuicide == other.IsSuicide &&
                IsTransportTeam == other.IsTransportTeam &&
                AvoidThreats == other.AvoidThreats &&
                MaxCost == other.MaxCost &&
                LimitToRTTI == other.LimitToRTTI;
        }

        bool operator!=(const AdvancedAITacticTeam& other) const
        {
            return !operator==(other);
        }
    };

    /**
     *  IUnknown
     */
    IFACEMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    IFACEMETHOD_(ULONG, AddRef)();
    IFACEMETHOD_(ULONG, Release)();
    
    /**
    *  IPersist
    */
    IFACEMETHOD(GetClassID)(CLSID* pClassID);

    /**
     *  IPersistStream
     */
    IFACEMETHOD(IsDirty)();
    IFACEMETHOD(Load)(IStream* pStm);
    IFACEMETHOD(Save)(IStream* pStm, BOOL fClearDirty);
    IFACEMETHOD_(LONG, GetSizeMax)(ULARGE_INTEGER* pcbSize);

public:
    AdvancedAITacticTypeClass();
    AdvancedAITacticTypeClass(const NoInitClass& noinit) {}
    AdvancedAITacticTypeClass(const char* name);
    virtual ~AdvancedAITacticTypeClass();

    char const* Name() const { return IniName; }
    bool Read_INI(CCINIClass& ini);
    
    bool Process(HouseClass* house);

    static int From_Name(const char *name);
    static const AdvancedAITacticTypeClass* Find_Or_Make(const char *name, bool air = false, bool naval = false);
    static const AdvAITacticType Get_AdvAI_Ground_Tactic_Type(const char *buffer);
    static const AdvAIAirTacticType Get_AdvAI_Air_Tactic_Type(const char* buffer);
    static const AdvAINavalTacticType Get_AdvAI_Naval_Tactic_Type(const char* buffer);
    
private:
    char IniName[128];
    
public:
    AdvAITacticType GroundTacticType;
    AdvAIAirTacticType AirTacticType;
    AdvAINavalTacticType NavalTacticType;

    DynamicVectorClass<AdvancedAITacticCondition> Conditions;
    DynamicVectorClass<AdvancedAITacticTeam> AdvAITeamTypes;

    int MinimumExpectedDuration;
    int MaximumExpectedDuration;
    bool ScaleDurationByBuildTime;
    HousesType House;
    int SetTacticalValueIndex;
    int SetTacticalValueMin;
    int SetTacticalValueMax;
    int SetTacticalValueChance;
    int FrameBasedDurationIncrease;
    int FrameBasedDurationTimeStep;
    int FrameBasedDurationIncreaseMax;
    bool IsAir;
    bool IsNaval;
};