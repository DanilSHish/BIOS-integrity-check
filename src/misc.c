#include "mainHeader.h"

VOID PrintF(CONST CHAR16* fmt, ...) {
    VA_LIST marker;
    CHAR16 buffer[DEBUG_BUFFER_SIZE];

    VA_START(marker, fmt);
    UnicodeVSPrint(buffer, sizeof(buffer), fmt, marker);
    VA_END(marker);

    gST->ConOut->OutputString(gST->ConOut, buffer);
}

EFI_STATUS WriteFile(EFI_FILE_PROTOCOL* file, CONST VOID* data, UINTN bytesToWrite) {
    EFI_STATUS status;
    UINTN bytesWritten = bytesToWrite;

    if (file == NULL || data == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    status = file->Write(file, &bytesWritten, (VOID*)data);
    if (EFI_ERROR(status)) {
        return status;
    }

    if (bytesWritten != bytesToWrite) {
        return EFI_VOLUME_FULL;
    }

    return EFI_SUCCESS;
}

UINTN GetBiosSize(void) {
    EFI_GUID smbiosGuid = SMBIOS3_TABLE_GUID;
    SMBIOS_ENTRY_POINT_STRUCTURE* entryPoint;
    EFI_CONFIGURATION_TABLE* configTable = gST->ConfigurationTable;
    UINTN entryCount = gST->NumberOfTableEntries;

    for (UINTN i = 0; i < entryCount; i++) {
        if (CompareGuid(&configTable[i].VendorGuid, &smbiosGuid)) {
            entryPoint = (SMBIOS_ENTRY_POINT_STRUCTURE*)configTable[i].VendorTable;
            if (entryPoint == NULL || entryPoint->StructureTableAddress == NULL) {
                return 0;
            }

            SMBIOS_TABLE_TYPE0* type0 = (SMBIOS_TABLE_TYPE0*)entryPoint->StructureTableAddress;
            UINT8 biosSizeRaw = type0->BiosSize;
            UINTN size = ((UINTN)biosSizeRaw + 1) * 64 * 1024;

            return size;
        }
    }

    return 0;
}

EFI_STATUS HexStringToBytes(CONST CHAR8* hexString, UINT8* bytes) {
    UINTN len;
    UINTN byteLen;

    if (hexString == NULL || bytes == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    len = AsciiStrLen(hexString);
    if (len == 0 || (len % 2) != 0) {
        return EFI_INVALID_PARAMETER;
    }

    byteLen = len / 2;

    for (UINTN i = 0; i < byteLen; i++) {
        CHAR8 byteString[3];
        UINT64 value;

        byteString[0] = hexString[2 * i];
        byteString[1] = hexString[2 * i + 1];
        byteString[2] = '\0';

        value = AsciiStrHexToUint64(byteString);
        if (value > 0xFF) {
            return EFI_INVALID_PARAMETER;
        }

        bytes[i] = (UINT8)value;
    }

    return EFI_SUCCESS;
}
