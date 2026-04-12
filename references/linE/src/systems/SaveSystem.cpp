#include "systems/SaveSystem.hpp"
#include "external/json.hpp"
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <chrono>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace LE
{
    // ═══════════════════════════════════════════════════════════
    // 辅助函数：BallColor 转字符串
    // ═══════════════════════════════════════════════════════════
    static std::string colorToString(BallColor color)
    {
        switch (color)
        {
            case BallColor::Red:       return "Red";
            case BallColor::Orange:    return "Orange";
            case BallColor::Yellow:    return "Yellow";
            case BallColor::Green:     return "Green";
            case BallColor::Cyan:      return "Cyan";
            case BallColor::Blue:      return "Blue";
            case BallColor::Purple:    return "Purple";
            case BallColor::Chameleon: return "Chameleon";
            case BallColor::Rainbow:   return "Rainbow";
            default:                   return "Red";
        }
    }
    
    static BallColor stringToColor(const std::string& str)
    {
        if (str == "Red")       return BallColor::Red;
        if (str == "Orange")    return BallColor::Orange;
        if (str == "Yellow")    return BallColor::Yellow;
        if (str == "Green")     return BallColor::Green;
        if (str == "Cyan")      return BallColor::Cyan;
        if (str == "Blue")      return BallColor::Blue;
        if (str == "Purple")    return BallColor::Purple;
        if (str == "Chameleon") return BallColor::Chameleon;
        if (str == "Rainbow")   return BallColor::Rainbow;
        return BallColor::Red;
    }
    
    // ═══════════════════════════════════════════════════════════
    // 存档路径
    // ═══════════════════════════════════════════════════════════
    
    std::string SaveSystem::getSaveDirectory()
    {
        // 使用游戏目录下的 saves 文件夹
        return "./saves";
    }
    
    std::string SaveSystem::getSavePath(int slot)
    {
        return getSaveDirectory() + "/save_" + std::to_string(slot) + ".json";
    }
    
    // ═══════════════════════════════════════════════════════════
    // 保存
    // ═══════════════════════════════════════════════════════════
    
    bool SaveSystem::save(const GameSaveData& data, int slot)
    {
        if (slot < 0 || slot >= MAX_SLOTS) return false;
        
        try
        {
            // 确保目录存在
            fs::create_directories(getSaveDirectory());
            
            // 构建 JSON
            json j;
            j["version"] = data.version;
            j["timestamp"] = data.timestamp;
            j["score"] = data.score;
            j["playTimeSeconds"] = data.playTimeSeconds;
            
            // 预览球
            json previewArr = json::array();
            for (const auto& color : data.previewColors)
            {
                previewArr.push_back(colorToString(color));
            }
            j["preview"] = previewArr;
            
            // 棋盘
            json boardArr = json::array();
            for (int y = 0; y < 9; ++y)
            {
                json rowArr = json::array();
                for (int x = 0; x < 9; ++x)
                {
                    const auto& cell = data.board[x][y];
                    json cellObj;
                    cellObj["hasBall"] = cell.hasBall;
                    if (cell.hasBall)
                    {
                        cellObj["color"] = colorToString(cell.color);
                        cellObj["isChameleon"] = cell.isChameleon;
                        if (cell.isChameleon)
                        {
                            cellObj["mimicColor"] = colorToString(cell.mimicColor);
                        }
                    }
                    rowArr.push_back(cellObj);
                }
                boardArr.push_back(rowArr);
            }
            j["board"] = boardArr;
            
            // 写入文件
            std::ofstream file(getSavePath(slot));
            if (!file.is_open()) return false;
            
            file << j.dump(2);  // 格式化输出，缩进2空格
            file.close();
            
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[SaveSystem] Save failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // 读取
    // ═══════════════════════════════════════════════════════════
    
    std::optional<GameSaveData> SaveSystem::load(int slot)
    {
        if (slot < 0 || slot >= MAX_SLOTS) return std::nullopt;
        
        try
        {
            std::ifstream file(getSavePath(slot));
            if (!file.is_open()) return std::nullopt;
            
            json j;
            file >> j;
            file.close();
            
            GameSaveData data;
            data.version = j.value("version", 1);
            data.timestamp = j.value("timestamp", "");
            data.score = j.value("score", 0);
            data.playTimeSeconds = j.value("playTimeSeconds", 0);
            
            // 预览球
            if (j.contains("preview") && j["preview"].is_array())
            {
                for (const auto& colorStr : j["preview"])
                {
                    data.previewColors.push_back(stringToColor(colorStr.get<std::string>()));
                }
            }
            
            // 棋盘
            if (j.contains("board") && j["board"].is_array())
            {
                const auto& boardArr = j["board"];
                for (int y = 0; y < 9 && y < static_cast<int>(boardArr.size()); ++y)
                {
                    const auto& rowArr = boardArr[y];
                    for (int x = 0; x < 9 && x < static_cast<int>(rowArr.size()); ++x)
                    {
                        const auto& cellObj = rowArr[x];
                        auto& cell = data.board[x][y];
                        
                        cell.hasBall = cellObj.value("hasBall", false);
                        if (cell.hasBall)
                        {
                            cell.color = stringToColor(cellObj.value("color", "Red"));
                            cell.isChameleon = cellObj.value("isChameleon", false);
                            if (cell.isChameleon)
                            {
                                cell.mimicColor = stringToColor(cellObj.value("mimicColor", "Red"));
                            }
                        }
                    }
                }
            }
            
            return data;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[SaveSystem] Load failed: " << e.what() << std::endl;
            return std::nullopt;
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // 存档管理
    // ═══════════════════════════════════════════════════════════
    
    bool SaveSystem::saveExists(int slot)
    {
        return fs::exists(getSavePath(slot));
    }
    
    bool SaveSystem::deleteSave(int slot)
    {
        if (slot < 0 || slot >= MAX_SLOTS) return false;
        
        try
        {
            return fs::remove(getSavePath(slot));
        }
        catch (...)
        {
            return false;
        }
    }
    
    SaveSlotInfo SaveSystem::getSlotInfo(int slot)
    {
        SaveSlotInfo info;
        info.exists = saveExists(slot);
        
        if (info.exists)
        {
            auto data = load(slot);
            if (data)
            {
                info.score = data->score;
                info.playTimeSeconds = data->playTimeSeconds;
                info.timestamp = data->timestamp;
            }
        }
        
        return info;
    }
    
    // ═══════════════════════════════════════════════════════════
    // 工具函数
    // ═══════════════════════════════════════════════════════════
    
    std::string SaveSystem::getCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
    
    std::string SaveSystem::formatPlayTime(int seconds)
    {
        int minutes = seconds / 60;
        int secs = seconds % 60;
        
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << minutes << ":"
            << std::setfill('0') << std::setw(2) << secs;
        return oss.str();
    }
    
    // ═══════════════════════════════════════════════════════════
    // 最后使用槽位管理
    // ═══════════════════════════════════════════════════════════
    
    std::string SaveSystem::getLastSlotPath()
    {
        return getSaveDirectory() + "/last_slot.json";
    }
    
    void SaveSystem::saveLastSlot(int slot)
    {
        if (slot < 0 || slot >= MAX_SLOTS) return;
        
        try
        {
            json j;
            j["lastSlot"] = slot;
            
            std::ofstream file(getLastSlotPath());
            if (file.is_open())
            {
                file << j.dump(2);
                std::cout << "[Save] Last slot saved: " << slot << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "[Save] Failed to save last slot: " << e.what() << std::endl;
        }
    }
    
    int SaveSystem::getLastSlot()
    {
        try
        {
            std::string path = getLastSlotPath();
            if (!fs::exists(path)) return -1;
            
            std::ifstream file(path);
            if (!file.is_open()) return -1;
            
            json j;
            file >> j;
            
            if (j.contains("lastSlot"))
            {
                int slot = j["lastSlot"].get<int>();
                if (slot >= 0 && slot < MAX_SLOTS && saveExists(slot))
                {
                    return slot;
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "[Save] Failed to read last slot: " << e.what() << std::endl;
        }
        
        return -1;
    }
}
