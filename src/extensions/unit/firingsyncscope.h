/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Scoped context for native synchronized firing calls.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#pragma once

class UnitClassExtension;


/**
 *  Identifies units whose Fire_At call owns BurstIndex and Arm.
 *  Stack-owned scopes keep callbacks for unrelated units independent and retain
 *  every active unit through nested calls.
 */
class FiringSyncShotScope final
{
    public:
        explicit FiringSyncShotScope(const UnitClassExtension* unit) :
            Unit(unit),
            Previous(Active)
        {
            if (Unit != nullptr) {
                Active = this;
            }
        }

        ~FiringSyncShotScope()
        {
            Active = Previous;
        }

        FiringSyncShotScope(const FiringSyncShotScope&) = delete;
        FiringSyncShotScope& operator=(const FiringSyncShotScope&) = delete;

        static bool Is_Active(const UnitClassExtension* unit)
        {
            for (const auto* scope = Active; scope != nullptr; scope = scope->Previous) {
                if (scope->Unit == unit) {
                    return true;
                }
            }
            return false;
        }

    private:
        inline static const FiringSyncShotScope* Active = nullptr;
        const UnitClassExtension* Unit;
        const FiringSyncShotScope* Previous;
};
