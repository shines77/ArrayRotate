
#ifndef JSTD_FAST_DIV_H
#define JSTD_FAST_DIV_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <cstdbool>
#include <algorithm>

namespace jstd {

static const std::uint32_t kMaxDivTable = 256;

struct DivRatio {
    uint32_t ratio;
    uint32_t shift;
#if 0
    DivRatio(uint32_t _ratio = 0, uint32_t _shift = 0)
        : ratio(_ratio), shift(_shift) {}

    DivRatio(int _ratio, int _shift)
        : ratio((uint32_t)_ratio), shift((uint32_t)_shift) {}

    DivRatio(const DivRatio & src)
        : ratio(src.ratio), shift(src.shift) {}

    DivRatio & operator = (const DivRatio & rhs) {
        this->ratio = rhs.ratio;
        this->shift = rhs.shift;
    };
#endif
};

static const DivRatio div_ratio_tbl[kMaxDivTable] = {
    { 0xFFFFFFFF,  0 }, { 0xFFFFFFFF,  1 }, { 0xFFFFFFFF,  2 }, { 0xAAAAAAAB,  1 },
    { 0xFFFFFFFF,  3 }, { 0xCCCCCCCD,  2 }, { 0xAAAAAAAB,  2 }, { 0x92492493,  2 },
    { 0xFFFFFFFF,  4 }, { 0xE38E38E4,  3 }, { 0xCCCCCCCD,  3 }, { 0xBA2E8BA3,  3 },
    { 0xAAAAAAAB,  3 }, { 0x9D89D89E,  3 }, { 0x92492493,  3 }, { 0x88888889,  3 },
    { 0xFFFFFFFF,  5 }, { 0xF0F0F0F1,  4 }, { 0xE38E38E4,  4 }, { 0xD79435E6,  4 },
    { 0xCCCCCCCD,  4 }, { 0xC30C30C4,  4 }, { 0xBA2E8BA3,  4 }, { 0xB21642C9,  4 },
    { 0xAAAAAAAB,  4 }, { 0xA3D70A3E,  4 }, { 0x9D89D89E,  4 }, { 0x97B425EE,  4 },
    { 0x92492493,  4 }, { 0x8D3DCB09,  4 }, { 0x88888889,  4 }, { 0x84210843,  4 },
    { 0xFFFFFFFF,  6 }, { 0xF83E0F84,  5 }, { 0xF0F0F0F1,  5 }, { 0xEA0EA0EB,  5 },
    { 0xE38E38E4,  5 }, { 0xDD67C8A7,  5 }, { 0xD79435E6,  5 }, { 0xD20D20D3,  5 },
    { 0xCCCCCCCD,  5 }, { 0xC7CE0C7D,  5 }, { 0xC30C30C4,  5 }, { 0xBE82FA0C,  5 },
    { 0xBA2E8BA3,  5 }, { 0xB60B60B7,  5 }, { 0xB21642C9,  5 }, { 0xAE4C415D,  5 },
    { 0xAAAAAAAB,  5 }, { 0xA72F053A,  5 }, { 0xA3D70A3E,  5 }, { 0xA0A0A0A1,  5 },
    { 0x9D89D89E,  5 }, { 0x9A90E7DA,  5 }, { 0x97B425EE,  5 }, { 0x94F20950,  5 },
    { 0x92492493,  5 }, { 0x8FB823EF,  5 }, { 0x8D3DCB09,  5 }, { 0x8AD8F2FC,  5 },
    { 0x88888889,  5 }, { 0x864B8A7E,  5 }, { 0x84210843,  5 }, { 0x82082083,  5 },
    { 0xFFFFFFFF,  7 }, { 0xFC0FC0FD,  6 }, { 0xF83E0F84,  6 }, { 0xF4898D60,  6 },
    { 0xF0F0F0F1,  6 }, { 0xED7303B6,  6 }, { 0xEA0EA0EB,  6 }, { 0xE6C2B449,  6 },
    { 0xE38E38E4,  6 }, { 0xE070381D,  6 }, { 0xDD67C8A7,  6 }, { 0xDA740DA8,  6 },
    { 0xD79435E6,  6 }, { 0xD4C77B04,  6 }, { 0xD20D20D3,  6 }, { 0xCF6474A9,  6 },
    { 0xCCCCCCCD,  6 }, { 0xCA4587E7,  6 }, { 0xC7CE0C7D,  6 }, { 0xC565C87C,  6 },
    { 0xC30C30C4,  6 }, { 0xC0C0C0C1,  6 }, { 0xBE82FA0C,  6 }, { 0xBC52640C,  6 },
    { 0xBA2E8BA3,  6 }, { 0xB81702E1,  6 }, { 0xB60B60B7,  6 }, { 0xB40B40B5,  6 },
    { 0xB21642C9,  6 }, { 0xB02C0B03,  6 }, { 0xAE4C415D,  6 }, { 0xAC769185,  6 },
    { 0xAAAAAAAB,  6 }, { 0xA8E83F58,  6 }, { 0xA72F053A,  6 }, { 0xA57EB503,  6 },
    { 0xA3D70A3E,  6 }, { 0xA237C32C,  6 }, { 0xA0A0A0A1,  6 }, { 0x9F1165E8,  6 },
    { 0x9D89D89E,  6 }, { 0x9C09C09D,  6 }, { 0x9A90E7DA,  6 }, { 0x991F1A52,  6 },
    { 0x97B425EE,  6 }, { 0x964FDA6D,  6 }, { 0x94F20950,  6 }, { 0x939A85C5,  6 },
    { 0x92492493,  6 }, { 0x90FDBC0A,  6 }, { 0x8FB823EF,  6 }, { 0x8E78356E,  6 },
    { 0x8D3DCB09,  6 }, { 0x8C08C08D,  6 }, { 0x8AD8F2FC,  6 }, { 0x89AE408A,  6 },
    { 0x88888889,  6 }, { 0x8767AB60,  6 }, { 0x864B8A7E,  6 }, { 0x85340854,  6 },
    { 0x84210843,  6 }, { 0x83126E98,  6 }, { 0x82082083,  6 }, { 0x81020409,  6 },
    { 0xFFFFFFFF,  8 }, { 0xFE03F810,  7 }, { 0xFC0FC0FD,  7 }, { 0xFA232CF3,  7 },
    { 0xF83E0F84,  7 }, { 0xF6603D99,  7 }, { 0xF4898D60,  7 }, { 0xF2B9D649,  7 },
    { 0xF0F0F0F1,  7 }, { 0xEF2EB720,  7 }, { 0xED7303B6,  7 }, { 0xEBBDB2A6,  7 },
    { 0xEA0EA0EB,  7 }, { 0xE865AC7C,  7 }, { 0xE6C2B449,  7 }, { 0xE525982B,  7 },
    { 0xE38E38E4,  7 }, { 0xE1FC780F,  7 }, { 0xE070381D,  7 }, { 0xDEE95C4D,  7 },
    { 0xDD67C8A7,  7 }, { 0xDBEB61EF,  7 }, { 0xDA740DA8,  7 }, { 0xD901B204,  7 },
    { 0xD79435E6,  7 }, { 0xD62B80D7,  7 }, { 0xD4C77B04,  7 }, { 0xD3680D37,  7 },
    { 0xD20D20D3,  7 }, { 0xD0B69FCC,  7 }, { 0xCF6474A9,  7 }, { 0xCE168A78,  7 },
    { 0xCCCCCCCD,  7 }, { 0xCB8727C1,  7 }, { 0xCA4587E7,  7 }, { 0xC907DA4F,  7 },
    { 0xC7CE0C7D,  7 }, { 0xC6980C6A,  7 }, { 0xC565C87C,  7 }, { 0xC4372F86,  7 },
    { 0xC30C30C4,  7 }, { 0xC1E4BBD6,  7 }, { 0xC0C0C0C1,  7 }, { 0xBFA02FE9,  7 },
    { 0xBE82FA0C,  7 }, { 0xBD691048,  7 }, { 0xBC52640C,  7 }, { 0xBB3EE722,  7 },
    { 0xBA2E8BA3,  7 }, { 0xB92143FB,  7 }, { 0xB81702E1,  7 }, { 0xB70FBB5B,  7 },
    { 0xB60B60B7,  7 }, { 0xB509E68B,  7 }, { 0xB40B40B5,  7 }, { 0xB30F6353,  7 },
    { 0xB21642C9,  7 }, { 0xB11FD3B9,  7 }, { 0xB02C0B03,  7 }, { 0xAF3ADDC7,  7 },
    { 0xAE4C415D,  7 }, { 0xAD602B59,  7 }, { 0xAC769185,  7 }, { 0xAB8F69E3,  7 },
    { 0xAAAAAAAB,  7 }, { 0xA9C84A48,  7 }, { 0xA8E83F58,  7 }, { 0xA80A80A9,  7 },
    { 0xA72F053A,  7 }, { 0xA655C43A,  7 }, { 0xA57EB503,  7 }, { 0xA4A9CF1E,  7 },
    { 0xA3D70A3E,  7 }, { 0xA3065E40,  7 }, { 0xA237C32C,  7 }, { 0xA16B312F,  7 },
    { 0xA0A0A0A1,  7 }, { 0x9FD809FE,  7 }, { 0x9F1165E8,  7 }, { 0x9E4CAD24,  7 },
    { 0x9D89D89E,  7 }, { 0x9CC8E161,  7 }, { 0x9C09C09D,  7 }, { 0x9B4C6F9F,  7 },
    { 0x9A90E7DA,  7 }, { 0x99D722DB,  7 }, { 0x991F1A52,  7 }, { 0x9868C80A,  7 },
    { 0x97B425EE,  7 }, { 0x97012E03,  7 }, { 0x964FDA6D,  7 }, { 0x95A02569,  7 },
    { 0x94F20950,  7 }, { 0x94458095,  7 }, { 0x939A85C5,  7 }, { 0x92F11385,  7 },
    { 0x92492493,  7 }, { 0x91A2B3C5,  7 }, { 0x90FDBC0A,  7 }, { 0x905A3864,  7 },
    { 0x8FB823EF,  7 }, { 0x8F1779DA,  7 }, { 0x8E78356E,  7 }, { 0x8DDA5203,  7 },
    { 0x8D3DCB09,  7 }, { 0x8CA29C05,  7 }, { 0x8C08C08D,  7 }, { 0x8B70344B,  7 },
    { 0x8AD8F2FC,  7 }, { 0x8A42F871,  7 }, { 0x89AE408A,  7 }, { 0x891AC73B,  7 },
    { 0x88888889,  7 }, { 0x87F78088,  7 }, { 0x8767AB60,  7 }, { 0x86D90545,  7 },
    { 0x864B8A7E,  7 }, { 0x85BF3762,  7 }, { 0x85340854,  7 }, { 0x84A9F9C9,  7 },
    { 0x84210843,  7 }, { 0x83993053,  7 }, { 0x83126E98,  7 }, { 0x828CBFBF,  7 },
    { 0x82082083,  7 }, { 0x81848DA9,  7 }, { 0x81020409,  7 }, { 0x80808081,  7 },
};

static inline
std::uint32_t get_log2_int(std::size_t val)
{
    std::uint32_t kMaxByteLen = sizeof(std::size_t) * 8;
    std::uint32_t log2_i = 0;

    if ((val & (val - 1)) == 0) {
        while (val != 0) {
            val >>= 1;
            log2_i++;
        }
        return log2_i;
    }

    std::size_t power2 = 1;
    do {
        if (val > power2 && val < power2 * 2) {
            return log2_i;
        }
        power2 <<= 1;
        log2_i++;
    } while (log2_i < kMaxByteLen);

    return log2_i;
}

static inline
DivRatio get_div_ratio_u32(std::uint32_t modulo)
{
    DivRatio modRatio;
    std::uint32_t shift = get_log2_int(modulo);
    std::uint32_t ratio;
    if ((modulo & (modulo - 1)) == 0) {
        //ratio = (uint32_t)ceil((double)((uint64_t)1u << (32 + shift - 1)) / modulo);
        ratio = 0xFFFFFFFFul;
    }
    else {
        ratio = (uint32_t)ceil((double)((uint64_t)1u << (32 + shift)) / modulo);
    }
    modRatio.ratio = ratio;
    modRatio.shift = shift;
    return modRatio;
}

static inline
void genDivRatioTbl()
{
    DivRatio divRatioTbl[kMaxDivTable];
    for (std::uint32_t n = 0; n < kMaxDivTable; n++) {
        divRatioTbl[n] = get_div_ratio_u32(n);
    }

    printf("\n");
    printf("static const DivRatio div_ratio_tbl[kMaxDivTable] = {\n");
    for (std::uint32_t n = 0; n < kMaxDivTable; n++) {
        if ((n % 4) == 0) {
            printf("    ");
        }
        printf("{ 0x%08X, %2u },", divRatioTbl[n].ratio, divRatioTbl[n].shift);
        if ((n % 4) == 3) {
            printf("\n");
        }
        else {
            printf(" ");
        }
    }
    printf("};\n\n");
}

static inline
std::uint32_t fast_div_u32(std::uint32_t value, std::uint32_t divisor)
{
    if (divisor < kMaxDivTable) {
        DivRatio mt = div_ratio_tbl[divisor];
        std::uint32_t result = ((std::uint32_t)(((std::uint64_t)value * mt.ratio) >> 32U) >> mt.shift);
        return result;
    }
    else {
        return (value / divisor);
    }
}

} // namespace jstd

#endif // JSTD_FAST_DIV_H
