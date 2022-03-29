
#ifndef JSTD_FAST_MOD_H
#define JSTD_FAST_MOD_H

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

#ifdef _MSC_VER
#include <intrin.h>     // For __umulh(), available only in x64 mode
#endif

//
// See: https://github.com/lemire/fastmod
// See: https://github.com/pcjentsch/FastModulo.jl
//

namespace jstd {

static const std::uint32_t kMaxModTable = 1024;

struct ModRatio {
    uint64_t M;
#if 0
    ModRatio(uint64_t _M = 0)
        : M(_M) {}

    ModRatio(int64_t _M)
        : M((uint64_t)_M) {}

    ModRatio(const ModRatio & src)
        : : M(src.M) {}

    ModRatio & operator = (const ModRatio & rhs) {
        this->M = rhs.M;
        return *this;
    };
#endif
};

static const ModRatio mod_ratio_tbl32[kMaxModTable] = {
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
    { 0x0100000000000000ull }, { 0x00FF00FF00FF0100ull }, { 0x00FE03F80FE03F81ull }, { 0x00FD08E5500FD08Full },
    { 0x00FC0FC0FC0FC0FDull }, { 0x00FB18856506DDACull }, { 0x00FA232CF252138Bull }, { 0x00F92FB2211855A9ull },
    { 0x00F83E0F83E0F83Full }, { 0x00F74E3FC22C7010ull }, { 0x00F6603D980F6604ull }, { 0x00F57403D5D00F58ull },
    { 0x00F4898D5F85BB3Aull }, { 0x00F3A0D52CBA8724ull }, { 0x00F2B9D6480F2B9Eull }, { 0x00F1D48BCEE0D39Aull },
    { 0x00F0F0F0F0F0F0F1ull }, { 0x00F00F00F00F00F1ull }, { 0x00EF2EB71FC43453ull }, { 0x00EE500EE500EE51ull },
    { 0x00ED7303B5CC0ED8ull }, { 0x00EC979118F3FC4Eull }, { 0x00EBBDB2A5C1619Dull }, { 0x00EAE56403AB9591ull },
    { 0x00EA0EA0EA0EA0EBull }, { 0x00E939651FE2D8D4ull }, { 0x00E865AC7B7603A2ull }, { 0x00E79372E225FE31ull },
    { 0x00E6C2B4481CD857ull }, { 0x00E5F36CB00E5F37ull }, { 0x00E525982AF70C89ull }, { 0x00E45932D7DC5211ull },
    { 0x00E38E38E38E38E4ull }, { 0x00E2C4A6886A4C2Full }, { 0x00E1FC780E1FC781ull }, { 0x00E135A9C97500E2ull },
    { 0x00E070381C0E0704ull }, { 0x00DFAC1F74346C58ull }, { 0x00DEE95C4CA037BBull }, { 0x00DE27EB2C41F3DAull },
    { 0x00DD67C8A60DD67Dull }, { 0x00DCA8F158C7F91Bull }, { 0x00DBEB61EED19C5Aull }, { 0x00DB2F171DF7702Aull },
    { 0x00DA740DA740DA75ull }, { 0x00D9BA4256C0366Full }, { 0x00D901B2036406C9ull }, { 0x00D84A598EC91520ull },
    { 0x00D79435E50D7944ull }, { 0x00D6DF43FCA482F1ull }, { 0x00D62B80D62B80D7ull }, { 0x00D578E97C3F5FE6ull },
    { 0x00D4C77B03531DEDull }, { 0x00D4173289870AC6ull }, { 0x00D3680D3680D369ull }, { 0x00D2BA083B445251ull },
    { 0x00D20D20D20D20D3ull }, { 0x00D161543E28E503ull }, { 0x00D0B69FCBD2580Eull }, { 0x00D00D00D00D00D1ull },
    { 0x00CF6474A8819EC9ull }, { 0x00CEBCF8BB5B416Aull }, { 0x00CE168A7725080Dull }, { 0x00CD712752A886D3ull },
    { 0x00CCCCCCCCCCCCCDull }, { 0x00CC29786C7607FAull }, { 0x00CB8727C065C394ull }, { 0x00CAE5D85F1BBD6Dull },
    { 0x00CA4587E6B74F04ull }, { 0x00C9A633FCD96731ull }, { 0x00C907DA4E871147ull }, { 0x00C86A78900C86A8ull },
    { 0x00C7CE0C7CE0C7CFull }, { 0x00C73293D789B9F9ull }, { 0x00C6980C6980C699ull }, { 0x00C5FE740317F9D1ull },
    { 0x00C565C87B5F9D4Eull }, { 0x00C4CE07B00C4CE1ull }, { 0x00C4372F855D824Dull }, { 0x00C3A13DE60495C8ull },
    { 0x00C30C30C30C30C4ull }, { 0x00C2780613C0309Full }, { 0x00C1E4BBD595F6EAull }, { 0x00C152500C152501ull },
    { 0x00C0C0C0C0C0C0C1ull }, { 0x00C0300C0300C031ull }, { 0x00BFA02FE80BFA03ull }, { 0x00BF112A8AD278E9ull },
    { 0x00BE82FA0BE82FA1ull }, { 0x00BDF59C91700BE0ull }, { 0x00BD69104707661Bull }, { 0x00BCDD535DB1CC5Cull },
    { 0x00BC52640BC52641ull }, { 0x00BBC8408CD6306Aull }, { 0x00BB3EE721A54D89ull }, { 0x00BAB656100BAB66ull },
    { 0x00BA2E8BA2E8BA2Full }, { 0x00B9A7862A0FF466ull }, { 0x00B92143FA36F5E1ull }, { 0x00B89BC36CE3E046ull },
    { 0x00B81702E05C0B82ull }, { 0x00B79300B79300B8ull }, { 0x00B70FBB5A19BE37ull }, { 0x00B68D31340E4308ull },
    { 0x00B60B60B60B60B7ull }, { 0x00B58A485518D1E8ull }, { 0x00B509E68A9B9483ull }, { 0x00B48A39D44685FFull },
    { 0x00B40B40B40B40B5ull }, { 0x00B38CF9B00B38D0ull }, { 0x00B30F63528917C9ull }, { 0x00B2927C29DA551Aull },
    { 0x00B21642C8590B22ull }, { 0x00B19AB5C45606F1ull }, { 0x00B11FD3B80B11FEull }, { 0x00B0A59B418D749Eull },
    { 0x00B02C0B02C0B02Dull }, { 0x00AFB321A1496FE0ull }, { 0x00AF3ADDC680AF3Bull }, { 0x00AEC33E1F67152Aull },
    { 0x00AE4C415C9882BAull }, { 0x00ADD5E6323FD48Bull }, { 0x00AD602B580AD603ull }, { 0x00ACEB0F891E6552ull },
    { 0x00AC7691840AC76Aull }, { 0x00AC02B00AC02B01ull }, { 0x00AB8F69E28359CEull }, { 0x00AB1CBDD3E29710ull },
    { 0x00AAAAAAAAAAAAABull }, { 0x00AA392F35DC17F1ull }, { 0x00A9C84A47A07F57ull }, { 0x00A957FAB5402A56ull },
    { 0x00A8E83F5717C0A9ull }, { 0x00A87917088E262Cull }, { 0x00A80A80A80A80A9ull }, { 0x00A79C7B16EA64D5ull },
    { 0x00A72F05397829CCull }, { 0x00A6C21DF6E1625Dull }, { 0x00A655C4392D7B74ull }, { 0x00A5E9F6ED347F08ull },
    { 0x00A57EB50295FAD5ull }, { 0x00A513FD6BB00A52ull }, { 0x00A4A9CF1D968338ull }, { 0x00A44029100A4403ull },
    { 0x00A3D70A3D70A3D8ull }, { 0x00A36E71A2CB0332ull }, { 0x00A3065E3FAE7CD1ull }, { 0x00A29ECF163BB651ull },
    { 0x00A237C32B16CFD8ull }, { 0x00A1D139855F7269ull }, { 0x00A16B312EA8FC38ull }, { 0x00A105A932F2CA8Aull },
    { 0x00A0A0A0A0A0A0A1ull }, { 0x00A03C1688732B31ull }, { 0x009FD809FD809FD9ull }, { 0x009F747A152D7837ull },
    { 0x009F1165E7254814ull }, { 0x009EAECC8D53AE2Eull }, { 0x009E4CAD23DD5F3Bull }, { 0x009DEB06C9194AA5ull },
    { 0x009D89D89D89D89Eull }, { 0x009D2921C3D64114ull }, { 0x009CC8E160C3FB1Aull }, { 0x009C69169B30446Eull },
    { 0x009C09C09C09C09Dull }, { 0x009BAADE8E4A2F6Full }, { 0x009B4C6F9EF03A3Dull }, { 0x009AEE72FCF957C2ull },
    { 0x009A90E7D95BC60Aull }, { 0x009A33CD67009A34ull }, { 0x0099D722DABDE590ull }, { 0x00997AE76B50EFD1ull },
    { 0x00991F1A515885FCull }, { 0x0098C3BAC74F5DB1ull }, { 0x009868C809868C81ull }, { 0x00980E4156201302ull },
    { 0x0097B425ED097B43ull }, { 0x00975A750FF68A59ull }, { 0x0097012E025C04B9ull }, { 0x0096A850096A8501ull },
    { 0x00964FDA6C0964FEull }, { 0x0095F7CC72D1B888ull }, { 0x0095A02568095A03ull }, { 0x009548E4979E082Aull },
    { 0x0094F2094F2094F3ull }, { 0x00949B92DDC02527ull }, { 0x0094458094458095ull }, { 0x0093EFD1C50E726Cull },
    { 0x00939A85C40939A9ull }, { 0x0093459BE6B00935ull }, { 0x0092F11384049789ull }, { 0x00929CEBF48BBD91ull },
    { 0x0092492492492493ull }, { 0x0091F5BCB8BB02DAull }, { 0x0091A2B3C4D5E6F9ull }, { 0x0091500915009151ull },
    { 0x0090FDBC090FDBC1ull }, { 0x0090ABCC0242AF31ull }, { 0x00905A38633E06C5ull }, { 0x0090090090090091ull },
    { 0x008FB823EE08FB83ull }, { 0x008F67A1E3FDC262ull }, { 0x008F1779D9FDC3A3ull }, { 0x008EC7AB397255E5ull },
    { 0x008E78356D1408E8ull }, { 0x008E2917E0E702C7ull }, { 0x008DDA5202376949ull }, { 0x008D8BE33F95D716ull },
    { 0x008D3DCB08D3DCB1ull }, { 0x008CF008CF008CF1ull }, { 0x008CA29C046514E1ull }, { 0x008C55841C815ED6ull },
    { 0x008C08C08C08C08Dull }, { 0x008BBC50C8DEB421ull }, { 0x008B70344A139BC8ull }, { 0x008B246A87E19009ull },
    { 0x008AD8F2FBA93869ull }, { 0x008A8DCD1FEEAE47ull }, { 0x008A42F8705669DCull }, { 0x0089F87469A23921ull },
    { 0x0089AE4089AE408Aull }, { 0x0089645C4F6E055Eull }, { 0x00891AC73AE9819Cull }, { 0x0088D180CD3A4134ull },
    { 0x0088888888888889ull }, { 0x00883FDDF00883FEull }, { 0x0087F78087F78088ull }, { 0x0087AF6FD5992D0Eull },
    { 0x008767AB5F34E47Full }, { 0x00872032AC130088ull }, { 0x0086D905447A34ADull }, { 0x00869222B1ACF1CFull },
    { 0x00864B8A7DE6D1D7ull }, { 0x0086053C345A0B85ull }, { 0x0085BF37612CEE3Dull }, { 0x0085797B917765ACull },
    { 0x0085340853408535ull }, { 0x0084EEDD357C1B01ull }, { 0x0084A9F9C8084AA0ull }, { 0x0084655D9BAB2F11ull },
    { 0x0084210842108422ull }, { 0x0083DCF94DC7570Dull }, { 0x00839930523FBE34ull }, { 0x008355ACE3C897DCull },
    { 0x0083126E978D4FE0ull }, { 0x0082CF750393AC34ull }, { 0x00828CBFBEB9A021ull }, { 0x00824A4E60B3262Cull },
    { 0x0082082082082083ull }, { 0x0081C635BC123FE0ull }, { 0x0081848DA8FAF0D3ull }, { 0x00814327E3B94F47ull },
    { 0x0081020408102041ull }, { 0x0080C121B28BD1BBull }, { 0x0080808080808081ull }, { 0x0080402010080403ull },
    { 0x0080000000000000ull }, { 0x007FC01FF007FC02ull }, { 0x007F807F807F8080ull }, { 0x007F411E528439AAull },
    { 0x007F01FC07F01FC1ull }, { 0x007EC3184357A4E4ull }, { 0x007E8472A807E848ull }, { 0x007E460ADA04EEBDull },
    { 0x007E07E07E07E07Full }, { 0x007DC9F3397D4C2Aull }, { 0x007D8C42B2836ED6ull }, { 0x007D4ECE8FE8813Aull },
    { 0x007D1196792909C6ull }, { 0x007CD49A166E33B1ull }, { 0x007C97D9108C2AD5ull }, { 0x007C5B5311007C5Cull },
    { 0x007C1F07C1F07C20ull }, { 0x007BE2F6CE27AEB4ull }, { 0x007BA71FE1163808ull }, { 0x007B6B82A6CF4E96ull },
    { 0x007B301ECC07B302ull }, { 0x007AF4F3FE142C31ull }, { 0x007ABA01EAE807ACull }, { 0x007A7F4841139E63ull },
    { 0x007A44C6AFC2DD9Dull }, { 0x007A0A7CE6BBD425ull }, { 0x0079D06A965D4392ull }, { 0x0079968F6F9D35ACull },
    { 0x00795CEB240795CFull }, { 0x0079237D65BCCE51ull }, { 0x0078EA45E77069CDull }, { 0x0078B1445C67B857ull },
    { 0x0078787878787879ull }, { 0x00783FE1F00783FFull }, { 0x0078078078078079ull }, { 0x0077CF53C5F7936Dull },
    { 0x0077975B8FE21A2Aull }, { 0x00775F978C5B6531ull }, { 0x0077280772807729ull }, { 0x0076F0AAF9F5C752ull },
    { 0x0076B981DAE6076Cull }, { 0x0076828BCE00ED06ull }, { 0x00764BC88C79FE27ull }, { 0x00761537D0076154ull },
    { 0x0075DED952E0B0CFull }, { 0x0075A8ACCFBDD11Full }, { 0x007572B201D5CAC9ull }, { 0x00753CE8A4DDA728ull },
    { 0x0075075075075076ull }, { 0x0074D1E92F0074D2ull }, { 0x00749CB28FF16C6Aull }, { 0x007467AC557C228Full },
    { 0x007432D63DBB01D1ull }, { 0x0073FE30073FE301ull }, { 0x0073C9B97112FF19ull }, { 0x007395723AB1E402ull },
    { 0x0073615A240E6C2Cull }, { 0x00732D70ED8DB8EAull }, { 0x0072F9B658072F9Cull }, { 0x0072C62A24C37980ull },
    { 0x007292CC157B8645ull }, { 0x00725F9BEC579134ull }, { 0x00722C996BEE2909ull }, { 0x0071F9C457433A53ull },
    { 0x0071C71C71C71C72ull }, { 0x007194A17F55A10Eull }, { 0x0071625344352618ull }, { 0x007130318515AA3Aull },
    { 0x0070FE3C070FE3C1ull }, { 0x0070CC728FA459E3ull }, { 0x00709AD4E4BA8071ull }, { 0x00706962CC9FD5D9ull },
    { 0x0070381C0E070382ull }, { 0x0070070070070071ull }, { 0x006FD60FBA1A362Cull }, { 0x006FA549B41DA7E8ull },
    { 0x006F74AE26501BDEull }, { 0x006F443CD95146D9ull }, { 0x006F13F59620F9EDull }, { 0x006EE3D8261E524Eull },
    { 0x006EB3E45306EB3Full }, { 0x006E8419E6F61222ull }, { 0x006E5478AC63FC8Eull }, { 0x006E25006E25006Full },
    { 0x006DF5B0F768CE2Dull }, { 0x006DC68A13B9ACD0ull }, { 0x006D978B8EFBB815ull }, { 0x006D68B5356C207Cull },
    { 0x006D3A06D3A06D3Bull }, { 0x006D0B803685C01Cull }, { 0x006CDD212B601B38ull }, { 0x006CAEE97FC9A88Cull },
    { 0x006C80D901B20365ull }, { 0x006C52EF7F5D8399ull }, { 0x006C252CC7648A90ull }, { 0x006BF790A8B2D208ull },
    { 0x006BCA1AF286BCA2ull }, { 0x006B9CCB7470A825ull }, { 0x006B6FA1FE524179ull }, { 0x006B429E605DDA4Bull },
    { 0x006B15C06B15C06Cull }, { 0x006AE907EF4B96C3ull }, { 0x006ABC74BE1FAFF3ull }, { 0x006A9006A9006A91ull },
    { 0x006A63BD81A98EF7ull }, { 0x006A37991A23AEAEull }, { 0x006A0B9944C38563ull }, { 0x0069DFBDD4295B67ull },
    { 0x0069B4069B4069B5ull }, { 0x006988736D3E3F7Dull }, { 0x00695D041DA22929ull }, { 0x006931B8803498DDull },
    { 0x006906906906906Aull }, { 0x0068DB8BAC710CB3ull }, { 0x0068B0AA1F147282ull }, { 0x006885EB95D7FCBCull },
    { 0x00685B4FE5E92C07ull }, { 0x006830D6E4BB37C3ull }, { 0x0068068068068069ull }, { 0x0067DC4C45C8033Full },
    { 0x0067B23A5440CF65ull }, { 0x0067884A69F57C29ull }, { 0x00675E7C5DADA0B5ull }, { 0x006734D006734D01ull },
    { 0x00670B453B928407ull }, { 0x0066E1DBD498B743ull }, { 0x0066B893A954436Aull }, { 0x00668F6C91D3EE60ull },
    { 0x0066666666666667ull }, { 0x00663D80FF99C280ull }, { 0x006614BC363B03FDull }, { 0x0065EC17E3559949ull },
    { 0x0065C393E032E1CAull }, { 0x00659B300659B301ull }, { 0x006572EC2F8DDEB7ull }, { 0x00654AC835CFBA5Dull },
    { 0x006522C3F35BA782ull }, { 0x0064FADF42A99D64ull }, { 0x0064D319FE6CB399ull }, { 0x0064AB740192ADD1ull },
    { 0x006483ED274388A4ull }, { 0x00645C854AE10773ull }, { 0x0064353C48064354ull }, { 0x00640E11FA873B05ull },
    { 0x0063E7063E7063E8ull }, { 0x0063C018F0063C02ull }, { 0x00639949EBC4DCFDull }, { 0x006372990E5F9020ull },
    { 0x00634C0634C0634Dull }, { 0x006325913C07BEF0ull }, { 0x0062FF3A018BFCE9ull }, { 0x0062D90062D90063ull },
    { 0x0062B2E43DAFCEA7ull }, { 0x00628CE5700628CFull }, { 0x00626703D8062671ull }, { 0x0062413F540DD12Dull },
    { 0x00621B97C2AEC127ull }, { 0x0061F60D02ADBA5Cull }, { 0x0061D09EF3024AE4ull }, { 0x0061AB4D72D66A11ull },
    { 0x0061861861861862ull }, { 0x006160FF9E9F0062ull }, { 0x00613C0309E01850ull }, { 0x00611722833944A6ull },
    { 0x0060F25DEACAFB75ull }, { 0x0060CDB520E5E88Full }, { 0x0060A928060A9281ull }, { 0x006084B67AE90061ull },
    { 0x0060606060606061ull }, { 0x00603C25977EAF2Eull }, { 0x0060180601806019ull }, { 0x005FF4017FD00600ull },
    { 0x005FD017F405FD02ull }, { 0x005FAC493FE814EEull }, { 0x005F889545693C75ull }, { 0x005F64FBE6A92D17ull },
    { 0x005F417D05F417D1ull }, { 0x005F1E1885C2527Dull }, { 0x005EFACE48B805F0ull }, { 0x005ED79E31A4DCCEull },
    { 0x005EB4882383B30Eull }, { 0x005E918C017A4631ull }, { 0x005E6EA9AED8E62Eull }, { 0x005E4BE10F1A270Cull },
    { 0x005E293205E29321ull }, { 0x005E069C77005E07ull }, { 0x005DE420466B1835ull }, { 0x005DC1BD58436341ull },
    { 0x005D9F7390D2A6C5ull }, { 0x005D7D42D48AC5F0ull }, { 0x005D5B2B0805D5B3ull }, { 0x005D392C1005D393ull },
    { 0x005D1745D1745D18ull }, { 0x005CF578316267DBull }, { 0x005CD3C31507FA33ull }, { 0x005CB22661C3E47Cull },
    { 0x005C90A1FD1B7AF1ull }, { 0x005C6F35CCBA5029ull }, { 0x005C4DE1B671F023ull }, { 0x005C2CA5A0399BE8ull },
    { 0x005C0B81702E05C1ull }, { 0x005BEA750C910E01ull }, { 0x005BC9805BC9805Cull }, { 0x005BA8A34462D1D1ull },
    { 0x005B87DDAD0CDF1Cull }, { 0x005B672F7C9BABBDull }, { 0x005B46989A072184ull }, { 0x005B2618EC6AD0A6ull },
    { 0x005B05B05B05B05Cull }, { 0x005AE55ECD39E00Cull }, { 0x005AC5242A8C68F4ull }, { 0x005AA5005AA5005Bull },
    { 0x005A84F3454DCA42ull }, { 0x005A64FCD2731C9Aull }, { 0x005A451CEA234300ull }, { 0x005A2553748E42E8ull },
    { 0x005A05A05A05A05Bull }, { 0x0059E60382FC231Eull }, { 0x0059C67CD8059C68ull }, { 0x0059A70C41D6AD01ull },
    { 0x005987B1A9448BE5ull }, { 0x0059686CF744CD5Cull }, { 0x0059493E14ED2A8Dull }, { 0x00592A24EB73497Eull },
    { 0x00590B21642C8591ull }, { 0x0058EC33688DB872ull }, { 0x0058CD5AE22B0379ull }, { 0x0058AE97BAB79977ull },
    { 0x00588FE9DC0588FFull }, { 0x0058715130058716ull }, { 0x005852CDA0C6BA4Full }, { 0x0058345F18768660ull },
    { 0x0058160581605817ull }, { 0x0057F7C0C5ED71BEull }, { 0x0057D990D0A4B7F0ull }, { 0x0057BB758C2A7ECDull },
    { 0x00579D6EE340579Eull }, { 0x00577F7CC0C4DED9ull }, { 0x0057619F0FB38A95ull }, { 0x005743D5BB24795Bull },
    { 0x00572620AE4C415Dull }, { 0x0057087FD47BC016ull }, { 0x0056EAF3191FEA46ull }, { 0x0056CD7A67C19C51ull },
    { 0x0056B015AC056B02ull }, { 0x005692C4D1AB74ACull }, { 0x00567587C48F32A9ull }, { 0x0056585E70A74B37ull },
    { 0x00563B48C20563B5ull }, { 0x00561E46A4D5F338ull }, { 0x0056015805601581ull }, { 0x0055E47CD0055E48ull },
    { 0x0055C7B4F141ACE7ull }, { 0x0055AB0055AB0056ull }, { 0x00558E5EE9F14B88ull }, { 0x005571D09ADE4A19ull },
    { 0x0055555555555556ull }, { 0x005538ED06533998ull }, { 0x00551C979AEE0BF9ull }, { 0x0055005500550056ull },
    { 0x0054E42523D03FACull }, { 0x0054C807F2C0BEC3ull }, { 0x0054ABFD5AA0152Bull }, { 0x0054900549005491ull },
    { 0x0054741FAB8BE055ull }, { 0x0054584C70054585ull }, { 0x00543C8B84471316ull }, { 0x005420DCD643B272ull },
    { 0x0054054054054055ull }, { 0x0053E9B5EBAD65F1ull }, { 0x0053CE3D8B75326Bull }, { 0x0053B2D721ACF48Full },
    { 0x005397829CBC14E6ull }, { 0x00537C3FEB20F006ull }, { 0x0053610EFB70B12Full }, { 0x005345EFBC572D37ull },
    { 0x00532AE21C96BDBAull }, { 0x00530FE60B081C8Eull }, { 0x0052F4FB769A3F84ull }, { 0x0052DA224E52346Full },
    { 0x0052BF5A814AFD6Bull }, { 0x0052A4A3FEB56D71ull }, { 0x005289FEB5D80529ull }, { 0x00526F6A960ED006ull },
    { 0x005254E78ECB419Cull }, { 0x00523A758F941346ull }, { 0x0052201488052202ull }, { 0x005205C467CF4C94ull },
    { 0x0051EB851EB851ECull }, { 0x0051D1569C9AAFC8ull }, { 0x0051B738D1658199ull }, { 0x00519D2BAD1C5FA5ull },
    { 0x0051832F1FD73E69ull }, { 0x0051694319C24E3Cull }, { 0x00514F678B1DDB29ull }, { 0x0051359C643E2D0Cull },
    { 0x00511BE1958B67ECull }, { 0x005102370F816C8Aull }, { 0x0050E89CC2AFB935ull }, { 0x0050CF129FB94AD0ull },
    { 0x0050B59897547E1Cull }, { 0x00509C2E9A4AF134ull }, { 0x005082D499796545ull }, { 0x0050698A85CFA083ull },
    { 0x0050505050505051ull }, { 0x00503725EA10EBA2ull }, { 0x00501E0B44399599ull }, { 0x0050050050050051ull },
    { 0x004FEC04FEC04FEDull }, { 0x004FD31941CAFDD2ull }, { 0x004FBA3D0A96BC1Cull }, { 0x004FA1704AA75946ull },
    { 0x004F88B2F392A40Aull }, { 0x004F7004F7004F71ull }, { 0x004F576646A9D717ull }, { 0x004F3ED6D45A63AEull },
    { 0x004F265691EEAF9Eull }, { 0x004F0DE57154EBEEull }, { 0x004EF583648CA553ull }, { 0x004EDD305DA6A970ull },
    { 0x004EC4EC4EC4EC4Full }, { 0x004EACB72A1A6E06ull }, { 0x004E9490E1EB208Aull }, { 0x004E7C79688BCDB9ull },
    { 0x004E6470B061FD8Dull }, { 0x004E4C76ABE3DC86ull }, { 0x004E348B4D982237ull }, { 0x004E1CAE8815F812ull },
    { 0x004E04E04E04E04Full }, { 0x004DED20921C9D12ull }, { 0x004DD56F472517B8ull }, { 0x004DBDCC5FF64847ull },
    { 0x004DA637CF781D1Full }, { 0x004D8EB188A262C4ull }, { 0x004D77397E7CABE1ull }, { 0x004D5FCFA41E396Eull },
    { 0x004D4873ECADE305ull }, { 0x004D31264B61FF66ull }, { 0x004D19E6B3804D1Aull }, { 0x004D02B5185DDB4Dull },
    { 0x004CEB916D5EF2C8ull }, { 0x004CD47BA5F6FF1Aull }, { 0x004CBD73B5A877E9ull }, { 0x004CA6799004CA68ull },
    { 0x004C8F8D28AC42FEull }, { 0x004C78AE734DF70Aull }, { 0x004C61DD63A7AED9ull }, { 0x004C4B19ED85CFB9ull },
    { 0x004C346404C34641ull }, { 0x004C1DBB9D4970B1ull }, { 0x004C0720AB100981ull }, { 0x004BF093221D1219ull },
    { 0x004BDA12F684BDA2ull }, { 0x004BC3A01C695C0Bull }, { 0x004BAD3A87FB452Dull }, { 0x004B96E22D78C410ull },
    { 0x004B8097012E025Dull }, { 0x004B6A58F774F3ECull }, { 0x004B542804B54281ull }, { 0x004B3E041D64399Cull },
    { 0x004B27ED3604B27Full }, { 0x004B11E34327004Cull }, { 0x004AFBE63968DC44ull }, { 0x004AE5F60D755237ull },
    { 0x004AD012B404AD02ull }, { 0x004ABA3C21DC6340ull }, { 0x004AA4724BCF0415ull }, { 0x004A8EB526BC241Eull },
    { 0x004A7904A7904A7Aull }, { 0x004A6360C344DE01ull }, { 0x004A4DC96EE01294ull }, { 0x004A383E9F74D68Bull },
    { 0x004A22C04A22C04Bull }, { 0x004A0D4E6415FBF4ull }, { 0x0049F7E8E2873936ull }, { 0x0049E28FBABB9941ull },
    { 0x0049CD42E2049CD5ull }, { 0x0049B8024DC0126Full }, { 0x0049A2CDF358049Bull }, { 0x00498DA5C842A85Full },
    { 0x00497889C2024BC5ull }, { 0x00496379D6254484ull }, { 0x00494E75FA45DEC9ull }, { 0x0049397E240A4C16ull },
    { 0x004924924924924Aull }, { 0x00490FB25F527AB9ull }, { 0x0048FADE5C5D816Dull }, { 0x0048E616361AC47Dull },
    { 0x0048D159E26AF37Dull }, { 0x0048BCA9573A3F14ull }, { 0x0048A8048A8048A9ull }, { 0x0048936B72401225ull },
    { 0x00487EDE0487EDE1ull }, { 0x00486A5C37716E9Bull }, { 0x004855E601215799ull }, { 0x0048417B57C78CD8ull },
    { 0x00482D1C319F0363ull }, { 0x004818C884EDB1B6ull }, { 0x0048048048048049ull }, { 0x0047F043713F3A2Cull },
    { 0x0047DC11F7047DC2ull }, { 0x0047C7EBCFC5AD91ull }, { 0x0047B3D0F1FEE131ull }, { 0x00479FC15436D651ull },
    { 0x00478BBCECFEE1D2ull }, { 0x004777C3B2F2E104ull }, { 0x004763D59CB92AF3ull }, { 0x00474FF2A10281D0ull },
    { 0x00473C1AB68A0474ull }, { 0x0047284DD4151FF8ull }, { 0x0047148BF0738164ull }, { 0x004700D5027F077Eull },
    { 0x0046ED29011BB4A5ull }, { 0x0046D987E337A0CCull }, { 0x0046C5F19FCAEB8Bull }, { 0x0046B2662DD7AE42ull },
    { 0x00469EE58469EE59ull }, { 0x00468B6F9A978F92ull }, { 0x0046780467804679ull }, { 0x004664A3E24D8ADDull },
    { 0x0046514E02328A71ull }, { 0x00463E02BE6C1B71ull }, { 0x00462AC20E40AF6Bull }, { 0x0046178BE9004618ull },
    { 0x0046046046046047ull }, { 0x0045F13F1CAFF2E3ull }, { 0x0045DE28646F5A11ull }, { 0x0045CB1C14B84C57ull },
    { 0x0045B81A2509CDE4ull }, { 0x0045A5228CEC23EAull }, { 0x0045923543F0C805ull }, { 0x00457F5241B25BC3ull },
    { 0x00456C797DD49C35ull }, { 0x004559AAF004559Bull }, { 0x004546E68FF75724ull }, { 0x0045342C556C66BAull },
    { 0x0045217C382B34EEull }, { 0x00450ED6300450EEull }, { 0x0044FC3A34D11C91ull }, { 0x0044E9A83E73C079ull },
    { 0x0044D72044D72045ull }, { 0x0044C4A23FEECED8ull }, { 0x0044B22E27B702AFull }, { 0x00449FC3F4348A53ull },
    { 0x00448D639D74C0CEull }, { 0x00447B0D1B8D8247ull }, { 0x004468C0669D209Aull }, { 0x0044567D76CA5818ull },
    { 0x0044444444444445ull }, { 0x00443214C74254B7ull }, { 0x00441FEEF80441FFull }, { 0x00440DD2CED202A9ull },
    { 0x0043FBC043FBC044ull }, { 0x0043E9B74FD9CC89ull }, { 0x0043D7B7EACC9687ull }, { 0x0043C5C20D3C9FE7ull },
    { 0x0043B3D5AF9A7240ull }, { 0x0043A1F2CA5E947Aull }, { 0x0043901956098044ull }, { 0x00437E494B239799ull },
    { 0x00436C82A23D1A57ull }, { 0x00435AC553EE1BE4ull }, { 0x0043491158D678E8ull }, { 0x00433766A99DCD11ull },
    { 0x004325C53EF368ECull }, { 0x0043142D118E47CCull }, { 0x0043029E1A2D05C3ull }, { 0x0042F1185195D5A4ull },
    { 0x0042DF9BB096771Full }, { 0x0042CE2830042CE3ull }, { 0x0042BCBDC8BBB2D6ull }, { 0x0042AB5C73A13459ull },
    { 0x00429A0429A0429Bull }, { 0x004288B4E3ABCAFCull }, { 0x0042776E9ABE0D81ull }, { 0x0042663147D89353ull },
    { 0x004254FCE4042550ull }, { 0x004243D16850C2A8ull }, { 0x004232AECDD59789ull }, { 0x004221950DB0F3DCull },
    { 0x0042108421084211ull }, { 0x0041FF7C0107FDF1ull }, { 0x0041EE7CA6E3AB87ull }, { 0x0041DD860BD5CE17ull },
    { 0x0041CC98291FDF1Aull }, { 0x0041BBB2F80A4554ull }, { 0x0041AAD671E44BEEull }, { 0x00419A02900419A1ull },
    { 0x004189374BC6A7F0ull }, { 0x004178749E8FBA71ull }, { 0x004167BA81C9D61Aull }, { 0x00415708EEE638AAull },
    { 0x0041465FDF5CD011ull }, { 0x004135BF4CAC31EEull }, { 0x0041252730599316ull }, { 0x0041149783F0BF2Dull },
    { 0x0041041041041042ull }, { 0x0040F391612C6681ull }, { 0x0040E31ADE091FF0ull }, { 0x0040D2ACB1401035ull },
    { 0x0040C246D47D786Aull }, { 0x0040B1E94173FEFEull }, { 0x0040A193F1DCA7A4ull }, { 0x00409146DF76CB4Aull },
    { 0x0040810204081021ull }, { 0x004070C5595C61ABull }, { 0x00406090D945E8DEull }, { 0x004050647D9D0446ull },
    { 0x0040404040404041ull }, { 0x004030241B144F3Cull }, { 0x0040201008040202ull }, { 0x0040100401004011ull },
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
    printf("static const ModRatio mod_ratio_tbl32[kMaxModTable] = {\n");
    for (std::uint32_t n = 0; n < kMaxModTable; n++) {
        if ((n % 4) == 0) {
            printf("    ");
        }
        printf("{ 0x%08X%08Xull },", (uint32_t)(modRatioTbl[n].M >> 32u),
                                     (uint32_t)(modRatioTbl[n].M & 0xFFFFFFFFul));
        if ((n % 4) == 3) {
            printf("\n");
        } else {
            printf(" ");
        }
    }
    printf("};\n\n");
}

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__)

