#pragma once
#include <string>
#include <optional>
#include <array>
#include <vector>
#include <chrono>
#include "entities/BallTypes.hpp"

namespace LE
{
    // ═══════════════════════════════════════════════════════════
    // 可序列化的游戏状态
    // ═══════════════════════════════════════════════════════════
    struct GameSaveData
    {
        int version = 1;
        std::string timestamp;
        int score = 0;
        int playTimeSeconds = 0;
        std::vector<BallColor> previewColors;
        
        // 棋盘状态: 每个格子存储 {颜色, 是否变色球, 拟态颜色}
        struct CellData
        {
            bool hasBall = false;
            BallColor color = BallColor::Red;
            bool isChameleon = false;
            BallColor mimicColor = BallColor::Red;
        };
        std::array<std::array<CellData, 9>, 9> board;
    };
    
    // ═══════════════════════════════════════════════════════════
    // 存档信息 (用于UI显示)
    // ═══════════════════════════════════════════════════════════
    struct SaveSlotInfo
    {
        bool exists = false;
        int score = 0;
        int playTimeSeconds = 0;
        std::string timestamp;
    };
    
    // ═══════════════════════════════════════════════════════════
    // 存读档管理器
    // ═══════════════════════════════════════════════════════════
    class SaveSystem
    {
    public:
        // 存档槽位数量
        static constexpr int MAX_SLOTS = 3;
        
        // 获取存档目录路径
        static std::string getSaveDirectory();
        
        // 获取存档文件路径
        static std::string getSavePath(int slot);
        
        // 保存游戏状态到 JSON 文件
        static bool save(const GameSaveData& data, int slot);
        
        // 从 JSON 文件读取游戏状态
        static std::optional<GameSaveData> load(int slot);
        
        // 检查存档是否存在
        static bool saveExists(int slot);
        
        // 删除存档
        static bool deleteSave(int slot);
        
        // 获取存档槽位信息
        static SaveSlotInfo getSlotInfo(int slot);
        
        // 获取当前时间戳字符串
        static std::string getCurrentTimestamp();
        
        // 格式化游玩时间 (秒 -> "MM:SS")
        static std::string formatPlayTime(int seconds);
        
        // ═══════════════════════════════════════════════════════════
        // 最后使用的存档槽位 (用于启动时自动载入)
        // ═══════════════════════════════════════════════════════════
        
        // 保存最后使用的槽位
        static void saveLastSlot(int slot);
        
        // 读取最后使用的槽位 (返回 -1 表示没有)
        static int getLastSlot();
        
        // 获取最后槽位配置文件路径
        static std::string getLastSlotPath();
    };
}
