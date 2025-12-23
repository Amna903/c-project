#include "file_manager.h"

std::vector<std::string> FileSystemManager::findPdfs(const std::string& rootPath) const {
    std::vector<std::string> pdfPaths;

    if (!fs::exists(rootPath)) {
        std::cerr << "[Error] Directory not found: " << rootPath << std::endl;
        return pdfPaths;
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".pdf") {
                pdfPaths.push_back(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[Filesystem Error] Accessing " << rootPath << ": " << e.what() << std::endl;
    }

    return pdfPaths;
}