#if defined(_MSC_VER)

//
// See: https://github.com/lemire/fastmod
//
// __umulh() is only available in x64 mode under Visual Studio: don't compile to 32-bit!
//           but, I write a version for x86 (32bit) mode: see below.
//
static inline
uint64_t mul128_high_u32(uint64_t low64_bits, uint32_t divisor) {
    return __umulh(low64_bits, divisor);
}

static inline
uint64_t mul128_high_u64(uint64_t low64_bits, uint64_t divisor) {
    return __umulh(low64_bits, divisor);
}

#else // !_MSC_VER

static inline
uint64_t mul128_high_u32(uint64_t low64_bits, uint32_t divisor) {
    return (((__uint128_t)low64_bits * divisor) >> 64u);
}

static inline
uint64_t mul128_high_u64(uint64_t low64_bits, uint64_t divisor) {
    return (((__uint128_t)low64_bits * divisor) >> 64u);
}

#endif // _MSC_VER

static inline
uint64_t mul128_high_u64_ex(uint64_t low64_bits, uint64_t divisor) {
    return mul128_high_u64(low64_bits, divisor);
}

#else // !__amd64__

/*****************************************************************

   low64_bits = low0, high0
   divisor32  = low1, 0

   low64_bits * divisor32 =

 |           |             |            |           |
 |           |             |      high0 * 0         |  product03
 |           |       low0  * 0          |           |  product02
 |           |       high0 * low1       |           |  product01
 |      low0 * low1        |            |           |  product00
 |           |             |            |           |

*****************************************************************/

