/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Extended TechnoClass class.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#include "always.h"

#include "technoext.h"

#include "anim.h"
#include "building.h"
#include "buildingtype.h"
#include "debughandler.h"
#include "dsurface.h"
#include "ebolt.h"
#include "extension.h"
#include "extension_globals.h"
#include "house.h"
#include "houseext.h"
#include "mouse.h"
#include "rules.h"
#include "rulesext.h"
#include "saveload.h"
#include "session.h"
#include "spawnmanager.h"
#include "storageext.h"
#include "tactical.h"
#include "team.h"
#include "teamtype.h"
#include "techno.h"
#include "technotype.h"
#include "technotypeext.h"
#include "tibsun_globals.h"
#include "tibsun_inline.h"
#include "unit.h"
#include "vinifera_globals.h"
#include "vinifera_saveload.h"
#include "voc.h"
#include "wwcrc.h"

#include <algorithm>
#include <cmath>


/**
 *  Class constructor.
 *
 *  @author: CCHyper
 */
TechnoClassExtension::TechnoClassExtension(const TechnoClass *this_ptr) :
    RadioClassExtension(this_ptr),
    Vinifera::Detach::Listener<TechnoClass>(),
    Vinifera::Detach::Listener<AnimClass>(),
    ElectricBolt(nullptr),
    Storage(Tiberiums.Count()),
    SpawnManager(nullptr),
    SpawnOwner(nullptr),
    HasOpportunityFireTarget(false),
    LastTargetFrame(Frame),
    IsToResetBurst(false),
    BurstResetTimer(),
    LastVeterancy(RANK_NONE),
    IdleWakeAnim(nullptr),
    IronCurtainTimer(),
    GapTimer()
{
    for (int i = 0; i < Tiberiums.Count(); i++)
    {
        Storage[i] = 0;
    }

    if (this_ptr)
    {
        new ((StorageClassExt*)&(this_ptr->Storage)) StorageClassExt(&Storage);

        const auto ttypeext = Extension::Fetch(this_ptr->TClass);
        if (ttypeext->Spawns)
            SpawnManager = new SpawnManagerClass(const_cast<TechnoClass*>(this_ptr), ttypeext->Spawns, ttypeext->SpawnsNumber, ttypeext->SpawnRegenRate, ttypeext->SpawnReloadRate, ttypeext->SpawnSpawnRate, ttypeext->SpawnLogicRate);
    }
}


/**
 *  Class no-init constructor.
 *  
 *  @author: CCHyper
 */
TechnoClassExtension::TechnoClassExtension(const NoInitClass &noinit) :
    RadioClassExtension(noinit),
    Vinifera::Detach::Listener<TechnoClass>(noinit),
    Vinifera::Detach::Listener<AnimClass>(noinit),
    Storage(noinit),
    BurstResetTimer(noinit),
    GapTimer(noinit)
{
}


/**
 *  Class destructor.
 *  
 *  @author: CCHyper
 */
TechnoClassExtension::~TechnoClassExtension()
{
    if (ElectricBolt) {
        delete ElectricBolt;
        ElectricBolt = nullptr;
    }

    if (SpawnManager) {
        delete SpawnManager;
        SpawnManager = nullptr;
    }

    if (IdleWakeAnim) {
        delete IdleWakeAnim;
        IdleWakeAnim = nullptr;
    }
}


/**
 *  Initializes an object from the stream where it was saved previously.
 *  
 *  @author: CCHyper
 */
HRESULT TechnoClassExtension::Load(IStream *pStm)
{
    HRESULT hr = RadioClassExtension::Load(pStm);
    if (FAILED(hr)) {
        return E_FAIL;
    }

    Load_Primitive_Vector(pStm, Storage);

    ElectricBolt = nullptr;

    VINIFERA_SWIZZLE_REQUEST_POINTER_REMAP(SpawnManager, "SpawnManager");
    VINIFERA_SWIZZLE_REQUEST_POINTER_REMAP(SpawnOwner, "SpawnOwner");

    VINIFERA_SWIZZLE_REQUEST_POINTER_REMAP(IdleWakeAnim, "IdleWakeAnim");
    
    return hr;
}


