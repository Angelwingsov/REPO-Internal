#pragma once
#include "config.h"
#include <filesystem>
#include <string>
#include <vector>

namespace Core {
class ConfigStore {
public:
    static ConfigStore& Get();
    explicit ConfigStore(std::filesystem::path directory);
    const std::filesystem::path& Directory() const { return m_Directory; }
    bool List(std::vector<std::string>& names, std::string& error) const;
    bool Save(const std::string& name, bool overwrite, std::string& error) const;
    bool Load(const std::string& name, std::string& error) const;
    bool Delete(const std::string& name, std::string& error) const;
    static bool ValidName(const std::string& name);
    static std::string Serialize(Config& config);
    static bool Deserialize(const std::string& text, Config& config, std::string& error);
private:
    std::filesystem::path m_Directory;
};
}
