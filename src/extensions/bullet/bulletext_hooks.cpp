/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Contains the hooks for the extended BulletClass.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 *  Copyright 2025 Electronic Arts Inc.
 *  Copyright 2026 OpenTS contributors
 *
 *  BulletClass::AI is adapted from OpenTS, with DTA modifications.
 *  See OPENTS-LICENSE.md for the upstream license, additional terms and
 *  supplemental warranty disclaimers applicable to that material.
 ******************************************************************************/

#include "always.h"

#include "bulletext_hooks.h"

#include "anim.h"
#include "asserthandler.h"
#include "building.h"
#include "bullet.h"
#include "bullettype.h"
#include "bullettypeext.h"
#include "extension.h"
#include "fastmath.h"
#include "hooker.h"
#include "house.h"
#include "infantry.h"
#include "iomap.h"
#include "matrix3d.h"
#include "tibsun_functions.h"
#include "overlaytype.h"
#include "rules.h"
#include "syringe.h"
#include "techno.h"
#include "warheadtype.h"
#include "warheadtypeext.h"

#include <algorithm>
#include <cmath>


/**
 *  A fake class for implementing new member functions which allow
 *  access to the "this" pointer of the intended class.
 *
 *  @note: This must not contain a constructor or destructor!
 *  @note: All functions must be prefixed with "_" to prevent accidental virtualization.
 */
static DECLARE_EXTENDING_CLASS_AND_PAIR(BulletClass)
{
public:
    bool _Is_Forced_To_Explode(Coord& coord);
    void _AI();
    int _Shape_Number(void);
};


/**
 *  #issue-444
 *
 *  Full replacement of BulletClass::Is_Forced_To_Explode.
 *
 *  @author: 10/10/1996 JLB : Created.
 *           22/10/2024 Rampastring : Adjustments for Tiberian Sun.
 */
bool BulletClassExt::_Is_Forced_To_Explode(Coord& coord)
{
    coord = Position;
    CellClass* cellptr = &Map[PositionCoord];
    int height = HeightAGL;

    /*
    **  Check for impact on a wall or other high obstacle.
    */
    if (!Class->IsHigh && cellptr->Overlay != OVERLAY_NONE && OverlayTypes[cellptr->Overlay]->IsHigh && height < 100) {
        return true;
    }

    /*
    **  Check for impact on the ground.
    */
    if (height < 0) {
        return true;
    }

    /*
    **  Check to make sure that underwater projectiles (torpedoes) will not
    **  travel in anything but water.
    */
    const auto bullettypeext = Extension::Fetch(Class);
    if (bullettypeext->IsTorpedo) {
        int distance = ::Distance(Coord_Fraction(coord), Coord(CELL_LEPTON_W / 2, CELL_LEPTON_W / 2));

        if (cellptr->Land_Type() != LAND_WATER ||
            (distance < CELL_LEPTON_W / 3 && cellptr->Cell_Techno() != nullptr &&
            (Payback == nullptr || !Payback->House->Is_Ally(cellptr->Cell_Techno())))) {

            /*
            **  If the torpedo was blocked by a bridge, then force the
            **  torpedo to explode on top of that bridge cell.
            */
            if (cellptr->Is_Bridge_Here()) {
                coord = Coord_Snap(coord);
            }

            return true;
        }

        /*
        **  Torpedoes can be blocked by enemy objects on their path.
        */
        TechnoClass* celltechno = cellptr->Cell_Techno();

        if (celltechno != nullptr)
        {
            int snapdistance = CELL_LEPTON_W * 2;

            if (celltechno == TarCom || 
                (Distance(celltechno) < snapdistance && (Payback == nullptr || !Payback->House->Is_Ally(celltechno))))
            {
                /*
                **  If the techno is not a building, force
                **  explosion to be at center of techno object.
                **  Otherwise, explode in the center of the cell.
                */
                if (celltechno->Fetch_RTTI() != RTTI_BUILDING) {
                    coord = cellptr->Cell_Techno()->Target_Coord();
                }
                else {
                    coord = cellptr->Center_Coord();
                }

                return true;
            }
        }
    }

    /*
    **  Bullets are generally more effective when they are fired at flying objects.
    */
    if (Class->IsAntiAircraft && TarCom != nullptr && TarCom->In_Air() && Distance(TarCom) < 0x0080) {
        return true;
    }

    return false;
}


