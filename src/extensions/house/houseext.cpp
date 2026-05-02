/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Extended HouseClass class.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#include "always.h"

#include "houseext.h"

#include "building.h"
#include "buildingtypeext.h"
#include "ccini.h"
#include "debughandler.h"
#include "extension.h"
#include "factory.h"
#include "factoryext.h"
#include "house.h"
#include "mouse.h"
#include "overlaytype.h"
#include "prerequisitegroup.h"
#include "rules.h"
#include "rulesext.h"
#include "saveload.h"
#include "session.h"
#include "battleui.h"
#include "storageext.h"
#include "team.h"
#include "teamext.h"
#include "teamtype.h"
#include "tibsun_functions.h"
#include "unit.h"
#include "unittypeext.h"
#include "utracker.h"
#include "vinifera_globals.h"
#include "vinifera_saveload.h"
#include "voc.h"
#include "vox.h"

#include <algorithm>


/**
 *  Class constructor.
 *
 *  @author: CCHyper
 */
HouseClassExtension::HouseClassExtension(const HouseClass* this_ptr) :
    AbstractClassExtension(this_ptr),
    TiberiumStorage(Tiberiums.Count()),
    WeedStorage(Tiberiums.Count()),
    NavalFactories(0),
    NavalFactory(nullptr),
    BuildNavalUnit(UNIT_NONE),
    SpawnWaypoint(WAYPOINT_NONE),
    IronCurtainAvailabilityTimer(),
    StrengthenDestroyedCost(0),
    NextExpansionPointLocation(0, 0),
    ArchivedExpansionPointLocation(0, 0),
    ShouldBuildRefinery(false),
    HasBuiltFirstBarracks(false),
    LastExcessRefineryCheckFrame(0),
    LastSleepingHarvesterCheckFrame(0),
    HasPerformedVehicleCharge(false),
    IsUnderStartRushThreat(false),
    NextOilRefineryCaptureCheckFrame(1000),
    NextEngineerCheckFrame(0),
    AdvAIGroundTactic(AdvAITacticType::TACTIC_NONE, -1, -1),
    AdvAIAirTactic(AIRTACTIC_NONE, -1, -1),
    AdvAINavalTactic(NAVALTACTIC_NONE, -1, -1),
    AdvAILastTacticExecutionFrame(0),
    EnemyNoneStrength(0),
    EnemyLightStrength(0),
    EnemyHeavyStrength(0),
    EnemyArtilleryStrength(0),
    EnemyBaseDefenseStrength(0),
    EnemyNavalStrength(0),
    EnemyHarvesterCount(0),
    EnemyRefineryCount(0),
    EnemyHasSensors(false),
    LastHarvesterBuildFrame(0),
    LastUnitValueDebugPrintFrame(0),
    LastInfantryValueDebugPrintFrame(0),
    LastAircraftValueDebugPrintFrame(0),
    LastNavalValueDebugPrintFrame(0),
    AttemptedBuildingCaptureCount(0),
    AdvAILastBuiltUnit(UNIT_NONE),
    AdvAILastBuiltUnitCount(0),
    AdvAILastBuiltInfantry(INFANTRY_NONE),
    AdvAILastBuiltInfantryCount(0),
    AdvAILastUndeployableUnitCheckFrame(0),
    AdvAIFunValue(0),
    IsNavalOnly(AdvancedAINavalOnlyState::NOT_CHECKED),
    LastNavalOnlyCheckFrame(-1)
{
    //if (this_ptr) EXT_DEBUG_TRACE("HouseClassExtension::HouseClassExtension - 0x%08X\n", (uintptr_t)(This()));

    for (int i = 0; i < Tiberiums.Count(); i++)
    {
        TiberiumStorage[i] = 0;
        WeedStorage[i] = 0;
    }

    if (this_ptr)
    {
        new ((StorageClassExt*)&this_ptr->Tiberium) StorageClassExt(&TiberiumStorage);
        new ((StorageClassExt*)&this_ptr->Weed) StorageClassExt(&WeedStorage);
    }

    EnemyAntiGroundStrength.Clear();
    EnemyAntiAirStrength.Clear();
    EnemyAntiNavalStrength.Clear();

    memset(FactionSpecificTacticalValues, 0, sizeof(FactionSpecificTacticalValues));
    memset(AdvAILastExecutionFrameForTactic, 0, sizeof(AdvAILastExecutionFrameForTactic));

    HouseExtensions.Add(this);
}


/**
 *  Class no-init constructor.
 *  
 *  @author: CCHyper
 */
HouseClassExtension::HouseClassExtension(const NoInitClass &noinit) :
    AbstractClassExtension(noinit),
    TiberiumStorage(noinit),
    WeedStorage(noinit)
{
    //EXT_DEBUG_TRACE("HouseClassExtension::HouseClassExtension(NoInitClass) - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));
}


/**
 *  Class destructor.
 *  
 *  @author: CCHyper
 */
HouseClassExtension::~HouseClassExtension()
{
    //EXT_DEBUG_TRACE("HouseClassExtension::~HouseClassExtension - 0x%08X\n", (uintptr_t)(This()));

    HouseExtensions.Delete(this);
}


/**
 *  Retrieves the class identifier (CLSID) of the object.
 *  
 *  @author: CCHyper
 */
