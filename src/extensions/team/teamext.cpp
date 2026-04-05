/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera
 *
 *  @file          TEAMEXT.CPP
 *
 *  @author        Rampastring
 *
 *  @brief         Extended TeamClass class.
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
#include "teamext.h"
#include "team.h"
#include "teamtype.h"
#include "foot.h"
#include "houseext.h"
#include "rulesext.h"
#include "technotypeext.h"
#include "unit.h"
#include "ccini.h"
#include "tibsun_inline.h"
#include "vinifera_globals.h"
#include "vinifera_saveload.h"
#include "wwcrc.h"
#include "extension.h"
#include "asserthandler.h"
#include "debughandler.h"


/**
 *  Class constructor.
 *
 *  @author: Rampastring
 */
TeamClassExtension::TeamClassExtension(const TeamClass *this_ptr) :
    AbstractClassExtension(this_ptr),
    AntiNoneStrength(0),
    AntiLightStrength(0),
    AntiHeavyStrength(0),
    ArtilleryStrength(0),
    DesiredAntiNoneRatio(0),
    DesiredAntiLightRatio(0),
    DesiredAntiHeavyRatio(0),
    DesiredArtilleryRatio(0),
    NoneArmorStrength(0),
    LightArmorStrength(0),
    HeavyArmorStrength(0),
    DesiredNoneStrengthRatio(0),
    DesiredLightStrengthRatio(0),
    DesiredHeavyStrengthRatio(0),
    IsAdvAITeam(false),
    OriginalTeamMemberCount(0),
    SpeedValueMultiplier(1.0),
    StrengthValueMultiplier(1.0),
    CloakValueMultiplier(0.0),
    CostWeightMultiplier(1.0),
    NoVehicles(false),
    NoInfantry(false),
    NoAircraft(false),
    MinimumReadyFrame(0),
    DesiredInfantryRatio(-1.0),
    DesiredVehicleRatio(-1.0),
    InfantryCost(0),
    VehicleCost(0),
    MaxCost(0),
    CurrentCost(0),
    IsTransportTeam(false),
    MaxInfantry(-1),
    AdvAIGroundTacticType(AdvAITacticType::TACTIC_NONE),
    AdvAITactic(nullptr),
    IsAircraftTeam(false),
    IsBiasedForEnemyStrength(false),
    PenalizeSameTypeUnits(false),
    ProdFlags(PRODFLAG_NONE)
{
    //if (this_ptr) EXT_DEBUG_TRACE("TeamClassExtension::TeamClassExtension - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));

    TeamExtensions.Add(this);
}


/**
 *  Class no-init constructor.
 *
 *  @author: Rampastring
 */
TeamClassExtension::TeamClassExtension(const NoInitClass &noinit) :
    AbstractClassExtension(noinit)
{
    //EXT_DEBUG_TRACE("TeamClassExtension::TeamClassExtension(NoInitClass) - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));
}


/**
 *  Class destructor.
 *
 *  @author: Rampastring
 */
TeamClassExtension::~TeamClassExtension()
{
    //EXT_DEBUG_TRACE("TeamClassExtension::~TeamClassExtension - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));

    TeamExtensions.Delete(this);
}


/**
 *  Retrieves the class identifier (CLSID) of the object.
 *
 *  @author: Rampastring
 */
HRESULT TeamClassExtension::GetClassID(CLSID *lpClassID)
{
    //EXT_DEBUG_TRACE("TeamClassExtension::GetClassID - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));

    if (lpClassID == nullptr) {
        return E_POINTER;
    }

    *lpClassID = __uuidof(this);

    return S_OK;
}


/**
 *  Initializes an object from the stream where it was saved previously.
 *
 *  @author: Rampastring
 */
HRESULT TeamClassExtension::Load(IStream *pStm)
{
    //EXT_DEBUG_TRACE("TeamClassExtension::Load - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));

    HRESULT hr = AbstractClassExtension::Internal_Load(pStm);
    if (FAILED(hr)) {
        return E_FAIL;
    }

    new (this) TeamClassExtension(NoInitClass());

    VINIFERA_SWIZZLE_REQUEST_POINTER_REMAP(AdvAITactic, "AdvAITactic");

    return hr;
}


/**
 *  Saves an object to the specified stream.
 *
 *  @author: Rampastring
 */
HRESULT TeamClassExtension::Save(IStream *pStm, BOOL fClearDirty)
{
    //EXT_DEBUG_TRACE("TeamClassExtension::Save - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));

    HRESULT hr = AbstractClassExtension::Internal_Save(pStm, fClearDirty);
    if (FAILED(hr)) {
        return hr;
    }

    return hr;
}


