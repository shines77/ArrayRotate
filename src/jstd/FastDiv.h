
#ifndef JSTD_FAST_DIV_H
#define JSTD_FAST_DIV_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
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

#include "jstd/FastMod.h"

//
// See: https://www.cnblogs.com/shines77/p/4189074.html
//

namespace jstd {

static const std::uint32_t kMaxDivTable = 512;

struct DivRatio32 {
    uint32_t ratio;
    uint32_t shift;
#if 0
    DivRatio32(uint32_t _ratio = 0, uint32_t _shift = 0)
        : ratio(_ratio), shift(_shift) {}

    DivRatio32(int _ratio, int _shift)
        : ratio((uint32_t)_ratio), shift((uint32_t)_shift) {}

    DivRatio32(const DivRatio32 & src)
        : ratio(src.ratio), shift(src.shift) {}

    DivRatio32 & operator = (const DivRatio32 & rhs) {
        this->ratio = rhs.ratio;
        this->shift = rhs.shift;
        return *this;
    };
#endif
};

struct DivRatio64 {
    uint64_t ratio;
    uint32_t shift;
    uint32_t reserve;
#if 0
    DivRatio64(uint64_t _ratio = 0, uint32_t _shift = 0)
        : ratio(_ratio), shift(_shift), reserve(0) {}

    DivRatio64(int64_t _ratio, int32_t _shift)
        : ratio((uint64_t)_ratio), shift((uint32_t)_shift), reserve(0) {}

    DivRatio64(const DivRatio64 & src)
        : ratio(src.ratio), shift(src.shift), shift(src.reserve) {}