/**
 *  Saves an object to the specified stream.
 *  
 *  @author: CCHyper
 */
HRESULT TechnoClassExtension::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr = RadioClassExtension::Save(pStm, fClearDirty);
    if (FAILED(hr)) {
        return hr;
    }

    Save_Primitive_Vector(pStm, Storage);

    return hr;
}


/**
 *  Clears SpawnOwner if it pointed at the destroyed techno.
 *  SpawnManager (when present) is itself an Abstract listener and handles
 *  its own pointer cleanup via the registry.
 */
void TechnoClassExtension::On_Detach(TechnoClass *target, bool all)
{
    if (target == SpawnOwner) {
        SpawnOwner = nullptr;
    }
}


/**
 *  Clears IdleWakeAnim if it pointed at the destroyed anim.
 */
void TechnoClassExtension::On_Detach(AnimClass *target, bool all)
{
    if (target == IdleWakeAnim) {
        IdleWakeAnim = nullptr;
    }
}


/**
 *  Compute a unique crc value for this instance.
 *  
 *  @author: CCHyper
 */
void TechnoClassExtension::Object_CRC(CRCEngine &crc) const
{
    RadioClassExtension::Object_CRC(crc);

    crc(GapTimer.Value());

    if (SpawnOwner) {
        crc(SpawnOwner->Fetch_Heap_ID());
    }
}


/**
 *  Creates a electric bolt zap from the firing techno to the target.
 * 
 *  @author: CCHyper
 */
EBoltClass * TechnoClassExtension::Electric_Zap(AbstractClass * target, int which, const WeaponTypeClass *weapontype, Coord &source_coord)
{
    EBoltClass *ebolt = new EBoltClass;
    if (!ebolt) {
        return nullptr;
    }

    int z_adj = 0;

    if (Is_Target_Building(target)) {
        Coord source = This()->Render_Coord();

        Point2D p1 = TacticalMap->func_60F150(source);
        Point2D p2 = TacticalMap->func_60F150(source_coord);

        z_adj = p2.Y - p1.Y;
        z_adj = std::min(z_adj, 0);
    }

    Coord target_coord = Is_Target_Object(target) ?
        reinterpret_cast<ObjectClass *>(target)->Target_Coord() : target->entry_5C();

    /**
     *  Spawn the electric bolt.
     */
    ebolt->Create(source_coord, target_coord, z_adj);

    return ebolt;
}


/**
 *  Creates an instance of the electric bolt from the firing techno to the target.
 * 
 *  @author: CCHyper
 */
EBoltClass * TechnoClassExtension::Electric_Bolt(AbstractClass * target)
{
    WeaponSlotType which = This()->What_Weapon_Should_I_Use(target);
    const WeaponTypeClass *weapontype = This()->Get_Weapon(which)->Weapon;
    Coord fire_coord = This()->Fire_Coord(which);

    EBoltClass *ebolt = Electric_Zap(target, which, weapontype, fire_coord);
    if (ebolt) {
        if (This()->IsActive) {
            /**
             *  Remove existing electric bolt from the object.
             */
            if (ElectricBolt) {
                ElectricBolt->Flag_To_Delete();
                ElectricBolt = nullptr;
            }

            if (!ElectricBolt) {
                ElectricBolt = ebolt;
                ElectricBolt->Set_Properties(This(), weapontype, which);
            }
        }
    }

    return ebolt;
}


/**
 *  Handles the voice response when given capture order.
 * 
 *  @author: CCHyper
 */
void TechnoClassExtension::Response_Capture()
{
    if (!AllowVoice) {
        return;
    }

    //if (!This()->House->Is_Player_Control()) {
    //    return;
    //}

    VocType response = VOC_NONE;

    const TechnoTypeClass *technotype = Techno_Type_Class();
    const TechnoTypeClassExtension *technotypeext = Techno_Type_Class_Ext();
    if (technotypeext->VoiceCapture.Count() > 0) {

        response = technotypeext->VoiceCapture[Sim_Random_Pick(0, technotypeext->VoiceCapture.Count()-1)];

    } else if (technotype->VoiceMove.Count() > 0) {
        
        response = technotype->VoiceMove[Sim_Random_Pick(0, technotype->VoiceMove.Count()-1)];
    
    }

    Sound_Effect(response);
}


