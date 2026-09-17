import os

bin_path = r"D:\desarrollos\ABDSynths\ABDCZ101\GITS\casio_cz101_rom_disassembly-master\casio_cz101_rom_disassembly-master\preset_patch_rom.bin"
out_path = r"D:\desarrollos\ABDSynths\ABDCZ101\Source\State\Presets\factory_data_bank_cz101.cpp"

with open(bin_path, "rb") as f:
    data = f.read()

# 16 patches * 128 bytes = 2048 bytes
if len(data) != 2048:
    print("Warning: expected 2048 bytes, got", len(data))

# SysEx header
# F0 44 00 00 70 20 20
header = [0xF0, 0x44, 0x00, 0x00, 0x70, 0x20, 0x20]

out_lines = []
out_lines.append('#include "../FactoryPresets.h"')
out_lines.append("")
out_lines.append("namespace CZ101 {")
out_lines.append("namespace State {")
out_lines.append("")
out_lines.append("const uint8_t FACTORY_PRESET_DATA_BANK_CZ101[SYSEX_PATCH_SIZE * 16] = {")

for i in range(16):
    patch_data = data[i*128:(i+1)*128]
    sysex = list(header)
    for b in patch_data:
        sysex.append(b & 0x0F)
        sysex.append((b >> 4) & 0x0F)
        
    # Checksum calculation: sum of all nibbles
    checksum_val = sum(sysex[7:])
    checksum = (0 - checksum_val) & 0x7F
    # sysex.append(checksum)
    sysex.append(0xF7)
    
    # Format into C++ array
    line = ",".join(f"0x{b:02X}" for b in sysex)
    if i < 15:
        line += ","
    out_lines.append(line)

out_lines.append("};")
out_lines.append("")
out_lines.append("} // namespace State")
out_lines.append("} // namespace CZ101")
out_lines.append("")

with open(out_path, "w") as f:
    f.write("\n".join(out_lines))

print("Created", out_path)