/**
 *  Return the raw size of class data for save/load purposes.
 *
 *  @author: Rampastring
 */
int TeamClassExtension::Get_Object_Size() const
{
    //EXT_DEBUG_TRACE("TeamClassExtension::Get_Object_Size - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));

    return sizeof(*this);
}


/**
 *  Removes the specified target from any targeting and reference trackers.
 *
 *  @author: Rampastring
 */
void TeamClassExtension::Detach(AbstractClass * target, bool all)
{
    //EXT_DEBUG_TRACE("TeamClassExtension::Detach - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));
}


/**
 *  Compute a unique crc value for this instance.
 *
 *  @author: Rampastring
 */
void TeamClassExtension::Object_CRC(CRCEngine &crc) const
{
    //EXT_DEBUG_TRACE("TeamClassExtension::Object_CRC - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));
}

/**
 *  Returns the name of this object type.
 *
 *  @author: Rampastring
 */
const char* TeamClassExtension::Name() const
{
    //EXT_DEBUG_TRACE("ObjectClassExtension::Name - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));

    return This()->Class->Name();
}


/**
 *  Returns the full name of this object type.
 *
 *  @author: CCHyper
 */
const char* TeamClassExtension::Full_Name() const
{
    //EXT_DEBUG_TRACE("ObjectClassExtension::Full_Name - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));

    return This()->Class->Full_Name();
}

/**
 *  Copies team execution related state from another instance.
 *  This function does NOT copy state that is only used for producing the team.
 *
 *  @author: Rampastring
 */
void TeamClassExtension::Copy_Executive_State_From(TeamClassExtension* other)
{
    IsAdvAITeam = other->IsAdvAITeam;
    MinimumReadyFrame = other->MinimumReadyFrame;
    IsTransportTeam = other->IsTransportTeam;
    AdvAIGroundTacticType = other->AdvAIGroundTacticType;
    AdvAITactic = other->AdvAITactic;
    IsAircraftTeam = other->IsAircraftTeam;
    ProdFlags = other->ProdFlags;
}

void TeamClassExtension::AdvAI_Team_Recruit_AI()
{
    Coord center = This()->Zone != NULL ? This()->Zone->Center_Coord() : COORD_NONE;

    if (This()->Class->Get_Origin() != CELL_NONE) {
        center = This()->Class->Get_Origin().As_Coord();
    }

    if (IsTransportTeam && Contains_Transport() && Infantry_Count() >= MaxInfantry) {
        return;
    }

    for (int i = 0; i < Foots.Count(); i++)
    {
        FootClass* foot = Foots[i];

        if (!foot->IsActive || !foot->IsDown || foot->House != This()->House) {
            continue;
        }

        if (NoAircraft && foot->RTTI == RTTI_AIRCRAFT) {
            continue;
        }

        if (NoInfantry && foot->RTTI == RTTI_INFANTRY) {
            continue;
        }

        if (NoVehicles && foot->RTTI == RTTI_UNIT) {
            continue;
        }

        if (ProdFlags != PRODFLAG_NAVAL && Extension::Fetch(foot->TClass)->IsNaval) {
            continue;
        }

        if (!foot->Is_Weapon_Equipped()) {
            continue;
        }

        // Do not recruit units that don't have full ammo
        if (foot->TClass->MaxAmmo > 0 && foot->Ammo != foot->TClass->MaxAmmo) {
            continue;
        }

        if (IsTransportTeam) {
            if (foot->RTTI != RTTI_INFANTRY) {
                if (foot->TClass->MaxPassengers <= 0 || Contains_Transport()) {
                    continue;
                }
            }
            else {
                if (Infantry_Count() >= MaxInfantry) {
                    continue;
                }
            }
        }

        if (foot->Team != nullptr && foot->Team->Class->RecruitPriority >= This()->Class->RecruitPriority) {
            continue;
        }

        if (This()->Class->Group == -2 || foot->Group == This()->Class->Group || This()->Class->IsRecruiter) {
            foot->Assign_Target(nullptr);
            This()->Add(foot);
        }
    }
}

void TeamClassExtension::AdvAI_Team_Maintenance_AI()
{
    // If we have members inside of an APC, then make sure they don't have any movement or attack targets.
    if (IsTransportTeam)
    {
        FootClass* member = This()->Member;

        while (member != nullptr) {
            if (member->IsInLimbo) {

                if (member->NavCom != nullptr) member->Assign_Destination(nullptr);
                if (member->TarCom != nullptr) member->Assign_Target(nullptr);
                if (member->ArchiveTarget != nullptr) member->Assign_Archive_Target(nullptr);
            }

            member = member->Member;
        }
    }
}