/**
 *  Handles the voice response when given enter order.
 * 
 *  @author: CCHyper
 */
void TechnoClassExtension::Response_Enter()
{
    if (!AllowVoice) {
        return;
    }

    //if (!This()->House->Is_Player_Control()) {
    //    return;
    //}

    VocType response = VOC_NONE;

    const TechnoTypeClass *technotype = Techno_Type_Class();
    const TechnoTypeClassExtension *technotypeext = Techno_Type_Class_Ext();
    if (technotypeext->VoiceEnter.Count() > 0) {

        response = technotypeext->VoiceEnter[Sim_Random_Pick(0, technotypeext->VoiceEnter.Count()-1)];

    } else if (technotype->VoiceMove.Count() > 0) {
        
        response = technotype->VoiceMove[Sim_Random_Pick(0, technotype->VoiceMove.Count()-1)];
    
    }

    Sound_Effect(response);
}


/**
 *  Handles the voice response when given deploy order.
 * 
 *  @author: CCHyper
 */
void TechnoClassExtension::Response_Deploy()
{
    if (!AllowVoice) {
        return;
    }

    //if (!This()->House->Is_Player_Control()) {
    //    return;
    //}

    VocType response = VOC_NONE;

    const TechnoTypeClass *technotype = Techno_Type_Class();
    const TechnoTypeClassExtension *technotypeext = Techno_Type_Class_Ext();
    if (technotypeext->VoiceDeploy.Count() > 0) {

        response = technotypeext->VoiceDeploy[Sim_Random_Pick(0, technotypeext->VoiceDeploy.Count()-1)];

    } else if (technotype->VoiceMove.Count() > 0) {
        
        response = technotype->VoiceMove[Sim_Random_Pick(0, technotype->VoiceMove.Count()-1)];
    
    }

    Sound_Effect(response);
}


/**
 *  Handles the voice response when given harvest order.
 * 
 *  @author: CCHyper
 */
void TechnoClassExtension::Response_Harvest()
{
    if (!AllowVoice) {
        return;
    }

    //if (!This()->House->Is_Player_Control()) {
    //    return;
    //}

    VocType response = VOC_NONE;

    const TechnoTypeClass *technotype = Techno_Type_Class();
    const TechnoTypeClassExtension *technotypeext = Techno_Type_Class_Ext();
    if (technotypeext->VoiceHarvest.Count() > 0) {

        response = technotypeext->VoiceHarvest[Sim_Random_Pick(0, technotypeext->VoiceHarvest.Count()-1)];

    } else if (technotype->VoiceMove.Count() > 0) {
        
        response = technotype->VoiceMove[Sim_Random_Pick(0, technotype->VoiceMove.Count()-1)];
    
    }

    Sound_Effect(response);
}


/**
 *  Returns if this object can acquire targets that are within range and attack them automatically.
 * 
 *  @author: CCHyper
 */
bool TechnoClassExtension::Can_Passive_Acquire() const
{
    if ((!This()->Is_Renovator() || !This()->House->Is_Human_Player()) && This()->Is_Weapon_Equipped()) {
        /**
         *  IsCanPassiveAcquire defaults to true to copy original behaviour, so all units can passive acquire unless told otherwise.
         */
        return Techno_Type_Class_Ext()->IsCanPassiveAcquire;
    }

    return false;
}


/**
 *  Returns the sight range of this techno after calculations.
 *  Takes into account veterancy bonuses as well height bonuses, if any.
 *
 *  @author: JoyfulShush
 */
int TechnoClassExtension::Get_Sight_Range() const
{
    auto techno_class_ext = Techno_Type_Class_Ext();

    int sight_range = This()->TClass->SightRange;
    if (This()->Crew.IsElite) {
        if (techno_class_ext->EliteSightRange > 0) {
            sight_range = techno_class_ext->EliteSightRange;
        } else if (techno_class_ext->VeteranSightRange > 0) {
            sight_range = techno_class_ext->VeteranSightRange;
        }
    } else if (This()->Crew.IsVeteran) {
        if (techno_class_ext->VeteranSightRange > 0) {
            sight_range = techno_class_ext->VeteranSightRange;
        }
    }

    sight_range *= (This()->SightIncrease * 0.01 + 1.0);
    if (This()->Has_Ability(ABILITY_SIGHT) && Rule->VeteranSight != 0.0) {
        sight_range *= Rule->VeteranSight + 1;
    }

    return sight_range;
}


