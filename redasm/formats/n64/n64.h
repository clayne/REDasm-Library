#ifndef N64_H
#define N64_H

#include "../../plugins/plugins.h"

#define N64_ROM_HEADER_SIZE    4096
#define N64_MEDIA_FORMAT_SIZE  4
#define N64_IMAGE_NAME_SIZE    20
#define N64_CART_ID_SIZE       2
#define N64_BOOT_CODE_SIZE     4032

#define N64_ROM_CHECKSUM_START      0x00001000
#define N64_ROM_CHECKSUM_LENGTH     0x00100000
#define N64_ROM_CHECKSUM_CIC_6102   0xF8CA4DDC      // From n64crc (http://n64dev.org/n64crc.html)
#define N64_ROM_CHECKSUM_CIC_6103   0xA3886759      // From n64crc (http://n64dev.org/n64crc.html)
#define N64_ROM_CHECKSUM_CIC_6105   0xDF26F436      // From n64crc (http://n64dev.org/n64crc.html)
#define N64_ROM_CHECKSUM_CIC_6106   0x1FEA617A      // From n64crc (http://n64dev.org/n64crc.html)

#define N64_BOOT_CODE_CIC_6101_CRC       0x6170A4A1     // CIC 6101 BOOT CODE CRC
#define N64_BOOT_CODE_CIC_7102_CRC       0x009E9EA3     // CIC 7102 BOOT CODE CRC
#define N64_BOOT_CODE_CIC_6102_CRC       0x90BB6CB5     // CIC 6102-7101 BOOT CODE CRC
#define N64_BOOT_CODE_CIC_6103_CRC       0x0B050EE0     // CIC 6103-7103 BOOT CODE CRC
#define N64_BOOT_CODE_CIC_6105_CRC       0x98BC2C86     // CIC 6105-7105 BOOT CODE CRC
#define N64_BOOT_CODE_CIC_6106_CRC       0xACC8580A     // CIC 6106-7106 BOOT CODE CRC

#define ROL(i, b) (((i) << (b)) | ((i) >> (32 - (b))))
#define BYTES2LONG(b) ( (b)[0] << 24 | \
                        (b)[1] << 16 | \
                        (b)[2] <<  8 | \
                        (b)[3] )


namespace REDasm {

struct N64RomHeader // From: http://en64.shoutwiki.com/wiki/ROM#Cartridge_ROM_Header
{
    u8 pi_bsb_dom1_lat_reg;
    u8 pi_bsb_dom1_pgs_reg;
    u8 pi_bsd_dom1_pwd_reg;
    u8 pi_bsb_dom1_pgs_reg2;
    u32 clock_rate_override;
    u32 program_counter;
    u32 release_address;
    u32 crc1;
    u32 crc2;
    u64 unknown1; // UNKNOWN/NOT USED
    char image_name[N64_IMAGE_NAME_SIZE];
    u32 unknown2; // UNKNOWN/NOT USED
    char media_format[N64_MEDIA_FORMAT_SIZE];
    char cart_id[N64_CART_ID_SIZE];
    char country_code;
    u8 version;
    char boot_code[N64_BOOT_CODE_SIZE];
};

class N64RomFormat: public FormatPluginT<N64RomHeader>
{
    public:
        N64RomFormat(Buffer& buffer);
        virtual const char* name() const;
        virtual u32 bits() const;
        virtual const char* assembler() const;
        virtual endianness_t endianness() const;
        virtual Analyzer* createAnalyzer(DisassemblerAPI *disassembler, const SignatureFiles &signatures) const;
        virtual bool load();

    private:
        u32 getEP();
        u32 calculateChecksum(u32 *crc);
        u32 getCICVersion();
        u8 checkMediaType();
        u8 checkCountryCode();
        u8 checkChecksum();
        bool validateRom();
};

DECLARE_FORMAT_PLUGIN(N64RomFormat, n64rom)

}

#endif // N64_H
