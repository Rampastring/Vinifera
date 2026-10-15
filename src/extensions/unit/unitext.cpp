/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Extended UnitClass class.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#include "always.h"

#include "unitext.h"

#include "building.h"
#include "debughandler.h"
#include "extension.h"
#include "firingsyncscope.h"
#include "unit.h"
#include "unittype.h"
#include "unittypeext.h"
#include "vinifera_saveload.h"
#include "weapontype.h"
#include "wwcrc.h"

#include <algorithm>
#include <climits>


/**
 *  Class constructor.
 *
 *  @author: CCHyper
 */
UnitClassExtension::UnitClassExtension(const UnitClass *this_ptr) :
    FootClassExtension(this_ptr),
    Vinifera::Detach::Listener<BuildingClass>(),
    LastDockedBuilding(nullptr),
    FiringSync(FiringSyncState::Inactive),
    FiringSyncWeapon(nullptr),
    FiringSyncRetryTimer(0),
    FiringSyncReloadTimer(0)
{
    UnitExtensions.Add(this);
}


/**
 *  Class no-init constructor.
 *  
 *  @author: CCHyper
 */
UnitClassExtension::UnitClassExtension(const NoInitClass &noinit) :
    FootClassExtension(noinit),
    Vinifera::Detach::Listener<BuildingClass>(noinit),
    FiringSyncRetryTimer(noinit),
    FiringSyncReloadTimer(noinit)
{
}


/**
 *  Class destructor.
 *  
 *  @author: CCHyper
 */
UnitClassExtension::~UnitClassExtension()
{
    UnitExtensions.Delete(this);
}


/**
 *  Retrieves the class identifier (CLSID) of the object.
 *  
 *  @author: CCHyper
 */
HRESULT UnitClassExtension::GetClassID(CLSID *lpClassID)
{
    if (lpClassID == nullptr) {
        return E_POINTER;
    }

    *lpClassID = __uuidof(this);

    return S_OK;
}


/**
 *  Initializes an object from the stream where it was saved previously.
 *  
 *  @author: CCHyper
 */
HRESULT UnitClassExtension::Load(IStream *pStm)
{
    HRESULT hr = FootClassExtension::Load(pStm);
    if (FAILED(hr)) {
        return E_FAIL;
    }

    new (this) UnitClassExtension(NoInitClass());
    
    VINIFERA_SWIZZLE_REQUEST_POINTER_REMAP(LastDockedBuilding, "LastDockedBuilding");
    VINIFERA_SWIZZLE_REQUEST_POINTER_REMAP(FiringSyncWeapon, "FiringSyncWeapon");

    return hr;
}


/**
 *  Saves an object to the specified stream.
 *  
 *  @author: CCHyper
 */
HRESULT UnitClassExtension::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr = FootClassExtension::Save(pStm, fClearDirty);
    if (FAILED(hr)) {
        return hr;
    }

    return hr;
}


/**
 *  Return the raw size of class data for save/load purposes.
 *  
 *  @author: CCHyper
 */
int UnitClassExtension::Get_Object_Size() const
{
    return sizeof(*this);
}


/**
 *  Removes the specified building from any targeting and reference trackers.
 */
void UnitClassExtension::On_Detach(BuildingClass *target, bool all)
{
    if (LastDockedBuilding == target) {
        LastDockedBuilding = nullptr;
    }
}


/**
 *  Compute a unique crc value for this instance.
 *  
 *  @author: CCHyper
 */
void UnitClassExtension::Object_CRC(CRCEngine &crc) const
{
    crc(LastDockedBuilding != nullptr ? LastDockedBuilding->Fetch_ID() : 0);
    crc(static_cast<int>(FiringSync));
    crc(FiringSyncWeapon != nullptr ? FiringSyncWeapon->Fetch_ID() : -1);
    crc(FiringSyncRetryTimer.Value());
    crc(FiringSyncReloadTimer.Value());
}


/**
 *  Checks whether this unit uses the firing sync logic with a two-burst
 *  primary weapon, and has our custom firing sync retry logic enabled.
 *  Vehicles with a secondary weapon retain the legacy sequence, which can
 *  synchronize primary shots with an animation started by the secondary.
 *
 *  @author: Rampastring
 */
bool UnitClassExtension::Uses_Firing_Sync_Retry(const WeaponTypeClass* weapon) const
{
    const UnitClass* unit = This_Const();
    const UnitTypeClass* type = unit->Class;
    const int last_frame = 2 * type->FiringFrames - 1;
    return Extension::Fetch(type)->IsFiringSyncRetry
        && weapon != nullptr && weapon == unit->Get_Weapon(WEAPON_SLOT_PRIMARY)->Weapon
        && unit->Get_Weapon(WEAPON_SLOT_SECONDARY)->Weapon == nullptr
        && (FiringSync != FiringSyncState::Inactive || unit->BurstIndex == 0)
        && weapon->Burst == 2 && type->FiringFrames > 0
        && type->FiringSyncFrame[0] <= last_frame
        && type->FiringSyncFrame[0] > type->FiringSyncFrame[1]
        && type->FiringSyncFrame[1] >= 0;
}


/**
 *  Prepares the firing sync logic for firing the first shot of the sequence.
 *
 *  @author: Rampastring
 */
