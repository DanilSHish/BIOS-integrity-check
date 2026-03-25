#pragma once

#include <Uefi.h>
#include <Library/PrintLib.h>
#include <Guid/SmBios.h>
#include <IndustryStandard/SmBios.h>
#include <VmmSimpleFileSystem.h>
#include <Pi/PiFirmwareVolume.h>
#include <Pi/PiFirmwareFile.h>
#include <Guid/FileInfo.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

extern EFI_SYSTEM_TABLE* gST;
extern EFI_BOOT_SERVICES* gBS;

/* Buffer size for debug output */
#define DEBUG_BUFFER_SIZE     0x200

/* Extract lower 8 bits of EFI_STATUS for printing */
#define CLEAR_EFI_STATUS(st)  ((st) & 0xFF)

/* SHA-256 produces 32 bytes (256 bits) hash */
#define SHA256_SIZE_BYTES     32

/* GUID is 16 bytes */
#define GUID_SIZE             16

/* Target firmware volume GUID to search for */
#define VOLUME_GUID           "78E58C8C3D8A1C4F9935896185C32DD3"

/* NVRAM volume GUID to exclude from hashing */
#define NVRAM_GUID            "A3B9F5CE6D477F499FDCE98143E0422C"

/* Firmware file types */
#define PEI_MODULE            EFI_FV_FILETYPE_PEIM
#define DXE_DRIVER            EFI_FV_FILETYPE_DRIVER
#define PEI_CORE              EFI_FV_FILETYPE_PEI_CORE
#define VOLUME_IMAGE          EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE

/* Alignment boundary for firmware files */
#define FILE_ALIGNMENT        8

/**
 * Structure representing a firmware volume hash entry.
 * Contains the SHA-256 hash and metadata for verification.
 */
typedef struct {
    UINT8 hash[SHA256_SIZE_BYTES];
    VOID* filePosition;
    UINTN size;
    VOID* baseAddress;
} HASH;

/**
 * SMBIOS 3.0 entry point structure for locating SMBIOS table.
 */
typedef struct {
    UINT8 AnchorString[5];
    UINT8 EntryPointStructureChecksum;
    UINT8 EntryPointLength;
    UINT8 SmbiosMajorVersion;
    UINT8 SmbiosMinorVersion;
    UINT8 SmbiosDocrev;
    UINT8 EntryPointRevision;
    UINT8 Reserved;
    UINT32 StructureTableMaxSize;
    VOID* StructureTableAddress;
} SMBIOS_ENTRY_POINT_STRUCTURE;

/**
 * Calculate SHA-256 hash of data.
 *
 * @param data  Input data to hash
 * @param len   Length of input data in bytes
 * @param hash  Output buffer (32 bytes) for hash result
 */
void sha256_hash(
    CONST UINT8* data,
    UINTN len,
    UINT8* hash
);

/**
 * Print formatted string to console.
 *
 * @param fmt  Format string (Unicode)
 * @param ...  Variable arguments
 */
VOID PrintF(CONST CHAR16* fmt, ...);

/**
 * Write data to file with error checking.
 *
 * @param file         File protocol handle
 * @param data         Data to write
 * @param bytesToWrite Number of bytes to write
 * @return EFI_STATUS  Operation status
 */
EFI_STATUS WriteFile(EFI_FILE_PROTOCOL* file, CONST VOID* data, UINTN bytesToWrite);

/**
 * Get BIOS size from SMBIOS information.
 *
 * @return UINTN  BIOS size in bytes, 0 on error
 */
UINTN GetBiosSize(void);

/**
 * Convert hexadecimal string to byte array.
 *
 * @param hexString  Input hex string (32 chars for GUID)
 * @param bytes      Output byte array (16 bytes for GUID)
 * @return EFI_STATUS  Operation status
 */
EFI_STATUS HexStringToBytes(CONST CHAR8* hexString, UINT8* bytes);
