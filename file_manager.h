#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

class FileSystemManager {
public:
    // Mark const if implementation is const
    std::vector<std::string> findPdfs(const std::string& directory) const;
};
