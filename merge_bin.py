# originally from here: https://github.com/platformio/platform-espressif32/issues/1078#issuecomment-1636793463
Import("env")
import os

APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"
LITTLEFS_BIN = "$BUILD_DIR/littlefs.bin"
MERGED_BIN = "$BUILD_DIR/${PROGNAME}_merged.bin"
BOARD_CONFIG = env.BoardConfig()


def get_littlefs_partition_address(env):
    """Get the LittleFS partition address from the partition table"""
    try:
        partitions_csv = None
        
        if hasattr(env, 'GetProjectOption'):
            partitions_csv = env.GetProjectOption("board_build.partitions", None)
        
        if not partitions_csv:
            # Get the board's partition table from the framework
            board_name = env.subst("$BOARD")
            framework_dir = env.subst("$PROJECT_PACKAGES_DIR/framework-arduinoespressif32")
            
            # Try to find the default partition table for the board
            variants_dir = os.path.join(framework_dir, "variants", board_name)
            if os.path.exists(variants_dir):
                for file in os.listdir(variants_dir):
                    if file.endswith(".csv"):
                        partitions_csv = os.path.join(variants_dir, file)
                        break
            
            # Fallback to default partition table
            if not partitions_csv:
                partitions_csv = os.path.join(framework_dir, "tools", "partitions", "default.csv")
        
        # If we still don't have a partition table file, try to get it from the build process
        if not partitions_csv or not os.path.exists(partitions_csv):
            # Try to find the generated partition table
            build_dir = env.subst("$BUILD_DIR")
            generated_partitions = os.path.join(build_dir, "partitions.csv")
            if os.path.exists(generated_partitions):
                partitions_csv = generated_partitions
            else:
                # Fall back to the default known address
                print("Warning: Could not find partition table, using default LittleFS address 0x3d0000")
                return "0x3d0000"
        
        # Read and parse the partition table
        with open(partitions_csv, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    parts = [p.strip() for p in line.split(',')]
                    if len(parts) >= 5:
                        name, type_field, subtype, offset, size = parts[:5]
                        # Look for spiffs or littlefs partition
                        if ('spiffs' in name.lower() or 'littlefs' in name.lower() or 
                            type_field.lower() == 'data' and subtype.lower() in ['spiffs', 'littlefs']):
                            # Clean up the offset (remove quotes and whitespace)
                            offset = offset.strip('"\'')
                            if not offset.startswith('0x'):
                                offset = '0x' + offset
                            print(f"Found LittleFS partition '{name}' at address {offset}")
                            return offset
        
        print("Warning: LittleFS partition not found in partition table, using default address 0x3d0000")
        return "0x3d0000"
        
    except Exception as e:
        print(f"Error reading partition table: {e}")
        print("Using default LittleFS address 0x3d0000")
        return "0x3d0000"


def merge_bin(source, target, env):
    # Get the dynamic LittleFS partition address
    littlefs_address = get_littlefs_partition_address(env)
    
    # The list contains all extra images (bootloader, partitions, eboot) and
    # the final application binary
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["$ESP32_APP_OFFSET", APP_BIN, littlefs_address, LITTLEFS_BIN]

    # Run esptool to merge images into a single binary
    env.Execute(
        " ".join(
            [
                "$PYTHONEXE",
                "$OBJCOPY",
                "--chip",
                BOARD_CONFIG.get("build.mcu", "esp32"),
                "merge_bin",
                "--fill-flash-size",
                BOARD_CONFIG.get("upload.flash_size", "4MB"),
                "-o",
                MERGED_BIN,
            ]
            + flash_images
        )
    )

# Add a post action that runs esptoolpy to merge available flash images
env.AddPostAction(APP_BIN , merge_bin)

# Patch the upload command to flash the merged binary at address 0x0
env.Replace(
    UPLOADERFLAGS=[
        ]
        + ["0x0", MERGED_BIN],
    UPLOADCMD='"$PYTHONEXE" "$UPLOADER" $UPLOADERFLAGS',
)