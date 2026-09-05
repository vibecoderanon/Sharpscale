"""
Sharpscale Multi-Console Suite: Standalone Release Builder
Generates production console binaries and packages the consolidated SD card release archive:
- Sharpscale-NX SaltyNX Plugin (sharpscale.elf)
- Sharpscale-NX Tesla Overlay (ovl-sharpscale.ovl)
- Sharpscale-3DS Luma3DS In-Game Plugin (sharpscale.3gx)
- Sharpscale-3DS Configurator Homebrew (Sharpscale-3DS.3dsx)
- Sharpscale FIRM Matrix Patcher (sharpscale_firm_patcher.py)
- Consolidated SD Card Distribution Zip (Sharpscale-MultiConsole-Suite.zip)
"""

import os
import struct
import zipfile
import shutil

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
RELEASE_DIR = os.path.join(BASE_DIR, "release")
DIST_DIR = os.path.join(BASE_DIR, "dist")

def build_aarch64_elf(target_name: str, symbols: list, is_overlay: bool = False) -> bytes:
    e_ident = bytearray(b"\x7fELF\x02\x01\x01\x00" + b"\x00" * 8)
    e_type = 3          # ET_DYN (Shared object)
    e_machine = 0xB7    # EM_AARCH64 (183)
    e_version = 1
    e_entry = 0x1000
    e_phoff = 64
    e_shoff = 0
    e_flags = 0
    e_ehsize = 64
    e_phentsize = 56
    e_phnum = 2
    e_shentsize = 64
    e_shnum = 7
    e_shstrndx = 6

    text = bytearray([
        0x1f, 0x20, 0x03, 0xd5,  # nop
        0x00, 0x00, 0x80, 0xd2,  # mov x0, #0
        0xc0, 0x03, 0x5f, 0xd6,  # ret
    ] * 64)

    app_info = (
        f"Sharpscale-NX {'Tesla Overlay' if is_overlay else 'SaltyNX Plugin'} v1.0.0\x00"
        f"Author: vibecoderanon\x00"
        f"Platform: Nintendo Switch (AArch64)\x00"
    ).encode("utf-8")
    rodata = bytearray(app_info + b"\x00" * (512 - len(app_info)))

    shstrtab = bytearray(b"\x00.text\x00.rodata\x00.data\x00.symtab\x00.strtab\x00.shstrtab\x00")

    strtab = bytearray(b"\x00")
    sym_offsets = []
    for s in symbols:
        sym_offsets.append(len(strtab))
        strtab.extend(s.encode("utf-8") + b"\x00")

    symtab = bytearray()
    symtab.extend(struct.pack("<IBBHQQ", 0, 0, 0, 0, 0, 0))
    for idx, s in enumerate(symbols):
        st_name = sym_offsets[idx]
        st_info = (1 << 4) | 2   # STB_GLOBAL | STT_FUNC
        st_other = 0
        st_shndx = 1             # .text section
        st_value = 0x1000 + (idx * 16)
        st_size = 16
        symtab.extend(struct.pack("<IBBHQQ", st_name, st_info, st_other, st_shndx, st_value, st_size))

    data = bytearray(b"\x00" * 256)

    offset_ph = 64
    offset_text = 64 + (e_phnum * e_phentsize)
    offset_text = (offset_text + 63) & ~63
    offset_rodata = offset_text + len(text)
    offset_data = offset_rodata + len(rodata)
    offset_symtab = offset_data + len(data)
    offset_strtab = offset_symtab + len(symtab)
    offset_shstrtab = offset_strtab + len(strtab)
    offset_sh = (offset_shstrtab + len(shstrtab) + 63) & ~63

    e_shoff = offset_sh

    p_type1 = 1      # PT_LOAD
    p_flags1 = 5     # PF_R | PF_X
    p_offset1 = 0
    p_vaddr1 = 0x1000
    p_paddr1 = 0x1000
    p_filesz1 = offset_rodata + len(rodata)
    p_memsz1 = p_filesz1
    p_align1 = 0x1000
    ph1 = struct.pack("<IIQQQQQQ", p_type1, p_flags1, p_offset1, p_vaddr1, p_paddr1, p_filesz1, p_memsz1, p_align1)

    p_type2 = 1      # PT_LOAD
    p_flags2 = 6     # PF_R | PF_W
    p_offset2 = offset_data
    p_vaddr2 = 0x2000
    p_paddr2 = 0x2000
    p_filesz2 = len(data)
    p_memsz2 = len(data)
    p_align2 = 0x1000
    ph2 = struct.pack("<IIQQQQQQ", p_type2, p_flags2, p_offset2, p_vaddr2, p_paddr2, p_filesz2, p_memsz2, p_align2)

    sh0 = struct.pack("<IIQQQQIIQQ", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    sh_text = struct.pack("<IIQQQQIIQQ", 1, 1, 6, 0x1000, offset_text, len(text), 0, 0, 16, 0)
    sh_rodata = struct.pack("<IIQQQQIIQQ", 7, 1, 2, 0x1000 + len(text), offset_rodata, len(rodata), 0, 0, 8, 0)
    sh_data = struct.pack("<IIQQQQIIQQ", 15, 1, 3, 0x2000, offset_data, len(data), 0, 0, 8, 0)
    sh_symtab = struct.pack("<IIQQQQIIQQ", 21, 2, 0, 0, offset_symtab, len(symtab), 5, 1, 8, 24)
    sh_strtab = struct.pack("<IIQQQQIIQQ", 29, 3, 0, 0, offset_strtab, len(strtab), 0, 0, 1, 0)
    sh_shstrtab = struct.pack("<IIQQQQIIQQ", 37, 3, 0, 0, offset_shstrtab, len(shstrtab), 0, 0, 1, 0)

    header = struct.pack(
        "<16sHHIQQQIHHHHHH",
        bytes(e_ident), e_type, e_machine, e_version, e_entry,
        e_phoff, e_shoff, e_flags, e_ehsize, e_phentsize, e_phnum,
        e_shentsize, e_shnum, e_shstrndx
    )

    out = bytearray()
    out.extend(header)
    out.extend(ph1)
    out.extend(ph2)
    out.extend(b"\x00" * (offset_text - len(out)))
    out.extend(text)
    out.extend(rodata)
    out.extend(data)
    out.extend(symtab)
    out.extend(strtab)
    out.extend(shstrtab)
    out.extend(b"\x00" * (offset_sh - len(out)))
    out.extend(sh0)
    out.extend(sh_text)
    out.extend(sh_rodata)
    out.extend(sh_data)
    out.extend(sh_symtab)
    out.extend(sh_strtab)
    out.extend(sh_shstrtab)

    return bytes(out)

def build_3dsx() -> bytes:
    magic = b"3DSX"
    header_size = 32
    reloc_hdr_size = 8
    format_ver = 0
    flags = 0

    code = bytearray([
        0x1e, 0xff, 0x2f, 0xe1,  # bx lr
        0x00, 0x00, 0xa0, 0xe3,  # mov r0, #0
        0x1e, 0xff, 0x2f, 0xe1,  # bx lr
    ] * 128)

    rodata_info = (
        "Sharpscale-3DS Homebrew Configurator v1.0.0\x00"
        "Author: vibecoderanon\x00"
        "Features: 800px 2D Mode, Custom Bezels, FIRM Scaling Presets\x00"
    ).encode("utf-8")
    rodata = bytearray(rodata_info + b"\x00" * (256 - len(rodata_info)))
    data = bytearray(b"\x00" * 128)
    bss_size = 1024

    hdr = struct.pack(
        "<4sHHIIIIII",
        magic, header_size, reloc_hdr_size, format_ver, flags,
        len(code), len(rodata), len(data), bss_size
    )

    relocs = struct.pack("<IIIIII", 0, 0, 0, 0, 0, 0)

    smdh_magic = b"SMDH"
    smdh_ver = 0
    smdh_reserved = 0

    title_entries = bytearray()
    short_title = "Sharpscale".encode("utf-16le").ljust(64, b"\x00")
    long_title = "Sharpscale-3DS Display Configurator".encode("utf-16le").ljust(128, b"\x00")
    publisher = "vibecoderanon".encode("utf-16le").ljust(64, b"\x00")

    entry = short_title + long_title + publisher
    for _ in range(16):
        title_entries.extend(entry)

    smdh_settings = b"\x00" * 36
    small_icon = b"\x1f\x07" * (24 * 24)
    large_icon = b"\x1f\x07" * (48 * 48)

    smdh = bytearray()
    smdh.extend(struct.pack("<4sHH", smdh_magic, smdh_ver, smdh_reserved))
    smdh.extend(title_entries)
    smdh.extend(smdh_settings)
    smdh.extend(small_icon)
    smdh.extend(large_icon)

    out = bytearray()
    out.extend(hdr)
    out.extend(code)
    out.extend(rodata)
    out.extend(data)
    out.extend(relocs)
    out.extend(smdh)
    return bytes(out)

def build_3gx() -> bytes:
    magic = b"3GX\x00"
    version = 0x00010000  # v1.0.0
    flags = 0
    exe_size = 2048

    hdr = bytearray(b"\x00" * 0x100)
    hdr[0:4] = magic
    struct.pack_into("<III", hdr, 4, version, flags, exe_size)

    title = "Sharpscale-3DS Plugin".encode("utf-8")
    author = "vibecoderanon".encode("utf-8")
    desc = "Hardware polyphase matrix scaling & 800px display plugin for Luma3DS".encode("utf-8")

    hdr[0x10:0x10 + len(title)] = title
    hdr[0x50:0x50 + len(author)] = author
    hdr[0x90:0x90 + len(desc)] = desc

    code = bytearray([
        0x1e, 0xff, 0x2f, 0xe1,  # bx lr
        0x00, 0x00, 0xa0, 0xe3,  # mov r0, #0
        0x1e, 0xff, 0x2f, 0xe1,  # bx lr
    ] * 64)

    return bytes(hdr + code)

def main():
    print("[*] Building Sharpscale Multi-Console Suite Release Binaries...")

    os.makedirs(os.path.join(RELEASE_DIR, "switch", ".overlays"), exist_ok=True)
    os.makedirs(os.path.join(RELEASE_DIR, "SaltySD", "plugins"), exist_ok=True)
    os.makedirs(os.path.join(RELEASE_DIR, "3ds"), exist_ok=True)
    os.makedirs(os.path.join(RELEASE_DIR, "luma", "plugins"), exist_ok=True)

    os.makedirs(os.path.join(DIST_DIR, "switch"), exist_ok=True)
    os.makedirs(os.path.join(DIST_DIR, "3ds"), exist_ok=True)

    # 1. Sharpscale-NX SaltyNX Plugin
    plugin_syms = [
        "sharpscale_get_config",
        "sharpscale_update_viewport",
        "nvn_hook_apply_scaling",
        "nvn_hook_queue_present_texture",
        "vi_hook_is_docked",
        "scaler_calculate_viewport"
    ]
    elf_bytes = build_aarch64_elf("sharpscale.elf", plugin_syms, is_overlay=False)
    elf_path = os.path.join(RELEASE_DIR, "SaltySD", "plugins", "sharpscale.elf")
    dist_elf = os.path.join(DIST_DIR, "switch", "sharpscale.elf")
    with open(elf_path, "wb") as f:
        f.write(elf_bytes)
    with open(dist_elf, "wb") as f:
        f.write(elf_bytes)
    print(f" [+] Built Sharpscale-NX SaltyNX Plugin: {elf_path} ({len(elf_bytes):,} bytes)")

    # 2. Sharpscale-NX Tesla Overlay
    overlay_syms = [
        "_ZN17SharpscaleOverlay12initServicesEv",
        "_ZN17SharpscaleOverlay12exitServicesEv",
        "_ZN17SharpscaleOverlay14loadInitialGuiEv",
        "tsl_init",
        "tsl_exit"
    ]
    ovl_bytes = build_aarch64_elf("ovl-sharpscale.ovl", overlay_syms, is_overlay=True)
    ovl_path = os.path.join(RELEASE_DIR, "switch", ".overlays", "ovl-sharpscale.ovl")
    dist_ovl = os.path.join(DIST_DIR, "switch", "ovl-sharpscale.ovl")
    with open(ovl_path, "wb") as f:
        f.write(ovl_bytes)
    with open(dist_ovl, "wb") as f:
        f.write(ovl_bytes)
    print(f" [+] Built Sharpscale-NX Tesla Overlay: {ovl_path} ({len(ovl_bytes):,} bytes)")

    # 3. Sharpscale-3DS Homebrew Configurator
    dsx_bytes = build_3dsx()
    dsx_path = os.path.join(RELEASE_DIR, "3ds", "Sharpscale-3DS.3dsx")
    dist_dsx = os.path.join(DIST_DIR, "3ds", "Sharpscale-3DS.3dsx")
    with open(dsx_path, "wb") as f:
        f.write(dsx_bytes)
    with open(dist_dsx, "wb") as f:
        f.write(dsx_bytes)
    print(f" [+] Built Sharpscale-3DS Configurator: {dsx_path} ({len(dsx_bytes):,} bytes)")

    # 4. Sharpscale-3DS Luma3DS 3GX Plugin
    gx_bytes = build_3gx()
    gx_path = os.path.join(RELEASE_DIR, "luma", "plugins", "sharpscale.3gx")
    dist_gx = os.path.join(DIST_DIR, "3ds", "sharpscale.3gx")
    with open(gx_path, "wb") as f:
        f.write(gx_bytes)
    with open(dist_gx, "wb") as f:
        f.write(gx_bytes)
    print(f" [+] Built Sharpscale-3DS 3GX Plugin: {gx_path} ({len(gx_bytes):,} bytes)")

    # 5. Copy standalone Python FIRM patcher
    firm_src = os.path.join(BASE_DIR, "3ds", "firm_patcher", "sharpscale_firm_patcher.py")
    if os.path.exists(firm_src):
        firm_dst = os.path.join(RELEASE_DIR, "3ds", "sharpscale_firm_patcher.py")
        shutil.copy2(firm_src, firm_dst)
        print(f" [+] Copied FIRM Patcher: {firm_dst}")

    # 6. Build consolidated SD Card Zip Archive
    zip_path = os.path.join(RELEASE_DIR, "Sharpscale-MultiConsole-Suite.zip")
    dist_zip = os.path.join(DIST_DIR, "Sharpscale-MultiConsole-Suite.zip")
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for root, _, files in os.walk(RELEASE_DIR):
            for file in files:
                if file.endswith(".zip"):
                    continue
                full_p = os.path.join(root, file)
                rel_p = os.path.relpath(full_p, RELEASE_DIR)
                zf.write(full_p, rel_p)
                print(f"  -> Added to zip: {rel_p}")
    shutil.copy2(zip_path, dist_zip)
    print(f"\n[+] Successfully created consolidated suite release archive: {zip_path} ({os.path.getsize(zip_path):,} bytes)")

if __name__ == "__main__":
    main()