static inline
uint32_t mul128_high_u32(uint64_t low64_bits, uint32_t divisor) {
    uint32_t low0  = (uint32_t)(low64_bits & 0xFFFFFFFFull);
    uint32_t high0 = (uint32_t)(low64_bits >> 32u);
    if (high0 == 0)
        return 0;

    uint64_t product00 = (uint64_t)divisor * low0;
    uint64_t product01 = (uint64_t)divisor * high0;

    uint32_t product00_high = (uint32_t)(product00 >> 32u);
    uint32_t product01_low  = (uint32_t)(product01 & 0xFFFFFFFFull);
    uint32_t product01_high = (uint32_t)(product01 >> 32u);

    uint32_t carry32 = (product00_high > ~product01_low) ? 1 : 0;
    uint32_t result = product01_high + carry32;
    return result;
}

/*****************************************************************

   low64_bits = low0, high0
   divisor64  = low1, high1

   low64_bits * divisor64 =

 |           |             |            |           |
 |           |             |      high0 * high1     |  product03
 |           |       low0  * high1      |           |  product02
 |           |       high0 * low1       |           |  product01
 |      low0 * low1        |            |           |  product00
 |           |             |            |           |

*****************************************************************/

static inline
uint64_t mul128_high_u64(uint64_t low64_bits, uint64_t divisor) {
    uint32_t low1  = (uint32_t)(divisor & 0xFFFFFFFFull);
    uint32_t high1 = (uint32_t)(divisor >> 32u);
    if (high1 == 0) {
        return mul128_high_u32(low64_bits, low1);
    }

    uint32_t low0  = (uint32_t)(low64_bits & 0xFFFFFFFFull);
    uint32_t high0 = (uint32_t)(low64_bits >> 32u);

    if (high0 == 0) {
        uint64_t product00 = (uint64_t)low0 * low1;
        uint64_t product02 = (uint64_t)low0 * high1;

        uint32_t product00_high = (uint32_t)(product00 >> 32u);
        uint32_t product02_low  = (uint32_t)(product02 & 0xFFFFFFFFull);
        uint32_t product02_high = (uint32_t)(product02 >> 32u);

        uint64_t product_mid64 = product00_high + product02_low;
        uint32_t carry32 = (uint32_t)(product_mid64 >> 32u);

        uint64_t result = product02_high + carry32;
        return result;
    }
    else {
        uint64_t product00 = (uint64_t)low0 * low1;
        uint64_t product01 = (uint64_t)high0 * low1;
        uint64_t product02 = (uint64_t)low0 * high1;
        uint64_t product03 = (uint64_t)high0 * high1;

        uint32_t product00_high = (uint32_t)(product00 >> 32u);
        uint32_t product01_low  = (uint32_t)(product01 & 0xFFFFFFFFull);
        uint32_t product02_low  = (uint32_t)(product02 & 0xFFFFFFFFull);
        uint32_t product01_high = (uint32_t)(product01 >> 32u);
        uint32_t product02_high = (uint32_t)(product02 >> 32u);

        uint64_t product_mid64 = product00_high + product01_low + product02_low;
        uint32_t carry32 = (uint32_t)(product_mid64 >> 32u);

        uint64_t result = product01_high + product02_high + carry32 + product03;
        return result;
    }
}