HRESULT HouseClassExtension::GetClassID(CLSID *lpClassID)
{
    //EXT_DEBUG_TRACE("HouseClassExtension::GetClassID - Name: %s (0x%08X)\n", Name(), (uintptr_t)(This()));

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
HRESULT HouseClassExtension::Load(IStream *pStm)
{
    //EXT_DEBUG_TRACE("HouseClassExtension::Load - 0x%08X\n", (uintptr_t)(This()));

    HRESULT hr = AbstractClassExtension::Internal_Load(pStm);
    if (FAILED(hr)) {
        return E_FAIL;
    }

    Load_Primitive_Vector(pStm, TiberiumStorage, "TiberiumStorage");
    Load_Primitive_Vector(pStm, WeedStorage, "WeedStorage");

    new (this) HouseClassExtension(NoInitClass());

    VINIFERA_SWIZZLE_REQUEST_POINTER_REMAP(NavalFactory, "NavalFactory");
    
    return hr;
}


/**
 *  Saves an object to the specified stream.
 *  
 *  @author: CCHyper
 */
HRESULT HouseClassExtension::Save(IStream *pStm, BOOL fClearDirty)
{
    //EXT_DEBUG_TRACE("HouseClassExtension::Save - 0x%08X\n", (uintptr_t)(This()));

    HRESULT hr = AbstractClassExtension::Internal_Save(pStm, fClearDirty);
    if (FAILED(hr)) {
        return hr;
    }

    Save_Primitive_Vector(pStm, TiberiumStorage, "TiberiumStorage");
    Save_Primitive_Vector(pStm, WeedStorage, "WeedStorage");

    return hr;
}


/**
 *  Return the raw size of class data for save/load purposes.
 *  
 *  @author: CCHyper
 */
int HouseClassExtension::Get_Object_Size() const
{
    //EXT_DEBUG_TRACE("HouseClassExtension::Get_Object_Size - 0x%08X\n", (uintptr_t)(This()));

    return sizeof(*this);
}


/**
 *  Removes the specified target from any targeting and reference trackers.
 *  
 *  @author: CCHyper
 */
void HouseClassExtension::Detach(AbstractClass * target, bool all)
{
    //EXT_DEBUG_TRACE("HouseClassExtension::Detach - 0x%08X\n", (uintptr_t)(This()));

    if (NavalFactory == target) {
        NavalFactory = nullptr;
    }
}


/**
 *  Compute a unique crc value for this instance.
 *  
 *  @author: CCHyper
 */
void HouseClassExtension::Object_CRC(CRCEngine &crc) const
{
    //EXT_DEBUG_TRACE("HouseClassExtension::Object_CRC - 0x%08X\n", (uintptr_t)(This()));

    crc(StrengthenDestroyedCost);
    crc(NextExpansionPointLocation.As_Cell_Number());
    crc(LastExcessRefineryCheckFrame);
    crc(LastSleepingHarvesterCheckFrame);
    crc(IsUnderStartRushThreat);
    crc(NextOilRefineryCaptureCheckFrame);
    crc(NextEngineerCheckFrame);
}


/**
 *  Extended replacement of HouseClass::Fetch_Factory.
 *
 *  @author: ZivDero
 */
FactoryClass* HouseClassExtension::Fetch_Factory(RTTIType rtti, ProductionFlags flags) const
{
    FactoryClass* factory = nullptr;

    switch (rtti) {
    case RTTI_INFANTRY:
    case RTTI_INFANTRYTYPE:
        factory = This()->InfantryFactory;
        break;

    case RTTI_UNIT:
    case RTTI_UNITTYPE:
        if (flags & PRODFLAG_NAVAL) {
            factory = NavalFactory;
        } else {
            factory = This()->UnitFactory;
        }
        break;

    case RTTI_BUILDING:
    case RTTI_BUILDINGTYPE:
        factory = This()->BuildingFactory;
        break;

    case RTTI_AIRCRAFT:
    case RTTI_AIRCRAFTTYPE:
        factory = This()->AircraftFactory;
        break;

    default:
        factory = nullptr;
        break;
    }
    return factory;
}


/**
 *  Extended replacement of HouseClass::Set_Factory.
 *
 *  @author: ZivDero
 */
void HouseClassExtension::Set_Factory(RTTIType rtti, FactoryClass* factory, ProductionFlags flags)
{
    switch (rtti) {
    case RTTI_UNIT:
    case RTTI_UNITTYPE:
        if (flags & PRODFLAG_NAVAL) {
            NavalFactory = factory;
        } else {
            This()->UnitFactory = factory;
        }
        break;

    case RTTI_INFANTRY:
    case RTTI_INFANTRYTYPE:
        This()->InfantryFactory = factory;
        break;

    case RTTI_BUILDING:
    case RTTI_BUILDINGTYPE:
        This()->BuildingFactory = factory;
        break;

    case RTTI_AIRCRAFT:
    case RTTI_AIRCRAFTTYPE:
        This()->AircraftFactory = factory;
        break;
    }
}


/**
 *  Extended replacement of HouseClass::Factory_Counter.
 *
 *  @author: ZivDero
 */
int* HouseClassExtension::Factory_Counter(RTTIType rtti, ProductionFlags flags)
{
    switch (rtti) {
    case RTTI_UNITTYPE:
    case RTTI_UNIT:
        if (flags & PRODFLAG_NAVAL) {
            return &NavalFactories;
        } else {
            return &This()->UnitFactories;
        }

    case RTTI_AIRCRAFTTYPE:
    case RTTI_AIRCRAFT:
        return &This()->AircraftFactories;

    case RTTI_INFANTRYTYPE:
    case RTTI_INFANTRY:
        return &This()->InfantryFactories;

    case RTTI_BUILDINGTYPE:
    case RTTI_BUILDING:
        return &This()->BuildingFactories;

    default:
        break;
    }
    return nullptr;
}


/**
 *  Extended replacement of HouseClass::Factory_Count.
 *
 *  @author: ZivDero
 */
int HouseClassExtension::Factory_Count(RTTIType rtti, ProductionFlags flags) const
{
    int const* ptr = const_cast<HouseClassExtension*>(this)->Factory_Counter(rtti, flags);
    if (ptr != nullptr) {
        return *ptr;
    }
    return 0;
}


/**
 *  Extended replacement of HouseClass::Suspend_Production.
 *
 *  @author: ZivDero
 */
ProdFailType HouseClassExtension::Suspend_Production(RTTIType type, ProductionFlags flags)
{
    FactoryClass* fptr = Fetch_Factory(type, flags);

    /*
    **  If the house is already busy producing the requested object, then
    **  return with this failure code.
    */
    if (fptr == nullptr) return PROD_CANT;

    /*
    **  Actually suspend the production.
    */
    fptr->Suspend();

    return PROD_OK;
}

/**
 *  Replacement of Fetch_Techno_Type that performs bounds checking.
 *
 *  @author: Rampastring
 */
TechnoTypeClass const* _Fetch_Techno_Type(RTTIType type, int id)
{
    switch (type) {
    case RTTI_UNITTYPE:
    case RTTI_UNIT:
        if (id < UnitTypes.Count())
            return (TechnoTypeClass*)(UnitTypes[id]);
        return nullptr;

    case RTTI_BUILDINGTYPE:
    case RTTI_BUILDING:
        if (id < BuildingTypes.Count())
            return (TechnoTypeClass*)(BuildingTypes[id]);
        return nullptr;

    case RTTI_INFANTRYTYPE:
    case RTTI_INFANTRY:
        if (id < InfantryTypes.Count())
            return (TechnoTypeClass*)(InfantryTypes[id]);
        return nullptr;

    case RTTI_AIRCRAFTTYPE:
    case RTTI_AIRCRAFT:
        if (id < AircraftTypes.Count())
            return (TechnoTypeClass*)(AircraftTypes[id]);
        return nullptr;

    default:
        break;
    }
    return nullptr;
}


/**
 *  Extended replacement of HouseClass::Begin_Production.
 *
 *  @author: ZivDero, Rampastring
 */
ProdFailType HouseClassExtension::Begin_Production(RTTIType type, int id, bool resume, ProductionFlags flags)
{
    int result = true;
    FactoryClass* fptr;
    const TechnoTypeClass* tech = _Fetch_Techno_Type(type, id);

    /*
    **  The event layer does not validate the validity of the production event,
    **  which allows players to build normally unbuildable objects through crafted events.
    **  Validate the request here.
    */
    if (tech == nullptr) {
        return PROD_ILLEGAL;
    }

    if (This()->Is_Human_Player() && tech->Level > This()->Control.TechLevel) {
        return PROD_ILLEGAL;
    }

    BuildingClass* who = tech->Who_Can_Build_Me(false, true, true, This());
    bool onhold = false;

    if (who == nullptr) {
        if (resume) {
            who = tech->Who_Can_Build_Me(true, false, true, This());
        }
        if (who != nullptr) {
            onhold = true;
        } else {
            DEBUG_INFO("Request to Begin_Production of '%s' was rejected. No-one can build.\n", tech->GivenName.c_str());
            return PROD_CANT;
        }
    }

    fptr = Fetch_Factory(type, flags);

    if (fptr == nullptr) {
        fptr = new FactoryClass;

        if (fptr == nullptr) {
            DEBUG_INFO("Request to Begin_Production of '%s' was rejected. Unable to create factory\n", tech->GivenName.c_str());
            return PROD_CANT;
        }
    }

    /*
    **  If the house is already busy producing the requested object, then
    **  return with this failure code, unless we are restarting production.
    */
    if (fptr != nullptr) {
        if (fptr->Is_Building() && type == RTTI_BUILDINGTYPE) {
            DEBUG_INFO("Request to Begin_Production of '%s' was rejected. Cannot queue buildings.\n", tech->GivenName.c_str());
            return PROD_CANT;
        }
    }

    Set_Factory(type, fptr, flags);

    /*
    **  Check if we have an object of this type currently suspended in production.
    */
    bool skipset = false;
    if (fptr->IsSuspended && !Extension::Fetch(fptr)->IsHoldingExit) {
        TechnoClass* object = fptr->Object;
        if (object != nullptr) {
            if (object->TClass == tech) {
                skipset = true;
            }
        }
    }

    if (!skipset) {
        result = fptr->Set(*tech, *This(), resume);
    }

    if (result) {
        if (fptr->QueuedObjects.Count() == 0 || resume || skipset) {
            fptr->Start(onhold);

            /*
            **  Link this factory to the sidebar so that proper graphic feedback
            **  can take place.
            */
            if (PlayerPtr == This()) {
                Map.Factory_Link(fptr, type, id);
            }
        }

        return PROD_OK;
    }

    DEBUG_INFO("Request to Begin_Production of '%s' was rejected. Factory was unable to create the requested object\n", tech->GivenName.c_str());

    /*
    **  Output debug information if production failed.
    */
    if (fptr->QueuedObjects.Count() == 0 && fptr->Object == nullptr) {
        DEBUG_INFO("type=%d\n", type);
        DEBUG_INFO("Frame == %d\n", Frame);
        DEBUG_INFO("fptr->QueuedObjects.Count() == %d\n", fptr->QueuedObjects.Count());
        DEBUG_INFO("Object->RTTI == %d\n", fptr->Object != nullptr ? fptr->Object->Fetch_RTTI() : -1);
        DEBUG_INFO("Object->HeapID == %d\n", fptr->Object != nullptr ? fptr->Object->Fetch_Heap_ID() : -1);
        DEBUG_INFO("IsSuspended\t= %d\n", fptr->IsSuspended);

        delete fptr;
        Set_Factory(type, nullptr, flags);
    }

    return PROD_CANT;
}


/**
 *  Extended replacement of HouseClass::Abandon_Production.
 *
 *  @author: ZivDero
 */
ProdFailType HouseClassExtension::Abandon_Production(RTTIType type, int id, ProductionFlags flags)
{
    FactoryClass* fptr = Fetch_Factory(type, flags);

    /*
    **  If there is no factory to abandon, then return with a failure code.
    */
    if (fptr == nullptr) {
        return PROD_CANT;
    }

    /*
    **  If we're just dequeuing a unit, redraw the strip.
    */
    if (fptr->Queued_Object_Count() > 0 && id >= 0) {
        const TechnoTypeClass* technotype = Fetch_Techno_Type(type, id);
        if (fptr->Remove_From_Queue(*technotype)) {
            return PROD_OK;
        }
    }

    if (id != -1) {
        TechnoClass* obj = fptr->Object;
        if (obj == nullptr || id != obj->Class_Of()->Fetch_Heap_ID()) {
            return PROD_OK;
        }
    }

    /*
    **  Drop any active building placement.
    */
    if (PlayerPtr == This()) {
        BattleUI.Get_Sidebar().Detach(fptr);

        if (type == RTTI_BUILDINGTYPE || type == RTTI_BUILDING) {
            Map.PendingObjectPtr = nullptr;
            Map.PendingObject = nullptr;
            Map.PendingHouse = HOUSE_NONE;
            Map.Set_Cursor_Shape(nullptr);
        }
    }

    /*
    **  Abandon production of the object.
    */
    fptr->Abandon();
    if (fptr->QueuedObjects.Count() == 0) {
        Set_Factory(type, nullptr, flags);
        delete fptr;
    } else {
        fptr->Resume_Queue();
    }

    return PROD_OK;
}


/**
 *  Extended replacement of HouseClass::Place_Object.
 *
 *  @author: ZivDero
 */
bool HouseClassExtension::Place_Object(RTTIType type, Cell const& cell, ProductionFlags flags)
{
    bool placed = false;
    TechnoClass* tech = nullptr;
    FactoryClass* factory = Fetch_Factory(type, flags);

    /*
    **  Only if there is a factory active for this type, can it be "placed".
    **  In the case of a missing factory, then this request is completely bogus --
    **  ignore it. This might occur if, between two events to exit the same
    **  object, the mouse was clicked on the sidebar to start building again.
    **  The second placement event should NOT try to place the object that is
    **  just starting construction.
    */
    if (factory && factory->Has_Completed()) {
        auto factory_ext = Extension::Fetch(factory);
        tech = factory->Get_Object();

        if (tech != nullptr) {

            /*
            **  Announce that the object is ready.
            */
            if (!factory_ext->HasSpoken && factory->House == PlayerPtr) {
                if (tech->Is_Foot()) {
                    Speak(VOX_UNIT_READY);
                    factory_ext->HasSpoken = true;
                }
            }

            /*
            **  For units, make sure there's a factory that is not busy exiting a unit, otherwise just hold the unit for now.
            */
            if (tech->RTTI == RTTI_UNIT && Extension::Fetch(tech->Class_Of())->Who_Can_Build_Me(false, false, false, tech->Owner_HouseClass(), true) == nullptr) {
                factory_ext->IsHoldingExit = true;
                return placed;
            }

            if (cell == CELL_NONE || cell == Cell(-1, -1)) {

                /*
                **  Try to find a place for the object to appear from. For helicopters, it has the
                **  option of finding a nearby helipad if no helipads are free.
                */
                TechnoClass* builder = tech->Who_Can_Build_Me(false, true);
                if (builder == nullptr && tech->RTTI == RTTI_AIRCRAFT) {
                    builder = tech->Who_Can_Build_Me(true, true);
                }

                if (builder != nullptr) {
                    int exit = builder->Exit_Object(tech);
                    if (exit == 2 || (exit == 1 && builder->RTTI == RTTI_BUILDING && static_cast<BuildingClass*>(builder)->Factory != nullptr)) {

                        /*
                        **  Since the object has left the factory under its own power, delete
                        **  the production manager tied to this slot in the sidebar. Its job
                        **  has been completed.
                        */

                        // Do not assign last radar event cell, it's an annoyance - Rampastring
                        // LastRadarEventCell = builder->Center_Coord().As_Cell();
                        factory->Completed();
                        Abandon_Production(type, -1, flags);
                        placed = true;
                    } else {
                        /*
                        **  The object could not leave under it's own power. Just wait
                        **  until the player tries to place the object again.
                        */
                        if (This()->RTTI != RTTI_BUILDING) {
                            DEBUG_INFO("Failed to exit object from factory - refunding money\n");
                            Abandon_Production(type, -1, flags);
                        }
                        return placed;
                    }
                }

            } else {
                TechnoClass* builder = tech->Who_Can_Build_Me(false, false);
                if (builder) {

                    builder->Transmit_Message(RADIO_HELLO, tech);
                    if (tech->Unlimbo(cell.As_Coord())) {
                        if (tech->RTTI == RTTI_BUILDING) {
                            if (static_cast<BuildingClass*>(tech)->Class->IsFirestormWall) {
                                Map.Place_Firestorm_Wall(cell, This(), static_cast<BuildingClass*>(tech)->Class);
                            } else if (static_cast<BuildingClass*>(tech)->Class->ToOverlay != nullptr && static_cast<BuildingClass*>(tech)->Class->ToOverlay->IsWall) {
                                Map.Place_Wall(cell, This(), static_cast<BuildingClass*>(tech)->Class);
                            }
                        }
                        factory->Completed();
                        tech->Transmit_Message(RADIO_COMPLETE, builder);
                        Abandon_Production(type, -1, flags);
                        placed = true;

                        if (PlayerPtr == This()) {
                            if (tech->IsActive && !tech->IsDiscoveredByPlayer) {
                                tech->Revealed(This());
                            }
                            Sound_Effect(Rule->BuildingSlam);
                            Map.Set_Cursor_Shape(nullptr);
                            Map.PendingObjectPtr = nullptr;
                            Map.PendingObject = nullptr;
                            Map.PendingHouse = HOUSE_NONE;
                        }
                    } else {
                        placed = false;
                        if (This() == PlayerPtr) {
                            Speak(VOX_DEPLOY);
                        }
                    }
                    builder->Transmit_Message(RADIO_OVER_OUT);
                }
            }
        }

        if (placed) {

            /*
            **  Record the construction of the object.
            */
            This()->Just_Built(tech);

            /*
            **  If the factory still exists, we need to "reset" it.
            */
            if (factory != nullptr && Factories.Is_Present(factory)) {

                /*
                **  Mark that the factory isn't holding anything waiting to exit anymore.
                */
                Extension::Fetch(factory)->IsHoldingExit = false;

                /*
                **  Clear the flag for announcing unit production.
                */
                Extension::Fetch(factory)->HasSpoken = false;

            }

            /*
            **  For foot units, plays the unit ready sound when they exit.
            */
            if (tech->Is_Foot() && tech->House == PlayerPtr) {
                Speak(VOX_UNIT_READY);
            }
        }
    }

    return placed;
}


/**
 *  Extended replacement of HouseClass::Update_Factories.
 *
 *  @author: ZivDero
 */
void HouseClassExtension::Update_Factories(RTTIType rtti, ProductionFlags flags)
{
    FactoryClass* factory = Fetch_Factory(rtti, flags);

    if (factory != nullptr) {
        for (int i = factory->QueuedObjects.Count() - 1; i >= 0; i--) {
            TechnoTypeClass const* ttype = factory->QueuedObjects[i];
            if (ttype->Who_Can_Build_Me(true, false, true, This()) == nullptr) {
                factory->QueuedObjects.Delete(i);
            }
        }
        if (factory->Object != nullptr) {
            if (factory->Object->TClass->Who_Can_Build_Me(true, false, true, This()) == nullptr) {
                factory->Abandon();
                factory->Resume_Queue();
            } else {
                if (factory->Object->TClass->Who_Can_Build_Me(true, true, true, This()) == nullptr) {
                    factory->Suspend(false);
                } else {
                    if (factory->IsSuspended && !factory->IsOnHold) {
                        factory->Start(false);
                    }
                }
            }
        }
        if (factory->Object == nullptr && factory->QueuedObjects.Count() == 0) {
            delete factory;
        }
    }
}


/**
 *  Extended replacement of HouseClass::Suggest_New_Object.
 *
 *  @author: ZivDero
 */
TechnoTypeClass const* HouseClassExtension::Suggest_New_Object(RTTIType objecttype, ProductionFlags flags) const
{
    TechnoTypeClass const* techno = nullptr;

    switch (objecttype) {
    case RTTI_AIRCRAFT:
    case RTTI_AIRCRAFTTYPE:
        if (This()->BuildAircraft != AIRCRAFT_NONE) {
            return AircraftTypes[This()->BuildAircraft];
        }
        return nullptr;

        /*
        **  Unit construction is based on the rule that up to twice the number required
        **  to fill all teams will be created.
        */
    case RTTI_UNIT:
    case RTTI_UNITTYPE:
        if (flags & PRODFLAG_NAVAL) {
            if (BuildNavalUnit != UNIT_NONE) {
                return UnitTypes[BuildNavalUnit];
            }
        } else {
            if (This()->BuildUnit != UNIT_NONE) {
                return UnitTypes[This()->BuildUnit];
            }
        }
        return nullptr;

        /*
        **  Infantry construction is based on the rule that up to twice the number required
        **  to fill all teams will be created.
        */
    case RTTI_INFANTRY:
    case RTTI_INFANTRYTYPE:
        if (This()->BuildInfantry != INFANTRY_NONE) {
            return InfantryTypes[This()->BuildInfantry];
        }
        return nullptr;

        /*
        **  Building construction is based upon the preconstruction list.
        */
    case RTTI_BUILDING:
    case RTTI_BUILDINGTYPE:
        if (This()->BuildStructure != STRUCT_NONE) {
            return BuildingTypes[This()->BuildStructure];
        }
        return nullptr;
    }
    return techno;
}

TeamClass* HouseClassExtension::Get_Team_In_Production(int& id, RTTIType for_type, ProductionFlags prodflags) const
{
    if (id < 0) {
        DEBUG_ERROR("AdvAI!! Get_Team_In_Production: negative ID supplied!\n");
        return nullptr;
    }

    for (int i = id; i < Teams.Count(); i++)
    {
        TeamClass* team = Teams[i];

        if (team->House != This())
            continue;

        if (team->IsHasBeen)
            continue;

        TeamClassExtension* teamext = Extension::Fetch(team);

        if (teamext->ProdFlags != prodflags)
            continue;

        if (for_type == RTTI_INFANTRY) {
            if (teamext->NoInfantry) {
                continue;
            }

            if (teamext->MaxInfantry > -1 && teamext->Infantry_Count() >= teamext->MaxInfantry) {
                continue;
            }
        }

        if (for_type == RTTI_UNIT) {
            if (teamext->NoVehicles) {
                continue;
            }

            if (teamext->IsTransportTeam && teamext->Contains_Transport()) {
                continue;
            }
        }

        if (for_type == RTTI_AIRCRAFT && teamext->NoAircraft)
            continue;

        id = i + 1;
        return team;
    }

    return nullptr;
}

UnitTypeClass* Find_Our_MCV(HouseClass* house)
{
    for (int i = 0; i < UnitTypes.Count(); i++)
    {
        UnitTypeClass* unittype = UnitTypes[i];

        if (unittype->DeploysInto != nullptr && unittype->DeploysInto->IsConstructionYard && 
           (unittype->Ownable & (1 << house->ActLike)) == (1 << house->ActLike))
        {
            return unittype;
        }
    }

    return nullptr;
}


int AdvancedAI_AI_Unit_Start_Rush_Counter(HouseClass* house)
{
    HouseClassExtension* houseext = Extension::Fetch(house);

    DynamicVectorClass<BuildingTypeClass const*> owned_buildings;
    houseext->Fill_Owned_Buildings_List(owned_buildings);

    UnitType mostvaluable = UNIT_NONE;
    int highestvalue = -1;

    // Build a list of all units that we can build, alongside their scores for our current tactic.
    for (int i = 0; i < UnitTypes.Count(); i++)
    {
        UnitTypeClass* unittype = UnitTypes[i];
        UnitTypeClassExtension* unittypeext = Extension::Fetch(unittype);

        if (unittypeext->IsNaval) {
            continue;
        }

        if ((unittype->Ownable & (1 << house->ActLike)) == (1 << house->ActLike) &&
            house->Can_Build(unittype, false, true) && houseext->_AI_Has_Prerequisites(unittype, owned_buildings, owned_buildings.Count())) {

            if (unittype->BuildLimit > 0 && house->ActiveUQuantity.Value(unittype->HeapID) >= unittype->BuildLimit)
                continue;

            int value = unittypeext->AntiNoneArmorValue() + unittypeext->AntiLightArmorValue();
            if (value > highestvalue) {
                mostvaluable = unittype->HeapID;
                highestvalue = value;
            }
        }
    }

    if (mostvaluable != UNIT_NONE) {
        house->BuildUnit = mostvaluable;
    }

    return TICKS_PER_SECOND;
}


int AdvancedAI_AI_Unit(HouseClass* house)
{
    HouseClassExtension* houseext = Extension::Fetch(house);

    // ew WW
    DynamicVectorClass<BuildingTypeClass const*> owned_buildings;
    houseext->Fill_Owned_Buildings_List(owned_buildings);

    // If we have no Construction Yard, see if we should build an MCV.
    if (house->ConstructionYards.Count() < 1) {
        UnitTypeClass* mcv = Find_Our_MCV(house);
        if (mcv != nullptr) {
            if (house->ActiveUQuantity.Value(mcv->HeapID) < 1 && house->Can_Build(mcv, true, true) &&
                houseext->_AI_Has_Prerequisites(mcv, owned_buildings, owned_buildings.Count()))
            {
                house->BuildUnit = mcv->HeapID;
                return TICKS_PER_SECOND;
            }
        }
    }

    int harv = house->ActiveUQuantity.Value(house->Get_First_ActLike(Rule->HarvesterUnit)->HeapID);
    int ref = house->ActiveBQuantity.Value(house->Get_First_ActLike(Rule->BuildRefinery)->HeapID);
    int refmult;
    int harvmult;
    if (house->Difficulty == DIFF_HARD) {
        refmult = 1;
        harvmult = 1;
    }
    else if (house->Difficulty == DIFF_NORMAL) {
        refmult = 3;
        harvmult = 2;

        if (house->IsTiberiumShort) {
            refmult = 1;
            harvmult = 1;
        }
    }
    else { // DIFF_EASY
        refmult = 2;
        harvmult = 1;

        if (house->IsTiberiumShort) {
            refmult = 3;
            harvmult = 2;
        }
    }

    /*
    **  A computer controlled house will try to build a replacement
    **  harvester if possible. Unlike regular AI, AdvancedAI does not
    **  care about IsTiberiumShort - the tiberium will regrow after all.
    **  Also, Advanced AI does not build harvesters if it's getting rushed or
    **  or is attempting to rush the enemy itself.
    */
    if (!houseext->IsUnderStartRushThreat && houseext->AdvAIGroundTactic.Tactic != AdvAITacticType::TACTIC_RUSH_ATTACK &&
        (harv == 0 || (Frame > houseext->LastHarvesterBuildFrame + 3000 && Frame > house->LATime + 240)) &&
        house->IQ >= Rule->IQHarvester && !house->Is_Human_Player() && ref * refmult > harv * harvmult) {
        if (house->Get_First_ActLike(Rule->HarvesterUnit)->Level <= house->Control.TechLevel) {
            houseext->LastHarvesterBuildFrame = Frame;
            house->BuildUnit = house->Get_First_ActLike(Rule->HarvesterUnit)->HeapID;
            return TICKS_PER_SECOND;
        }
    }

    int id = 0;
    TeamClass* team = houseext->Get_Team_In_Production(id, RTTI_UNIT, PRODFLAG_NONE);

    if (team == nullptr) {

        // We need to produce something if we are under start rush threat, even if there's no team.
        if (houseext->IsUnderStartRushThreat) {
            return AdvancedAI_AI_Unit_Start_Rush_Counter(house);
        }

        return TICKS_PER_SECOND;
    }

    TeamClassExtension* teamext = Extension::Fetch(team);

    UnitType mostvaluable = UNIT_NONE;
    int highestvalue = 0;

    bool debugprint = Frame > houseext->LastUnitValueDebugPrintFrame + 10000;

    if (debugprint) {
        houseext->LastUnitValueDebugPrintFrame = Frame;
        DEBUG_INFO("AdvAI: House %d: Unit values: Frame: %d, Current Tactic: %s\n", house->HeapID, Frame, AdvAITacticType_To_Name(houseext->AdvAIGroundTactic.Tactic));
    }

    // Build a list of all non-naval vehicles that we can build, alongside their scores for our current tactic.
    for (int i = 0; i < UnitTypes.Count(); i++)
    {
        UnitTypeClass* unittype = UnitTypes[i];
        UnitTypeClassExtension* unittypeext = Extension::Fetch(unittype);

        if (unittypeext->IsNaval) {
            continue;
        }

        if (unittypeext->Buildability != TechnoTypeBuildability::BUILDABILITY_HUMAN_ONLY &&
            unittype->Level <= house->Control.TechLevel &&
            (unittype->Ownable & (1 << house->ActLike)) == (1 << house->ActLike) &&
            house->Can_Build(unittype, false, true) && houseext->_AI_Has_Prerequisites(unittype, owned_buildings, owned_buildings.Count())) {

            if (unittype->BuildLimit > 0) {
                // Those damn Enforcers...

                int quantity = house->UQuantity.Value(unittype->HeapID);
                if (unittype->DeploysInto != nullptr && unittype->DeploysInto->UndeploysInto == unittype) {
                    quantity += house->ActiveBQuantity.Value(unittype->DeploysInto->HeapID);
                }

                if (quantity >= unittype->BuildLimit)
                    continue;
            }

            // Don't waste epic units on risky suicide missions.
            if (team->Class->IsSuicide && unittype->BuildLimit < 5) {
                continue;
            }

            int value = teamext->AdvAI_Get_Object_Value_For_Team(unittype, debugprint);

            if (debugprint) {
                DEBUG_INFO("    %s: %d\n", unittype->IniName.c_str(), value);
            }

            if (value > highestvalue) {
                mostvaluable = unittype->HeapID;
                highestvalue = value;
            }
        }
    }

    if (mostvaluable != UNIT_NONE) {
        if (debugprint) {
            DEBUG_INFO("    Selected: %s\n", UnitTypes[mostvaluable]->IniName.c_str());
        }

        house->BuildUnit = mostvaluable;

        if (houseext->AdvAILastBuiltUnit == mostvaluable) {
            houseext->AdvAILastBuiltUnitCount++;
        }
        else {
            houseext->AdvAILastBuiltUnit = mostvaluable;
            houseext->AdvAILastBuiltUnitCount = 1;
        }
    }

    return TICKS_PER_SECOND;
}


/**
 *  Reimplementation of HouseClass::AI_Unit.
 *
 *  @author: ZivDero
 */
int HouseClassExtension::AI_Unit()
{
    HouseClass* this_ptr = This();
    if (this_ptr->BuildUnit != UNIT_NONE) return TICKS_PER_SECOND;

    if (RuleExtension->AdvancedAIUnitProduction) {
        return AdvancedAI_AI_Unit(This());
    }

    int harv = This()->ActiveUQuantity.Value(This()->Get_First_ActLike(Rule->HarvesterUnit)->HeapID);
    int ref = This()->ActiveBQuantity.Value(This()->Get_First_ActLike(Rule->BuildRefinery)->HeapID);
    int mult = RuleExtension->AIHarvestersPerRefinery[This()->Difficulty];

    if (Session.Type == GAME_NORMAL && RuleExtension->IsAIOneHarvesterInSingleplayer) {
        mult = 1;
    }

    /*
    **  A computer controlled house will try to build a replacement
    **  harvester if possible.
    */
    if (This()->IQ >= Rule->IQHarvester && !This()->IsTiberiumShort && !This()->Is_Human_Player() && ref * mult > harv) {
        if (This()->Get_First_ActLike(Rule->HarvesterUnit)->Level <= This()->Control.TechLevel) {
            This()->BuildUnit = This()->Get_First_ActLike(Rule->HarvesterUnit)->HeapID;
            return TICKS_PER_SECOND;
        }
    }

    int counter[1000]; // size increased replicating ts-patches
    int value[std::size(counter)];
    memset(counter, 0x00, sizeof(counter));
    for (int& i : value) {
        i = 0x7FFFFFFF;
    }

    /*
    **  Build a list of the maximum of each type we wish to produce. This will be
    **  twice the number required to fill all teams.
    */
    for (TeamClass* tptr : Teams) {
        if (tptr != nullptr) {

            int val = tptr->field_40;

            if (((tptr->Class->IsReinforcable && !tptr->IsFullStrength) || (!tptr->IsForcedActive && !tptr->IsHasBeen)) && tptr->House == This()) {
                DynamicVectorClass<TechnoTypeClass const*> _members;
                tptr->Team_Members(_members);

                for (int subindex = 0; subindex < _members.Count(); subindex++) {
                    if (_members[subindex]->RTTI == RTTI_UNITTYPE) {
                        UnitTypeClass const* memtype = static_cast<UnitTypeClass const*>(_members[subindex]);
                        counter[memtype->HeapID]++;
                        value[memtype->HeapID] = std::min(val, value[memtype->HeapID]);
                    }
                }
            }
        }
    }

    /*
    **  Reduce the theoretical maximum by the actual number of objects currently
    **  in play.
    */
    for (UnitClass* obj : Units) {
        if (obj != nullptr && obj->Is_Recruitable(This()) && counter[obj->Class->HeapID] > 0) {
            counter[obj->Class->HeapID]--;
        }
    }

    /*
    **  Pick to build the most needed object but don't consider those object that
    **  can't be built because of scenario restrictions or insufficient cash.
    */
    int bestval = -1;
    int bestcount = 0;
    UnitType lasttype = UNIT_NONE;
    int lastval = 0x7FFFFFFF;
    UnitType bestlist[std::size(counter)];
    for (UnitType type = UNIT_FIRST; type < UnitTypes.Count(); type++) {
        if (counter[type] > 0 && This()->Can_Build(UnitTypes[type], false, false) && UnitTypes[type]->Cost_Of(This()) <= This()->Available_Money() && !Extension::Fetch(UnitTypes[type])->IsNaval) {
            if (bestval == -1 || bestval < counter[type]) {
                bestval = counter[type];
                bestcount = 0;
            }
            bestlist[bestcount++] = type;

            if (lasttype == UNIT_NONE || value[type] < lastval) {
                lasttype = type;
                lastval = value[type];
            }
        }
    }

    if (Random_Pick2(0, 0x7FFFFFFE) < Rule->FillEarliestTeamProbability[This()->Difficulty] / 100.0) {
        This()->BuildUnit = lasttype;
    } else {
        /*
        **  The object type to build is now known. Fetch a pointer to the techno type class.
        */
        if (bestcount) {
            This()->BuildUnit = bestlist[Random_Pick(0, bestcount - 1)];
        }
    }

    return TICKS_PER_SECOND;
}


int AdvancedAI_AI_Naval_Unit(HouseClass* house)
{
    HouseClassExtension* houseext = Extension::Fetch(house);

    DynamicVectorClass<BuildingTypeClass const*> owned_buildings;
    houseext->Fill_Owned_Buildings_List(owned_buildings);

    int id = 0;
    TeamClass* team = houseext->Get_Team_In_Production(id, RTTI_UNIT, PRODFLAG_NAVAL);

    if (team == nullptr) {
        return TICKS_PER_SECOND;
    }

    TeamClassExtension* teamext = Extension::Fetch(team);

    UnitType mostvaluable = UNIT_NONE;
    int highestvalue = 0;

    bool debugprint = Frame > houseext->LastUnitValueDebugPrintFrame + 10000;

    if (debugprint) {
        houseext->LastUnitValueDebugPrintFrame = Frame;
        DEBUG_INFO("AdvAI: House %d: Naval unit values: Frame: %d\n", house->HeapID, Frame);
    }

    // Build a list of all non-naval vehicles that we can build, alongside their scores for our current tactic.
    for (int i = 0; i < UnitTypes.Count(); i++)
    {
        UnitTypeClass* unittype = UnitTypes[i];
        UnitTypeClassExtension* unittypeext = Extension::Fetch(unittype);

        if (!unittypeext->IsNaval) {
            continue;
        }

        if (unittypeext->Buildability != TechnoTypeBuildability::BUILDABILITY_HUMAN_ONLY &&
            (unittype->Ownable & (1 << house->ActLike)) == (1 << house->ActLike) &&
            house->Can_Build(unittype, false, true) && houseext->_AI_Has_Prerequisites(unittype, owned_buildings, owned_buildings.Count())) {

            if (unittype->BuildLimit > 0) {
                int quantity = house->UQuantity.Value(unittype->HeapID);

                if (quantity >= unittype->BuildLimit)
                    continue;
            }

            int value = teamext->AdvAI_Get_Object_Value_For_Team(unittype, debugprint);

            if (debugprint) {
                DEBUG_INFO("    %s: %d\n", unittype->IniName.c_str(), value);
            }

            if (value > highestvalue) {
                mostvaluable = unittype->HeapID;
                highestvalue = value;
            }
        }
    }

    if (mostvaluable != UNIT_NONE) {
        if (debugprint) {
            DEBUG_INFO("    Selected: %s\n", UnitTypes[mostvaluable]->IniName.c_str());
        }

        houseext->BuildNavalUnit = mostvaluable;
    }

    return TICKS_PER_SECOND;
}


/**
 *  A new AI naval unit production handler.
 *
 *  @author: ZivDero
 */
int HouseClassExtension::AI_Naval_Unit()
{
    if (BuildNavalUnit != UNIT_NONE) return TICKS_PER_SECOND;

    if (RuleExtension->AdvancedAIUnitProduction) {
        return AdvancedAI_AI_Naval_Unit(This());
    }

    int counter[1000]; // size increased replicating ts-patches
    int value[std::size(counter)];
    memset(counter, 0x00, sizeof(counter));
    for (int& i : value) {
        i = 0x7FFFFFFF;
    }

    /*
    **  Build a list of the maximum of each type we wish to produce. This will be
    **  twice the number required to fill all teams.
    */
    for (TeamClass* tptr : Teams) {
        if (tptr != nullptr) {

            int val = tptr->field_40;

            if (((tptr->Class->IsReinforcable && !tptr->IsFullStrength) || (!tptr->IsForcedActive && !tptr->IsHasBeen)) && tptr->House == This()) {
                DynamicVectorClass<TechnoTypeClass const*> _members;
                tptr->Team_Members(_members);

                for (int subindex = 0; subindex < _members.Count(); subindex++) {

                    UnitTypeClass const* memtype = static_cast<UnitTypeClass const*>(_members[subindex]);

                    if (memtype->RTTI == RTTI_UNITTYPE) {
                        counter[memtype->HeapID]++;
                        value[memtype->HeapID] = std::min(val, value[memtype->HeapID]);
                    }
                }
            }
        }
    }

    /*
    **  Reduce the theoretical maximum by the actual number of objects currently
    **  in play.
    */
    for (UnitClass* obj : Units) {
        if (obj != nullptr && obj->Is_Recruitable(This()) && counter[obj->Class->HeapID] > 0) {
            counter[obj->Class->HeapID]--;
        }
    }

    /*
    **  Pick to build the most needed object but don't consider those object that
    **  can't be built because of scenario restrictions or insufficient cash.
    */
    int bestval = -1;
    int bestcount = 0;
    UnitType lasttype = UNIT_NONE;
    int lastval = 0x7FFFFFFF;
    UnitType bestlist[std::size(counter)];
    for (UnitType type = UNIT_FIRST; type < UnitTypes.Count(); type++) {
        if (counter[type] > 0 && This()->Can_Build(UnitTypes[type], false, false) && UnitTypes[type]->Cost_Of(This()) <= This()->Available_Money() && Extension::Fetch(UnitTypes[type])->IsNaval) {
            if (bestval == -1 || bestval < counter[type]) {
                bestval = counter[type];
                bestcount = 0;
            }
            bestlist[bestcount++] = type;

            if (lasttype == UNIT_NONE || value[type] < lastval) {
                lasttype = type;
                lastval = value[type];
            }
        }
    }

    if (Random_Pick2(0, 0x7FFFFFFE) < Rule->FillEarliestTeamProbability[This()->Difficulty] / 100.0) {
        BuildNavalUnit = lasttype;
    } else {
        /*
        **  The object type to build is now known. Fetch a pointer to the techno type class.
        */
        if (bestcount) {
            BuildNavalUnit = bestlist[Random_Pick(0, bestcount - 1)];
        }
    }

    return TICKS_PER_SECOND;
}


/**
 *  Checks if the house owns this prerequisite, as it appears in the Prerequisite= list.
 *
 *  @author: ZivDero
 */
bool HouseClassExtension::Has_Prerequisite(int prerequisite)
{
    if (prerequisite >= STRUCT_FIRST) {
        return Has_Prerequisite(static_cast<StructType>(prerequisite));
    } else {
        return Has_Prerequisite(PrerequisiteGroupClass::Decode(prerequisite));
    }
}


/**
 *  Checks if the house owns a building that satisfies this prerequisite group.
 *
 *  @author: ZivDero
 */
bool HouseClassExtension::Has_Prerequisite(PrerequisiteGroupType group)
{
    /*
    **  Invalid prerequisite groups can't be owned.
    */
    if (group < PREREQ_GROUP_FIRST || group > PrerequisiteGroups.Count()) {
        return false;
    }

    PrerequisiteGroupClass* group_ptr = PrerequisiteGroups[group];

    /*
    **  Check if we own at least one of the types in this group.
    */
    for (int building : group_ptr->Prerequisites) {
        if (Has_Prerequisite(static_cast<StructType>(building))) {
            return true;
        }
    }

    return false;
}


/**
 *  Checks if the house owns a specific building prerequisite.
 *
 *  @author: ZivDero
 */
bool HouseClassExtension::Has_Prerequisite(StructType building)
{
    /*
    **  Invalid buildings can't be owned.
    */
    if (building < STRUCT_FIRST || building > BuildingTypes.Count()) {
        return false;
    }

    BuildingTypeClass* btype = BuildingTypes[building];

    /*
    **  If this isn't an upgrade, just check the counter.
    */
    if (btype->PowersUpBuilding.empty()) {
        return This()->ActiveBQuantity.Value(building) > 0;
    }

    /*
    **  For upgrades, we have to scan all of the buildings on the map...
    */
    for (int i = 0; i < Buildings.Count(); i++) {
        BuildingClass* bptr = Buildings[i];
        if (!bptr->IsInLimbo && bptr->House == This() && bptr->IsOn) {
            if (bptr->Mission != MISSION_DECONSTRUCTION && bptr->MissionQueue != MISSION_DECONSTRUCTION) {
                for (int j = 0; j < std::size(bptr->Upgrades); j++) {

                    /*
                    **  If this building is upgraded with our desired prerequisite, return true.
                    */
                    if (bptr->Upgrades[j] == btype) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}


/**
 *  Puts pointers to the storage extension into the storage class.
 *
 *  @author: ZivDero
 */
void HouseClassExtension::Put_Storage_Pointers()
{
    new (reinterpret_cast<StorageClassExt*>(&This()->Tiberium)) StorageClassExt(&TiberiumStorage);
    new (reinterpret_cast<StorageClassExt*>(&This()->Weed)) StorageClassExt(&WeedStorage);
}


bool HouseClassExtension::_AI_Has_Prerequisites(const TechnoTypeClass* type, DynamicVectorClass<const BuildingTypeClass*>& owned, int ownedcount) const
{
    for (int i = 0; i < type->Prerequisite.Count(); i++) {

        if (type->Prerequisite[i] >= STRUCT_FIRST) {

            BuildingTypeClass* btype = BuildingTypes[type->Prerequisite[i]];

            // Ignore service depots and naval yards as prerequisites
            if (btype->IsCanUnitRepair)
                continue;

            if (Extension::Fetch(btype)->IsNaval)
                continue;

            if (!Rule->BuildConst.Is_Present(btype)) {

                bool found = false;
                for (int j = 0; j < ownedcount; j++) {
                    if (owned[j] == btype) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    return false;
                }
            }

        }
        else {

            if (type->Prerequisite[i] == -1) {
                continue;
            }

            PrerequisiteGroupType grouptype = PrerequisiteGroupClass::Decode(type->Prerequisite[i]);
            if (grouptype == PREREQ_GROUP_NONE) {
                return false;
            }

            PrerequisiteGroupClass* group = PrerequisiteGroups[grouptype];

            if (group->Prerequisites.Count() > 0 && BuildingTypes[group->Prerequisites[0]]->IsCanUnitRepair)
                continue;

            bool found = false;
            for (int j = 0; j < ownedcount; j++) {
                if (group->Prerequisites.Is_Present(owned[j]->HeapID)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                return false;
            }
        }
    }

    return true;
}

/**
 *  A fake class for implementing new member functions which allow
 *  access to the "this" pointer of the intended class.
 *
 *  @note: This must not contain a constructor or destructor!
 *  @note: All functions must be prefixed with "_" to prevent accidental virtualization.
 */
static class UnitTrackerClassExt : public UnitTrackerClass
{
public:
    HRESULT _Load(IStream* pStm);
    HRESULT _Save(IStream* pStm);
};


/**
 *  Saves a unit tracker's counts
 *
 *  @author: ZivDero
 */
HRESULT UnitTrackerClassExt::_Load(IStream* pStm)
{
    int count;
    HRESULT hr = pStm->Read(&count, sizeof(count), nullptr);
    if (FAILED(hr)) {
        return hr;
    }

    new (this) UnitTrackerClass(count);

    for (int i = 0; i < UnitCount; i++) {
        hr = pStm->Read(&UnitTotals[i], sizeof(UnitTotals[i]), nullptr);
        if (FAILED(hr)) {
            return hr;
        }

    }

    return S_OK;
}


/**
 *  Saves a unit tracker's counts.
 *
 *  @author: ZivDero
 */
HRESULT UnitTrackerClassExt::_Save(IStream* pStm)
{
    HRESULT hr = pStm->Write(&UnitCount, sizeof(UnitCount), nullptr);
    if (FAILED(hr)) {
        return hr;
    }

    for (int i = 0; i < UnitCount; i++) {
        hr = pStm->Write(&UnitTotals[i], sizeof(UnitTotals[i]), nullptr);
        if (FAILED(hr)) {
            return hr;
        }

    }

    return S_OK;
}


/**
 *  Reads a house's unit trackers.
 *
 *  @author: ZivDero
 */
void HouseClassExtension::Load_Unit_Trackers(HouseClass* house, IStream* pStm)
{
    /**
     *  Trackers store their counts in a dynamically allocated array (AARGH WW!).
     *  Thus, we need to save/load them manually.
     *  But we can't do this in the extension because ThisPtr isn't remapped yet.
     */

    house->AircraftTotals = new UnitTrackerClass(0);
    house->InfantryTotals = new UnitTrackerClass(0);
    house->UnitTotals = new UnitTrackerClass(0);
    house->BuildingTotals = new UnitTrackerClass(0);
    house->DestroyedAircraft = new UnitTrackerClass(0);
    house->DestroyedInfantry = new UnitTrackerClass(0);
    house->DestroyedUnits = new UnitTrackerClass(0);
    house->DestroyedBuildings = new UnitTrackerClass(0);
    house->CapturedBuildings = new UnitTrackerClass(0);
    house->TotalCrates = new UnitTrackerClass(0);

    reinterpret_cast<UnitTrackerClassExt*>(house->AircraftTotals)->_Load(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->InfantryTotals)->_Load(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->UnitTotals)->_Load(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->BuildingTotals)->_Load(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->DestroyedAircraft)->_Load(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->DestroyedInfantry)->_Load(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->DestroyedUnits)->_Load(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->DestroyedBuildings)->_Load(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->CapturedBuildings)->_Load(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->TotalCrates)->_Load(pStm);
}


/**
 *  Saves a house's unit trackers.
 *
 *  @author: ZivDero
 */
void HouseClassExtension::Save_Unit_Trackers(HouseClass* house, IStream* pStm)
{
    reinterpret_cast<UnitTrackerClassExt*>(house->AircraftTotals)->_Save(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->InfantryTotals)->_Save(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->UnitTotals)->_Save(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->BuildingTotals)->_Save(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->DestroyedAircraft)->_Save(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->DestroyedInfantry)->_Save(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->DestroyedUnits)->_Save(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->DestroyedBuildings)->_Save(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->CapturedBuildings)->_Save(pStm);
    reinterpret_cast<UnitTrackerClassExt*>(house->TotalCrates)->_Save(pStm);
}


/**
 *  Sets this house's spawn point from its coordinate.
 *
 *  @author: ZivDero
 */
void HouseClassExtension::Set_Spawn_Point(const Cell& cell)
{
    for (WAYPOINT i = 0; i < MAX_PLAYERS; i++) {
        if (Scen->Is_Waypoint_Valid(i) && Scen->Waypoint_Cell(i) == cell) {
            SpawnWaypoint = i;
            return;
        }
    }

    SpawnWaypoint = WAYPOINT_NONE;
}


/**
 *  Tries to fetch a house spawned at this waypoint
 *
 *  @author: ZivDero
 */
HouseClass* HouseClassExtension::House_At_Spawn_Point(WAYPOINT waypoint)
{
    for (auto& house_ext : HouseExtensions) {
        if (house_ext->SpawnWaypoint != WAYPOINT_NONE && house_ext->SpawnWaypoint == waypoint) {
            return house_ext->This();
        }
    }

    return nullptr;
}


/**
 *  Fetches a house by its houses type.
 *  Also takes care of ts-patches spawn houses.
 *
 *  @author: ZivDero
 */
HouseClass* HouseClassExtension::House_From_HousesType(HousesType house)
{
    /**
     *  Houses between 50 and 57 are "spawn houses".
     *  Try to fetch the house at this waypoint.
     */
    if (Session.Type != GAME_NORMAL) {
        if (house >= 50 && house <= 57) {
            return House_At_Spawn_Point(static_cast<WAYPOINT>(house - 50));
        }
    }

    /**
     *  Otherwise, just perform the normal logic to fetch the house.
     */
    return ::House_From_HousesType(house);
}

/**
 *  Checks whether this house is able to use the Iron Curtain.
 *
 *  @author: Rampastring
 */
bool HouseClassExtension::Can_Use_Iron_Curtain() const
{
    if (!IronCurtainAvailabilityTimer.Expired()) {
        return false;
    }

    if (!This()->Is_Powered()) {
        return false;
    }

    for (int i = 0; i < RuleExtension->IronCurtains.Count(); i++) {
        if (This()->ActiveBQuantity.Value(RuleExtension->IronCurtains[i]->HeapID) > 0) {
            return true;
        }
    }

    return false;
}


/**
 *  Marks the house's Iron Curtain as used, making it recharge.
 *
 *  @author: Rampastring
 */
void HouseClassExtension::Expend_Iron_Curtain()
{
    IronCurtainAvailabilityTimer = RuleExtension->IronCurtainRechargeTime;
}

bool HouseClassExtension::AdvAI_Is_Outnumbered() const
{
    int enemytotalstrength = EnemyNoneStrength + EnemyLightStrength + EnemyHeavyStrength;
    int ourstrength = 0;

    for (int i = 0; i < Teams.Count(); i++)
    {
        TeamClass* team = Teams[i];
        if (team->House != This())
            continue;

        TeamClassExtension* teamext = Extension::Fetch(team);
        if (teamext->IsAircraftTeam)
            continue;

        ourstrength += teamext->CurrentCost;
    }

    return enemytotalstrength > ourstrength * 2;
}

bool HouseClassExtension::AdvAI_Is_Disadvantaged() const
{
    int enemytotalstrength = EnemyNoneStrength + EnemyLightStrength + EnemyHeavyStrength;
    int ourstrength = 0;

    for (int i = 0; i < Teams.Count(); i++)
    {
        TeamClass* team = Teams[i];
        if (team->House != This())
            continue;

        TeamClassExtension* teamext = Extension::Fetch(team);
        ourstrength += teamext->CurrentCost;
    }

    return enemytotalstrength > ourstrength;
}

bool HouseClassExtension::Enemy_Building_Scan(DynamicVectorClass<BuildingTypeClass*>& list, int threatvalue) const
{
    if (This()->Enemy == HOUSE_NONE) {
        return false;
    }

    for (int i = 0; i < Buildings.Count(); i++)
    {
        BuildingClass* building = Buildings[i];

        if (!building->IsActive || building->IsInLimbo || !building->IsDown || building->House->HeapID != This()->Enemy)
            continue;

        if (!list.Is_Present(building->Class))
            continue;

        if (Map.Cell_Threat(building->Center_Coord().As_Cell(), This()) <= threatvalue) {
            return true;
        }
    }

    return false;
}


bool HouseClassExtension::AdvAI_Enemy_Has_Vulnerable_Buildings(DynamicVectorClass<BuildingTypeClass*>& list) const
{
    return Enemy_Building_Scan(list, 50);
}


bool HouseClassExtension::AdvAI_Enemy_Has_Lightly_Defended_Buildings(DynamicVectorClass<BuildingTypeClass*>& list) const
{
    return Enemy_Building_Scan(list, 500);
}

void HouseClassExtension::AdvAI_Bias_Team_Strength_Ratios_By_Enemy_Strength(TeamClass* team) const
{
    TeamClassExtension* teamext = Extension::Fetch(team);
    HouseClassExtension* houseext = Extension::Fetch(team->House);

    teamext->IsBiasedForEnemyStrength = true;

    int enemytotal = houseext->EnemyBaseDefenseStrength + houseext->EnemyHeavyStrength +
        houseext->EnemyLightStrength + houseext->EnemyNoneStrength + houseext->EnemyArtilleryStrength;

    double enemyBaseDefenses = houseext->EnemyBaseDefenseStrength / (double)enemytotal;
    double enemyHeavy = houseext->EnemyHeavyStrength / (double)enemytotal;
    double enemyLight = houseext->EnemyLightStrength / (double)enemytotal;
    double enemyNone = houseext->EnemyNoneStrength / (double)enemytotal;
    double enemyArtillery = houseext->EnemyArtilleryStrength / (double)enemytotal;

    if (teamext->DesiredAntiNoneRatio > 0.0) {
        teamext->DesiredAntiNoneRatio = (teamext->DesiredAntiNoneRatio + enemyNone * 2) / 3.0;
    }

    if (teamext->DesiredAntiLightRatio > 0.0) {
        teamext->DesiredAntiNoneRatio = (teamext->DesiredAntiLightRatio + enemyLight * 2) / 3.0;
    }

    if (teamext->DesiredAntiHeavyRatio > 0.0) {
        teamext->DesiredAntiHeavyRatio = (teamext->DesiredAntiHeavyRatio + enemyHeavy * 2) / 3.0;
    }

    if (enemyBaseDefenses > 0.0 && teamext->DesiredArtilleryRatio > 0.0) {
        teamext->DesiredArtilleryRatio = (teamext->DesiredArtilleryRatio + enemyBaseDefenses) / 2.0;
    }

    if (enemyArtillery > teamext->DesiredArtilleryRatio) {
        teamext->DesiredArtilleryRatio = (teamext->DesiredArtilleryRatio + enemyArtillery) / 2.0;
    }

    EnemyStrengthStruct* strengthstruct = &houseext->EnemyAntiGroundStrength;

    if (teamext->ProdFlags == PRODFLAG_NAVAL) {
        strengthstruct = &houseext->EnemyAntiNavalStrength;
    }

    if (!teamext->IsAircraftTeam && !teamext->IsTransportTeam) {
        int enemyCounterTotal = strengthstruct->Total();

        if (enemyCounterTotal > 0) {
            if (teamext->DesiredNoneStrengthRatio > 0.0) {
                double enemyAntiNoneRatio = (strengthstruct->None * 2.0) / (double)enemyCounterTotal;
                double ratio = enemyAntiNoneRatio / teamext->DesiredNoneStrengthRatio;
                teamext->DesiredNoneStrengthRatio = teamext->DesiredNoneStrengthRatio / std::max(ratio, 0.1);

                if (enemyArtillery > 0) {
                    double multi = houseext->EnemyArtilleryStrength > 2500 ? 2.0 : 1.0;

                    teamext->DesiredNoneStrengthRatio = std::max(teamext->DesiredNoneStrengthRatio * (1.0 - (enemyArtillery * multi)), 0.0);
                }
            }

            if (teamext->DesiredLightStrengthRatio > 0.0) {
                double enemyAntiLightRatio = strengthstruct->Light / (double)enemyCounterTotal;
                double ratio = enemyAntiLightRatio / teamext->DesiredLightStrengthRatio;
                teamext->DesiredLightStrengthRatio = teamext->DesiredLightStrengthRatio / std::max(ratio, 0.1);
            }

            if (teamext->DesiredHeavyStrengthRatio > 0.0) {
                double enemyAntiHeavyRatio = strengthstruct->Heavy / (double)enemyCounterTotal;
                double ratio = enemyAntiHeavyRatio / teamext->DesiredHeavyStrengthRatio;
                teamext->DesiredHeavyStrengthRatio = teamext->DesiredHeavyStrengthRatio / std::max(ratio, 0.1);
            }
        }
    }
}

void HouseClassExtension::AdvAI_Set_Aircraft_Team_Desired_Ratios(TeamClass* team) const
{
    TeamClassExtension* teamext = Extension::Fetch(team);
    HouseClassExtension* houseext = Extension::Fetch(team->House);

    switch (houseext->AdvAIAirTactic.Tactic)
    {
    case AIRTACTIC_ATTACK_INFANTRY:
        teamext->DesiredAntiNoneRatio = 0.8;
        teamext->DesiredAntiLightRatio = 0.1;
        teamext->DesiredAntiHeavyRatio = 0.1;
        break;
    case AIRTACTIC_ATTACK_HARVESTERS:
        if (Rule->HarvesterUnit[0]->Armor == ARMOR_STEEL) {
            teamext->DesiredAntiHeavyRatio = 1.0;
            break;
        }
        teamext->DesiredAntiLightRatio = 1.0;
        break;
    case AIRTACTIC_ATTACK_REFINERIES:
    case AIRTACTIC_ATTACK_FACTORIES:
        teamext->DesiredAntiLightRatio = 1.0;
        break;
    case AIRTACTIC_ATTACK_VEHICLES:
        if (houseext->EnemyArtilleryStrength > 2000 || houseext->EnemyLightStrength > houseext->EnemyHeavyStrength || Rule->HarvesterUnit[0]->Armor != ARMOR_STEEL) {
            teamext->DesiredAntiLightRatio = 0.5;
            teamext->DesiredAntiHeavyRatio = 0.5;
        }
        else {
            teamext->DesiredAntiLightRatio = 0.25;
            teamext->DesiredAntiHeavyRatio = 0.75;
        }

        break;
    default:
    case AIRTACTIC_NONE:
        teamext->DesiredAntiNoneRatio = 0.0;
        teamext->DesiredAntiLightRatio = 0.5;
        teamext->DesiredAntiHeavyRatio = 0.5;
        break;
    }
}

void HouseClassExtension::AdvAI_Set_Naval_Team_Desired_Ratios(TeamClass* team) const
{
    TeamClassExtension* teamext = Extension::Fetch(team);
    HouseClassExtension* houseext = Extension::Fetch(team->House);

    switch (houseext->AdvAINavalTactic.Tactic)
    {
    case NAVALTACTIC_DIRECT_ATTACK:
    default:
        teamext->DesiredAntiNoneRatio = 0.0;
        teamext->DesiredAntiLightRatio = 0.5;
        teamext->DesiredAntiHeavyRatio = 0.5;
        teamext->DesiredArtilleryRatio = 0.3;
        teamext->DesiredLightStrengthRatio = 0.5;
        teamext->DesiredHeavyStrengthRatio = 0.5;
        teamext->PenalizeSameTypeUnits = true;
        houseext->AdvAI_Bias_Team_Strength_Ratios_By_Enemy_Strength(team);
        break;
    }
}

void HouseClassExtension::AdvAI_Set_Ground_Team_Desired_Ratios(TeamClass* team, AdvAITacticType tacticoverride) const
{
    TeamClassExtension* teamext = Extension::Fetch(team);

    AdvAITacticType tactic = AdvAIGroundTactic.Tactic;
    if (tacticoverride != AdvAITacticType::TACTIC_NONE)
        tactic = tacticoverride;

    switch (tactic)
    {
    case AdvAITacticType::TACTIC_SCOUT:
        teamext->DesiredAntiNoneRatio = 0.75;
        teamext->DesiredAntiLightRatio = 0.25;
        teamext->SpeedValueMultiplier = 2.5;
        teamext->CostWeightMultiplier = 1.0;
        teamext->DesiredNoneStrengthRatio = 1.0;
        break;
    case AdvAITacticType::TACTIC_RUSH_ATTACK:
        teamext->DesiredAntiLightRatio = 1.0;
        teamext->SpeedValueMultiplier = team->House->Class->HeapID == 1 ? 3.0 : 4.0;
        if (teamext->NoInfantry) {
            teamext->DesiredLightStrengthRatio = 1.0;
        }
        else if (teamext->NoVehicles) {
            teamext->DesiredNoneStrengthRatio = 1.0;
        }
        else {
            teamext->DesiredLightStrengthRatio = 0.67;
            teamext->DesiredNoneStrengthRatio = 0.33;
        }
        break;
    case AdvAITacticType::TACTIC_DIRECT_ATTACK_REGULAR:
        teamext->DesiredAntiNoneRatio = 0.33;
        teamext->DesiredAntiLightRatio = 0.33;
        teamext->DesiredAntiHeavyRatio = 0.33;
        teamext->DesiredArtilleryRatio = 0.5;
        teamext->SpeedValueMultiplier = team->House->Class->HeapID > 0 ? -0.2 : 0.0;
        teamext->StrengthValueMultiplier = 1.2;
        teamext->CostWeightMultiplier = 0.85;
        teamext->DesiredNoneStrengthRatio = 0.2;
        teamext->DesiredLightStrengthRatio = 0.35;
        teamext->DesiredHeavyStrengthRatio = 0.45;
        teamext->PenalizeSameTypeUnits = true;
        AdvAI_Bias_Team_Strength_Ratios_By_Enemy_Strength(team);
        break;
    case AdvAITacticType::TACTIC_DIRECT_ATTACK_FAST:
        teamext->DesiredAntiNoneRatio = 0.2;
        teamext->DesiredAntiLightRatio = 0.5;
        teamext->DesiredAntiHeavyRatio = 0.3;
        teamext->SpeedValueMultiplier = 2.0;
        teamext->CostWeightMultiplier = 0.9;
        teamext->DesiredNoneStrengthRatio = 0.2;
        teamext->DesiredLightStrengthRatio = 0.3;
        teamext->DesiredHeavyStrengthRatio = 0.5;
        teamext->PenalizeSameTypeUnits = true;
        AdvAI_Bias_Team_Strength_Ratios_By_Enemy_Strength(team);
        break;
    case AdvAITacticType::TACTIC_ATTACK_HARVESTERS:
        teamext->DesiredAntiHeavyRatio = 0.7;
        teamext->DesiredAntiLightRatio = 0.3;
        teamext->SpeedValueMultiplier = 2.0;
        teamext->CloakValueMultiplier = EnemyHasSensors ? 0.3 : 2.0;
        AdvAI_Bias_Team_Strength_Ratios_By_Enemy_Strength(team);
        break;
    case AdvAITacticType::TACTIC_ATTACK_REFINERIES:
        teamext->DesiredAntiLightRatio = 0.7;
        teamext->DesiredAntiHeavyRatio = 0.3;
        teamext->SpeedValueMultiplier = 1.2;
        teamext->StrengthValueMultiplier = 1.2;
        break;
    case AdvAITacticType::TACTIC_APC_ATTACK:
        teamext->SpeedValueMultiplier = 0.0; // Speed doesn't matter for inf that is carried by APCs
        teamext->DesiredAntiNoneRatio = 0.1;
        teamext->DesiredAntiLightRatio = 0.8;
        teamext->DesiredAntiHeavyRatio = 0.1;
        teamext->StrengthValueMultiplier = 2.0;
        teamext->CostWeightMultiplier = 0.75;
        teamext->CloakValueMultiplier = 3.0;
        break;
    default:
    case AdvAITacticType::TACTIC_NONE:
        teamext->DesiredAntiNoneRatio = 0.33;
        teamext->DesiredAntiLightRatio = 0.33;
        teamext->DesiredAntiHeavyRatio = 0.33;
        teamext->DesiredNoneStrengthRatio = 0.33;
        teamext->DesiredLightStrengthRatio = 0.33;
        teamext->DesiredHeavyStrengthRatio = 0.33;
        AdvAI_Bias_Team_Strength_Ratios_By_Enemy_Strength(team);
        break;
    }
}

void HouseClassExtension::Assign_AdvAI_Tactic(AdvAITacticType tactic, int expected_duration)
{
    DEBUG_INFO("AdvAI: House %d: Assigning tactic %d with a duration of %d. Frame: %d\n", This()->HeapID, tactic, expected_duration, Frame);

    assert(AdvAIGroundTactic.Tactic == TACTIC_NONE);

    AdvAIGroundTactic = AdvAITacticInfo<AdvAITacticType>(tactic, Frame, expected_duration);
    LastInfantryValueDebugPrintFrame = INT_MIN;
    LastUnitValueDebugPrintFrame = INT_MIN;
}

void HouseClassExtension::Assign_AdvAI_Air_Tactic(AdvAIAirTacticType airtactic, int expected_duration)
{
    DEBUG_INFO("AdvAI: House %d: Assigning air tactic %d with a duration of %d. Frame: %d\n", This()->HeapID, airtactic, expected_duration, Frame);

    assert(AdvAIAirTactic.Tactic == TACTIC_NONE);

    AdvAIAirTactic = AdvAITacticInfo<AdvAIAirTacticType>(airtactic, Frame, expected_duration);
    LastAircraftValueDebugPrintFrame = INT_MIN;
}

void HouseClassExtension::Assign_AdvAI_Naval_Tactic(AdvAINavalTacticType navaltactic, int expected_duration)
{
    DEBUG_INFO("AdvAI: House %d: Assigning naval tactic %d with a duration of %d. Frame: %d\n", This()->HeapID, navaltactic, expected_duration, Frame);

    assert(AdvAINavalTactic.Tactic == TACTIC_NONE);

    AdvAINavalTactic = AdvAITacticInfo<AdvAINavalTacticType>(navaltactic, Frame, expected_duration);
    LastNavalValueDebugPrintFrame = INT_MIN;
}

void HouseClassExtension::Fill_Owned_Buildings_List(DynamicVectorClass<const BuildingTypeClass*>& owned) const
{
    for (int i = 0; i < Buildings.Count(); i++) {
        BuildingClass* b2 = Buildings[i];
        if (b2->House == This() && !b2->IsInLimbo && b2->IsDown) {
            owned.Add(b2->Class);
        }
    }
}

int HouseClassExtension::Get_Building_Capture_Attempt_Index_For(BuildingClass* building) const
{
    for (int j = 0; j < AttemptedBuildingCaptureCount; j++)
    {
        auto attempt = AttemptedBuildingCaptures[j];

        if (attempt.BuildingLocation == building->PositionCell) {
            return j;
        }
    }

    return -1;
}

bool Should_Try_To_Capture(BuildingClass* building)
{
    BuildingTypeClassExtension* btypeext = Extension::Fetch(building->Class);

    if (btypeext->ProduceCashAmount > 0 && (building->House->Class->IsMultiplayPassive || !Session.Options.CrapEngineers)) {
        return true;
    }

    if (building->Class->SuperWeapon != SUPER_NONE && building->House->Class->IsMultiplayPassive) {
        return true;
    }

    return false;
}

bool HouseClassExtension::Is_Valid_Building_For_Capturing(BuildingClass* building, Cell zonecell, bool & okintheory) const
{
    okintheory = false;

    BuildingTypeClassExtension* btypeext = Extension::Fetch(building->Class);

    if (building->IsActive && !building->IsInLimbo && building->IsDown &&
        Should_Try_To_Capture(building) &&
        building->Class->IsCaptureable &&
        !This()->Is_Ally(building->House) &&
        Map.Is_Same_Zone(zonecell, building->PositionCell))
    {
        okintheory = true;

        if (Map.Cell_Threat(building->Center_Coord().As_Cell(), This()) > 0) {
            return false;
        }

        // Check if we haven't attempted to capture this building recently. 
        // If we have, then don't try again - our engineer is either heading there or died on the way,
        // meaning trying to capture it is currently too dangerous.
        int attemptindex = Get_Building_Capture_Attempt_Index_For(building);
        if (attemptindex > -1) {
            auto attempt = AttemptedBuildingCaptures[attemptindex];

            if (AttemptedBuildingCaptures[attemptindex].Frame > Frame - 20000) {
                return false;
            }
        }

        return true;
    }

    return false;
}

void HouseClassExtension::Add_Building_Capture_Attempt(BuildingClass* building)
{
    if (AttemptedBuildingCaptureCount >= std::size(AttemptedBuildingCaptures))
    {
        AttemptedBuildingCaptures[0] = AttemptedBuildingCaptureStruct(building->PositionCell, Frame);
    }
    else
    {
        AttemptedBuildingCaptures[AttemptedBuildingCaptureCount] = AttemptedBuildingCaptureStruct(building->PositionCell, Frame);
        AttemptedBuildingCaptureCount++;
    }
}

bool HouseClassExtension::AdvAI_Is_Recently_Attacked() const
{
    return This()->LATime > 0 && This()->LATime + TICKS_PER_MINUTE > Frame;
}

bool HouseClassExtension::Has_One_Of(DynamicVectorClass<BuildingTypeClass*> buildingtypes) const
{
    for (int i = 0; i < buildingtypes.Count(); i++)
    {
        if (This()->ActiveBQuantity.Value(buildingtypes[i]->HeapID) > 0)
            return true;
    }

    return false;
}

bool HouseClassExtension::Has_Barracks() const
{
    return Has_One_Of(Rule->BuildBarracks);
}

bool HouseClassExtension::Has_War_Factory() const
{
    return Has_One_Of(Rule->BuildWeapons);
}

bool HouseClassExtension::Has_Naval_Yard() const
{
    return Has_One_Of(RuleExtension->BuildNavalYard);
}

bool HouseClassExtension::Has_Helipad() const
{
    return Has_One_Of(Rule->BuildHelipad);
}

bool HouseClassExtension::Has_Construction_Yard() const
{
    return Has_One_Of(Rule->BuildConst);
}

bool HouseClassExtension::Has_Radar() const
{
    return Has_One_Of(Rule->BuildRadar);
}

bool HouseClassExtension::Has_Tech_Center() const
{
    return Has_One_Of(Rule->BuildTech);
}
