/* p_ps1.h --

   This file is part of the UPX executable compressor.

   Copyright (C) 1996-2025 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996-2025 Laszlo Molnar
   Copyright (C) 2002-2025 Jens Medoch
   All Rights Reserved.

   UPX and the UCL library are free software; you can redistribute them
   and/or modify them under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer              Laszlo Molnar
   <markus@oberhumer.com>               <ezerotven+github@gmail.com>

   Jens Medoch
   <jssg@users.sourceforge.net>
 */

#pragma once

/*************************************************************************
// ps1/exe
**************************************************************************/

class PackPs1 final : public Packer {
    typedef Packer super;

public:
    explicit PackPs1(InputFile *f);
    virtual int getVersion() const override { return 13; }
    virtual int getFormat() const override { return UPX_F_PS1_EXE; }
    virtual const char *getName() const override { return "ps1/exe"; }
    virtual const char *getFullName(const Options *) const override { return "mipsel.r3000-ps1"; }
    virtual const int *getCompressionMethods(int method, int level) const override;
    virtual const int *getFilters() const override;

    virtual void pack(OutputFile *fo) override;
    virtual void unpack(OutputFile *fo) override;

    virtual tribool canPack() override;
    virtual tribool canUnpack() override;

protected:
    void putBkupHeader(const byte *src, byte *dst, unsigned *len);
    bool getBkupHeader(byte *src, byte *dst);
    bool readBkupHeader();
    virtual void buildLoader(const Filter *ft) override;
    bool findBssSection();
    virtual Linker *newLinker() const override;

    int readFileHeader();
    bool checkFileHeader();

    struct alignas(1) ps1_exe_t {
        // ident string
        byte id[8];
        // is nullptr
        LE32 text;
        // is nullptr
        LE32 data;
        // initial program counter
        LE32 epc;
        // initial gp register value
        LE32 gp;
        // load offset of binary data
        LE32 tx_ptr, tx_len;
        LE32 da_ptr, da_len;
        LE32 bs_ptr, bs_len;
        // initial stack params
        LE32 is_ptr, is_len;
        // saved regs on execution
        LE32 sp, fp, gp0, ra, k0;
        // origin Jap/USA/Europe
        byte origin[60];
        // backup of the original header (epc - is_len)
        // id & the upx header ...
    };

    // for unpack
    struct alignas(1) ps1_exe_hb_t {
        LE32 ih_bkup[10];
        // plus checksum for the backup
        LE32 ih_csum;
    };

    struct alignas(1) ps1_exe_chb_t {
        byte id;
        byte len;
        LE16 ih_csum;
        byte ih_bkup;
    };

    struct alignas(1) bss_nfo {
        LE16 hi1, op1, lo1, op2;
        LE16 hi2, op3, lo2, op4;
    };

    ps1_exe_t ih = {}, oh = {};
    ps1_exe_hb_t bh = {};

    bool isCon = false;
    bool is32Bit = false;
    bool buildPart2 = false;
    bool foundBss = false;
    unsigned ram_size = 0;
    unsigned sa_cnt = 0, overlap = 0;
    unsigned sz_lunc = 0, sz_lcpr = 0;
    unsigned pad_code = 0;
    unsigned bss_start = 0, bss_end = 0;
    // filesize-PS_HDR_SIZE
    unsigned fdata_size = 0;
};

/* vim:set ts=4 sw=4 et: */
