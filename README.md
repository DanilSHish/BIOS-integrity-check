# BiosGuard 

> **UEFI Firmware Integrity Verification Tool** — A pre-boot security solution for detecting unauthorized BIOS modifications using SHA-256 cryptographic hashing.

[Русская версия](README.ru.md)

---

## Overview

**BiosGuard** is an EFI application that executes at the firmware level (before OS boot) to verify the integrity of BIOS firmware components. It provides cryptographic assurance that critical firmware volumes haven't been tampered with.

### Key Features

- **Cryptographic Verification** — SHA-256 hashing of firmware volumes
- **Pre-Boot Execution** — Runs at UEFI level, independent of OS
- **Firmware Volume Parsing** — Locates and analyzes UEFI PI volumes by GUID
- **Hash Database** — Embedded whitelist of known-good firmware hashes
- **Audit Logging** — Generates integrity reports and unknown hash records
- **Lightweight** — Optimized for minimal memory footprint

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        UEFI Environment                          │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │              BiosGuard EFI Application                   │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │    │
│  │  │ BIOS Dumper │→│ FW Volume   │→│ SHA-256 Verify  │  │    │
│  │  │  (SMBIOS)   │  │   Parser    │  │   & Reporting   │  │    │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘  │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
                              ↓
                    ┌─────────────────┐
                    │   Filesystem    │
                    │  ┌───────────┐  │
                    │  │ uefi.bin  │  │  ← Full BIOS dump
                    │  │ hash.bin  │  │  ← Integrity database
                    │  └───────────┘  │
                    └─────────────────┘
```

See [ARCHITECTURE.md](ARCHITECTURE.md) for detailed technical documentation.

---

## Requirements

### Build Requirements

- [Visual Studio 2022](https://visualstudio.microsoft.com/) (v17.0+)
- [EDK2](https://github.com/tianocore/edk2) (UEFI Development Kit II)
- Windows SDK 10.0 or later
- 64-bit UEFI firmware (target)

### Runtime Requirements

- x86_64 UEFI system (no Legacy BIOS support)
- SMBIOS 3.0+ for BIOS size detection
- UEFI Shell access
- Secure Boot disabled (for unsigned builds)

---

## Quick Start

### 1. Clone Repository

```bash
git clone https://github.com/username/BiosGuard.git
cd BiosGuard
```

### 2. Configure Build Environment

Set the `EDK2_PATH` environment variable:

```powershell
# PowerShell (Administrator)
[Environment]::SetEnvironmentVariable("EDK2_PATH", "C:\Workspace\edk2", "Machine")

# Or Command Prompt
setx EDK2_PATH "C:\Workspace\edk2" /M
```

### 3. Build

Open `BiosGuard.sln` in Visual Studio:

```powershell
BiosGuard.sln
```

Select configuration `Release|x64` and build (`Ctrl+Shift+B`).

Output: `bin/BiosGuard.efi`

### 4. Run

Copy `BiosGuard.efi` to a FAT32 USB drive and boot into UEFI Shell:

```shell
FS0:
BiosGuard.efi
```

---

## Example Output

```
BIOS Size: 16777216 bytes
BIOS Start address: 0xFF000000

[Volume at offset 0xa70000]
Volume Size: 0x90000 bytes
SHA-256: e50258d216fde19c23127a5c20273007d950efa11281fef034f053efd18a0176
[OK] Hash verified - integrity check passed

[Volume at offset 0xbb0000]
Volume Size: 0xb0000 bytes
SHA-256: 64950a850277d4cda1eae7c78c698681594607b454e3b3ad746b9b58b7541144
[FAIL] Hash mismatch - potential tampering detected!
```

---

## 🛠️ Project Structure

```
BiosGuard/
├── src/                          # Source code
│   ├── Main.c                   # Entry point & main logic
│   ├── misc.c                   # Utility functions
│   ├── SHA256.c                 # SHA-256 implementation
│   ├── mainHeader.h             # Main header file
│   ├── SHA256header.h           # SHA-256 header
│   └── BiosGuard.vcxproj        # VS project file
├── tools/                        # Utility tools
│   ├── HashAdder.py             # Hash database generator
│   ├── HashBase.h               # Embedded hash database
│   └── Temp_hash.txt            # Template for generator
├── BiosGuard.sln                # Visual Studio solution
├── README.md                    # This file (English)
├── README.ru.md                 # Russian documentation
├── ARCHITECTURE.md              # Technical architecture (EN)
├── ARCHITECTURE.ru.md           # Technical architecture (RU)
├── CONTRIBUTING.md              # Contribution guidelines
├── LICENSE                      # MIT License
├── .gitignore                   # Git exclusions
└── .editorconfig                # Code style configuration
```

---

## Updating Hash Database

After first run, new firmware hashes are appended to `hash.bin`. To embed them:

```bash
cd tools
python3 HashAdder.py -i hash.bin -o HashBase.h
```

Rebuild the project to include updated hashes.

---

## Security Considerations

> **Warning**: This tool operates at Ring -1 privilege level.

- **Not a silver bullet** — Protects against firmware modifications but not against hardware attacks
- **Secure Boot** — Requires unsigned EFI execution; sign with your own keys for production
- **Hash coverage** — Only validates volumes with PEI/DXE modules; NVRAM excluded by design
- **Physical access** — Malicious actor with physical access can bypass software protections

### Threat Model

| Threat | Protection | Status |
|--------|-----------|--------|
| BIOS malware (rootkit) | SHA-256 verification | ✅ Protected |
| Firmware downgrade | Version-aware hashes | ⚠️ Requires update |
| Evil Maid attack | Pre-boot verification | ⚠️ Hardware dependent |
| Supply chain attack | Manufacturer signatures | ❌ Not covered |

---

## Roadmap

- [ ] TPM 2.0 integration for measured boot
- [ ] Network attestation protocol
- [ ] UEFI HII graphical interface
- [ ] Secure Boot signature support
- [ ] Automated hash update via HTTPS
- [ ] Multi-platform support (ARM64)

---

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Code Style

- 4 spaces for indentation
- K&R brace style
- `snake_case` for functions/variables
- `PascalCase` for UEFI types
- 100 character line limit

---