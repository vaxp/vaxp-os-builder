# VAXP-OS Builder

A commercial-grade, high-performance operating system distribution engine written in **C++20** and built with **Meson & Ninja**.

Designed for orchestrating, building, and customizing **VAXP-OS** live and installable images with a fully native modular pipeline.

---

## Architecture & Features

- **Native C++20 Core**: Structured object-oriented pipeline replacing shell scripts with robust process management.
- **Full Modular Pipeline**: 47 self-contained build steps covering system bootstrap, desktop environment integration, and image finalization.
- **Hardware-Accelerated Cryptography**: Uses OpenSSL 3.0 EVP for SHA256 and MD5 integrity verification.
- **RAII Resource Management**: Guarantees clean, reverse unmounting of virtual filesystems (`/dev`, `/proc`, `/sys`, `/run`, `/dev/pts`) on normal exit, failures, and termination signals.
- **Embedded Assets**: Local storage for deb packages, desktop configurations, wallpapers, and installer themes under `assets/`.
- **State Persistence & Resume**: Tracks completed steps in `.build_state.txt` for fast incremental rebuilding.
- **Selective Step Execution**: Supports running any single step individually via `--step <id>` for debugging and development.

---

## Dependencies

### One-Command Quick Install (Ubuntu / Debian)

```bash
sudo apt update && sudo apt install -y build-essential meson ninja-build libssl-dev nlohmann-json3-dev debootstrap xorriso squashfs-tools grub-pc-bin grub-efi-amd64 mtools dosfstools
```

Or formatted for readability:

```bash
sudo apt update && sudo apt install -y \
  build-essential \
  meson \
  ninja-build \
  libssl-dev \
  nlohmann-json3-dev \
  debootstrap \
  xorriso \
  squashfs-tools \
  grub-pc-bin \
  grub-efi-amd64 \
  mtools \
  dosfstools
```

### Dependencies Verification Matrix

All dependencies are checked both at build configuration time (`meson setup`) and during runtime pre-flight verification (Step `00-check-host`):

| Package | Components / Utilities | Role in VAXP-OS Builder | Checked By |
| :--- | :--- | :--- | :--- |
| `build-essential` | `g++` (C++20 standard) | Compiles the native engine and build steps | Meson |
| `meson` | `meson` (>= 0.60) | Build definition and dependency discovery | Host |
| `ninja-build` | `ninja` | High-speed multi-threaded build backend | Host |
| `libssl-dev` | OpenSSL 3.0 EVP | Hardware-accelerated cryptographic SHA256 & MD5 | Meson (`openssl`) |
| `nlohmann-json3-dev` | `<nlohmann/json.hpp>` | High-performance JSON profile and configuration parsing | Meson (`nlohmann_json`) |
| `debootstrap` | `/usr/sbin/debootstrap` | Bootstraps minimal base Ubuntu rootfs | Meson & Step `00` |
| `xorriso` | `/usr/bin/xorriso` | Masters hybrid bootable ISO (UEFI + BIOS) | Meson & Step `00` |
| `squashfs-tools` | `mksquashfs`, `unsquashfs` | Compresses the OS rootfs with zstd | Meson & Step `00` |
| `grub-pc-bin` | `/usr/lib/grub/i386-pc/cdboot.img` | Legacy BIOS bootloader generation | Meson & Step `00` |
| `grub-efi-amd64` | `/usr/lib/grub/x86_64-efi` | UEFI x86_64 bootloader modules | Meson & Step `00` |
| `mtools` | `mcopy` | Injects EFI binaries into FAT boot image | Meson & Step `00` |
| `dosfstools` | `mkfs.vfat` | Creates FAT filesystem for `efiboot.img` | Meson & Step `00` |

---

## Building

```bash
cd VAXP-OS-Builder
meson setup build
ninja -C build
```

The compiled binary will be located at `./build/vaxp-builder`.

---

## Usage

```bash
# Display help and command-line options
./build/vaxp-builder --help

# List all registered build pipeline steps and their status
./build/vaxp-builder --list-steps

# Run the complete OS build pipeline (requires root privileges)
sudo ./build/vaxp-builder

# Run using a specific JSON configuration file
sudo ./build/vaxp-builder -c config/default.json

# Execute a single step by ID
sudo ./build/vaxp-builder --step 20-deskmon-mod

# Clean the build workspace
sudo ./build/vaxp-builder --clean
```
