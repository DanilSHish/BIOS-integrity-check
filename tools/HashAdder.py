#!/usr/bin/env python3
"""
Hash Database Generator for BIOS Integrity Checker.

Converts binary hash database (hash.bin) to C header file (HashBase.h)
for embedding known-good hashes into the EFI application.

Entry format (48 bytes):
- Bytes 0-31:   SHA-256 hash
- Bytes 32-39:  File position (UINT64, little-endian)
- Bytes 40-47:  Volume size (UINT64, little-endian)
"""

import os
import sys
import argparse
from pathlib import Path


HASH_SIZE_BYTES = 32
ENTRY_SIZE_BYTES = 48  # 32 (hash) + 8 (position) + 8 (size)


def parse_hash_database(input_path: str, template_path: str, output_path: str) -> int:
    """
    Parse binary hash database and generate C header file.

    Args:
        input_path: Path to binary hash file (hash.bin)
        template_path: Path to template header file
        output_path: Path to output header file

    Returns:
        0 on success, 1 on error
    """
    input_file = Path(input_path)

    if not input_file.exists():
        print(f"Error: Input file not found: {input_path}", file=sys.stderr)
        return 1

    file_size = input_file.stat().st_size

    if file_size == 0:
        print("Warning: Empty hash database, creating empty array")
        num_entries = 0
    elif file_size % ENTRY_SIZE_BYTES != 0:
        print(f"Error: Invalid file size {file_size}. Expected multiple of {ENTRY_SIZE_BYTES}",
              file=sys.stderr)
        return 1
    else:
        num_entries = file_size // ENTRY_SIZE_BYTES

    try:
        with open(template_path, 'r', encoding='utf-8') as template, \
             open(input_path, 'rb') as data, \
             open(output_path, 'w', encoding='utf-8') as output:

            # Write template header
            for line in template:
                output.write(line)

            # Write array declaration
            output.write(f'[{num_entries}]')
            output.write(' = {\n')

            # Write each hash entry
            for entry_idx in range(num_entries):
                output.write('    {\n')

                # Write hash bytes
                output.write('        {')
                hash_bytes = data.read(HASH_SIZE_BYTES)
                for i, byte in enumerate(hash_bytes):
                    if isinstance(byte, int):
                        output.write(f'0x{byte:02x}')
                    else:
                        output.write(f'0x{byte:02x}')
                    if i < HASH_SIZE_BYTES - 1:
                        output.write(', ')
                output.write('},\n')

                # Write file position
                position_bytes = data.read(8)
                position = int.from_bytes(position_bytes, byteorder='little', signed=False)
                output.write(f'        (VOID*){hex(position)},\n')

                # Write volume size
                size_bytes = data.read(8)
                size = int.from_bytes(size_bytes, byteorder='little', signed=False)
                output.write(f'        {hex(size)}')

                if entry_idx < num_entries - 1:
                    output.write(',')
                output.write('\n    },\n')

            output.write('};\n')

        print(f"Successfully generated {output_path} with {num_entries} entries")
        return 0

    except IOError as e:
        print(f"Error: File I/O failed: {e}", file=sys.stderr)
        return 1


def main():
    parser = argparse.ArgumentParser(
        description='Convert binary hash database to C header file'
    )
    parser.add_argument('-i', '--input', default='hash.bin',
                       help='Input binary hash file (default: hash.bin)')
    parser.add_argument('-t', '--template', default='Temp_hash.txt',
                       help='Template header file (default: Temp_hash.txt)')
    parser.add_argument('-o', '--output', default='HashBase.h',
                       help='Output header file (default: HashBase.h)')

    args = parser.parse_args()

    return parse_hash_database(args.input, args.template, args.output)


if __name__ == '__main__':
    sys.exit(main())