/**
 *  #issue-415
 *
 *  Implements screen shake values for WarheadTypes.
 *
 *  @author: CCHyper
 */
DEFINE_HOOK(0x00446652, _BulletClass_Logic_ShakeScreen_Patch, 0)
{
    GET(WarheadTypeClass *, warhead, EAX);
    GET_STACK(Coord*, coord, 0x0A8);

    R->EDI(coord);

    /**
     *  Fetch the extension instance.
     */
    auto warheadext = Extension::Fetch(warhead);

    /**
     *  If this warhead has screen shake values defined, then set the blitter
     *  offset values. GScreenClass::Blit will handle the rest for us.
     */
    if (warheadext->ShakePixelXLo > 0 || warheadext->ShakePixelXHi > 0) {
        Map.ScreenX = Sim_Random_Pick(warheadext->ShakePixelXLo, warheadext->ShakePixelXHi);
    }
    if (warheadext->ShakePixelYLo > 0 || warheadext->ShakePixelYHi > 0) {
        Map.ScreenY = Sim_Random_Pick(warheadext->ShakePixelYLo, warheadext->ShakePixelYHi);
    }

    /**
     *  Jumps back to IsEMEffect check.
     */
    return 0x00446659;
}


/**
 *  Full replacement of BulletClass::AI, adapted from OpenTS code/bullet.cpp.
 *  DTA retains missiles after target loss, changes degeneration, and applies
 *  SnapDistance to non-airburst homing detonations.
 */
