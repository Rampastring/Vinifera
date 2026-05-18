/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Extended TeamClass class.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#pragma once

#include "abstracttypeext.h"
#include "houseext.h"
#include "team.h"

typedef enum ViniferaQuarryType {
    VINIFERA_QUARRY_NONE,

    VINIFERA_QUARRY_ANYTHING,					// Attack any enemy (same as "hunt").
    VINIFERA_QUARRY_BUILDINGS,					// Attack buildings (in general).
    VINIFERA_QUARRY_TIBERIUM,				    // Attack harvesters or refineries.
    VINIFERA_QUARRY_INFANTRY,					// Attack infantry.
    VINIFERA_QUARRY_VEHICLES,					// Attack combat vehicles.
    VINIFERA_QUARRY_FACTORIES,					// Attack factories (all types).
    VINIFERA_QUARRY_DEFENSE,					// Attack base defense buildings.
    VINIFERA_QUARRY_THREAT,						// Attack enemies near friendly base.
    VINIFERA_QUARRY_POWER,						// Attack power facilities.
    VINIFERA_QUARRY_HARVESTERS,                 // Attack harvesters only (no refineries).

    VINIFERA_QUARRY_COUNT,
    VINIFERA_QUARRY_FIRST = 0
} ViniferaQuarryType;

class DECLSPEC_UUID(UUID_TEAM_EXTENSION)
TeamClassExtension final : public AbstractClassExtension
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
        TeamClassExtension(const TeamClass *this_ptr = nullptr);
        TeamClassExtension(const NoInitClass &noinit);
        virtual ~TeamClassExtension();

        virtual int Get_Object_Size() const override;
        virtual void Object_CRC(CRCEngine &crc) const override;

        virtual const char* Name() const override;
        virtual const char* Full_Name() const override;

        virtual TeamClass *This() const override { return reinterpret_cast<TeamClass *>(AbstractClassExtension::This()); }
        virtual const TeamClass *This_Const() const override { return reinterpret_cast<const TeamClass *>(AbstractClassExtension::This_Const()); }
        virtual RTTIType Fetch_RTTI() const override { return RTTI_TEAM; }

        void Copy_Executive_State_From(TeamClassExtension* other);
        void AdvAI_Team_Recruit_AI();
        void AdvAI_Team_Maintenance_AI();
        int AdvAI_Get_Object_Value_For_Team(TechnoTypeClass* technotype, bool debug) const;
        bool Contains_Transport() const;
        int Infantry_Count() const;
        int Object_Type_Count(TechnoTypeClass* technotype) const;

    public:
        int AntiNoneStrength;
        int AntiLightStrength;
        int AntiHeavyStrength;
        int ArtilleryStrength;
        double DesiredAntiNoneRatio;
        double DesiredAntiLightRatio;
        double DesiredAntiHeavyRatio;
        double DesiredArtilleryRatio;
        int NoneArmorStrength;
        int LightArmorStrength;
        int HeavyArmorStrength;
        double DesiredNoneStrengthRatio;
        double DesiredLightStrengthRatio;
        double DesiredHeavyStrengthRatio;
        bool IsAdvAITeam;
        int OriginalTeamMemberCount;
        double SpeedValueMultiplier;
        double StrengthValueMultiplier;
        double CloakValueMultiplier;
        double CostWeightMultiplier;
        bool NoVehicles;
        bool NoInfantry;
        bool NoAircraft;
        int MinimumReadyFrame;
        double DesiredInfantryRatio;
        double DesiredVehicleRatio;
        int InfantryCost;
        int VehicleCost;
        int MaxCost;
        int CurrentCost;
        bool IsTransportTeam;
        int MaxInfantry;
        AdvAITacticType AdvAIGroundTacticType;
        AdvancedAITacticTypeClass* AdvAITactic;
        bool IsAircraftTeam;
        bool IsBiasedForEnemyStrength;
        bool PenalizeSameTypeUnits;
        ProductionFlags ProdFlags;
};