/**
 *  Retain the native cloak/gap projection. Calculate in double precision
 *  because squaring large gap radii in leptons can overflow an int.
 */
static int Radial_Cell_Radius_To_Pixels(int cells)
{
    const double leptons = double(cells) * CELL_LEPTON_W + CELL_LEPTON_W / 2;
    const double xspan = std::sqrt(leptons * leptons * (16.0 / 17.0));
    const double yspan = xspan * 0.25;
    return int(std::sqrt(0.25 * (4.0 * yspan * yspan + xspan * xspan)) * 0.265625 - 34.0);
}


/**
 *  Draw an ellipse with optional rotating spokes using the native indicator style.
 *  The radius is the horizontal screen radius, in pixels.
 */
static void Draw_Radial_Indicator(const Coord& coord, int radius, const RGBClass& color, bool draw_scanline)
{
    if (radius <= 1) {
        return;
    }

    Point2D center;
    TacticalMap->Coord_To_Pixel(coord, center);
    center += TacticalRect.TopLeft;
    const Rect bounds(center - Point2D(radius, radius / 2), 2 * radius, radius);
    if (!Intersect(bounds, TacticalRect).Is_Valid()) {
        return;
    }

    LogicalSurface->Draw_Ellipse(center, radius, radius / 2, TacticalRect,
        DSurface::Build_Hicolor_Pixel(color.Get_Red(), color.Get_Green(), color.Get_Blue()));

    if (!draw_scanline) {
        return;
    }

    static const float transparencies[] = { 0.05f, 0.2f, 0.4f, 1.0f };
    const double radius_x = radius;
    const double radius_y = radius / 2;
    for (int i = 0; i < 4; ++i) {
        const double angle = (double(Frame) + i) * 0.005;
        const double dx = std::cos(angle);
        const double dy = -std::sin(angle);
        const double length = 1.0 / std::sqrt(dx * dx / (radius_x * radius_x) + dy * dy / (radius_y * radius_y));
        const Point2D end = center + Point2D(int(dx * length), int(dy * length));

        // This virtual entry is TS's depth-antialiased line renderer.
        LogicalSurface->Draw_Line_entry_3C(TacticalRect, center - TacticalRect.TopLeft,
            end - TacticalRect.TopLeft, color, -500, -500, false, false, true, false, transparencies[i]);
    }
}


/**
 *  Draw ranges for selected, player-owned technos or a pending building at
 *  its placement center. Placement previews omit scanlines and also include
 *  the native cloak/sensor range, which normally requires an active building.
 *  Gap/cloak/sensor ranges opt in with HasRadialIndicator; armed buildings
 *  always show their primary weapon range in their owner's remap color.
 */
void TechnoClassExtension::Draw_Radial_Indicators(const Coord* placement_center) const
{
    const auto techno = This_Const();
    if (!PlayerPtr || techno->House != PlayerPtr || (!placement_center && !techno->IsSelected)) {
        return;
    }

    const Coord center = placement_center ? *placement_center : techno->Center_Coord();
    const int cells = Techno_Type_Class_Ext()->GapRadiusInCells;
    if (cells > 0 && techno->TClass->IsHasRadialIndicator) {
        Draw_Radial_Indicator(center, Radial_Cell_Radius_To_Pixels(cells),
            RGBClass(techno->TClass->RadialColor), placement_center == nullptr);
    }

    if (techno->RTTI == RTTI_BUILDING) {
        // The virtual lookup includes weapons supplied by building upgrades
        // and returns zero when the primary slot has no weapon.
        int range = techno->Weapon_Range(WEAPON_SLOT_PRIMARY);

        // If the techno's guard range is lower than its weapon range, use guard range instead.
        if (range > techno->TClass->ThreatRange && techno->TClass->ThreatRange > 0)
            range = techno->TClass->ThreatRange;

        if (range > 0) {
            // Project a world-space circle through TS's isometric transform.
            // Keep the range in leptons to preserve fractional-cell ranges.
            const int radius = int(double(range) * CELL_PIXEL_W / (std::sqrt(2.0) * CELL_LEPTON_W));
            Draw_Radial_Indicator(center, radius, techno->House->RemapColorRGB, false);
        }

        const auto type = static_cast<const BuildingClass*>(techno)->Class;
        if (placement_center && type->IsHasRadialIndicator && type->CloakRadiusInCells > 0 &&
            (type->IsCloakGenerator || type->IsSensorArray)) {
            Draw_Radial_Indicator(center, Radial_Cell_Radius_To_Pixels(type->CloakRadiusInCells),
                RGBClass(type->RadialColor), false);
        }
    }
}