    DivRatio64 & operator = (const DivRatio64 & rhs) {
        this->ratio = rhs.ratio;
        this->shift = rhs.shift;
        return *this;
    };
#endif
};

static const DivRatio32 div_ratio_tbl32[kMaxDivTable] = {
    { 0x00000000,  0 }, { 0xFFFFFFFF,  0 }, { 0xFFFFFFFF,  1 }, { 0xAAAAAAAB,  1 },
    { 0xFFFFFFFF,  2 }, { 0xCCCCCCCD,  2 }, { 0xAAAAAAAB,  2 }, { 0x92492493,  2 },
    { 0xFFFFFFFF,  3 }, { 0xE38E38E4,  3 }, { 0xCCCCCCCD,  3 }, { 0xBA2E8BA3,  3 },
    { 0xAAAAAAAB,  3 }, { 0x9D89D89E,  3 }, { 0x92492493,  3 }, { 0x88888889,  3 },
    { 0xFFFFFFFF,  4 }, { 0xF0F0F0F1,  4 }, { 0xE38E38E4,  4 }, { 0xD79435E6,  4 },
    { 0xCCCCCCCD,  4 }, { 0xC30C30C4,  4 }, { 0xBA2E8BA3,  4 }, { 0xB21642C9,  4 },
    { 0xAAAAAAAB,  4 }, { 0xA3D70A3E,  4 }, { 0x9D89D89E,  4 }, { 0x97B425EE,  4 },
    { 0x92492493,  4 }, { 0x8D3DCB09,  4 }, { 0x88888889,  4 }, { 0x84210843,  4 },
    { 0xFFFFFFFF,  5 }, { 0xF83E0F84,  5 }, { 0xF0F0F0F1,  5 }, { 0xEA0EA0EB,  5 },
    { 0xE38E38E4,  5 }, { 0xDD67C8A7,  5 }, { 0xD79435E6,  5 }, { 0xD20D20D3,  5 },
    { 0xCCCCCCCD,  5 }, { 0xC7CE0C7D,  5 }, { 0xC30C30C4,  5 }, { 0xBE82FA0C,  5 },
    { 0xBA2E8BA3,  5 }, { 0xB60B60B7,  5 }, { 0xB21642C9,  5 }, { 0xAE4C415D,  5 },
    { 0xAAAAAAAB,  5 }, { 0xA72F053A,  5 }, { 0xA3D70A3E,  5 }, { 0xA0A0A0A1,  5 },
    { 0x9D89D89E,  5 }, { 0x9A90E7DA,  5 }, { 0x97B425EE,  5 }, { 0x94F20950,  5 },
    { 0x92492493,  5 }, { 0x8FB823EF,  5 }, { 0x8D3DCB09,  5 }, { 0x8AD8F2FC,  5 },
    { 0x88888889,  5 }, { 0x864B8A7E,  5 }, { 0x84210843,  5 }, { 0x82082083,  5 },
    { 0xFFFFFFFF,  6 }, { 0xFC0FC0FD,  6 }, { 0xF83E0F84,  6 }, { 0xF4898D60,  6 },
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
    { 0xFFFFFFFF,  7 }, { 0xFE03F810,  7 }, { 0xFC0FC0FD,  7 }, { 0xFA232CF3,  7 },
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
    { 0xFFFFFFFF,  8 }, { 0xFF00FF01,  8 }, { 0xFE03F810,  8 }, { 0xFD08E551,  8 },
    { 0xFC0FC0FD,  8 }, { 0xFB188566,  8 }, { 0xFA232CF3,  8 }, { 0xF92FB222,  8 },
    { 0xF83E0F84,  8 }, { 0xF74E3FC3,  8 }, { 0xF6603D99,  8 }, { 0xF57403D6,  8 },
    { 0xF4898D60,  8 }, { 0xF3A0D52D,  8 }, { 0xF2B9D649,  8 }, { 0xF1D48BCF,  8 },
    { 0xF0F0F0F1,  8 }, { 0xF00F00F1,  8 }, { 0xEF2EB720,  8 }, { 0xEE500EE6,  8 },
    { 0xED7303B6,  8 }, { 0xEC979119,  8 }, { 0xEBBDB2A6,  8 }, { 0xEAE56404,  8 },
    { 0xEA0EA0EB,  8 }, { 0xE9396520,  8 }, { 0xE865AC7C,  8 }, { 0xE79372E3,  8 },
    { 0xE6C2B449,  8 }, { 0xE5F36CB1,  8 }, { 0xE525982B,  8 }, { 0xE45932D8,  8 },
    { 0xE38E38E4,  8 }, { 0xE2C4A689,  8 }, { 0xE1FC780F,  8 }, { 0xE135A9CA,  8 },
    { 0xE070381D,  8 }, { 0xDFAC1F75,  8 }, { 0xDEE95C4D,  8 }, { 0xDE27EB2D,  8 },
    { 0xDD67C8A7,  8 }, { 0xDCA8F159,  8 }, { 0xDBEB61EF,  8 }, { 0xDB2F171E,  8 },
    { 0xDA740DA8,  8 }, { 0xD9BA4257,  8 }, { 0xD901B204,  8 }, { 0xD84A598F,  8 },
    { 0xD79435E6,  8 }, { 0xD6DF43FD,  8 }, { 0xD62B80D7,  8 }, { 0xD578E97D,  8 },
    { 0xD4C77B04,  8 }, { 0xD417328A,  8 }, { 0xD3680D37,  8 }, { 0xD2BA083C,  8 },
    { 0xD20D20D3,  8 }, { 0xD161543F,  8 }, { 0xD0B69FCC,  8 }, { 0xD00D00D1,  8 },
    { 0xCF6474A9,  8 }, { 0xCEBCF8BC,  8 }, { 0xCE168A78,  8 }, { 0xCD712753,  8 },
    { 0xCCCCCCCD,  8 }, { 0xCC29786D,  8 }, { 0xCB8727C1,  8 }, { 0xCAE5D860,  8 },
    { 0xCA4587E7,  8 }, { 0xC9A633FD,  8 }, { 0xC907DA4F,  8 }, { 0xC86A7891,  8 },
    { 0xC7CE0C7D,  8 }, { 0xC73293D8,  8 }, { 0xC6980C6A,  8 }, { 0xC5FE7404,  8 },
    { 0xC565C87C,  8 }, { 0xC4CE07B1,  8 }, { 0xC4372F86,  8 }, { 0xC3A13DE7,  8 },
    { 0xC30C30C4,  8 }, { 0xC2780614,  8 }, { 0xC1E4BBD6,  8 }, { 0xC152500D,  8 },
    { 0xC0C0C0C1,  8 }, { 0xC0300C04,  8 }, { 0xBFA02FE9,  8 }, { 0xBF112A8B,  8 },
    { 0xBE82FA0C,  8 }, { 0xBDF59C92,  8 }, { 0xBD691048,  8 }, { 0xBCDD535E,  8 },
    { 0xBC52640C,  8 }, { 0xBBC8408D,  8 }, { 0xBB3EE722,  8 }, { 0xBAB65611,  8 },
    { 0xBA2E8BA3,  8 }, { 0xB9A7862B,  8 }, { 0xB92143FB,  8 }, { 0xB89BC36D,  8 },
    { 0xB81702E1,  8 }, { 0xB79300B8,  8 }, { 0xB70FBB5B,  8 }, { 0xB68D3135,  8 },
    { 0xB60B60B7,  8 }, { 0xB58A4856,  8 }, { 0xB509E68B,  8 }, { 0xB48A39D5,  8 },
    { 0xB40B40B5,  8 }, { 0xB38CF9B1,  8 }, { 0xB30F6353,  8 }, { 0xB2927C2A,  8 },
    { 0xB21642C9,  8 }, { 0xB19AB5C5,  8 }, { 0xB11FD3B9,  8 }, { 0xB0A59B42,  8 },
    { 0xB02C0B03,  8 }, { 0xAFB321A2,  8 }, { 0xAF3ADDC7,  8 }, { 0xAEC33E20,  8 },
    { 0xAE4C415D,  8 }, { 0xADD5E633,  8 }, { 0xAD602B59,  8 }, { 0xACEB0F8A,  8 },
    { 0xAC769185,  8 }, { 0xAC02B00B,  8 }, { 0xAB8F69E3,  8 }, { 0xAB1CBDD4,  8 },
    { 0xAAAAAAAB,  8 }, { 0xAA392F36,  8 }, { 0xA9C84A48,  8 }, { 0xA957FAB6,  8 },
    { 0xA8E83F58,  8 }, { 0xA8791709,  8 }, { 0xA80A80A9,  8 }, { 0xA79C7B17,  8 },
    { 0xA72F053A,  8 }, { 0xA6C21DF7,  8 }, { 0xA655C43A,  8 }, { 0xA5E9F6EE,  8 },
    { 0xA57EB503,  8 }, { 0xA513FD6C,  8 }, { 0xA4A9CF1E,  8 }, { 0xA4402911,  8 },
    { 0xA3D70A3E,  8 }, { 0xA36E71A3,  8 }, { 0xA3065E40,  8 }, { 0xA29ECF17,  8 },
    { 0xA237C32C,  8 }, { 0xA1D13986,  8 }, { 0xA16B312F,  8 }, { 0xA105A933,  8 },
    { 0xA0A0A0A1,  8 }, { 0xA03C1689,  8 }, { 0x9FD809FE,  8 }, { 0x9F747A16,  8 },
    { 0x9F1165E8,  8 }, { 0x9EAECC8E,  8 }, { 0x9E4CAD24,  8 }, { 0x9DEB06CA,  8 },
    { 0x9D89D89E,  8 }, { 0x9D2921C4,  8 }, { 0x9CC8E161,  8 }, { 0x9C69169C,  8 },
    { 0x9C09C09D,  8 }, { 0x9BAADE8F,  8 }, { 0x9B4C6F9F,  8 }, { 0x9AEE72FD,  8 },
    { 0x9A90E7DA,  8 }, { 0x9A33CD68,  8 }, { 0x99D722DB,  8 }, { 0x997AE76C,  8 },
    { 0x991F1A52,  8 }, { 0x98C3BAC8,  8 }, { 0x9868C80A,  8 }, { 0x980E4157,  8 },
    { 0x97B425EE,  8 }, { 0x975A7510,  8 }, { 0x97012E03,  8 }, { 0x96A8500A,  8 },
    { 0x964FDA6D,  8 }, { 0x95F7CC73,  8 }, { 0x95A02569,  8 }, { 0x9548E498,  8 },
    { 0x94F20950,  8 }, { 0x949B92DE,  8 }, { 0x94458095,  8 }, { 0x93EFD1C6,  8 },
    { 0x939A85C5,  8 }, { 0x93459BE7,  8 }, { 0x92F11385,  8 }, { 0x929CEBF5,  8 },
    { 0x92492493,  8 }, { 0x91F5BCB9,  8 }, { 0x91A2B3C5,  8 }, { 0x91500916,  8 },
    { 0x90FDBC0A,  8 }, { 0x90ABCC03,  8 }, { 0x905A3864,  8 }, { 0x90090091,  8 },
    { 0x8FB823EF,  8 }, { 0x8F67A1E4,  8 }, { 0x8F1779DA,  8 }, { 0x8EC7AB3A,  8 },
    { 0x8E78356E,  8 }, { 0x8E2917E1,  8 }, { 0x8DDA5203,  8 }, { 0x8D8BE340,  8 },
    { 0x8D3DCB09,  8 }, { 0x8CF008D0,  8 }, { 0x8CA29C05,  8 }, { 0x8C55841D,  8 },
    { 0x8C08C08D,  8 }, { 0x8BBC50C9,  8 }, { 0x8B70344B,  8 }, { 0x8B246A88,  8 },
    { 0x8AD8F2FC,  8 }, { 0x8A8DCD20,  8 }, { 0x8A42F871,  8 }, { 0x89F8746A,  8 },
    { 0x89AE408A,  8 }, { 0x89645C50,  8 }, { 0x891AC73B,  8 }, { 0x88D180CE,  8 },
    { 0x88888889,  8 }, { 0x883FDDF1,  8 }, { 0x87F78088,  8 }, { 0x87AF6FD6,  8 },
    { 0x8767AB60,  8 }, { 0x872032AD,  8 }, { 0x86D90545,  8 }, { 0x869222B2,  8 },
    { 0x864B8A7E,  8 }, { 0x86053C35,  8 }, { 0x85BF3762,  8 }, { 0x85797B92,  8 },
    { 0x85340854,  8 }, { 0x84EEDD36,  8 }, { 0x84A9F9C9,  8 }, { 0x84655D9C,  8 },
    { 0x84210843,  8 }, { 0x83DCF94E,  8 }, { 0x83993053,  8 }, { 0x8355ACE4,  8 },
    { 0x83126E98,  8 }, { 0x82CF7504,  8 }, { 0x828CBFBF,  8 }, { 0x824A4E61,  8 },
    { 0x82082083,  8 }, { 0x81C635BD,  8 }, { 0x81848DA9,  8 }, { 0x814327E4,  8 },
    { 0x81020409,  8 }, { 0x80C121B3,  8 }, { 0x80808081,  8 }, { 0x80402011,  8 },
};

static const DivRatio64 div_ratio_tbl64[kMaxDivTable] = {
    { 0x0000000000000000ull,  0, 0 }, { 0xFFFFFFFFFFFFFFFFull,  0, 0 },
    { 0xFFFFFFFFFFFFFFFFull,  1, 0 }, { 0xAAAAAAAAAAAAAAAAull,  1, 0 },
    { 0xFFFFFFFFFFFFFFFFull,  2, 0 }, { 0xCCCCCCCCCCCCCCCCull,  2, 0 },
    { 0xAAAAAAAAAAAAAAAAull,  2, 0 }, { 0x9249249249249249ull,  2, 0 },
    { 0xFFFFFFFFFFFFFFFFull,  3, 0 }, { 0xE38E38E38E38E38Eull,  3, 0 },
    { 0xCCCCCCCCCCCCCCCCull,  3, 0 }, { 0xBA2E8BA2E8BA2E8Bull,  3, 0 },
    { 0xAAAAAAAAAAAAAAAAull,  3, 0 }, { 0x9D89D89D89D89D89ull,  3, 0 },
    { 0x9249249249249249ull,  3, 0 }, { 0x8888888888888888ull,  3, 0 },
    { 0xFFFFFFFFFFFFFFFFull,  4, 0 }, { 0xF0F0F0F0F0F0F0F0ull,  4, 0 },
    { 0xE38E38E38E38E38Eull,  4, 0 }, { 0xD79435E50D79435Eull,  4, 0 },
    { 0xCCCCCCCCCCCCCCCCull,  4, 0 }, { 0xC30C30C30C30C30Cull,  4, 0 },
    { 0xBA2E8BA2E8BA2E8Bull,  4, 0 }, { 0xB21642C8590B2164ull,  4, 0 },
    { 0xAAAAAAAAAAAAAAAAull,  4, 0 }, { 0xA3D70A3D70A3D70Aull,  4, 0 },
    { 0x9D89D89D89D89D89ull,  4, 0 }, { 0x97B425ED097B425Eull,  4, 0 },
    { 0x9249249249249249ull,  4, 0 }, { 0x8D3DCB08D3DCB08Dull,  4, 0 },
    { 0x8888888888888888ull,  4, 0 }, { 0x8421084210842108ull,  4, 0 },
    { 0xFFFFFFFFFFFFFFFFull,  5, 0 }, { 0xF83E0F83E0F83E0Full,  5, 0 },
    { 0xF0F0F0F0F0F0F0F0ull,  5, 0 }, { 0xEA0EA0EA0EA0EA0Eull,  5, 0 },
    { 0xE38E38E38E38E38Eull,  5, 0 }, { 0xDD67C8A60DD67C8Aull,  5, 0 },
    { 0xD79435E50D79435Eull,  5, 0 }, { 0xD20D20D20D20D20Dull,  5, 0 },
    { 0xCCCCCCCCCCCCCCCCull,  5, 0 }, { 0xC7CE0C7CE0C7CE0Cull,  5, 0 },
    { 0xC30C30C30C30C30Cull,  5, 0 }, { 0xBE82FA0BE82FA0BEull,  5, 0 },
    { 0xBA2E8BA2E8BA2E8Bull,  5, 0 }, { 0xB60B60B60B60B60Bull,  5, 0 },
    { 0xB21642C8590B2164ull,  5, 0 }, { 0xAE4C415C9882B931ull,  5, 0 },
    { 0xAAAAAAAAAAAAAAAAull,  5, 0 }, { 0xA72F05397829CBC1ull,  5, 0 },
    { 0xA3D70A3D70A3D70Aull,  5, 0 }, { 0xA0A0A0A0A0A0A0A0ull,  5, 0 },
    { 0x9D89D89D89D89D89ull,  5, 0 }, { 0x9A90E7D95BC609A9ull,  5, 0 },
    { 0x97B425ED097B425Eull,  5, 0 }, { 0x94F2094F2094F209ull,  5, 0 },
    { 0x9249249249249249ull,  5, 0 }, { 0x8FB823EE08FB823Eull,  5, 0 },
    { 0x8D3DCB08D3DCB08Dull,  5, 0 }, { 0x8AD8F2FBA9386822ull,  5, 0 },
    { 0x8888888888888888ull,  5, 0 }, { 0x864B8A7DE6D1D608ull,  5, 0 },
    { 0x8421084210842108ull,  5, 0 }, { 0x8208208208208208ull,  5, 0 },
    { 0xFFFFFFFFFFFFFFFFull,  6, 0 }, { 0xFC0FC0FC0FC0FC0Full,  6, 0 },
    { 0xF83E0F83E0F83E0Full,  6, 0 }, { 0xF4898D5F85BB3950ull,  6, 0 },
    { 0xF0F0F0F0F0F0F0F0ull,  6, 0 }, { 0xED7303B5CC0ED730ull,  6, 0 },
    { 0xEA0EA0EA0EA0EA0Eull,  6, 0 }, { 0xE6C2B4481CD85689ull,  6, 0 },
    { 0xE38E38E38E38E38Eull,  6, 0 }, { 0xE070381C0E070381ull,  6, 0 },
    { 0xDD67C8A60DD67C8Aull,  6, 0 }, { 0xDA740DA740DA740Dull,  6, 0 },
    { 0xD79435E50D79435Eull,  6, 0 }, { 0xD4C77B03531DEC0Dull,  6, 0 },
    { 0xD20D20D20D20D20Dull,  6, 0 }, { 0xCF6474A8819EC8E9ull,  6, 0 },
    { 0xCCCCCCCCCCCCCCCCull,  6, 0 }, { 0xCA4587E6B74F0329ull,  6, 0 },
    { 0xC7CE0C7CE0C7CE0Cull,  6, 0 }, { 0xC565C87B5F9D4D1Bull,  6, 0 },
    { 0xC30C30C30C30C30Cull,  6, 0 }, { 0xC0C0C0C0C0C0C0C0ull,  6, 0 },
    { 0xBE82FA0BE82FA0BEull,  6, 0 }, { 0xBC52640BC52640BCull,  6, 0 },
    { 0xBA2E8BA2E8BA2E8Bull,  6, 0 }, { 0xB81702E05C0B8170ull,  6, 0 },
    { 0xB60B60B60B60B60Bull,  6, 0 }, { 0xB40B40B40B40B40Bull,  6, 0 },
    { 0xB21642C8590B2164ull,  6, 0 }, { 0xB02C0B02C0B02C0Bull,  6, 0 },
    { 0xAE4C415C9882B931ull,  6, 0 }, { 0xAC7691840AC76918ull,  6, 0 },
    { 0xAAAAAAAAAAAAAAAAull,  6, 0 }, { 0xA8E83F5717C0A8E8ull,  6, 0 },
    { 0xA72F05397829CBC1ull,  6, 0 }, { 0xA57EB50295FAD40Aull,  6, 0 },
    { 0xA3D70A3D70A3D70Aull,  6, 0 }, { 0xA237C32B16CFD772ull,  6, 0 },
    { 0xA0A0A0A0A0A0A0A0ull,  6, 0 }, { 0x9F1165E7254813E2ull,  6, 0 },
    { 0x9D89D89D89D89D89ull,  6, 0 }, { 0x9C09C09C09C09C09ull,  6, 0 },
    { 0x9A90E7D95BC609A9ull,  6, 0 }, { 0x991F1A515885FB37ull,  6, 0 },
    { 0x97B425ED097B425Eull,  6, 0 }, { 0x964FDA6C0964FDA6ull,  6, 0 },
    { 0x94F2094F2094F209ull,  6, 0 }, { 0x939A85C40939A85Cull,  6, 0 },
    { 0x9249249249249249ull,  6, 0 }, { 0x90FDBC090FDBC090ull,  6, 0 },
    { 0x8FB823EE08FB823Eull,  6, 0 }, { 0x8E78356D1408E783ull,  6, 0 },
    { 0x8D3DCB08D3DCB08Dull,  6, 0 }, { 0x8C08C08C08C08C08ull,  6, 0 },
    { 0x8AD8F2FBA9386822ull,  6, 0 }, { 0x89AE4089AE4089AEull,  6, 0 },
    { 0x8888888888888888ull,  6, 0 }, { 0x8767AB5F34E47EF1ull,  6, 0 },
    { 0x864B8A7DE6D1D608ull,  6, 0 }, { 0x8534085340853408ull,  6, 0 },
    { 0x8421084210842108ull,  6, 0 }, { 0x83126E978D4FDF3Bull,  6, 0 },
    { 0x8208208208208208ull,  6, 0 }, { 0x8102040810204081ull,  6, 0 },
    { 0xFFFFFFFFFFFFFFFFull,  7, 0 }, { 0xFE03F80FE03F80FEull,  7, 0 },
    { 0xFC0FC0FC0FC0FC0Full,  7, 0 }, { 0xFA232CF252138ABFull,  7, 0 },
    { 0xF83E0F83E0F83E0Full,  7, 0 }, { 0xF6603D980F6603D9ull,  7, 0 },
    { 0xF4898D5F85BB3950ull,  7, 0 }, { 0xF2B9D6480F2B9D64ull,  7, 0 },
    { 0xF0F0F0F0F0F0F0F0ull,  7, 0 }, { 0xEF2EB71FC4345238ull,  7, 0 },
    { 0xED7303B5CC0ED730ull,  7, 0 }, { 0xEBBDB2A5C1619C8Bull,  7, 0 },
    { 0xEA0EA0EA0EA0EA0Eull,  7, 0 }, { 0xE865AC7B7603A196ull,  7, 0 },
    { 0xE6C2B4481CD85689ull,  7, 0 }, { 0xE525982AF70C880Eull,  7, 0 },
    { 0xE38E38E38E38E38Eull,  7, 0 }, { 0xE1FC780E1FC780E1ull,  7, 0 },
    { 0xE070381C0E070381ull,  7, 0 }, { 0xDEE95C4CA037BA57ull,  7, 0 },
    { 0xDD67C8A60DD67C8Aull,  7, 0 }, { 0xDBEB61EED19C5957ull,  7, 0 },
    { 0xDA740DA740DA740Dull,  7, 0 }, { 0xD901B2036406C80Dull,  7, 0 },
    { 0xD79435E50D79435Eull,  7, 0 }, { 0xD62B80D62B80D62Bull,  7, 0 },
    { 0xD4C77B03531DEC0Dull,  7, 0 }, { 0xD3680D3680D3680Dull,  7, 0 },
    { 0xD20D20D20D20D20Dull,  7, 0 }, { 0xD0B69FCBD2580D0Bull,  7, 0 },
    { 0xCF6474A8819EC8E9ull,  7, 0 }, { 0xCE168A7725080CE1ull,  7, 0 },
    { 0xCCCCCCCCCCCCCCCCull,  7, 0 }, { 0xCB8727C065C393E0ull,  7, 0 },
    { 0xCA4587E6B74F0329ull,  7, 0 }, { 0xC907DA4E871146ACull,  7, 0 },
    { 0xC7CE0C7CE0C7CE0Cull,  7, 0 }, { 0xC6980C6980C6980Cull,  7, 0 },
    { 0xC565C87B5F9D4D1Bull,  7, 0 }, { 0xC4372F855D824CA5ull,  7, 0 },
    { 0xC30C30C30C30C30Cull,  7, 0 }, { 0xC1E4BBD595F6E947ull,  7, 0 },
    { 0xC0C0C0C0C0C0C0C0ull,  7, 0 }, { 0xBFA02FE80BFA02FEull,  7, 0 },
    { 0xBE82FA0BE82FA0BEull,  7, 0 }, { 0xBD69104707661AA2ull,  7, 0 },
    { 0xBC52640BC52640BCull,  7, 0 }, { 0xBB3EE721A54D880Bull,  7, 0 },
    { 0xBA2E8BA2E8BA2E8Bull,  7, 0 }, { 0xB92143FA36F5E02Eull,  7, 0 },
    { 0xB81702E05C0B8170ull,  7, 0 }, { 0xB70FBB5A19BE3658ull,  7, 0 },
    { 0xB60B60B60B60B60Bull,  7, 0 }, { 0xB509E68A9B94821Full,  7, 0 },
    { 0xB40B40B40B40B40Bull,  7, 0 }, { 0xB30F63528917C80Bull,  7, 0 },
    { 0xB21642C8590B2164ull,  7, 0 }, { 0xB11FD3B80B11FD3Bull,  7, 0 },
    { 0xB02C0B02C0B02C0Bull,  7, 0 }, { 0xAF3ADDC680AF3ADDull,  7, 0 },
    { 0xAE4C415C9882B931ull,  7, 0 }, { 0xAD602B580AD602B5ull,  7, 0 },
    { 0xAC7691840AC76918ull,  7, 0 }, { 0xAB8F69E28359CD11ull,  7, 0 },
    { 0xAAAAAAAAAAAAAAAAull,  7, 0 }, { 0xA9C84A47A07F5637ull,  7, 0 },
    { 0xA8E83F5717C0A8E8ull,  7, 0 }, { 0xA80A80A80A80A80Aull,  7, 0 },
    { 0xA72F05397829CBC1ull,  7, 0 }, { 0xA655C4392D7B73A7ull,  7, 0 },
    { 0xA57EB50295FAD40Aull,  7, 0 }, { 0xA4A9CF1D96833751ull,  7, 0 },
    { 0xA3D70A3D70A3D70Aull,  7, 0 }, { 0xA3065E3FAE7CD0E0ull,  7, 0 },
    { 0xA237C32B16CFD772ull,  7, 0 }, { 0xA16B312EA8FC377Cull,  7, 0 },
    { 0xA0A0A0A0A0A0A0A0ull,  7, 0 }, { 0x9FD809FD809FD809ull,  7, 0 },
    { 0x9F1165E7254813E2ull,  7, 0 }, { 0x9E4CAD23DD5F3A20ull,  7, 0 },
    { 0x9D89D89D89D89D89ull,  7, 0 }, { 0x9CC8E160C3FB19B8ull,  7, 0 },
    { 0x9C09C09C09C09C09ull,  7, 0 }, { 0x9B4C6F9EF03A3CA9ull,  7, 0 },
    { 0x9A90E7D95BC609A9ull,  7, 0 }, { 0x99D722DABDE58F06ull,  7, 0 },
    { 0x991F1A515885FB37ull,  7, 0 }, { 0x9868C809868C8098ull,  7, 0 },
    { 0x97B425ED097B425Eull,  7, 0 }, { 0x97012E025C04B809ull,  7, 0 },
    { 0x964FDA6C0964FDA6ull,  7, 0 }, { 0x95A02568095A0256ull,  7, 0 },
    { 0x94F2094F2094F209ull,  7, 0 }, { 0x9445809445809445ull,  7, 0 },
    { 0x939A85C40939A85Cull,  7, 0 }, { 0x92F113840497889Cull,  7, 0 },
    { 0x9249249249249249ull,  7, 0 }, { 0x91A2B3C4D5E6F809ull,  7, 0 },
    { 0x90FDBC090FDBC090ull,  7, 0 }, { 0x905A38633E06C43Aull,  7, 0 },
    { 0x8FB823EE08FB823Eull,  7, 0 }, { 0x8F1779D9FDC3A218ull,  7, 0 },
    { 0x8E78356D1408E783ull,  7, 0 }, { 0x8DDA520237694808ull,  7, 0 },
    { 0x8D3DCB08D3DCB08Dull,  7, 0 }, { 0x8CA29C046514E023ull,  7, 0 },
    { 0x8C08C08C08C08C08ull,  7, 0 }, { 0x8B70344A139BC75Aull,  7, 0 },
    { 0x8AD8F2FBA9386822ull,  7, 0 }, { 0x8A42F8705669DB46ull,  7, 0 },
    { 0x89AE4089AE4089AEull,  7, 0 }, { 0x891AC73AE9819B50ull,  7, 0 },
    { 0x8888888888888888ull,  7, 0 }, { 0x87F78087F78087F7ull,  7, 0 },
    { 0x8767AB5F34E47EF1ull,  7, 0 }, { 0x86D905447A34ACC6ull,  7, 0 },
    { 0x864B8A7DE6D1D608ull,  7, 0 }, { 0x85BF37612CEE3C9Aull,  7, 0 },
    { 0x8534085340853408ull,  7, 0 }, { 0x84A9F9C8084A9F9Cull,  7, 0 },
    { 0x8421084210842108ull,  7, 0 }, { 0x839930523FBE3367ull,  7, 0 },
    { 0x83126E978D4FDF3Bull,  7, 0 }, { 0x828CBFBEB9A020A3ull,  7, 0 },
    { 0x8208208208208208ull,  7, 0 }, { 0x81848DA8FAF0D277ull,  7, 0 },
    { 0x8102040810204081ull,  7, 0 }, { 0x8080808080808080ull,  7, 0 },
    { 0xFFFFFFFFFFFFFFFFull,  8, 0 }, { 0xFF00FF00FF00FF00ull,  8, 0 },
    { 0xFE03F80FE03F80FEull,  8, 0 }, { 0xFD08E5500FD08E55ull,  8, 0 },
    { 0xFC0FC0FC0FC0FC0Full,  8, 0 }, { 0xFB18856506DDABA5ull,  8, 0 },
    { 0xFA232CF252138ABFull,  8, 0 }, { 0xF92FB2211855A865ull,  8, 0 },
    { 0xF83E0F83E0F83E0Full,  8, 0 }, { 0xF74E3FC22C700F74ull,  8, 0 },
    { 0xF6603D980F6603D9ull,  8, 0 }, { 0xF57403D5D00F5740ull,  8, 0 },
    { 0xF4898D5F85BB3950ull,  8, 0 }, { 0xF3A0D52CBA872336ull,  8, 0 },
    { 0xF2B9D6480F2B9D64ull,  8, 0 }, { 0xF1D48BCEE0D399FAull,  8, 0 },
    { 0xF0F0F0F0F0F0F0F0ull,  8, 0 }, { 0xF00F00F00F00F00Full,  8, 0 },
    { 0xEF2EB71FC4345238ull,  8, 0 }, { 0xEE500EE500EE500Eull,  8, 0 },
    { 0xED7303B5CC0ED730ull,  8, 0 }, { 0xEC979118F3FC4DA1ull,  8, 0 },
    { 0xEBBDB2A5C1619C8Bull,  8, 0 }, { 0xEAE56403AB95900Eull,  8, 0 },
    { 0xEA0EA0EA0EA0EA0Eull,  8, 0 }, { 0xE939651FE2D8D35Cull,  8, 0 },
    { 0xE865AC7B7603A196ull,  8, 0 }, { 0xE79372E225FE30D9ull,  8, 0 },
    { 0xE6C2B4481CD85689ull,  8, 0 }, { 0xE5F36CB00E5F36CBull,  8, 0 },
    { 0xE525982AF70C880Eull,  8, 0 }, { 0xE45932D7DC52100Eull,  8, 0 },
    { 0xE38E38E38E38E38Eull,  8, 0 }, { 0xE2C4A6886A4C2E0Full,  8, 0 },
    { 0xE1FC780E1FC780E1ull,  8, 0 }, { 0xE135A9C97500E135ull,  8, 0 },
    { 0xE070381C0E070381ull,  8, 0 }, { 0xDFAC1F74346C575Full,  8, 0 },
    { 0xDEE95C4CA037BA57ull,  8, 0 }, { 0xDE27EB2C41F3D9D1ull,  8, 0 },
    { 0xDD67C8A60DD67C8Aull,  8, 0 }, { 0xDCA8F158C7F91AB8ull,  8, 0 },
    { 0xDBEB61EED19C5957ull,  8, 0 }, { 0xDB2F171DF7702918ull,  8, 0 },
    { 0xDA740DA740DA740Dull,  8, 0 }, { 0xD9BA4256C0366E90ull,  8, 0 },
    { 0xD901B2036406C80Dull,  8, 0 }, { 0xD84A598EC9151F42ull,  8, 0 },
    { 0xD79435E50D79435Eull,  8, 0 }, { 0xD6DF43FCA482F00Dull,  8, 0 },
    { 0xD62B80D62B80D62Bull,  8, 0 }, { 0xD578E97C3F5FE550ull,  8, 0 },
    { 0xD4C77B03531DEC0Dull,  8, 0 }, { 0xD4173289870AC52Dull,  8, 0 },
    { 0xD3680D3680D3680Dull,  8, 0 }, { 0xD2BA083B445250ABull,  8, 0 },
    { 0xD20D20D20D20D20Dull,  8, 0 }, { 0xD161543E28E50274ull,  8, 0 },
    { 0xD0B69FCBD2580D0Bull,  8, 0 }, { 0xD00D00D00D00D00Dull,  8, 0 },
    { 0xCF6474A8819EC8E9ull,  8, 0 }, { 0xCEBCF8BB5B4169CAull,  8, 0 },
    { 0xCE168A7725080CE1ull,  8, 0 }, { 0xCD712752A886D241ull,  8, 0 },
    { 0xCCCCCCCCCCCCCCCCull,  8, 0 }, { 0xCC29786C7607F99Eull,  8, 0 },
    { 0xCB8727C065C393E0ull,  8, 0 }, { 0xCAE5D85F1BBD6C95ull,  8, 0 },
    { 0xCA4587E6B74F0329ull,  8, 0 }, { 0xC9A633FCD967300Cull,  8, 0 },
    { 0xC907DA4E871146ACull,  8, 0 }, { 0xC86A78900C86A789ull,  8, 0 },
    { 0xC7CE0C7CE0C7CE0Cull,  8, 0 }, { 0xC73293D789B9F838ull,  8, 0 },
    { 0xC6980C6980C6980Cull,  8, 0 }, { 0xC5FE740317F9D00Cull,  8, 0 },
    { 0xC565C87B5F9D4D1Bull,  8, 0 }, { 0xC4CE07B00C4CE07Bull,  8, 0 },
    { 0xC4372F855D824CA5ull,  8, 0 }, { 0xC3A13DE60495C773ull,  8, 0 },
    { 0xC30C30C30C30C30Cull,  8, 0 }, { 0xC2780613C0309E01ull,  8, 0 },
    { 0xC1E4BBD595F6E947ull,  8, 0 }, { 0xC152500C152500C1ull,  8, 0 },
    { 0xC0C0C0C0C0C0C0C0ull,  8, 0 }, { 0xC0300C0300C0300Cull,  8, 0 },
    { 0xBFA02FE80BFA02FEull,  8, 0 }, { 0xBF112A8AD278E8DCull,  8, 0 },
    { 0xBE82FA0BE82FA0BEull,  8, 0 }, { 0xBDF59C91700BDF59ull,  8, 0 },
    { 0xBD69104707661AA2ull,  8, 0 }, { 0xBCDD535DB1CC5B7Bull,  8, 0 },
    { 0xBC52640BC52640BCull,  8, 0 }, { 0xBBC8408CD63069A0ull,  8, 0 },
    { 0xBB3EE721A54D880Bull,  8, 0 }, { 0xBAB656100BAB6561ull,  8, 0 },
    { 0xBA2E8BA2E8BA2E8Bull,  8, 0 }, { 0xB9A7862A0FF46587ull,  8, 0 },
    { 0xB92143FA36F5E02Eull,  8, 0 }, { 0xB89BC36CE3E0453Aull,  8, 0 },
    { 0xB81702E05C0B8170ull,  8, 0 }, { 0xB79300B79300B793ull,  8, 0 },
    { 0xB70FBB5A19BE3658ull,  8, 0 }, { 0xB68D31340E4307D8ull,  8, 0 },
    { 0xB60B60B60B60B60Bull,  8, 0 }, { 0xB58A485518D1E7E3ull,  8, 0 },
    { 0xB509E68A9B94821Full,  8, 0 }, { 0xB48A39D44685FE96ull,  8, 0 },
    { 0xB40B40B40B40B40Bull,  8, 0 }, { 0xB38CF9B00B38CF9Bull,  8, 0 },
    { 0xB30F63528917C80Bull,  8, 0 }, { 0xB2927C29DA5519CFull,  8, 0 },
    { 0xB21642C8590B2164ull,  8, 0 }, { 0xB19AB5C45606F00Bull,  8, 0 },
    { 0xB11FD3B80B11FD3Bull,  8, 0 }, { 0xB0A59B418D749D53ull,  8, 0 },
    { 0xB02C0B02C0B02C0Bull,  8, 0 }, { 0xAFB321A1496FDF0Eull,  8, 0 },
    { 0xAF3ADDC680AF3ADDull,  8, 0 }, { 0xAEC33E1F671529A4ull,  8, 0 },
    { 0xAE4C415C9882B931ull,  8, 0 }, { 0xADD5E6323FD48A86ull,  8, 0 },
    { 0xAD602B580AD602B5ull,  8, 0 }, { 0xACEB0F891E6551BBull,  8, 0 },
    { 0xAC7691840AC76918ull,  8, 0 }, { 0xAC02B00AC02B00ACull,  8, 0 },
    { 0xAB8F69E28359CD11ull,  8, 0 }, { 0xAB1CBDD3E2970F5Full,  8, 0 },
    { 0xAAAAAAAAAAAAAAAAull,  8, 0 }, { 0xAA392F35DC17F00Aull,  8, 0 },
    { 0xA9C84A47A07F5637ull,  8, 0 }, { 0xA957FAB5402A55FEull,  8, 0 },
    { 0xA8E83F5717C0A8E8ull,  8, 0 }, { 0xA87917088E262B6Full,  8, 0 },
    { 0xA80A80A80A80A80Aull,  8, 0 }, { 0xA79C7B16EA64D422ull,  8, 0 },
    { 0xA72F05397829CBC1ull,  8, 0 }, { 0xA6C21DF6E1625C7Full,  8, 0 },
    { 0xA655C4392D7B73A7ull,  8, 0 }, { 0xA5E9F6ED347F0721ull,  8, 0 },
    { 0xA57EB50295FAD40Aull,  8, 0 }, { 0xA513FD6BB00A513Full,  8, 0 },
    { 0xA4A9CF1D96833751ull,  8, 0 }, { 0xA44029100A440291ull,  8, 0 },
    { 0xA3D70A3D70A3D70Aull,  8, 0 }, { 0xA36E71A2CB033128ull,  8, 0 },
    { 0xA3065E3FAE7CD0E0ull,  8, 0 }, { 0xA29ECF163BB6500Aull,  8, 0 },
    { 0xA237C32B16CFD772ull,  8, 0 }, { 0xA1D139855F7268EDull,  8, 0 },
    { 0xA16B312EA8FC377Cull,  8, 0 }, { 0xA105A932F2CA891Eull,  8, 0 },
    { 0xA0A0A0A0A0A0A0A0ull,  8, 0 }, { 0xA03C1688732B3032ull,  8, 0 },
    { 0x9FD809FD809FD809ull,  8, 0 }, { 0x9F747A152D7836D0ull,  8, 0 },
    { 0x9F1165E7254813E2ull,  8, 0 }, { 0x9EAECC8D53AE2DDEull,  8, 0 },
    { 0x9E4CAD23DD5F3A20ull,  8, 0 }, { 0x9DEB06C9194AA416ull,  8, 0 },
    { 0x9D89D89D89D89D89ull,  8, 0 }, { 0x9D2921C3D6411307ull,  8, 0 },
    { 0x9CC8E160C3FB19B8ull,  8, 0 }, { 0x9C69169B30446DF9ull,  8, 0 },
    { 0x9C09C09C09C09C09ull,  8, 0 }, { 0x9BAADE8E4A2F6E0Full,  8, 0 },
    { 0x9B4C6F9EF03A3CA9ull,  8, 0 }, { 0x9AEE72FCF957C10Full,  8, 0 },
    { 0x9A90E7D95BC609A9ull,  8, 0 }, { 0x9A33CD67009A33CDull,  8, 0 },
    { 0x99D722DABDE58F06ull,  8, 0 }, { 0x997AE76B50EFD009ull,  8, 0 },
    { 0x991F1A515885FB37ull,  8, 0 }, { 0x98C3BAC74F5DB009ull,  8, 0 },
    { 0x9868C809868C8098ull,  8, 0 }, { 0x980E4156201301C8ull,  8, 0 },
    { 0x97B425ED097B425Eull,  8, 0 }, { 0x975A750FF68A58AFull,  8, 0 },
    { 0x97012E025C04B809ull,  8, 0 }, { 0x96A850096A850096ull,  8, 0 },
    { 0x964FDA6C0964FDA6ull,  8, 0 }, { 0x95F7CC72D1B887E8ull,  8, 0 },
    { 0x95A02568095A0256ull,  8, 0 }, { 0x9548E4979E0829FCull,  8, 0 },
    { 0x94F2094F2094F209ull,  8, 0 }, { 0x949B92DDC02526E4ull,  8, 0 },
    { 0x9445809445809445ull,  8, 0 }, { 0x93EFD1C50E726B7Cull,  8, 0 },
    { 0x939A85C40939A85Cull,  8, 0 }, { 0x93459BE6B0093459ull,  8, 0 },
    { 0x92F113840497889Cull,  8, 0 }, { 0x929CEBF48BBD90E5ull,  8, 0 },
    { 0x9249249249249249ull,  8, 0 }, { 0x91F5BCB8BB02D9CCull,  8, 0 },
    { 0x91A2B3C4D5E6F809ull,  8, 0 }, { 0x9150091500915009ull,  8, 0 },
    { 0x90FDBC090FDBC090ull,  8, 0 }, { 0x90ABCC0242AF3009ull,  8, 0 },
    { 0x905A38633E06C43Aull,  8, 0 }, { 0x9009009009009009ull,  8, 0 },
    { 0x8FB823EE08FB823Eull,  8, 0 }, { 0x8F67A1E3FDC26178ull,  8, 0 },
    { 0x8F1779D9FDC3A218ull,  8, 0 }, { 0x8EC7AB397255E41Dull,  8, 0 },
    { 0x8E78356D1408E783ull,  8, 0 }, { 0x8E2917E0E702C6CDull,  8, 0 },
    { 0x8DDA520237694808ull,  8, 0 }, { 0x8D8BE33F95D71590ull,  8, 0 },
    { 0x8D3DCB08D3DCB08Dull,  8, 0 }, { 0x8CF008CF008CF008ull,  8, 0 },
    { 0x8CA29C046514E023ull,  8, 0 }, { 0x8C55841C815ED5CAull,  8, 0 },
    { 0x8C08C08C08C08C08ull,  8, 0 }, { 0x8BBC50C8DEB420C0ull,  8, 0 },
    { 0x8B70344A139BC75Aull,  8, 0 }, { 0x8B246A87E19008B2ull,  8, 0 },
    { 0x8AD8F2FBA9386822ull,  8, 0 }, { 0x8A8DCD1FEEAE465Cull,  8, 0 },
    { 0x8A42F8705669DB46ull,  8, 0 }, { 0x89F87469A23920E0ull,  8, 0 },
    { 0x89AE4089AE4089AEull,  8, 0 }, { 0x89645C4F6E055DEBull,  8, 0 },
    { 0x891AC73AE9819B50ull,  8, 0 }, { 0x88D180CD3A4133D7ull,  8, 0 },
    { 0x8888888888888888ull,  8, 0 }, { 0x883FDDF00883FDDFull,  8, 0 },
    { 0x87F78087F78087F7ull,  8, 0 }, { 0x87AF6FD5992D0D40ull,  8, 0 },
    { 0x8767AB5F34E47EF1ull,  8, 0 }, { 0x872032AC13008720ull,  8, 0 },
    { 0x86D905447A34ACC6ull,  8, 0 }, { 0x869222B1ACF1CE96ull,  8, 0 },
    { 0x864B8A7DE6D1D608ull,  8, 0 }, { 0x86053C345A0B8473ull,  8, 0 },
    { 0x85BF37612CEE3C9Aull,  8, 0 }, { 0x85797B917765AB89ull,  8, 0 },
    { 0x8534085340853408ull,  8, 0 }, { 0x84EEDD357C1B0084ull,  8, 0 },
    { 0x84A9F9C8084A9F9Cull,  8, 0 }, { 0x84655D9BAB2F1008ull,  8, 0 },
    { 0x8421084210842108ull,  8, 0 }, { 0x83DCF94DC7570CE0ull,  8, 0 },
    { 0x839930523FBE3367ull,  8, 0 }, { 0x8355ACE3C897DB0Full,  8, 0 },
    { 0x83126E978D4FDF3Bull,  8, 0 }, { 0x82CF750393AC3319ull,  8, 0 },
    { 0x828CBFBEB9A020A3ull,  8, 0 }, { 0x824A4E60B3262BC4ull,  8, 0 },
    { 0x8208208208208208ull,  8, 0 }, { 0x81C635BC123FDF8Eull,  8, 0 },
    { 0x81848DA8FAF0D277ull,  8, 0 }, { 0x814327E3B94F462Full,  8, 0 },
    { 0x8102040810204081ull,  8, 0 }, { 0x80C121B28BD1BA97ull,  8, 0 },
    { 0x8080808080808080ull,  8, 0 }, { 0x8040201008040201ull,  8, 0 },
};

template <typename Integal>
static inline
std::uint32_t getIntLog2(Integal val)
{
    std::uint32_t kMaxByteLen = sizeof(Integal) * 8;
    std::uint32_t log2_i = 0;

    if ((val & (val - 1)) == 0) {
        while (val != 0) {
            val >>= 1;
            log2_i++;
        }
        return ((log2_i > 0) ? (log2_i - 1) : 0);
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
DivRatio32 getDivRatio_u32(std::uint32_t modulo)
{
    DivRatio32 divRatio;
    std::uint32_t shift = getIntLog2<std::uint32_t>(modulo);
    std::uint32_t ratio;
    if ((modulo & (modulo - 1)) == 0) {
        if (modulo != 0)
            ratio = 0xFFFFFFFFul;
        else
            ratio = 0;
    } else {
        ratio = (uint32_t)ceil((double)((uint64_t)1u << (32 + shift)) / modulo);
    }
    divRatio.ratio = ratio;
    divRatio.shift = shift;
    return divRatio;
}

static inline
DivRatio64 getDivRatio_u64(std::uint64_t modulo)
{
    DivRatio64 divRatio;
    std::uint32_t shift = getIntLog2<std::uint64_t>(modulo);
    std::uint64_t ratio;
    if ((modulo & (modulo - 1)) == 0) {
        if (modulo != 0)
            ratio = 0xFFFFFFFFFFFFFFFFull;
        else
            ratio = 0;
    } else {
#if defined(_MSC_VER)
        ratio = (uint64_t)ceil((double)((uint64_t)1 << (32 + shift)) * (double)((uint64_t)1 << 32) / modulo);
#else
        __uint128_t shift_64 = (__uint128_t)((uint64_t)1 << (32 + shift)) * ((uint64_t)1 << 32);
        if ((shift_64 % modulo) != 0)
            ratio = (uint64_t)(shift_64 / modulo) + 1;
        else
            ratio = (uint64_t)(shift_64 / modulo);
#endif
    }
    divRatio.ratio = ratio;
    divRatio.shift = shift;
    divRatio.reserve = 0;
    return divRatio;
}

static inline
void genDivRatioTbl_u32()
{
    DivRatio32 divRatioTbl[kMaxDivTable];
    for (std::uint32_t n = 0; n < kMaxDivTable; n++) {
        divRatioTbl[n] = getDivRatio_u32(n);
    }

    printf("\n");
    printf("static const DivRatio32 div_ratio_tbl32[kMaxDivTable] = {\n");
    for (std::uint32_t n = 0; n < kMaxDivTable; n++) {
        if ((n % 4) == 0) {
            printf("    ");
        }
        printf("{ 0x%08X, %2u },", divRatioTbl[n].ratio, divRatioTbl[n].shift);
        if ((n % 4) == 3) {
            printf("\n");
        } else {
            printf(" ");
        }
    }
    printf("};\n\n");
}

static inline
void genDivRatioTbl_u64()
{
    DivRatio64 divRatioTbl[kMaxDivTable];
    for (std::uint32_t n = 0; n < kMaxDivTable; n++) {
        divRatioTbl[n] = getDivRatio_u64(n);
    }

    printf("\n");
    printf("static const DivRatio64 div_ratio_tbl64[kMaxDivTable] = {\n");
    for (std::uint32_t n = 0; n < kMaxDivTable; n++) {
        if ((n % 2) == 0) {
            printf("    ");
        }
        printf("{ 0x%08X%08Xull, %2u, 0 },", (uint32_t)(divRatioTbl[n].ratio >> 32u),
                                             (uint32_t)(divRatioTbl[n].ratio & 0xFFFFFFFFul),
                                             divRatioTbl[n].shift);
        if ((n % 2) == 1) {
            printf("\n");
        } else {
            printf(" ");
        }
    }
    printf("};\n\n");
}

static inline
void genDivRatioTbl()
{
    genDivRatioTbl_u32();
    genDivRatioTbl_u64();
}

static inline
std::uint32_t fast_div_u32(std::uint32_t value, std::uint32_t divisor)
{
    if (divisor < kMaxDivTable) {
        DivRatio32 dt = div_ratio_tbl32[divisor];
        std::uint32_t result = ((std::uint32_t)(((std::uint64_t)value * dt.ratio) >> 32u) >> dt.shift);
        return result;
    } else {
        return (value / divisor);
    }
}

static inline
std::uint64_t fast_div_u64(std::uint64_t value, std::uint64_t divisor)
{
    if (divisor < kMaxDivTable) {
        DivRatio64 dt = div_ratio_tbl64[divisor];
        std::uint64_t result = (mul128_high_u64_ex(value, dt.ratio) >> dt.shift);
        return result;
    } else {
        return (value / divisor);
    }
}

} // namespace jstd

#endif // JSTD_FAST_DIV_H
