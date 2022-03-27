
#ifndef JSTD_FAST_MOD_H
#define JSTD_FAST_MOD_H

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

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace jstd {

static const std::uint32_t kMaxModTable = 256;

struct ModRatio {
    uint64_t M;
    //uint32_t divisor;
    //uint32_t reserve;
#if 0
    ModRatio(uint64_t _M = 0, uint32_t _divisor = 0)
        : M(_M), divisor(_divisor), reserve(0u) {}

    ModRatio(int64_t _M, int _divisor)
        : M((uint64_t)_M), divisor((uint32_t)_divisor), reserve(0u) {}

    ModRatio(const ModRatio & src)
        : : M(src.M), divisor(src.divisor), reserve(src.reserve) {}

    ModRatio & operator = (const ModRatio & rhs) {
        this->M = rhs.M;
        this->divisor = rhs.divisor;
        this->reserve = rhs.reserve;
    };
#endif
};

static const ModRatio mod_ratio_tbl[kMaxModTable] = {
    { 0x0000000000000000ull }, { 0x0000000000000000ull }, { 0x8000000000000000ull }, { 0x5555555555555556ull },
    { 0x4000000000000000ull }, { 0x3333333333333334ull }, { 0x2AAAAAAAAAAAAAABull }, { 0x2492492492492493ull },
    { 0x2000000000000000ull }, { 0x1C71C71C71C71C72ull }, { 0x199999999999999Aull }, { 0x1745D1745D1745D2ull },
    { 0x1555555555555556ull }, { 0x13B13B13B13B13B2ull }, { 0x124924924924924Aull }, { 0x1111111111111112ull },
    { 0x1000000000000000ull }, { 0x0F0F0F0F0F0F0F10ull }, { 0x0E38E38E38E38E39ull }, { 0x0D79435E50D79436ull },
    { 0x0CCCCCCCCCCCCCCDull }, { 0x0C30C30C30C30C31ull }, { 0x0BA2E8BA2E8BA2E9ull }, { 0x0B21642C8590B217ull },
    { 0x0AAAAAAAAAAAAAABull }, { 0x0A3D70A3D70A3D71ull }, { 0x09D89D89D89D89D9ull }, { 0x097B425ED097B426ull },
    { 0x0924924924924925ull }, { 0x08D3DCB08D3DCB09ull }, { 0x0888888888888889ull }, { 0x0842108421084211ull },
    { 0x0800000000000000ull }, { 0x07C1F07C1F07C1F1ull }, { 0x0787878787878788ull }, { 0x0750750750750751ull },
    { 0x071C71C71C71C71Dull }, { 0x06EB3E45306EB3E5ull }, { 0x06BCA1AF286BCA1Bull }, { 0x0690690690690691ull },
    { 0x0666666666666667ull }, { 0x063E7063E7063E71ull }, { 0x0618618618618619ull }, { 0x05F417D05F417D06ull },
    { 0x05D1745D1745D175ull }, { 0x05B05B05B05B05B1ull }, { 0x0590B21642C8590Cull }, { 0x0572620AE4C415CAull },
    { 0x0555555555555556ull }, { 0x05397829CBC14E5Full }, { 0x051EB851EB851EB9ull }, { 0x0505050505050506ull },
    { 0x04EC4EC4EC4EC4EDull }, { 0x04D4873ECADE304Eull }, { 0x04BDA12F684BDA13ull }, { 0x04A7904A7904A791ull },
    { 0x0492492492492493ull }, { 0x047DC11F7047DC12ull }, { 0x0469EE58469EE585ull }, { 0x0456C797DD49C342ull },
    { 0x0444444444444445ull }, { 0x04325C53EF368EB1ull }, { 0x0421084210842109ull }, { 0x0410410410410411ull },
    { 0x0400000000000000ull }, { 0x03F03F03F03F03F1ull }, { 0x03E0F83E0F83E0F9ull }, { 0x03D226357E16ECE6ull },
    { 0x03C3C3C3C3C3C3C4ull }, { 0x03B5CC0ED7303B5Dull }, { 0x03A83A83A83A83A9ull }, { 0x039B0AD12073615Bull },
    { 0x038E38E38E38E38Full }, { 0x0381C0E070381C0Full }, { 0x03759F22983759F3ull }, { 0x0369D0369D0369D1ull },
    { 0x035E50D79435E50Eull }, { 0x03531DEC0D4C77B1ull }, { 0x0348348348348349ull }, { 0x033D91D2A2067B24ull },
    { 0x0333333333333334ull }, { 0x0329161F9ADD3C0Dull }, { 0x031F3831F3831F39ull }, { 0x03159721ED7E7535ull },
    { 0x030C30C30C30C30Dull }, { 0x0303030303030304ull }, { 0x02FA0BE82FA0BE83ull }, { 0x02F149902F149903ull },
    { 0x02E8BA2E8BA2E8BBull }, { 0x02E05C0B81702E06ull }, { 0x02D82D82D82D82D9ull }, { 0x02D02D02D02D02D1ull },
    { 0x02C8590B21642C86ull }, { 0x02C0B02C0B02C0B1ull }, { 0x02B9310572620AE5ull }, { 0x02B1DA46102B1DA5ull },
    { 0x02AAAAAAAAAAAAABull }, { 0x02A3A0FD5C5F02A4ull }, { 0x029CBC14E5E0A730ull }, { 0x0295FAD40A57EB51ull },
    { 0x028F5C28F5C28F5Dull }, { 0x0288DF0CAC5B3F5Eull }, { 0x0282828282828283ull }, { 0x027C45979C952050ull },
    { 0x0276276276276277ull }, { 0x0270270270270271ull }, { 0x026A439F656F1827ull }, { 0x02647C69456217EDull },
    { 0x025ED097B425ED0Aull }, { 0x02593F69B02593F7ull }, { 0x0253C8253C8253C9ull }, { 0x024E6A171024E6A2ull },
    { 0x024924924924924Aull }, { 0x0243F6F0243F6F03ull }, { 0x023EE08FB823EE09ull }, { 0x0239E0D5B450239Full },
    { 0x0234F72C234F72C3ull }, { 0x0230230230230231ull }, { 0x022B63CBEEA4E1A1ull }, { 0x0226B90226B90227ull },
    { 0x0222222222222223ull }, { 0x021D9EAD7CD391FCull }, { 0x02192E29F79B4759ull }, { 0x0214D0214D0214D1ull },
    { 0x0210842108421085ull }, { 0x020C49BA5E353F7Dull }, { 0x0208208208208209ull }, { 0x0204081020408103ull },
    { 0x0200000000000000ull }, { 0x01FC07F01FC07F02ull }, { 0x01F81F81F81F81F9ull }, { 0x01F44659E4A42716ull },
    { 0x01F07C1F07C1F07Dull }, { 0x01ECC07B301ECC08ull }, { 0x01E9131ABF0B7673ull }, { 0x01E573AC901E573Bull },
    { 0x01E1E1E1E1E1E1E2ull }, { 0x01DE5D6E3F8868A5ull }, { 0x01DAE6076B981DAFull }, { 0x01D77B654B82C33Aull },
    { 0x01D41D41D41D41D5ull }, { 0x01D0CB58F6EC0744ull }, { 0x01CD85689039B0AEull }, { 0x01CA4B3055EE1911ull },
    { 0x01C71C71C71C71C8ull }, { 0x01C3F8F01C3F8F02ull }, { 0x01C0E070381C0E08ull }, { 0x01BDD2B899406F75ull },
    { 0x01BACF914C1BACFAull }, { 0x01B7D6C3DDA338B3ull }, { 0x01B4E81B4E81B4E9ull }, { 0x01B2036406C80D91ull },
    { 0x01AF286BCA1AF287ull }, { 0x01AC5701AC5701ADull }, { 0x01A98EF606A63BD9ull }, { 0x01A6D01A6D01A6D1ull },
    { 0x01A41A41A41A41A5ull }, { 0x01A16D3F97A4B01Bull }, { 0x019EC8E951033D92ull }, { 0x019C2D14EE4A101Aull },
    { 0x019999999999999Aull }, { 0x01970E4F80CB8728ull }, { 0x01948B0FCD6E9E07ull }, { 0x01920FB49D0E228Eull },
    { 0x018F9C18F9C18F9Dull }, { 0x018D3018D3018D31ull }, { 0x018ACB90F6BF3A9Bull }, { 0x01886E5F0ABB049Aull },
    { 0x0186186186186187ull }, { 0x0183C977AB2BEDD3ull }, { 0x0181818181818182ull }, { 0x017F405FD017F406ull },
    { 0x017D05F417D05F42ull }, { 0x017AD2208E0ECC36ull }, { 0x0178A4C8178A4C82ull }, { 0x01767DCE434A9B11ull },
    { 0x01745D1745D1745Eull }, { 0x01724287F46DEBC1ull }, { 0x01702E05C0B81703ull }, { 0x016E1F76B4337C6Dull },
    { 0x016C16C16C16C16Dull }, { 0x016A13CD15372905ull }, { 0x0168168168168169ull }, { 0x01661EC6A5122F91ull },
    { 0x01642C8590B21643ull }, { 0x01623FA7701623FBull }, { 0x0160581605816059ull }, { 0x015E75BB8D015E76ull },
    { 0x015C9882B9310573ull }, { 0x015AC056B015AC06ull }, { 0x0158ED2308158ED3ull }, { 0x01571ED3C506B39Bull },
    { 0x0155555555555556ull }, { 0x015390948F40FEADull }, { 0x0151D07EAE2F8152ull }, { 0x0150150150150151ull },
    { 0x014E5E0A72F05398ull }, { 0x014CAB88725AF6E8ull }, { 0x014AFD6A052BF5A9ull }, { 0x0149539E3B2D066Full },
    { 0x0147AE147AE147AFull }, { 0x01460CBC7F5CF9A2ull }, { 0x01446F86562D9FAFull }, { 0x0142D6625D51F86Full },
    { 0x0141414141414142ull }, { 0x013FB013FB013FB1ull }, { 0x013E22CBCE4A9028ull }, { 0x013C995A47BABE75ull },
    { 0x013B13B13B13B13Cull }, { 0x013991C2C187F634ull }, { 0x0138138138138139ull }, { 0x013698DF3DE0747Aull },
    { 0x013521CFB2B78C14ull }, { 0x0133AE45B57BCB1Full }, { 0x01323E34A2B10BF7ull }, { 0x0130D190130D1902ull },
    { 0x012F684BDA12F685ull }, { 0x012E025C04B80971ull }, { 0x012C9FB4D812C9FCull }, { 0x012B404AD012B405ull },
    { 0x0129E4129E4129E5ull }, { 0x01288B01288B0129ull }, { 0x0127350B88127351ull }, { 0x0125E22708092F12ull },
    { 0x0124924924924925ull }, { 0x0123456789ABCDF1ull }, { 0x0121FB78121FB782ull }, { 0x0120B470C67C0D89ull },
    { 0x011F7047DC11F705ull }, { 0x011E2EF3B3FB8745ull }, { 0x011CF06ADA2811D0ull }, { 0x011BB4A4046ED291ull },
    { 0x011A7B9611A7B962ull }, { 0x0119453808CA29C1ull }, { 0x0118118118118119ull }, { 0x0116E0689427378Full },
    { 0x0115B1E5F75270D1ull }, { 0x011485F0E0ACD3B7ull }, { 0x01135C81135C8114ull }, { 0x0112358E75D30337ull },
    { 0x0111111111111112ull }, { 0x010FEF010FEF0110ull }, { 0x010ECF56BE69C8FEull }, { 0x010DB20A88F4695Aull },
    { 0x010C9714FBCDA3ADull }, { 0x010B7E6EC259DC7Aull }, { 0x010A6810A6810A69ull }, { 0x010953F390109540ull },
    { 0x0108421084210843ull }, { 0x01073260A47F7C67ull }, { 0x010624DD2F1A9FBFull }, { 0x0105197F7D734042ull },
    { 0x0104104104104105ull }, { 0x0103091B51F5E1A5ull }, { 0x0102040810204082ull }, { 0x0101010101010102ull },
};

static inline
std::size_t fast_mod(std::size_t a, std::size_t b)
{
    return ((a < b) ? a : ((a < b * 2) ? (a - b) : (a % b)));
}

static inline
std::uint64_t computeM_u32(uint32_t divisor) {
    if (divisor != 0)
        return (UINT64_C(0xFFFFFFFFFFFFFFFF) / divisor + 1);
    else
        return 0;
}

static inline
ModRatio getModRatio_u32(std::uint32_t divisor)
{
    ModRatio modRatio;
    modRatio.M = computeM_u32(divisor);
    return modRatio;
}

static inline
void genModRatioTbl()
{
    ModRatio modRatioTbl[kMaxModTable];
    for (std::uint32_t n = 0; n < kMaxModTable; n++) {
        modRatioTbl[n] = getModRatio_u32(n);
    }

    printf("\n");
    printf("static const ModRatio mod_ratio_tbl[kMaxModTable] = {\n");
    for (std::uint32_t n = 0; n < kMaxModTable; n++) {
        if ((n % 4) == 0) {
            printf("    ");
        }
        printf("{ 0x%08X%08Xull },", (uint32_t)(modRatioTbl[n].M >> 32u),
                                     (uint32_t)modRatioTbl[n].M);
        if ((n % 4) == 3) {
            printf("\n");
        }
        else {
            printf(" ");
        }
    }
    printf("};\n\n");
}

#if defined(_MSC_VER)

// __umulh() is only available in x64 mode under Visual Studio: don't compile to 32-bit!
static inline
uint64_t mul128_u32(uint64_t low_bits, uint32_t divisor) {
  return __umulh(low_bits, divisor);
}

#else // !_MSC_VER

static inline
uint64_t mul128_u32(uint64_t low_bits, uint32_t divisor) {
  return (((__uint128_t)low_bits * divisor) >> 64u);
}

#endif // _MSC_VER

static inline
std::uint32_t fast_mod_u32(std::uint32_t value, std::uint32_t divisor)
{
    if (divisor >= kMaxModTable) {
        return (value % divisor);
    }
    else {
        ModRatio mt = mod_ratio_tbl[divisor];
        std::uint64_t low_bits = value * mt.M;
        std::uint32_t result = (std::uint32_t)mul128_u32(low_bits, divisor);
        return result;
    }
}

} // namespace jstd

#endif // JSTD_FAST_MOD_H
