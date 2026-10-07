#include "core/Config.hpp"
#include "core/Logger.hpp"
#include "core/SignalHandler.hpp"
#include "fs/MountManager.hpp"
#include "fs/FileOps.hpp"
#include "engine/StepPipeline.hpp"
#include "engine/BootstrapEngine.hpp"
#include "engine/IsoGenerator.hpp"
#include "steps/BuildSteps.hpp"

#include <iostream>
#include <chrono>

using namespace vaxp;

void print_banner() {
    std::cout << "\n"
              << "\033[1;36m===============================================================\033[0m\n"
              << "\033[1;32m      __      __     __   __ _____          ____   _____   \033[0m\n"
              << "\033[1;32m      \\ \\    / /\\   \\ \\ / /|  __ \\        / __ \\ / ____|  \033[0m\n"
              << "\033[1;32m       \\ \\  / /  \\   \\ V / | |__) |______| |  | | (___    \033[0m\n"
              << "\033[1;32m        \\ \\/ / /\\ \\   > <  |  ___/|______| |  | |\\___ \\   \033[0m\n"
              << "\033[1;32m         \\  / ____ \\ / . \\ | |           | |__| |____) |  \033[0m\n"
              << "\033[1;32m          \\/_/    \\_/_/ \\_\\|_|            \\____/|_____/   \033[0m\n"
              << "\033[1;33m       Commercial OS Distribution Builder Engine (C++20)       \033[0m\n"
              << "\033[1;36m===============================================================\033[0m\n\n";
}

