/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *
 *  @project       Vinifera
 *
 *  @file          VOXELANIMTYPEEXT.CPP
 *
 *  @author        CCHyper
 *
 *  @brief         Extended VoxelAnimTypeClass class.
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
#include "voxelanimtypeext.h"
#include "voxelanimtype.h"
#include "ccini.h"
#include "asserthandler.h"
#include "debughandler.h"


/**
 *  Provides the map for all VoxelAnimTypeClass extension instances.
 */
ExtensionMap<VoxelAnimTypeClass, VoxelAnimTypeClassExtension> VoxelAnimTypeClassExtensions;


/**
 *  Class constructor.
 *  
 *  @author: CCHyper
 */
VoxelAnimTypeClassExtension::VoxelAnimTypeClassExtension(VoxelAnimTypeClass *this_ptr) :
    Extension(this_ptr)
{
    ASSERT(ThisPtr != nullptr);
    //EXT_DEBUG_TRACE("VoxelAnimTypeClassExtension constructor - Name: %s (0x%08X)\n", ThisPtr->Name(), (uintptr_t)(ThisPtr));
    //EXT_DEBUG_WARNING("VoxelAnimTypeClassExtension constructor - Name: %s (0x%08X)\n", ThisPtr->Name(), (uintptr_t)(ThisPtr));

    IsInitialized = true;
}


/**
 *  Class no-init constructor.
 *  
 *  @author: CCHyper
 */
VoxelAnimTypeClassExtension::VoxelAnimTypeClassExtension(const NoInitClass &noinit) :
    Extension(noinit)
{
    IsInitialized = false;
}


/**
 *  Class destructor.
 *  
 *  @author: CCHyper
 */
VoxelAnimTypeClassExtension::~VoxelAnimTypeClassExtension()
{
    //EXT_DEBUG_TRACE("VoxelAnimTypeClassExtension destructor - Name: %s (0x%08X)\n", ThisPtr->Name(), (uintptr_t)(ThisPtr));
    //EXT_DEBUG_WARNING("VoxelAnimTypeClassExtension destructor - Name: %s (0x%08X)\n", ThisPtr->Name(), (uintptr_t)(ThisPtr));

    IsInitialized = false;
}


/**
 *  Initializes an object from the stream where it was saved previously.
 *  
 *  @author: CCHyper
 */
HRESULT VoxelAnimTypeClassExtension::Load(IStream *pStm)
{
    ASSERT(ThisPtr != nullptr);
    //EXT_DEBUG_TRACE("VoxelAnimTypeClassExtension::Size_Of - Name: %s (0x%08X)\n", ThisPtr->Name(), (uintptr_t)(ThisPtr));

    HRESULT hr = Extension::Load(pStm);
    if (FAILED(hr)) {
        return E_FAIL;
    }

    new (this) VoxelAnimTypeClassExtension(NoInitClass());
    
    return hr;
}


/**
 *  Saves an object to the specified stream.
 *  
 *  @author: CCHyper
 */
HRESULT VoxelAnimTypeClassExtension::Save(IStream *pStm, BOOL fClearDirty)
{
    ASSERT(ThisPtr != nullptr);
    //EXT_DEBUG_TRACE("VoxelAnimTypeClassExtension::Size_Of - Name: %s (0x%08X)\n", ThisPtr->Name(), (uintptr_t)(ThisPtr));

    HRESULT hr = Extension::Save(pStm, fClearDirty);
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
int VoxelAnimTypeClassExtension::Size_Of() const
{
    ASSERT(ThisPtr != nullptr);
    //EXT_DEBUG_TRACE("VoxelAnimTypeClassExtension::Size_Of - Name: %s (0x%08X)\n", ThisPtr->Name(), (uintptr_t)(ThisPtr));

    return sizeof(*this);
}


/**
 *  Removes the specified target from any targeting and reference trackers.
 *  
 *  @author: CCHyper
 */
void VoxelAnimTypeClassExtension::Detach(TARGET target, bool all)
{
    ASSERT(ThisPtr != nullptr);
    //EXT_DEBUG_TRACE("VoxelAnimTypeClassExtension::Detach - Name: %s (0x%08X)\n", ThisPtr->Name(), (uintptr_t)(ThisPtr));
}


/**
 *  Compute a unique crc value for this instance.
 *  
 *  @author: CCHyper
 */
void VoxelAnimTypeClassExtension::Compute_CRC(WWCRCEngine &crc) const
{
    ASSERT(ThisPtr != nullptr);
    //EXT_DEBUG_TRACE("VoxelAnimTypeClassExtension::Compute_CRC - Name: %s (0x%08X)\n", ThisPtr->Name(), (uintptr_t)(ThisPtr));
}


/**
 *  Fetches the extension data from the INI database.  
 *  
 *  @author: CCHyper
 */
bool VoxelAnimTypeClassExtension::Read_INI(CCINIClass &ini)
{
    ASSERT(ThisPtr != nullptr);
    //EXT_DEBUG_TRACE("VoxelAnimTypeClassExtension::Read_INI - Name: %s (0x%08X)\n", ThisPtr->Name(), (uintptr_t)(ThisPtr));
    EXT_DEBUG_WARNING("VoxelAnimTypeClassExtension::Read_INI - Name: %s (0x%08X)\n", ThisPtr->Name(), (uintptr_t)(ThisPtr));

    const char *ini_name = ThisPtr->Name();

    if (!ini.Is_Present(ini_name)) {
        return false;
    }
    
    return true;
}
