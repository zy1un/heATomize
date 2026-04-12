#pragma once
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>

// ═══════════════════════════════════════════════════════════
// 消除阶段枚举 (Elimination Phase State Machine)
// ═══════════════════════════════════════════════════════════
enum class EliminationPhase
{
    IDLE,           // 空闲：无消除进行中
    LINE_MARKING,   // 线标记：线指示器展示，线球不变化
    LINE_FADING,    // 线消退：线球渐隐，线指示器保持
    BLOB_MARKING,   // 块标记：块指示器展示，块球不变化
    BLOB_FADING,    // 块消退：块球渐隐
    CLEANUP         // 清理：删除所有球，检查连锁
};

// ═══════════════════════════════════════════════════════════
// 消除结果结构体 (用于计分系统)
// ═══════════════════════════════════════════════════════════
struct EliminationResult
{
    bool hasElimination = false;
    std::vector<int> lineLengths;  // 每条线的长度 (非线性计分)
    std::vector<int> blobSizes;    // 纯连通块的大小 (需满足7阈值)
    int leftoverBalls = 0;         // 散球数量 (线消后的剩余球, 每个2分)
    int totalBalls = 0;            // 总共消除的球数
    
    // 计算直线数（仅线消参与交叉奖励判定）
    int getLineCount() const { return static_cast<int>(lineLengths.size()); }
};

// ═══════════════════════════════════════════════════════════
// 线消指示器 (Line Indicator for elimination animation)
// ═══════════════════════════════════════════════════════════
struct LineIndicator
{
    sf::Vector2i startPos;      // 线起点 (网格坐标)
    sf::Vector2i endPos;        // 线终点 (网格坐标)
    sf::Color color;            // 线颜色 (使用球的边框色/深色)
    float progress = 0.0f;      // 渐入进度 (0.0 -> 1.0)
    float fadeOutProgress = 0.0f;  // 渐隐进度 (0.0 -> 1.0)
    bool finished = false;      // 渐入动画是否完成
};

// ═══════════════════════════════════════════════════════════
// 连通块指示器 (Blob Indicator for elimination animation)
// ═══════════════════════════════════════════════════════════
struct BlobIndicator
{
    std::vector<sf::Vector2i> cells;  // 块内所有格子 (网格坐标)
    sf::Color color;                   // 轮廓颜色 (使用球的边框色/深色)
    float progress = 0.0f;             // 渐入进度 (0.0 -> 1.0)
    float fadeOutProgress = 0.0f;      // 渐隐进度 (0.0 -> 1.0)
    bool finished = false;             // 渐入动画是否完成
};

// ═══════════════════════════════════════════════════════════
// 计分弹窗 (Score Popup for scoring animation)
// ═══════════════════════════════════════════════════════════
struct ScorePopup
{
    sf::Vector2f position;     // 起始像素位置
    int score;                 // 分数值
    float progress = 0.0f;     // 动画进度 (0.0 -> 1.0)
    sf::Color color;           // 文字颜色
};
