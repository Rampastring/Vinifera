/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Read-only cell and pathfinding snapshots for desync debugging.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2026 Vinifera contributors
 ******************************************************************************/

#include "always.h"
#include "syncrecorder.h"
#include "syncgrid.h"

#include "cell.h"
#include "mouse.h"
#include "object.h"
#include "tibsun_globals.h"


namespace
{
    /** Never use Map[cell]: accessing an absent cell can allocate it. */
    const CellClass* Stored_Cell(int x, int y)
    {
        const int index = x + y * MAP_CELL_W;
        if (index >= Map.Array.Length()) return nullptr;
        const CellClass* cell = Map.Array[index];
        return cell == &BlubCell ? nullptr : cell;
    }

    /**
     * Zone caches use a different stride from Map.Array. Calculate the index
     * directly, without calling any engine pathfinding or terrain functions.
     */
    int Stored_Zone_Index(int x, int y)
    {
        const auto stride = static_cast<std::int64_t>(Map.PlayRect.Width) + Map.PlayRect.Height + 1;
        if (x >= stride || y >= stride) return -1;
        const auto index = x + y * stride;
        return index >= 0 && index < Map.CellZoneCount ? static_cast<int>(index) : -1;
    }

    template<typename Reader>
    void Print_Cells(FILE* fp, const char* name, Reader read)
    {
        SyncGrid::Print<MAP_CELL_W>(fp, name, MAP_CELL_H, [&](int x, int y) -> SyncGrid::Value {
            const CellClass* cell = Stored_Cell(x, y);
            return cell ? static_cast<SyncGrid::Value>(read(*cell, x, y)) : SyncGrid::Missing;
        });
    }

    template<typename Reader>
    void Print_Zones(FILE* fp, const char* name, Reader read)
    {
        Print_Cells(fp, name, [&](const CellClass&, int x, int y) -> SyncGrid::Value {
            const int index = Stored_Zone_Index(x, y);
            return index >= 0 && Map.CellZones ? read(Map.CellZones[index]) : SyncGrid::Unavailable;
        });
    }

    template<typename Reader>
    void Print_Subzones(FILE* fp, const char* name, Reader read)
    {
        Print_Cells(fp, name, [&](const CellClass&, int x, int y) -> SyncGrid::Value {
            const int index = Stored_Zone_Index(x, y);
            return index >= 0 && Map.CellSubzones ? read(Map.CellSubzones[index]) : SyncGrid::Unavailable;
        });
    }

    void Print_Occupiers(FILE* fp, const ObjectClass* object)
    {
        // Preserve linked-list order, but bound traversal if a list is corrupt.
        // Do not serialize pointer addresses: they naturally differ on peers.
        int count = 0;
        while (object && count < 256) {
            if (count != 0) std::fputc(',', fp);
            std::fprintf(fp, "%d:%d", static_cast<int>(object->Fetch_RTTI()), object->ID);
            object = object->Next;
            ++count;
        }
        if (object) std::fprintf(fp, ",TRUNCATED");
        if (count == 0) std::fputc('-', fp);
    }
}


/**
 * Capture stored simulation state only. In particular, do NOT recalculate
 * attributes, load tile art, reset map iterators, or query/rebuild pathfinding:
 * a stale Land/Passability/zone value is exactly what this snapshot must retain.
 * Visibility, lighting, redraw bits and path search scratch flags are excluded.
 */