int main(int argc, char* argv[]) {
    // 1. Initialize Signal Handler & Logger
    SignalHandler::instance().initialize();
    Logger::instance().init("vaxp-builder.log", false);

    print_banner();

    // 2. Load Configuration
    BuildConfig config;
    // Set workspace root to parent directory if running from VAXP-OS-Builder/
    fs::path current_dir = fs::current_path();
    if (current_dir.filename() == "VAXP-OS-Builder") {
        config.workspace_root = current_dir.parent_path();
    } else {
        config.workspace_root = current_dir;
    }

    if (!config.parse_cli(argc, argv)) {
        return 0;
    }

    if (config.verbose) {
        Logger::instance().set_verbose(true);
    }

    LOG_INFO("Target Distribution: " + config.business_name + " " + config.version);
    LOG_INFO("Base Ubuntu Codename: " + config.ubuntu_codename + " (" + config.architecture + ")");
    LOG_INFO("Workspace Root: " + config.workspace_root.string());

    // 3. Handle Clean Flag
    if (config.force_clean) {
        LOG_INFO("Performing full cleanup of build artifacts...");
        MountManager::instance().unmount_all(config.get_absolute_rootfs());
        FileOps::remove_all_safe(config.get_absolute_rootfs());
        FileOps::remove_all_safe(config.get_absolute_image());
        FileOps::remove_all_safe(config.get_absolute_dist());
        FileOps::remove_all_safe(config.workspace_root / ".build_state.txt");
        LOG_OK("Cleanup finished.");
        return 0;
    }

    // 4. Initialize Pipeline with all 47 modular steps
    fs::path state_file = config.workspace_root / ".build_state.txt";
    StepPipeline pipeline(state_file);

    // 00 - 08
    pipeline.register_step(std::make_unique<CheckHostStep>());
    pipeline.register_step(std::make_unique<AptSourceStep>());
    pipeline.register_step(std::make_unique<SetHostnameStep>());
    pipeline.register_step(std::make_unique<SystemdStep>());
    pipeline.register_step(std::make_unique<MachineIdStep>());
    pipeline.register_step(std::make_unique<InitctlStep>());
    pipeline.register_step(std::make_unique<AptUpgradeStep>());
    pipeline.register_step(std::make_unique<SystemToolsInstallStep>());
    pipeline.register_step(std::make_unique<CasperKernelInstallStep>());

    // 10 - 20
    pipeline.register_step(std::make_unique<NoSnapStep>());
    pipeline.register_step(std::make_unique<NoMotdStep>());
    pipeline.register_step(std::make_unique<VaxpAppsStep>());
    pipeline.register_step(std::make_unique<FontsStep>());
    pipeline.register_step(std::make_unique<AppStoreStep>());
    pipeline.register_step(std::make_unique<AvaxpInstallAppsStep>());
    pipeline.register_step(std::make_unique<PlymouthPatchStep>());
    pipeline.register_step(std::make_unique<DeskmonStep>());
    pipeline.register_step(std::make_unique<NoSudoHintStep>());

    // 21 - 39
    pipeline.register_step(std::make_unique<UbiquityStep>());
    pipeline.register_step(std::make_unique<UbiquityPatchStep>());
    pipeline.register_step(std::make_unique<SoftwarePropertiesGtkStep>());
    pipeline.register_step(std::make_unique<WallpaperStep>());
    pipeline.register_step(std::make_unique<FluentIconThemeStep>());
    pipeline.register_step(std::make_unique<FluentGtkThemeStep>());
    pipeline.register_step(std::make_unique<InputMethodStep>());
    pipeline.register_step(std::make_unique<UbuntuLogoTextStep>());
    pipeline.register_step(std::make_unique<XdgMimeStep>());
    pipeline.register_step(std::make_unique<TemplatesStep>());
    pipeline.register_step(std::make_unique<AetherConfigsStep>());

    // 40 - 88
    pipeline.register_step(std::make_unique<CopPkgeStep>());
    pipeline.register_step(std::make_unique<ManualControlStep>());
    pipeline.register_step(std::make_unique<TargetAptMirrorStep>());
    pipeline.register_step(std::make_unique<IntelTheSofStep>());
    pipeline.register_step(std::make_unique<SessionsPatchStep>());
    pipeline.register_step(std::make_unique<EtcBrandingStep>());
    pipeline.register_step(std::make_unique<CasperPatchStep>());
    pipeline.register_step(std::make_unique<EtcIssuePatchStep>());
    pipeline.register_step(std::make_unique<NoAdvertisementsStep>());
    pipeline.register_step(std::make_unique<UselessPackageRemoverStep>());
    pipeline.register_step(std::make_unique<InitramfsUpdateStep>());
    pipeline.register_step(std::make_unique<LocalesConfigStep>());
    pipeline.register_step(std::make_unique<NetworkManagerPatchStep>());
    pipeline.register_step(std::make_unique<AptCacheCleanerStep>());
    pipeline.register_step(std::make_unique<MachineIdWiperStep>());
    pipeline.register_step(std::make_unique<DiversionRemoverStep>());
    pipeline.register_step(std::make_unique<HistoryCleanerStep>());
    pipeline.register_step(std::make_unique<UselessFoldersCleanerStep>());

    // 5. Handle List Steps Flag
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-l" || a == "--list-steps") {
            pipeline.list_steps();
            return 0;
        }
    }

    StepContext ctx(config, MountManager::instance());

    // 6. Handle Single Step Execution
    if (!config.single_step_id.empty()) {
        bool res = pipeline.execute_single(config.single_step_id, ctx);
        return res ? 0 : 1;
    }

    // 7. Full Build Execution
    auto build_start = std::chrono::steady_clock::now();

    // Step A: Host validation & Bootstrap
    CheckHostStep check_host;
    if (!check_host.execute(ctx)) {
        LOG_ERROR("Host environment validation failed. Aborting.");
        return 1;
    }

    if (!BootstrapEngine::bootstrap(ctx)) {
        LOG_ERROR("Bootstrap phase failed. Aborting.");
        return 1;
    }

    // Step B: Execute Pipeline (Modifications, packages, customization)
    if (!pipeline.execute_all(ctx, config.resume)) {
        LOG_ERROR("Pipeline execution failed.");
        return 1;
    }

    // Step C: Generate Live ISO & Checksums
    if (!IsoGenerator::generate(ctx)) {
        LOG_ERROR("ISO Generation failed.");
        return 1;
    }

    auto build_end = std::chrono::steady_clock::now();
    auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(build_end - build_start).count();

    std::cout << "\n\033[1;32m===============================================================\033[0m\n"
              << "\033[1;32m   VAXP-OS BUILD SUCCEEDED! Total Time: " << total_seconds << "s\033[0m\n"
              << "\033[1;32m===============================================================\033[0m\n\n";

    return 0;
}