void BulletClassExt::_AI()
{
    static const int closure_limit = 4 * TICKS_PER_SECOND;

    // Names recovered by OpenTS for fields still unnamed in TS++.
    int& bounce_count = field_A0;
    bool& is_launching = field_A45;
    int& closure_samples = field_B0;
    double& smoothed_closure = field_B8;

    ObjectClass::AI();
    if (!IsActive) return;

    const auto bullettypeext = Extension::Fetch(Class);
    const bool homing = Class->ROT > 0;
    const bool lost_target = homing && TarCom == nullptr;
    bool forced = Class->IsDropping && !IsFalling;
    bool collided = false;

    if (Class->AnimLow || Class->AnimHigh) {
        if (--AnimFrameDelay == 0) {
            AnimFrameDelay = Class->AnimRate;
            if (++AnimFrame > Class->AnimHigh) {
                AnimFrame = Class->AnimLow;
            }
        }
    }

    const Coord previous_coord = PositionCoord;
    Coord coord = previous_coord;
    if (Class->Trailer != nullptr && bullettypeext->SpawnDelay > 0
        && Frame % bullettypeext->SpawnDelay == 0) {
        new AnimClass(Class->Trailer, coord, 1, 1);
    }

    ImpactType impact = IMPACT_NONE;
    if (homing) {
        double speed = Fly.Length_3D();
        if (MaxSpeed >= 40 || speed + 0.5 >= MaxSpeed) {
            is_launching = false;
        }

        const int acceleration = is_launching ? ((Frame % 2 == 0) ? 1 : 0) : Class->Acceleration;
        if (speed < MaxSpeed || speed > MaxSpeed) {
            if (speed < MaxSpeed) {
                speed = std::min(speed + acceleration, static_cast<double>(MaxSpeed));
            } else {
                speed = std::max(speed - acceleration / 2, 0.0);
            }

            // Equivalent to TVelocity3D::Set_Speed; normalize only after the
            // zero-vector check, so a projectile launched at rest can accelerate.
            Fly.If_XYZ_0_Set_X_100();
            const double scalar = speed / Fly.Length_3D();
            Fly.field_88 *= scalar;
            Fly.field_90 *= scalar;
            Fly.field_98 *= scalar;
        }

        const Coord old_coord = coord;
        if (lost_target) {
            // #issue-19: no autopilot, proximity fuse, or closure detector after
            // target loss. Preserve the flight direction, including its pitch.
            coord += Coord(static_cast<int>(Fly.field_88), static_cast<int>(Fly.field_90), static_cast<int>(Fly.field_98));
            if (coord.Z <= Map.Get_Height_GL(coord)) {
                forced = true;
                impact = IMPACT_NORMAL;
            }
        } else {
            // entry_5C is AbstractClass::As_Coord in OpenTS. Unlike
            // Center_Coord, a cell's implementation includes its bridge deck.
            Coord target_coord = TarCom->entry_5C();
            if (const auto target = dynamic_cast<ObjectClass*>(TarCom)) {
                target_coord = target->Target_Coord();
            }

            const double phase = ((Frame + Fetch_ID()) % TICKS_PER_SECOND) * (1.0 / TICKS_PER_SECOND);
            const double full_circle = 6.283185307179586;
            int rot = static_cast<int>((FastMath::Sin(phase * full_circle) * Rule->MissileROTVar
                + (Rule->MissileROTVar + 1.0)) * Class->ROT);
            if (::Distance(Center_Coord(), target_coord) < CELL_LEPTON_W) {
                rot = static_cast<int>(rot * 1.5);
            }

            // ROT is a 256-direction value, not a fixed-point number or raw
            // 16-bit direction. This also preserves the original launch phase.
            DirType turn_rate(static_cast<Dir256>(is_launching ? 0 : rot));
            TVelocity3D velocity = Fly;
            // Native FastMath::Atan2 overflows its table index when Y/X is
            // enormous. Cos(pi/2) leaves a tiny X residue, so northbound
            // missiles can abruptly turn east with DTA's 24-bit FPU precision.
            // Treat this sub-resolution heading as exactly axial, while still
            // accepting the autopilot's subsequent steering and pitch changes.
            if (std::abs(velocity.field_88) <= std::abs(velocity.field_90) * 1e-6) {
                velocity.field_88 = 0.0;
            }
            const int distance = Projectile_Motion(coord, velocity, target_coord, turn_rate,
                TarCom->RTTI == RTTI_AIRCRAFT, Class->IsAirburst, Class->IsVeryHigh);
            Fly = velocity;

            if (distance <= Fly.Length_3D() * 0.5 || HeightAGL <= 0) {
                forced = true;
                impact = IMPACT_NORMAL;
                // Snapping is handled below, after every explosion condition,
                // so arrival cannot bypass SnapDistance or distort fuel usage.
            }

            const int delta = ::Distance(old_coord, target_coord) - ::Distance(coord, target_coord);
            if (!is_launching) {
                if (closure_samples < closure_limit) {
                    ++closure_samples;
                    smoothed_closure += delta;
                } else {
                    smoothed_closure = smoothed_closure * ((closure_limit - 1.0) / closure_limit) + delta;
                    if (smoothed_closure >= 0 && smoothed_closure < closure_limit
                        && !Class->IsAirburst && !Class->IsVeryHigh) {
                        forced = true;
                        impact = IMPACT_NORMAL;
                    }
                }
            }
        }

        // Both guided and unguided missiles can strike either side of a bridge.
        if (impact == IMPACT_NONE && (Map[coord].IsUnderBridge || Map[old_coord].IsUnderBridge)) {
            const int bridge_height = Map.Get_Height_GL(coord) + BRIDGE_LEPTON_HEIGHT;
            if ((coord.Z > bridge_height && old_coord.Z < bridge_height)
                || (coord.Z < bridge_height && old_coord.Z > bridge_height)) {
                coord.Z = bridge_height;
                forced = true;
                impact = IMPACT_NORMAL;
            }
        }

        // Targetless missiles can now travel past their original destination.
        if (!Map.In_Radar(coord)) {
            impact = IMPACT_EDGE;
        }
    } else {
        TVelocity3D velocity = Fly;
        if (velocity.Length_3D() < 8) {
            impact = IMPACT_NORMAL;
        }

        // Floaters use half gravity; retain double precision until the final
        // coordinate conversion, as in the original ballistic flight model.
        velocity.field_98 -= Class->IsFloater ? Rule->Gravity * 0.5 : Rule->Gravity;
        TVelocity3D position = {
            coord.X + velocity.field_88,
            coord.Y + velocity.field_90,
            coord.Z + velocity.field_98
        };
        const Coord old_coord = coord;
        const Coord new_coord(static_cast<int>(position.field_88), static_cast<int>(position.field_90), static_cast<int>(position.field_98));
        const int height = Map.Get_Height_GL(new_coord);
        const int bridge_height = height + BRIDGE_LEPTON_HEIGHT;
        CellClass& cell = Map[new_coord];

        bool fell_through_bridge = false;
        bool rose_through_bridge = false;
        if (cell.IsUnderBridge || Map[old_coord].IsUnderBridge) {
            if (new_coord.Z >= bridge_height) {
                rose_through_bridge = old_coord.Z < bridge_height;
            } else {
                fell_through_bridge = old_coord.Z >= bridge_height;
            }
        }

        bool hit_obstacle = false;
        if (!fell_through_bridge && !rose_through_bridge && position.field_98 >= height
            && position.field_98 - 150 < height) {
            BuildingClass* building = cell.Cell_Building();
            const bool has_wall = cell.Overlay != OVERLAY_NONE && OverlayTypes[cell.Overlay]->IsWall;
            if (building != nullptr || has_wall) {
                hit_obstacle = true;
                if (building != nullptr && (building == Payback
                    || (building->Class->IsLaserFence && building->LaserFenceFrame >= 8)
                    || building->Considered_Vehicle()
                    || (Payback != nullptr && Payback->House->Is_Ally(building)))) {
                    hit_obstacle = false;
                }
            }
        }

        if (position.field_98 < height || fell_through_bridge || rose_through_bridge || hit_obstacle) {
            if (fell_through_bridge) {
                position.field_98 = bridge_height;
            } else if (rose_through_bridge) {
                position.field_98 = bridge_height - 20;
            } else if (height - 100 < position.field_98) {
                position.field_98 = height;
            }

            // Reflect in the ramp's local coordinate system, then transform
            // back into world space. TS uses an inverted Y for these matrices.
            Matrix3D slope = Get_Voxel_Ramp_Matrix(static_cast<TileRampType>(cell.Ramp));
            Vector3 bounced(static_cast<float>(velocity.field_88), static_cast<float>(-velocity.field_90), static_cast<float>(velocity.field_98));
            Matrix3D::Inverse_Rotate_Vector(slope, bounced, &bounced);
            bounced *= static_cast<float>(Class->Elasticity);
            bounced.Z = -bounced.Z;
            bounced = slope.Rotate_Vector(bounced);
            velocity.field_88 = bounced.X;
            velocity.field_90 = -bounced.Y;
            velocity.field_98 = bounced.Z;

            const bool on_bridge = Map[coord].IsUnderBridge
                && Map.Get_Height_GL(coord) + BRIDGE_LEPTON_HEIGHT <= position.field_98;
            TechnoClass* techno = Map[coord].Cell_Techno(Point2D(0, 0), on_bridge);
            if (Payback != nullptr && (coord.As_Cell() == Payback->Center_Coord().As_Cell()
                || (techno != nullptr && Payback->House->Is_Ally(techno)))) {
                techno = nullptr;
            }
            if (!Class->IsBouncy || (techno != nullptr && (Payback == nullptr || techno != Payback))) {
                impact = IMPACT_NORMAL;
                forced = true;
                collided = true;
            }
            if (++bounce_count >= 3 && !forced) {
                impact = IMPACT_NORMAL;
                forced = true;
            }
        }

        coord = Coord(static_cast<int>(position.field_88), static_cast<int>(position.field_90), static_cast<int>(position.field_98));
        if (!forced) {
            TechnoClass* techno = Map[coord].Cell_Techno();
            if (techno != nullptr && techno != Payback
                && (Payback == nullptr || !Payback->House->Is_Ally(techno))
                && ::Distance(coord, techno->PositionCoord) < CELL_LEPTON_W / 2) {
                forced = true;
                impact = IMPACT_NORMAL;
                coord = techno->PositionCoord;
            }
        }

        if (!Map.In_Radar(coord)) {
            coord = PositionCoord;
            impact = IMPACT_EDGE;
        }
        Fly = velocity;
        if (Fly.Length_3D() < 10 && HeightAGL < 10) {
            forced = true;
            impact = IMPACT_NORMAL;
        }
    }

    Mark();
    if (impact == IMPACT_EDGE) {
        delete this;
        return;
    }

    if (Class->IsFueled) {
        Range -= ::Distance(coord, PositionCoord);
        if (Range <= 0) {
            forced = true;
        }
    }
    PositionCoord = coord;

    CellClass& cell = Map[coord];
    BuildingClass* building = cell.Cell_Building();
    if (building != nullptr && building->Class->IsFirestormWall && building->House->IsFirestormActive
        && (Payback == nullptr || building->House != Payback->House)
        && Class_Of() != nullptr && !Class_Of()->IsIgnoresFirestorm) {
        building->Crossing_Firestorm(this, false);
        delete this;
        return;
    }

    if (!forced) {
        Coord impact_coord = PositionCoord;
        forced = Is_Forced_To_Explode(impact_coord);
        PositionCoord = impact_coord;
    }

    if (lost_target && !forced) {
        // Preserve DTA's interception by enemy cell occupiers, using the bridge
        // occupation list when the missile is above a bridge deck.
        HouseClass* house = Payback != nullptr ? Payback->House : nullptr;
        const bool on_bridge = cell.IsUnderBridge && coord.Z >= Map.Get_Height_GL(coord) + BRIDGE_LEPTON_HEIGHT;
        for (ObjectClass* occupier = cell.Cell_Occupier(on_bridge); occupier != nullptr; occupier = occupier->Next) {
            if (occupier != Payback && occupier->Owner_HouseClass() != house
                && (house == nullptr || !house->Is_Ally(occupier))) {
                forced = true;
                break;
            }
        }
    }

    // The fuse remembers the original target coordinate. It must not kill a
    // missile continuing past that coordinate after the target has disappeared.
    FuseResultType fuse = !lost_target && Homes_In() ? Fuse.Fuse_Checkup(coord) : FUSE_DONT_IGNITE;

    // An optional fuse against the current target, rather than the coordinate
    // captured when the native fuse was armed. Physical impacts take precedence;
    // arming delay, airbursts, dropping projectiles and target loss are respected.
    const int proximity_radius = bullettypeext->ProximityFuseMaxTriggerDistance;
    if (!forced && homing && TarCom != nullptr && !Class->IsAirburst && !Class->IsDropping
        && proximity_radius > 0 && Fuse.Is_Armed()) {
        Coord target_coord = TarCom->entry_5C();
        if (const auto target = dynamic_cast<ObjectClass*>(TarCom)) {
            target_coord = target->Target_Coord();
        }

        // Test the entire frame's flight segment, so even a fast projectile
        // whose endpoints are both outside the radius can register a near miss.
        const double dx = static_cast<double>(coord.X) - previous_coord.X;
        const double dy = static_cast<double>(coord.Y) - previous_coord.Y;
        const double dz = static_cast<double>(coord.Z) - previous_coord.Z;
        const double tx = static_cast<double>(target_coord.X) - previous_coord.X;
        const double ty = static_cast<double>(target_coord.Y) - previous_coord.Y;
        const double tz = static_cast<double>(target_coord.Z) - previous_coord.Z;
        const double length_squared = dx * dx + dy * dy + dz * dz;
        const double fraction = length_squared > 0
            ? std::clamp((tx * dx + ty * dy + tz * dz) / length_squared, 0.0, 1.0) : 0.0;
        const double miss_x = tx - dx * fraction;
        const double miss_y = ty - dy * fraction;
        const double miss_z = tz - dz * fraction;
        if (miss_x * miss_x + miss_y * miss_y + miss_z * miss_z
            <= static_cast<double>(proximity_radius) * proximity_radius) {
            fuse = FUSE_IGNITE;
            PositionCoord = Coord(static_cast<int>(previous_coord.X + dx * fraction),
                static_cast<int>(previous_coord.Y + dy * fraction),
                static_cast<int>(previous_coord.Z + dz * fraction));
            // The common detonation code below still applies SnapDistance.
        }
    }

    if (!forced && (Class->IsDropping || fuse == FUSE_DONT_IGNITE)) {
        // #issue-234: lose two strength per tick, with a true floor of ten
        // (odd strengths must not fall to nine).
        if (Class->IsDegenerate && Strength > 10) {
            Strength = std::max(10, Strength - 2);
        }
        return;
    }

    // #issue-19: homing detonation paths share the same distance check. Use the
    // same aim point as the homing code, including an object's target height.
    // Airbursts must remain overhead; non-positive SnapDistance disables this
    // additional snapping (Bullet_Explodes retains its native impact adjustments).
    if (homing && TarCom != nullptr && !Class->IsAirburst && bullettypeext->SnapDistance > 0) {
        Coord target_coord = TarCom->entry_5C();
        if (const auto target = dynamic_cast<ObjectClass*>(TarCom)) {
            target_coord = target->Target_Coord();
        }
        if (::Distance(PositionCoord, target_coord) <= bullettypeext->SnapDistance) {
            PositionCoord = target_coord;
        }
    } else if (!homing && collided && TarCom != nullptr && !Class->IsAirburst) {
        // Preserve vanilla ballistic collision snapping. Applying SnapDistance
        // to every ballistic detonation would undo launch-time scatter, notably
        // when an artillery shell bounces to a stop near its intended target.
        Coord midpoint = coord;
        const Coord target_coord = TarCom->entry_5C();
        midpoint.Z = (midpoint.Z + target_coord.Z) / 2;
        const int target_distance = ::Distance(midpoint, target_coord) / 3;
        if (target_distance <= std::max(CELL_LEPTON_W / 2.0, Fly.Length_3D() * 2)) {
            PositionCoord = TarCom->Center_Coord();
        }
    }
    Bullet_Explodes(forced);
    delete this;
}


