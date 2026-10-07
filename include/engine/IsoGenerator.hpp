#pragma once

#include "engine/StepContext.hpp"

namespace vaxp {

class IsoGenerator {
public:
    static bool generate(StepContext& ctx);

private:
    static bool prepare_image_layout(StepContext& ctx);
    static bool copy_kernel_and_initrd(StepContext& ctx);
    static bool generate_grub_config(StepContext& ctx);
    static bool generate_manifests(StepContext& ctx);
    static bool generate_disk_metadata(StepContext& ctx);
    static bool build_efi_boot_image(StepContext& ctx);
    static bool build_bios_boot_image(StepContext& ctx);
    static bool generate_md5sums(StepContext& ctx);
    static bool master_iso(StepContext& ctx, fs::path& output_iso_path);
};

} // namespace vaxp