static inline
uint64_t mul128_high_u64_ex(uint64_t low64_bits, uint64_t divisor) {
    uint32_t low0  = (uint32_t)(low64_bits & 0xFFFFFFFFull);
    uint32_t high0 = (uint32_t)(low64_bits >> 32u);
    uint32_t low1  = (uint32_t)(divisor & 0xFFFFFFFFull);
    uint32_t high1 = (uint32_t)(divisor >> 32u);

    if (high0 == 0) {
        uint64_t product00 = (uint64_t)low0 * low1;
        uint64_t product02 = (uint64_t)low0 * high1;

        uint32_t product00_high = (uint32_t)(product00 >> 32u);
        uint32_t product02_low  = (uint32_t)(product02 & 0xFFFFFFFFull);
        uint32_t product02_high = (uint32_t)(product02 >> 32u);

        uint64_t product_mid64 = product00_high + product02_low;
        uint32_t carry32 = (uint32_t)(product_mid64 >> 32u);

        uint64_t result = product02_high + carry32;
        return result;
    }
    else {
        uint64_t product00 = (uint64_t)low0 * low1;
        uint64_t product01 = (uint64_t)high0 * low1;
        uint64_t product02 = (uint64_t)low0 * high1;
        uint64_t product03 = (uint64_t)high0 * high1;

        uint32_t product00_high = (uint32_t)(product00 >> 32u);
        uint32_t product01_low  = (uint32_t)(product01 & 0xFFFFFFFFull);
        uint32_t product02_low  = (uint32_t)(product02 & 0xFFFFFFFFull);
        uint32_t product01_high = (uint32_t)(product01 >> 32u);
        uint32_t product02_high = (uint32_t)(product02 >> 32u);

        uint64_t product_mid64 = product00_high + product01_low + product02_low;
        uint32_t carry32 = (uint32_t)(product_mid64 >> 32u);

        uint64_t result = product01_high + product02_high + carry32 + product03;
        return result;
    }
}

#endif // __amd64__

static inline
std::uint32_t fast_mod_u32(std::uint32_t value, std::uint32_t divisor)
{
    if (divisor >= kMaxModTable) {
        return (value % divisor);
    } else {
        ModRatio ratio = mod_ratio_tbl32[divisor];
        std::uint64_t low64_bits = (std::uint64_t)value * ratio.M;
        std::uint32_t result = (std::uint32_t)mul128_high_u32(low64_bits, divisor);
        return result;
    }
}

static inline
std::uint64_t fast_mod_u64(std::uint64_t value, std::uint64_t divisor)
{
    if (divisor >= kMaxModTable) {
        return (value % divisor);
    } else {
        std::uint32_t divisor32 = (std::uint32_t)(divisor & 0xFFFFFFFFul);
        ModRatio ratio = mod_ratio_tbl32[divisor32];
        std::uint64_t low64_bits = value * ratio.M;
        std::uint32_t result = (std::uint32_t)mul128_high_u32(low64_bits, divisor32);
        return result;
    }
}

} // namespace jstd

#endif // JSTD_FAST_MOD_H