// Custom replacement for BulletClass::Shape_Number
int BulletClassExt::_Shape_Number()
{
    static const double halfpi = 1.570796326794897;
    static const double magic = -10430.06004058427;
    static int facing_to_frame_table[32] = { 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 31, 30, 29 };

    int facing = 0;

    if (!Class->IsFaceless)
    {
        // Something in the FP math fails with the synchronous FP mode when X is close to 0, work around it
        if ((int)Fly.field_88 == 0) {
           if (Fly.field_90 > 0) {
                facing = 12;
           }
           else
           {
               facing = 28;
           }
        }
        else {
            float atan = FastMath::Atan2(-this->Fly.field_90, this->Fly.field_88); // note inversed Y
            facing = (int)((atan - halfpi) * magic); // the game's _ftol is not defined in TS++
            facing = facing_to_frame_table[(((facing >> 10) + 1) >> 1) & 0x1F];
        }
    }

    if (Class->AnimLow != 0 || Class->AnimHigh != 0)
    {
        facing = AnimFrame;
    }

    return facing;
}


/**
 *  Main function for patching the hooks.
 */
void BulletClassExtension_Hooks()
{
    Patch_Jump(0x004446F0, &BulletClassExt::_AI);
    Patch_Jump(0x004462C0, &BulletClassExt::_Is_Forced_To_Explode);
    Patch_Jump(0x00445B70, &BulletClassExt::_Shape_Number);
}