int TeamClassExtension::Object_Type_Count(TechnoTypeClass* technotype) const
{
    int count = 0;
    FootClass* member = This()->Member;

    while (member != nullptr) {
        if (member->TClass == technotype)
            count++;

        member = member->Member;
    }

    return count;
}

int TeamClassExtension::AdvAI_Get_Object_Value_For_Team(TechnoTypeClass* technotype, bool debug) const
{
    if (MaxCost > 0 && CurrentCost + technotype->Cost > MaxCost) {
        return -1;
    }

    if (technotype->RTTI == RTTI_INFANTRYTYPE) {

        int infcount = Infantry_Count();

        if (MaxInfantry > -1 && infcount >= MaxInfantry)
        {
            return -1;
        }

        if (infcount >= RuleExtension->AdvancedAITeamCheapInfantryMax &&
            technotype->Cost < RuleExtension->AdvancedAICheapInfantryCostThreshold)
        {
            return -1;
        }
    }

    if (IsTransportTeam)
    {
        if (technotype->RTTI == RTTI_UNITTYPE || technotype->RTTI == RTTI_AIRCRAFTTYPE) 
        {
            if (technotype->MaxPassengers <= 0) {
                return -1;
            }

            if (Contains_Transport()) {
                return -1;
            }

            return technotype->MaxPassengers * technotype->MaxSpeed * (technotype->IsCloakable ? 2 : 1);
        }
    }

    TechnoTypeClassExtension* technotypeext = Extension::Fetch(technotype);

    int antinonevalue = technotypeext->AntiNoneArmorValue();
    int antilightvalue = technotypeext->AntiLightArmorValue();
    int antiheavyvalue = technotypeext->AntiHeavyArmorValue();
    int artilleryvalue = technotypeext->ArtilleryValue();

    if (debug) {
        DEBUG_INFO("        Info for %s: AntiNone: %d, AntiLight: %d, AntiHeavy: %d, Artillery: %d", technotype->IniName.c_str(), antinonevalue, antilightvalue, antiheavyvalue, artilleryvalue);
    }

    if (AdvAIGroundTacticType == AdvAITacticType::TACTIC_APC_ATTACK && technotype->RTTI == RTTI_INFANTRYTYPE) {
        if (reinterpret_cast<InfantryTypeClass*>(technotype)->IsBomber) {
            antilightvalue += 1000;
        }
    }

    int teamtotalstrength = AntiNoneStrength + AntiLightStrength + AntiHeavyStrength;
    double teamnoneratio = 0;
    double teamlightratio = 0;
    double teamheavyratio = 0;
    double teamartilleryratio = 0;

    if (teamtotalstrength > 0) {
        teamnoneratio = AntiNoneStrength / (double)teamtotalstrength;
        teamlightratio = AntiLightStrength / (double)teamtotalstrength;
        teamheavyratio = AntiHeavyStrength / (double)teamtotalstrength;
        teamartilleryratio = ArtilleryStrength / (double)teamtotalstrength;
    }

    double desirednoneratio = DesiredAntiNoneRatio;
    if (teamnoneratio > 0.0)
        desirednoneratio = DesiredAntiNoneRatio * (DesiredAntiNoneRatio / teamnoneratio);

    double desiredlightratio = DesiredAntiLightRatio;
    if (teamlightratio > 0.0)
        desiredlightratio = DesiredAntiLightRatio * (DesiredAntiLightRatio / teamlightratio);

    double desiredheavyratio = DesiredAntiHeavyRatio;
    if (teamheavyratio > 0)
        desiredheavyratio = DesiredAntiHeavyRatio * (DesiredAntiHeavyRatio / teamheavyratio);

    double desiredartilleryratio = DesiredArtilleryRatio;
    if (teamartilleryratio > 0)
        desiredartilleryratio = DesiredArtilleryRatio * (DesiredArtilleryRatio / teamartilleryratio);

    int totaloffensivevalue = (int)(antinonevalue * desirednoneratio + 
        antilightvalue * desiredlightratio + 
        antiheavyvalue * desiredheavyratio +
        artilleryvalue * desiredartilleryratio);

    if (totaloffensivevalue <= 0)
        return INT_MIN;

    if (debug)
    {
        DEBUG_INFO(", weighted offensive: %d", totaloffensivevalue);
    }

    HouseClassExtension* houseext = Extension::Fetch(This()->House);

    int teamtotaldefensive = NoneArmorStrength + LightArmorStrength + HeavyArmorStrength;

    int defensivevalue = technotypeext->DefensiveValue() * StrengthValueMultiplier;

    if (debug)
    {
        DEBUG_INFO(", base defensive: %d", defensivevalue);
    }

    if (teamtotaldefensive > 0 && 
        (DesiredNoneStrengthRatio > 0.0 || DesiredLightStrengthRatio > 0.0 || DesiredHeavyStrengthRatio > 0.0))
    {
        if (technotype->Armor == ARMOR_NONE) {
            double currentratio = NoneArmorStrength / (double)teamtotaldefensive;
            double diff = DesiredNoneStrengthRatio - currentratio;
            diff *= RuleExtension->AdvancedAIOverweightedArmorTypeAntiBias;
            diff = std::max(-0.9, diff);
            defensivevalue = defensivevalue * (1.0 + diff);
        }
        else if (Extension::Fetch(technotype)->CategorizedAsLightlyArmored()) {
            double currentratio = LightArmorStrength / (double)teamtotaldefensive;
            double diff = DesiredLightStrengthRatio - currentratio;
            diff *= RuleExtension->AdvancedAIOverweightedArmorTypeAntiBias;
            diff = std::max(-0.9, diff);
            defensivevalue = defensivevalue * (1.0 + diff);
        }
        else if (technotype->Armor == ARMOR_STEEL) {
            double currentratio = HeavyArmorStrength / (double)teamtotaldefensive;
            double diff = DesiredHeavyStrengthRatio - currentratio;
            diff *= RuleExtension->AdvancedAIOverweightedArmorTypeAntiBias;
            diff = std::max(-0.9, diff);
            defensivevalue = defensivevalue * (1.0 + diff);
        }
    }

    if (debug)
    {
        DEBUG_INFO(", weighted defensive: %d", defensivevalue);
    }

    int totalvalue = (int)std::pow(totaloffensivevalue + defensivevalue, 1.2) / 2;

    if (debug)
    {
        DEBUG_INFO(", total: %d", totalvalue);
    }

    if (technotype->RTTI == RTTI_INFANTRYTYPE) {
        if (DesiredInfantryRatio >= 0.0) {
            double infratio = InfantryCost / (double)CurrentCost;
            totalvalue = totalvalue * (DesiredInfantryRatio / infratio);
        }

        if (PenalizeSameTypeUnits) {
            int count = Object_Type_Count(technotype);
            double reduction = count * RuleExtension->AdvancedAISameUnitAntiBias;
            reduction = std::min(0.9, reduction);
            totalvalue = totalvalue * (1.0 - reduction);
        }

        // Avoid infantry spam
        auto houseext = Extension::Fetch(This()->House);
        if (houseext->Has_War_Factory() && InfantryCost > 0)
        {
            double ratio = RuleExtension->AdvancedAITeamInfantryCostPunishmentThreshold / (double)InfantryCost;

            if (ratio < 1.0)
                totalvalue = totalvalue * ratio;
        }
    }
    else if (technotype->RTTI == RTTI_UNITTYPE) {
        if (DesiredVehicleRatio >= 0.0) {
            double vehicleratio = VehicleCost / (double)CurrentCost;
            totalvalue = totalvalue * (DesiredVehicleRatio / vehicleratio);
        }

        if (PenalizeSameTypeUnits) {
            int count = Object_Type_Count(technotype);
            double multi = ProdFlags == PRODFLAG_NAVAL ? 3.0 : 1.0;
            double reduction = count * RuleExtension->AdvancedAISameUnitAntiBias * multi;
            reduction = std::min(0.9, reduction);
            totalvalue = totalvalue * (1.0 - reduction);
        }
    }

    int adjusted = technotypeext->Scale_Value_By_Properties(totalvalue, SpeedValueMultiplier, StrengthValueMultiplier, CloakValueMultiplier, CostWeightMultiplier);

    if (debug)
    {
        DEBUG_INFO(", after adjustments: %d\n", adjusted);
    }

    // Add some randomness for fun and unpredictability
    // return adjusted;
    return Random_Pick((adjusted * 2)  / 3, (adjusted * 3) / 2);
}

bool TeamClassExtension::Contains_Transport() const
{
    FootClass* member = This()->Member;

    while (member != nullptr) {
        if ((member->RTTI == RTTI_UNIT || member->RTTI == RTTI_AIRCRAFT) && member->TClass->Max_Passengers() > 0) {
            return true;
        }

        member = member->Member;
    }

    return false;
}

int TeamClassExtension::Infantry_Count() const
{
    int inf = 0;
    FootClass* member = This()->Member;

    while (member != nullptr) {
        if (member->RTTI == RTTI_INFANTRY) {
            inf++;
        }

        member = member->Member;
    }

    return inf;
}