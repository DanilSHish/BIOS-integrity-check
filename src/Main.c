#include "mainHeader.h"
#include "../tools/HashBase.h"

EFI_SYSTEM_TABLE* gST = NULL;
EFI_BOOT_SERVICES* gBS = NULL;

/* Output file names */
#define DUMP_FILENAME     L"uefi.bin"
#define HASH_DB_FILENAME  L"hash.bin"

/* Forward declarations */
static EFI_STATUS parseVolume(EFI_FILE_PROTOCOL* romFile);
static BOOLEAN parseFile(UINT64 position, UINT64 basePosition,
                         EFI_FILE_PROTOCOL* romFile, UINT64 volumeSize);
static EFI_STATUS processHash(const HASH* hash);
static EFI_STATUS alignPosition(UINT64* position, EFI_FILE_PROTOCOL* file);

/**
 * Entry point for UEFI application.
 * Dumps BIOS firmware and verifies integrity of firmware volumes.
 */
EFI_STATUS EFIAPI UefiMain(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* systemTable) {
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* romFile = NULL;
    EFI_FILE_PROTOCOL* root = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystem = NULL;
    EFI_PHYSICAL_ADDRESS memoryAddress;

    gST = systemTable;
    gBS = systemTable->BootServices;

    /* Get BIOS size from SMBIOS */
    UINTN biosSize = GetBiosSize();
    if (biosSize == 0) {
        PrintF(L"Error: Failed to determine BIOS size\n");
        return EFI_NOT_FOUND;
    }
    PrintF(L"BIOS Size: %u bytes\n", biosSize);

    /* Locate file system protocol */
    status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&fileSystem);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to locate file system protocol. Status: %d\n", CLEAR_EFI_STATUS(status));
        return status;
    }

    /* Open root volume */
    status = fileSystem->OpenVolume(fileSystem, &root);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to open file system volume. Status: %d\n", CLEAR_EFI_STATUS(status));
        return status;
    }

    /* Create dump file for writing */
    status = root->Open(root, &romFile, DUMP_FILENAME,
                        EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to create dump file. Status: %d\n", CLEAR_EFI_STATUS(status));
        root->Close(root);
        return status;
    }

    /* Allocate memory for BIOS copy */
    status = gBS->AllocatePages(AllocateAnyPages, EfiLoaderData,
                                EFI_SIZE_TO_PAGES(biosSize), &memoryAddress);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to allocate memory. Status: %d\n", CLEAR_EFI_STATUS(status));
        goto cleanup;
    }

    gBS->SetMem((VOID*)memoryAddress, biosSize, 0);

    /* Calculate BIOS start address (top of 4GB address space) */
    VOID* startAddress = (VOID*)(MAX_UINT32 - biosSize + 1);
    PrintF(L"BIOS Start address: 0x%p\n", startAddress);

    /* Copy BIOS to allocated memory */
    gBS->CopyMem((VOID*)memoryAddress, startAddress, biosSize);

    /* Write BIOS dump to file */
    status = WriteFile(romFile, (VOID*)memoryAddress, biosSize);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to write dump file. Status: %d\n", CLEAR_EFI_STATUS(status));
        gBS->FreePages(memoryAddress, EFI_SIZE_TO_PAGES(biosSize));
        goto cleanup;
    }

    romFile->Close(romFile);
    romFile = NULL;

    /* Re-open dump file for reading */
    status = root->Open(root, &romFile, DUMP_FILENAME, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to open dump file for reading. Status: %d\n", CLEAR_EFI_STATUS(status));
        gBS->FreePages(memoryAddress, EFI_SIZE_TO_PAGES(biosSize));
        root->Close(root);
        return status;
    }

    /* Parse firmware volumes and verify hashes */
    status = parseVolume(romFile);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to parse firmware volume. Status: %d\n", CLEAR_EFI_STATUS(status));
    }

cleanup:
    if (romFile != NULL) {
        romFile->Close(romFile);
    }
    gBS->FreePages(memoryAddress, EFI_SIZE_TO_PAGES(biosSize));
    root->Close(root);

    return status;
}

/**
 * Align file position to 8-byte boundary.
 */
static EFI_STATUS alignPosition(UINT64* position, EFI_FILE_PROTOCOL* file) {
    EFI_STATUS status;
    UINTN remainder = *position % FILE_ALIGNMENT;

    if (remainder != 0) {
        *position += (FILE_ALIGNMENT - remainder);
        status = file->SetPosition(file, *position);
        if (EFI_ERROR(status)) {
            return status;
        }
    }

    return EFI_SUCCESS;
}

/**
 * Parse firmware volumes and calculate their hashes.
 */