/**
 *  Shroud cells use the same height projection as MapClass::Sight_From.
 */
static Cell Gap_Shroud_Cell(Coord coord)
{
    const int offset = Tactical::Z_Lepton_To_Pixel(coord.Z) / -CELL_PIXEL_W;
    coord.X += offset * CELL_LEPTON_W;
    coord.Y += offset * CELL_LEPTON_H;
    return coord.As_Cell();
}


/**
 *  Periodically reshrouds an enemy's view, then restores overlapping sight.
 *  The timer advances on every client; only shroud changes are player-local.
 */
void TechnoClassExtension::Gap_Generator_AI()
{
    const auto techno = This();
    const int radius = Techno_Type_Class_Ext()->GapRadiusInCells;
    if (radius <= 0) {
        return;
    }

    if (!techno->IsActive || techno->IsInLimbo || !techno->IsLocked ||
        techno->Strength <= 0 || techno->IsSinking || techno->EMPFramesRemaining > 0 ||
        (techno->RTTI == RTTI_UNIT && static_cast<UnitClass*>(techno)->DeathCounter >= 0)) {
        GapTimer = 0;
        return;
    }

    if (techno->RTTI == RTTI_BUILDING) {
        const auto building = static_cast<BuildingClass*>(techno);
        if (building->CurrentMission == MISSION_CONSTRUCTION ||
            building->CurrentMission == MISSION_DECONSTRUCTION ||
            !building->Is_Powered_On() ||
            (building->Class->IsPowered && building->House->Power_Fraction() < 1.0f)) {
            GapTimer = 0;
            return;
        }
    }

    if (!GapTimer.Expired()) {
        return;
    }
    GapTimer = RuleExtension->GapRegenInterval;

    // TS maintains shroud for PlayerPtr on each client, not for every house.
    if (!PlayerPtr || techno->House->Is_Ally(PlayerPtr) ||
        Extension::Fetch(PlayerPtr)->IsObserver || Session.ObiWan ||
        Debug_Unshroud || Vinifera_Developer_Unshroud) {
        return;
    }

    const Cell center = Gap_Shroud_Cell(techno->Center_Coord());
    if (!Map.In_Radar(center)) {
        return;
    }

    const int min_x = std::max(0, center.X - radius);
    const int max_x = std::min(MAP_CELL_W - 1, center.X + radius);
    const int min_y = std::max(0, center.Y - radius);
    const int max_y = std::min(MAP_CELL_H - 1, center.Y + radius);
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            const int dx = x - center.X;
            const int dy = y - center.Y;
            Cell cell(x, y);
            if (dx * dx + dy * dy <= radius * radius && Map.In_Radar(cell)) {
                Map.Shroud_Cell(cell);
            }
        }
    }

    // Include flying units and shared sight. Look/Sight_From decide which
    // houses can reveal this player's map (including allies and limpet sight).
    for (int index = 0; index < Technos.Count(); ++index) {
        const auto observer = Technos[index];
        if (!observer->IsActive || observer->IsInLimbo || !observer->IsLocked ||
            observer->Strength <= 0 || techno->House->Is_Ally(observer->House)) {
            continue;
        }

        // Look refreshes SightIncrease, which participates in the techno CRC.
        // Use its current height bonus for the range check, but preserve the
        // synchronized value throughout this player-local reveal pass.
        const auto previous_sight_increase = observer->SightIncrease;
        observer->SightIncrease = 10 * (observer->Get_Coord().Z / Rule->LeptonsPerSightIncrease);
        const int sight = Extension::Fetch(observer)->Get_Sight_Range();
        observer->SightIncrease = previous_sight_increase;
        if (sight <= 0) {
            continue;
        }

        const Cell observer_cell = Gap_Shroud_Cell(observer->PositionCoord);
        const double dx = observer_cell.X - center.X;
        const double dy = observer_cell.Y - center.Y;
        // Shroud_Cell also changes visibility on adjacent cells.
        const double range = double(radius) + sight + 2;
        if (dx * dx + dy * dy <= range * range) {
            observer->Look();
            observer->SightIncrease = previous_sight_increase;
        }
    }

    // Shroud_Cell updates tactical visibility without invalidating radar pixels.
    if (Map.RadarSurface) {
        Map.Total_Radar_Refresh();
    }
    Map.Flag_To_Redraw(GS_REDRAW_TACTICAL);
}


