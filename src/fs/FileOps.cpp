#include "fs/FileOps.hpp"
#include "core/Logger.hpp"

#include <openssl/evp.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <sstream>
#include <iomanip>

namespace vaxp {

namespace {

std::string compute_hash_evp(const fs::path& file_path, const EVP_MD* md) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for hashing: " + file_path.string());
        return "";
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";

    if (EVP_DigestInit_ex(ctx, md, nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    char buffer[65536];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        if (EVP_DigestUpdate(ctx, buffer, file.gcount()) != 1) {
            EVP_MD_CTX_free(ctx);
            return "";
        }
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < digest_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
}

} // anonymous namespace

std::string FileOps::calculate_sha256(const fs::path& file_path) {
    return compute_hash_evp(file_path, EVP_sha256());
}

std::string FileOps::calculate_md5(const fs::path& file_path) {
    return compute_hash_evp(file_path, EVP_md5());
}

bool FileOps::write_file_atomic(const fs::path& file_path, 
                                const std::string& content, 
                                mode_t mode) {
    std::error_code ec;
    if (file_path.has_parent_path()) {
        fs::create_directories(file_path.parent_path(), ec);
    }

    fs::path temp_path = file_path;
    temp_path += ".tmp." + std::to_string(getpid());

    std::ofstream out(temp_path, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        LOG_ERROR("Failed to open temporary file for atomic write: " + temp_path.string());
        return false;
    }

    out << content;
    out.flush();
    out.close();

    chmod(temp_path.c_str(), mode);

    fs::rename(temp_path, file_path, ec);
    if (ec) {
        LOG_ERROR("Atomic rename failed from " + temp_path.string() + " to " + file_path.string() + ": " + ec.message());
        fs::remove(temp_path, ec);
        return false;
    }
    return true;
}

std::string FileOps::read_file(const fs::path& file_path) {
    std::ifstream in(file_path, std::ios::in);
    if (!in.is_open()) return "";
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool FileOps::copy_directory_recursive(const fs::path& source, 
                                      const fs::path& destination) {
    std::error_code ec;
    if (!fs::exists(source, ec)) {
        LOG_WARN("Source directory does not exist: " + source.string());
        return false;
    }

    fs::create_directories(destination, ec);
    fs::copy(source, destination, 
             fs::copy_options::recursive | 
             fs::copy_options::overwrite_existing | 
             fs::copy_options::copy_symlinks, 
             ec);

    if (ec) {
        LOG_ERROR("Failed to copy directory: " + ec.message());
        return false;
    }
    return true;
}

bool FileOps::remove_all_safe(const fs::path& target_path) {
    std::error_code ec;
    if (!fs::exists(target_path, ec)) return true;
    
    // Safety check: Never delete root or home directly
    std::string canonical_str = target_path.string();
    if (canonical_str == "/" || canonical_str == "/root" || canonical_str == "/home") {
        LOG_ERROR("CRITICAL: Attempted to delete protected system path: " + canonical_str);
        return false;
    }

    fs::remove_all(target_path, ec);
    return !ec;
}

uint64_t FileOps::get_available_disk_space(const fs::path& path) {
    struct statvfs stat{};
    fs::path check_path = fs::exists(path) ? path : (path.has_parent_path() ? path.parent_path() : fs::current_path());
    if (statvfs(check_path.c_str(), &stat) == 0) {
        return static_cast<uint64_t>(stat.f_bavail) * stat.f_frsize;
    }
    return 0;
}

uint64_t FileOps::get_directory_size_bytes(const fs::path& dir_path) {
    uint64_t total_size = 0;
    std::error_code ec;
    if (!fs::exists(dir_path, ec) || !fs::is_directory(dir_path, ec)) return 0;

    for (const auto& entry : fs::recursive_directory_iterator(dir_path, fs::directory_options::skip_permission_denied, ec)) {
        if (entry.is_regular_file(ec) && !entry.is_symlink(ec)) {
            total_size += entry.file_size(ec);
        }
    }
    return total_size;
}

bool FileOps::ensure_directory_exists(const fs::path& dir_path) {
    std::error_code ec;
    return fs::create_directories(dir_path, ec) || fs::exists(dir_path, ec);
}

} // namespace vaxp