static EFI_STATUS parseVolume(EFI_FILE_PROTOCOL* romFile) {
    HASH hash;
    EFI_STATUS status;
    UINT8 targetGuid[GUID_SIZE];
    UINT8 buffer[GUID_SIZE];
    UINTN bufferLength = GUID_SIZE;
    UINTN matches = 0;

    status = HexStringToBytes(VOLUME_GUID, targetGuid);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Invalid volume GUID format\n");
        return status;
    }

    while (!EFI_ERROR(romFile->Read(romFile, &bufferLength, buffer)) && bufferLength > 0) {
        if (memcmp(buffer, targetGuid, GUID_SIZE) != 0) {
            continue;
        }

        matches++;

        /* Get current position and calculate volume base */
        UINT64 position = 0;
        status = romFile->GetPosition(romFile, &position);
        if (EFI_ERROR(status)) {
            PrintF(L"Error: Failed to get file position. Status: %d\n", CLEAR_EFI_STATUS(status));
            return status;
        }

        position -= GUID_SIZE;
        UINT64 basePosition = position - GUID_SIZE;
        hash.filePosition = (VOID*)basePosition;

        PrintF(L"\n[Volume at offset 0x%lx]\n", basePosition);

        /* Read volume size */
        UINT64 volumeSize;
        bufferLength = sizeof(UINT64);
        status = romFile->Read(romFile, &bufferLength, &volumeSize);
        if (EFI_ERROR(status) || bufferLength != sizeof(UINT64)) {
            PrintF(L"Warning: Failed to read volume size\n");
            continue;
        }

        PrintF(L"Volume Size: 0x%lx bytes\n", volumeSize);

        /* Reset to volume base and skip header */
        status = romFile->SetPosition(romFile, basePosition);
        if (EFI_ERROR(status)) {
            PrintF(L"Error: Failed to set file position. Status: %d\n", CLEAR_EFI_STATUS(status));
            return status;
        }

        bufferLength = sizeof(EFI_FIRMWARE_VOLUME_HEADER);
        position = basePosition + bufferLength + 8;

        status = romFile->SetPosition(romFile, position);
        if (EFI_ERROR(status)) {
            PrintF(L"Error: Failed to set file position. Status: %d\n", CLEAR_EFI_STATUS(status));
            return status;
        }

        /* Parse files within volume */
        BOOLEAN shouldHash = parseFile(position, basePosition, romFile, volumeSize);

        if (shouldHash) {
            hash.size = volumeSize;

            status = gBS->AllocatePool(EfiLoaderData, hash.size, (VOID**)&hash.baseAddress);
            if (EFI_ERROR(status)) {
                PrintF(L"Error: Failed to allocate memory. Status: %d\n", CLEAR_EFI_STATUS(status));
                return status;
            }

            bufferLength = hash.size;
            status = romFile->Read(romFile, &bufferLength, hash.baseAddress);
            if (EFI_ERROR(status) || bufferLength != hash.size) {
                PrintF(L"Error: Failed to read volume data. Status: %d\n", CLEAR_EFI_STATUS(status));
                gBS->FreePool(hash.baseAddress);
                return EFI_ERROR(status) ? status : EFI_DEVICE_ERROR;
            }

            processHash(&hash);
            gBS->FreePool(hash.baseAddress);
        }

        /* Move to next volume */
        status = romFile->SetPosition(romFile, basePosition + volumeSize);
        if (EFI_ERROR(status)) {
            PrintF(L"Error: Failed to set file position. Status: %d\n", CLEAR_EFI_STATUS(status));
            return status;
        }

        bufferLength = GUID_SIZE;
    }

    if (matches == 0) {
        PrintF(L"\nNo firmware volumes found.\n");
    } else {
        PrintF(L"\nTotal volumes found: %d\n", matches);
    }

    return EFI_SUCCESS;
}

/**
 * Parse files within a firmware volume.
 * Returns TRUE if volume should be hashed.
 */
static BOOLEAN parseFile(UINT64 position, UINT64 basePosition,
                         EFI_FILE_PROTOCOL* romFile, UINT64 volumeSize) {
    BOOLEAN shouldHash = FALSE;
    EFI_STATUS status;
    EFI_FFS_FILE_HEADER fileHeader;
    UINT8 nvramGuid[GUID_SIZE];
    UINTN headerSize = sizeof(EFI_FFS_FILE_HEADER);

    status = HexStringToBytes(NVRAM_GUID, nvramGuid);
    if (EFI_ERROR(status)) {
        return FALSE;
    }

    while (!EFI_ERROR(romFile->Read(romFile, &headerSize, &fileHeader)) && headerSize > 0) {
        /* Extract file size (24 bits) */
        UINTN fileSize = (*(UINT32*)fileHeader.Size) & 0x00FFFFFF;

        if (position >= volumeSize + basePosition) {
            break;
        }

        /* Skip NVRAM volumes */
        if (memcmp(&fileHeader.Name, nvramGuid, GUID_SIZE) == 0 && fileSize != 0) {
            shouldHash = FALSE;
            break;
        }

        /* Check for hashable file types */
        if ((fileHeader.Type == PEI_MODULE || fileHeader.Type == PEI_CORE) && fileSize != 0) {
            shouldHash = TRUE;
            position = position + fileSize - headerSize;

            status = alignPosition(&position, romFile);
            if (EFI_ERROR(status)) {
                PrintF(L"Warning: Failed to align position\n");
                return FALSE;
            }
            break;
        }

        if ((fileHeader.Type == VOLUME_IMAGE || fileHeader.Type == DXE_DRIVER) && fileSize != 0) {
            EFI_COMMON_SECTION_HEADER section;
            UINTN sectionSize = sizeof(EFI_COMMON_SECTION_HEADER);
            shouldHash = TRUE;

            status = romFile->GetPosition(romFile, &position);
            if (EFI_ERROR(status)) {
                return FALSE;
            }

            status = alignPosition(&position, romFile);
            if (EFI_ERROR(status)) {
                return FALSE;
            }

            /* Read section header (ignore result for now) */
            romFile->Read(romFile, &sectionSize, &section);
            break;
        }

        /* Move to next file */
        status = romFile->GetPosition(romFile, &position);
        if (EFI_ERROR(status)) {
            return FALSE;
        }

        position = position + fileSize - headerSize;
        status = alignPosition(&position, romFile);
        if (EFI_ERROR(status)) {
            return FALSE;
        }

        shouldHash = FALSE;
    }

    return shouldHash;
}