void UnitClassExtension::Begin_Firing_Sync(const WeaponTypeClass* weapon)
{
    UnitClass* unit = This();
    FiringSync = FiringSyncState::WaitingForFirstShot;
    FiringSyncWeapon = weapon;
    FiringSyncReloadTimer = 0;
    IsToResetBurst = false;
    BurstResetTimer = 0;
    unit->BurstIndex = 0;
    unit->FiringSyncDelay = 2 * unit->Class->FiringFrames - 1;
    FiringSyncRetryTimer = unit->FiringSyncDelay - unit->Class->FiringSyncFrame[0]
        + 2 * unit->Class->FiringFrames;
}


/**
 *  Per-frame update of the firing sync logic. Always run for a unit placed on the map.
 *
 *  UnitClass::AI has already decremented the animation countdown, so we can restore a pending shot's frame when necessary.
 *  Its independent timer bounds the hold and continues through target changes.
 */
void UnitClassExtension::Update_Firing_Sync()
{
    if (FiringSync == FiringSyncState::Inactive || FiringSyncShotScope::Is_Active(this)) {
        return;
    }

    UnitClass* unit = This();
    if (FiringSync == FiringSyncState::Reloading) {
        const int remaining = FiringSyncReloadTimer.Value();
        if (unit->Arm.Value() < remaining) {
            unit->Arm = remaining;
        }
        if (unit->Arm == 0) {
            unit->BurstIndex = 0;
            unit->FiringSyncDelay = -1;
            FiringSync = FiringSyncState::Inactive;
            FiringSyncReloadTimer = 0;
        }
        return;
    }

    if (unit->TarCom == nullptr || !Uses_Firing_Sync_Retry(FiringSyncWeapon))
    {
        Cancel_Firing_Sync();
        return;
    }

    if (FiringSync == FiringSyncState::FinishingAnimation) {
        if (unit->FiringSyncDelay < 0) {
            FiringSync = FiringSyncState::Inactive;
            FiringSyncWeapon = nullptr;
        }
        return;
    }

    if (FiringSyncRetryTimer == 0) {
        Cancel_Firing_Sync();
        return;
    }

    const int shot = FiringSync == FiringSyncState::WaitingForFirstShot ? 0 : 1;
    if (unit->BurstIndex != shot) {
        Cancel_Firing_Sync();
        return;
    }
    const int frame = unit->Class->FiringSyncFrame[shot];
    if (unit->FiringSyncDelay < frame) {
        unit->FiringSyncDelay = frame;
    }
}


/**
 *  Records a shot performed under the firing sync logic.
 *
 *  Called after Fire_At has advanced BurstIndex.
 */
void UnitClassExtension::Record_Firing_Sync_Shot(const WeaponTypeClass* weapon)
{
    UnitClass* unit = This();
    if (FiringSync == FiringSyncState::WaitingForFirstShot) {
        int reload;
        if (weapon->IsSonic
            || (weapon->IsUseSparkParticles && unit->ParticleSystems[ATTACHED_PARTICLE_SPARK])
            || (weapon->IsUseFireParticles && unit->ParticleSystems[ATTACHED_PARTICLE_FIRE])
            || (weapon->IsRailgun && unit->ParticleSystems[ATTACHED_PARTICLE_RAILGUN])) {
            // The native particle branch has already assigned the full ROF.
            reload = unit->Arm.Value();
        } else {
            const int old_burst = unit->BurstIndex;
            unit->BurstIndex = INT_MAX;
            reload = unit->Rearm_Delay(WEAPON_SLOT_PRIMARY);
            unit->BurstIndex = old_burst;
        }
        // Keep a full ROF deadline from the first shot so abandoning the burst cannot skip reloading.
        FiringSyncReloadTimer = std::max(0, reload);
        FiringSync = FiringSyncState::WaitingForSecondShot;
        FiringSyncRetryTimer = std::max(0, unit->FiringSyncDelay - unit->Class->FiringSyncFrame[1])
            + 2 * unit->Class->FiringFrames;
    } else if (FiringSync == FiringSyncState::WaitingForSecondShot) {
        FiringSync = FiringSyncState::FinishingAnimation;
        FiringSyncRetryTimer = 0;
        FiringSyncReloadTimer = 0;
    }
}


bool UnitClassExtension::Cancel_Firing_Sync()
{
    if (FiringSync == FiringSyncState::Inactive) {
        return false;
    }

    if (FiringSyncShotScope::Is_Active(this)) {
        // Native Fire_At still owns BurstIndex until the call returns.
        FiringSync = FiringSyncState::Reloading;
        return true;
    }

    UnitClass* unit = This();
    if (FiringSync != FiringSyncState::Reloading) {
        const bool fired = FiringSync != FiringSyncState::WaitingForFirstShot;
        FiringSync = fired ? FiringSyncState::Reloading : FiringSyncState::Inactive;
    }
    const int remaining = FiringSyncReloadTimer.Value();
    if (unit->Arm.Value() < remaining) {
        unit->Arm = remaining;
    }
    unit->BurstIndex = 0;
    unit->FiringSyncDelay = -1;
    IsToResetBurst = false;
    BurstResetTimer = 0;
    FiringSyncWeapon = nullptr;
    FiringSyncRetryTimer = 0;
    return true;
}
