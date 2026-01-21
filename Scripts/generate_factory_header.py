import os

# Paths
BASE_PATH = r"D:\desarrollos\ABDZ101\DOCS\patches\cz5000\cz1org64"
OUTPUT_HEADER = r"D:\desarrollos\ABDZ101\Source\State\FactoryPresets.h"
TXT_FILE = os.path.join(BASE_PATH, "factorypatchcz-1.txt")
SYX_FILES = [
    os.path.join(BASE_PATH, "CZ1ORGB1.SYX"),
    os.path.join(BASE_PATH, "CZ1ORGB2.SYX"),
    os.path.join(BASE_PATH, "CZ1ORGB3.SYX"),
    os.path.join(BASE_PATH, "CZ1ORGB4.SYX"),
]

def parse_names(txt_path):
    names = []
    with open(txt_path, 'r') as f:
        lines = f.readlines()
        
    # Simple parser: look for lines with names.
    # The file has names indented.
    # We expect 64 names.
    # Let's just grab non-empty lines that are not headers like "Bank X" or descriptions.
    
    current_names = []
    
    # Heuristic: Valid names are uppercase/mixed, < 20 chars, not containing "Bank" or "patches"
    skip_keywords = ["Bank", "patches", "Missing", "converted", "Use", "---"]
    
    for line in lines:
        line = line.strip()
        if not line: continue
        
        is_header = False
        for kw in skip_keywords:
            if kw in line: 
                is_header = True
                break
        if is_header: continue
        
        # Check if line looks like a patch name
        # Most are "BRASS 1", "STRINGS 1", etc.
        # Some are short like "HARP"
        current_names.append(line)
        
    return current_names

def generate_header():
    # 1. Read SysEx Data
    all_data = bytearray()
    for syx in SYX_FILES:
        with open(syx, 'rb') as f:
            all_data.extend(f.read())
            
    # 2. Read Names
    names = parse_names(TXT_FILE)
    
    # Safety Check
    if len(all_data) != 64 * 264:
        print(f"Warning: Data size {len(all_data)} != {64*264}")
        # Pad or trim?
        pass
        
    if len(names) < 64:
        print(f"Warning: Only found {len(names)} names, expected 64")
        while len(names) < 64: names.append("Unknown")
        
    # 3. Write Header
    with open(OUTPUT_HEADER, 'w') as f:
        f.write("#pragma once\n\n")
        f.write("#include <cstdint>\n\n")
        f.write("namespace CZ101 {\n")
        f.write("namespace State {\n\n")
        
        f.write(f"// Generated from {BASE_PATH}\n")
        f.write(f"static constexpr int FACTORY_PRESET_COUNT = 64;\n")
        f.write(f"static constexpr int SYSEX_PATCH_SIZE = 264;\n\n")
        
        # Write Names
        f.write("static const char* FACTORY_PRESET_NAMES[FACTORY_PRESET_COUNT] = {\n")
        for i, name in enumerate(names[:64]):
            safe_name = name.replace('"', '\\"')
            f.write(f'    "{safe_name}",\n')
        f.write("};\n\n")
        
        # Write Data
        f.write("static const uint8_t FACTORY_PRESET_DATA[] = {\n")
        for i, byte in enumerate(all_data):
            f.write(f"0x{byte:02X},")
            if (i+1) % 16 == 0: f.write("\n")
        f.write("};\n\n")
        
        f.write("} // namespace State\n")
        f.write("} // namespace CZ101\n")

if __name__ == "__main__":
    generate_header()
    print("Header generated successfully.")