/**
 * Compare calculated hash against known good hashes.
 * If not found, append to hash database file.
 */
static EFI_STATUS processHash(const HASH* hash) {
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file = NULL;
    EFI_FILE_PROTOCOL* root = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystem = NULL;
    EFI_FILE_INFO* fileInfo = NULL;
    UINTN fileInfoSize = 0;
    UINT64 currentFileSize = 0;

    status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&fileSystem);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to locate file system protocol. Status: %d\n", CLEAR_EFI_STATUS(status));
        return status;
    }

    status = fileSystem->OpenVolume(fileSystem, &root);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to open file system volume. Status: %d\n", CLEAR_EFI_STATUS(status));
        return status;
    }

    /* Open or create hash database file */
    status = root->Open(root, &file, HASH_DB_FILENAME,
                        EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to open hash database. Status: %d\n", CLEAR_EFI_STATUS(status));
        root->Close(root);
        return status;
    }

    /* Get current file size */
    status = file->GetInfo(file, &gEfiFileInfoGuid, &fileInfoSize, NULL);
    if (status == EFI_BUFFER_TOO_SMALL) {
        status = gBS->AllocatePool(EfiLoaderData, fileInfoSize, (VOID**)&fileInfo);
        if (!EFI_ERROR(status)) {
            status = file->GetInfo(file, &gEfiFileInfoGuid, &fileInfoSize, fileInfo);
            if (!EFI_ERROR(status)) {
                currentFileSize = fileInfo->FileSize;
            }
        }
    }

    /* Calculate hash */
    UINT8 calculatedHash[SHA256_SIZE_BYTES];
    sha256_hash(hash->baseAddress, hash->size, calculatedHash);

    PrintF(L"SHA-256: ");
    for (UINTN i = 0; i < SHA256_SIZE_BYTES; i++) {
        PrintF(L"%02x", calculatedHash[i]);
    }
    PrintF(L"\n");

    /* Search for matching hash in database */
    UINTN numEntries = sizeof(HashBase) / sizeof(HashBase[0]);
    UINT8* targetPosition = (UINT8*)hash->filePosition;

    for (UINTN i = 0; i < numEntries; i++) {
        if (HashBase[i].filePosition != targetPosition) {
            continue;
        }

        /* Compare hashes */
        BOOLEAN hashMatches = TRUE;
        for (UINTN j = 0; j < SHA256_SIZE_BYTES; j++) {
            if (HashBase[i].hash[j] != calculatedHash[j]) {
                hashMatches = FALSE;
                break;
            }
        }

        if (hashMatches) {
            PrintF(L"[OK] Hash verified - integrity check passed\n");
        } else {
            PrintF(L"[FAIL] Hash mismatch - potential tampering detected!\n");
            PrintF(L"Expected: ");
            for (UINTN j = 0; j < SHA256_SIZE_BYTES; j++) {
                PrintF(L"%02x", HashBase[i].hash[j]);
            }
            PrintF(L"\n");
        }

        if (fileInfo != NULL) {
            gBS->FreePool(fileInfo);
        }
        file->Close(file);
        root->Close(root);
        return EFI_SUCCESS;
    }

    /* Hash not found - append to database */
    PrintF(L"[NEW] Unknown volume - adding to hash database\n");

    status = file->SetPosition(file, currentFileSize);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to seek in hash database\n");
        goto cleanup;
    }

    /* Write hash entry: hash(32) + position(8) + size(8) = 48 bytes */
    status = WriteFile(file, calculatedHash, SHA256_SIZE_BYTES);
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to write hash\n");
        goto cleanup;
    }

    status = WriteFile(file, &hash->filePosition, sizeof(hash->filePosition));
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to write position\n");
        goto cleanup;
    }

    status = WriteFile(file, &hash->size, sizeof(hash->size));
    if (EFI_ERROR(status)) {
        PrintF(L"Error: Failed to write size\n");
        goto cleanup;
    }

    PrintF(L"[OK] Hash saved to database\n");

cleanup:
    if (fileInfo != NULL) {
        gBS->FreePool(fileInfo);
    }
    file->Close(file);
    root->Close(root);

    return status;
}