/**
 *  Determines the time it would take to build this object.
 * 
 *  @author: CCHyper, ZivDero
 */
int TechnoClassExtension::Time_To_Build() const
{
    const TechnoTypeClassExtension* technotypeext = Techno_Type_Class_Ext();

    int time = Techno_Type_Class()->Time_To_Build();

    /**
     *  Adjust the time based on the house's build speed bonus.
     */
    time *= This()->House->BuildSpeedBias;

    /**
     *  #issue-657
     * 
     *  Implements BuildTimeMultiplier for TechnoTypes.
     * 
     *  @author: CCHyper
     */
    time *= technotypeext->BuildTimeMultiplier;

    /**
     *  Adjust the time to build based on the power output of the owning house.
     */
    double power = This()->House->Power_Fraction();

    /**
     *  #issue-656
     * 
     *  Implements LowPowerPenaltyModifier for RulesClass.
     * 
     *  @author: CCHyper
     */
    double scale = 1.0f - (1.0f - power) * RuleExtension->LowPowerPenaltyModifier;

    /**
     *  #issue-658
     *
     *  Restores the affect of "WorstLowPowerBuildRateCoefficient".
     *
     *  @author: CCHyper
     */
    if (scale <= Rule->WorstLowPowerBuildRateCoefficient) scale = Rule->WorstLowPowerBuildRateCoefficient;

    /**
     *  #issue-658
     *
     *  Restores the affect of "BestLowPowerBuildRateCoefficient".
     *
     *  @author: CCHyper
     */
    if (power < 1.0 && scale >= Rule->BestLowPowerBuildRateCoefficient) scale = Rule->BestLowPowerBuildRateCoefficient; // Was "0.75"

    /**
     *  Ensure we don't end up doing division by zero.
     */
    if (scale == 0.0) scale = 0.01;

    scale = std::max(scale, Rule->MinProductionSpeed);

    time /= scale;

    /**
     *  Calculate the bonus based on the current factory count.
     */
    int divisor = Extension::Fetch(This()->House)->Factory_Count(This()->RTTI, TechnoTypeClassExtension::Get_Production_Flags(This())) - 1;

    /**
     *  #issue-106
     * 
     *  "MultipleFactory" calculation back ported from Red Alert 2.
     * 
     *  @author: CCHyper
     */
    if (Rule->MultipleFactory > 0.0 && divisor > 0) {

        /**
         *  #issue-659
         * 
         *  Implements MultipleFactoryCap for RulesClass.
         * 
         *  @author: CCHyper
         */
        if (RuleExtension->MultipleFactoryCap > 0) {
            if (divisor > RuleExtension->MultipleFactoryCap - 1) {
                divisor = RuleExtension->MultipleFactoryCap - 1;
            }
        }

        while (divisor) {
            time *= Rule->MultipleFactory;
            divisor--;
        }
    }

    /**
     *  Walls have a coefficient as they are really cheap.
     */
    if (This()->RTTI == RTTI_BUILDING && reinterpret_cast<const BuildingTypeClass *>(This()->TClass)->IsWall) {
        time *= Rule->WallBuildSpeedCoefficient;
    }

    return time;
}


/**
 *  Can this unit opportunity fire?
 *
 *  @author: ZivDero
 */
