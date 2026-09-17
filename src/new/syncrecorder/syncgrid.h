/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Lossless, row-compressed integer grids for desync snapshots.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2026 Vinifera contributors
 ******************************************************************************/

#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>


namespace SyncGrid
{
    using Value = std::int64_t;
    constexpr Value Missing = (std::numeric_limits<Value>::min)();
    constexpr Value Unavailable = Missing + 1;

    /**
     * Each line expands to WIDTH values, starting at x=0. Consecutive identical
     * rows share an inclusive y range. Values are decimal; value*count repeats
     * a value horizontally. '.' denotes a missing cell, '?' unavailable data.
     *
     * Keep only two rows on the stack, regardless of map size. There are no
     * dictionaries whose indices change when just one cell differs in a peer's
     * snapshot. The reader must be read-only: it is called once per coordinate.
     */
    template<std::size_t WIDTH, typename Reader>
    void Print(FILE* fp, const char* name, int height, Reader read)
    {
        static_assert(WIDTH > 0, "A sync grid must have a nonzero width.");
        std::fprintf(fp, "grid %s width=%zu height=%d\n", name, WIDTH, height);

        std::array<Value, WIDTH> previous{};
        std::array<Value, WIDTH> current{};
        auto print_rows = [&](int first, int last) {
            std::fprintf(fp, "y=%d", first);
            if (last != first) std::fprintf(fp, "-%d", last);
            std::fputc(':', fp);
            for (std::size_t x = 0; x < WIDTH;) {
                const Value value = previous[x];
                std::size_t end = x + 1;
                while (end < WIDTH && previous[end] == value) ++end;

                if (value == Missing) {
                    std::fprintf(fp, " .");
                } else if (value == Unavailable) {
                    std::fprintf(fp, " ?");
                } else {
                    std::fprintf(fp, " %lld", static_cast<long long>(value));
                }
                if (end - x > 1) std::fprintf(fp, "*%zu", end - x);
                x = end;
            }
            std::fputc('\n', fp);
        };

        int first = 0;
        for (int y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < WIDTH; ++x) {
                current[x] = read(static_cast<int>(x), y);
            }
            if (y != 0 && current != previous) {
                print_rows(first, y - 1);
                first = y;
            }
            previous = current;
        }
        if (height > 0) print_rows(first, height - 1);
        std::fprintf(fp, "endgrid\n");
    }
}
