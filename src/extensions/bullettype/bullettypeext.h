/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Extended BulletTypeClass class.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#pragma once

#include "bullettype.h"
#include "objecttypeext.h"


class DECLSPEC_UUID(UUID_BULLETTYPE_EXTENSION)
BulletTypeClassExtension final : public ObjectTypeClassExtension
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
        BulletTypeClassExtension(const BulletTypeClass *this_ptr = nullptr);
        BulletTypeClassExtension(const NoInitClass &noinit);
        virtual ~BulletTypeClassExtension();

        virtual int Get_Object_Size() const override;
        virtual void Object_CRC(CRCEngine &crc) const override;
        
        virtual BulletTypeClass *This() const override { return reinterpret_cast<BulletTypeClass *>(ObjectTypeClassExtension::This()); }
        virtual const BulletTypeClass *This_Const() const override { return reinterpret_cast<const BulletTypeClass *>(ObjectTypeClassExtension::This_Const()); }
        virtual RTTIType Fetch_RTTI() const override { return RTTI_BULLETTYPE; }

        virtual bool Read_INI(CCINIClass &ini) override;

    public:
        /**
         *  The number of frames between trailer anim spawns.
         */
        unsigned SpawnDelay;

        /**
         *  If set, this projectile can only be used against targets on water.
         */
        bool IsTorpedo;

        /**
         *  Maximum 3D distance in leptons for AI detonation snapping to the
         *  target's aim point. Applies to homing forced and fuse detonations,
         *  except airbursts. Ballistic projectiles retain vanilla snapping.
         *  Non-positive values disable this additional homing snapping.
         */
        int SnapDistance;

        /**
         *  Extra proximity fuse radius in 3D leptons around the current target.
         *  Applies to armed homing projectiles, except dropping and airburst
         *  projectiles. Non-positive values disable the extra trigger.
         */
        int ProximityFuseMaxTriggerDistance;

        /**
         *  Use vanilla terrain-following cruise altitude for homing projectiles.
         *  If false, steer directly toward the target at any range instead.
         */
        bool IsArcingHoming;
};