bool TechnoClassExtension::Can_Opportunity_Fire() const
{
    if (This()->TarCom != nullptr && !This()->House->Is_Human_Player() && This()->Is_Foot()) {
        FootClass* foot = static_cast<FootClass*>(This());
        if (foot->Team != nullptr && !foot->Team->Class->IsSuicide && foot->Team->Class->IsAggressive && foot->CurrentMission == MISSION_MOVE) {
            return true;
        }
    }

    if (!Can_Passive_Acquire()) {
        return false;
    }

    if (Techno_Type_Class_Ext()->IsOpportunityFire) {
        return true;
    }

    return false;
}


/**
 *  Perform opportunity fire.
 *
 *  @author: ZivDero
 */
bool TechnoClassExtension::Opportunity_Fire()
{
    if (Can_Opportunity_Fire() && (This()->TarCom == nullptr || HasOpportunityFireTarget)) {
        AbstractClass* old_target = This()->TarCom;
        bool result = This()->Target_Something_Nearby(This()->Center_Coord(), THREAT_RANGE);
        if (result && This()->TarCom != old_target) {
            HasOpportunityFireTarget = true;
        }
        return result;
    }

    return false;
}


/**
 *  Determines the coordinate where bullets appear.
 *  Contains an additional argument to add an offset to the firing coordinate,
 *  used by the spawn manager.
 *
 *  @author: ZivDero
 */
Coord TechnoClassExtension::Fire_Coord(WeaponSlotType which, TPoint3D<int> offset) const
{
    const TechnoTypeClass *ttype = This()->TClass;
    const auto weaponinfo = This()->Get_Weapon(which);

    Matrix3D matrix;
    matrix.Make_Identity();

    float theta = This()->Turret_Facing().Get_Radian<32>();
    matrix.Rotate_Z(theta);

    const TPoint3D<int> flh = weaponinfo->FireFLH + offset;

    const float trans_x = static_cast<float>(flh.X + ttype->TurretOffset);
    const float trans_y = static_cast<float>(flh.Y * (This()->BurstIndex % 2 == 0 ? 1 : -1));
    const float trans_z = static_cast<float>(flh.Z + weaponinfo->BarrelThickness);
    matrix.Translate(trans_x, trans_y, trans_z);

    theta = -This()->BarrelFacing.Current().Get_Radian<32>();
    matrix.Rotate_Y(theta);

    matrix.Translate(static_cast<float>(weaponinfo->BarrelLength), 0, 0);

    const Vector3 fire_coord = matrix * Vector3(0, 0, 0);
    Coord render_coord = This()->Render_Coord();

    return { render_coord.X + static_cast<int>(fire_coord.X), render_coord.Y - static_cast<int>(fire_coord.Y), render_coord.Z + static_cast<int>(fire_coord.Z) };
}


/**
 *  Applies Iron Curtain to the unit. Can optionally skip legality checks.
 *
 *  @author: Rampastring
 */
bool TechnoClassExtension::Iron_Curtain_Me(bool forced)
{
    if (!forced) {
        HouseClassExtension* houseext = Extension::Fetch(This()->House);

        if (!houseext->Can_Use_Iron_Curtain()) {
            return false;
        }
    }

    IronCurtainTimer = RuleExtension->IronCurtainDuration;
    Static_Sound(RuleExtension->IronCurtainSound, This()->Center_Coord());
    return true;
}


/**
 *  Puts pointers to the storage extension into the storage class.
 *
 *  @author: ZivDero
 */
void TechnoClassExtension::Put_Storage_Pointers()
{
    new (reinterpret_cast<StorageClassExt*>(&This()->Storage)) StorageClassExt(&Storage);
}


/**
 *  Provides access to the TechnoTypeClass instance for this extension. 
 * 
 *  @author: CCHyper
 */
const TechnoTypeClass *TechnoClassExtension::Techno_Type_Class() const
{
    return reinterpret_cast<TechnoClass *>(This())->TClass;
}


/**
 *  Provides access to the TechnoTypeClass extension instance for this extension.
 *
 *  @author: CCHyper
 */
const TechnoTypeClassExtension *TechnoClassExtension::Techno_Type_Class_Ext() const
{
    return Extension::Fetch(Techno_Type_Class());
}