void SyncRecorder::Print_Cell_State(FILE* fp)
{
    std::fprintf(fp, "--- BEGIN CELL PATHFINDING STATE v1 ---\n");
    std::fprintf(fp, "SnapshotFrame=%ld ArrayLength=%d PlayRect=%d,%d,%d,%d CellZoneCount=%d ZoneCount=%d\n",
        Frame, Map.Array.Length(), Map.PlayRect.X, Map.PlayRect.Y, Map.PlayRect.Width, Map.PlayRect.Height,
        Map.CellZoneCount, Map.ZoneCount);
    std::fprintf(fp,
        "Format: each grid covers all 512x512 array coordinates; x starts at 0 on every row.\n"
        "y=a-b repeats the row for inclusive y coordinates a..b. value*n repeats n columns.\n"
        "All numbers are decimal; .=absent/dummy cell, ?=unavailable cache, -1 is a real value.\n"
        "Example y=7-9: .*2 0*3 2 ... means x=0..1 absent, x=2..4 clear, x=5 water on rows 7..9.\n"
        "Land: 0=clear 1=road 2=water 3=rock 4=wall 5=tiberium 6=beach 7=rough 8=ice 9=railroad 10=tunnel 11=weeds.\n"
        "Passability: 0=land 1=crush 2=blocked 3=water 4=partially_blocked 5=no 6=outside.\n"
        "BridgeFlags bits: 1=IsBridgeOwner 2=IsUnderBridge 4=Bit2_32(traversable) 8=WasUnderBridge\n"
        "  16=IsBridgeEastWest 32=Bit3_1(surface) 64=IsBridgeEndDamaged. Low bridges also use Overlay/OverlayData.\n"
        "Occupancy bits: 1=center 2=NW 4=NE 8=SW 16=SE 32=vehicle 64=monolith 128=building.\n"
        "BridgeOwnerCell: -1=null, otherwise x+y*512. Owners are HousesType indices; masks are unsigned house bits.\n"
        "ITType/SubTile and Overlay/OverlayData are raw type/frame indices. Tube=-1 means no tube.\n"
        "IceFlags: low byte=IsIceGrowthAllowed, bit 256=IsToGrowIce. Zone/Subzone fields are stored caches, not recomputed.\n");

    // Separate planes compress uniform terrain and empty occupancy independently.
    // Keep full coverage: a stale cell need not be close to the diverging units.
#define CELL_GRID(field) Print_Cells(fp, #field, [](const CellClass& c, int, int) { return c.field; })
    CELL_GRID(Land);
    CELL_GRID(Passability);
    CELL_GRID(ITType);
    CELL_GRID(SubTile);
    CELL_GRID(Height);
    CELL_GRID(Ramp);
    CELL_GRID(Overlay);
    CELL_GRID(OverlayData);
    CELL_GRID(Tube);
    CELL_GRID(Owner);
    CELL_GRID(InfType);
    CELL_GRID(AltInfType);
    CELL_GRID(OccupiedBy);
    CELL_GRID(CloakedBy);
    CELL_GRID(SensedBy);
    CELL_GRID(AdjacentObjectCount);
#undef CELL_GRID

    Print_Cells(fp, "Occupancy", [](const CellClass& c, int, int) { return c.Flag.Composite; });
    Print_Cells(fp, "AltOccupancy", [](const CellClass& c, int, int) { return c.AltFlag.Composite; });
    Print_Cells(fp, "BridgeFlags", [](const CellClass& c, int, int) {
        return c.IsBridgeOwner | (c.IsUnderBridge << 1) | (c.Bit2_32 << 2) | (c.WasUnderBridge << 3)
            | (c.IsBridgeEastWest << 4) | (c.Bit3_1 << 5) | (c.IsBridgeEndDamaged << 6);
    });
    Print_Cells(fp, "BridgeOwnerCell", [](const CellClass& c, int, int) {
        return c.BridgeOwner ? c.BridgeOwner->CellID.X + c.BridgeOwner->CellID.Y * MAP_CELL_W : -1;
    });
    Print_Cells(fp, "IceFlags", [](const CellClass& c, int, int) {
        return static_cast<unsigned>(c.IsIceGrowthAllowed) | (c.IsToGrowIce << 8);
    });

    Print_Zones(fp, "Zone.Passability", [](const CellZoneStruct& z) { return z.Passability; });
    Print_Zones(fp, "Zone.Height", [](const CellZoneStruct& z) { return z.Height; });
    Print_Zones(fp, "Zone.ID", [](const CellZoneStruct& z) { return z.ZoneID; });
    Print_Subzones(fp, "Subzone.ZoneID", [](const CellSubzoneStruct& z) { return z.ZoneID; });
    Print_Subzones(fp, "Subzone.Height", [](const CellSubzoneStruct& z) { return z.Height; });
    Print_Subzones(fp, "Subzone.Fine", [](const CellSubzoneStruct& z) { return z.SubzoneID[SUBZONE_FINE]; });
    Print_Subzones(fp, "Subzone.Rough", [](const CellSubzoneStruct& z) { return z.SubzoneID[SUBZONE_ROUGH]; });
    Print_Subzones(fp, "Subzone.Coarse", [](const CellSubzoneStruct& z) { return z.SubzoneID[SUBZONE_COARSE]; });

    std::fprintf(fp, "Occupiers: sparse x,y ground=RTTI:ID,... bridge=RTTI:ID,... in linked-list order; -=empty.\n");
    for (int y = 0; y < MAP_CELL_H; ++y) {
        for (int x = 0; x < MAP_CELL_W; ++x) {
            const CellClass* cell = Stored_Cell(x, y);
            if (!cell || (!cell->OccupierPtr && !cell->AltOccupierPtr)) continue;
            std::fprintf(fp, "occupiers %d,%d ground=", x, y);
            Print_Occupiers(fp, cell->OccupierPtr);
            std::fprintf(fp, " bridge=");
            Print_Occupiers(fp, cell->AltOccupierPtr);
            std::fputc('\n', fp);
        }
    }

    // These ordered queues help distinguish pending terrain updates from stale
    // caches. Their order matters; do not sort them just to simplify a diff.
    std::fprintf(fp, "PendingBridgeCells:");
    for (int i = 0; i < Map.PendingBridgeCells.Count(); ++i) {
        const Cell& cell = Map.PendingBridgeCells[i];
        std::fprintf(fp, " %d,%d", cell.X, cell.Y);
    }
    std::fprintf(fp, "\nDirtyIceCells:");
    for (int i = 0; i < Map.DirtyIceCells.Count(); ++i) {
        const Cell& cell = Map.DirtyIceCells[i];
        std::fprintf(fp, " %d,%d", cell.X, cell.Y);
    }
    std::fprintf(fp, "\n--- END CELL PATHFINDING STATE v1 ---\n\n");
}
