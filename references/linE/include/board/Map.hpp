#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include "board/GridCell.hpp"
#include "board/EliminationTypes.hpp"
#include "entities/Ball.hpp"
#include "core/Defaults.hpp"

class Map
{
public:
    Map(sf::Color backGroundColor1, sf::Color backGroundColor2);

    // 批量绘制方法
    void draw(sf::RenderTarget &target) const;
    
    // 更新动画状态，返回 true 表示消除完成（需要检查连锁）
    bool update(float deltaTime);
    
    // 检查是否有动画正在进行
    bool isAnimating() const { return m_eliminationPhase != EliminationPhase::IDLE; }
    
    // 检查是否有球正在移动
    bool isBallMoving() const;
    
    // 启动球移动动画（不阻塞）
    void startBallMove(sf::Vector2i from, sf::Vector2i to, const std::vector<sf::Vector2i>& path);

    // 获取特定格子
    LE::GridCell *getGridCell(int x, int y);
    
    // 寻路，并返回是否存在可行路径
    bool pathSeeking(sf::Vector2i start, sf::Vector2i end) const;
    
    // 寻找路径，返回完整路径（用于动画）
    std::vector<sf::Vector2i> findPath(sf::Vector2i start, sf::Vector2i end) const;
    
    // 移动球从start到end（包含寻路检查）
    bool moveBall(sf::Vector2i start, sf::Vector2i end);
    
    // 对球进行寻亲并消除，返回详细结果（用于计分）
    EliminationResult eliminate();
    
    // 随机生成指定数量的球，返回实际生成的数量
    int spawnRandomBalls(int count = 3);
    
    // 更新所有变色球的状态
    void updateChameleonBalls();
    
    // 设置悬停的格子（用于拖拽放置时的激活效果）
    void setHoveredCell(sf::Vector2i gridPos, bool isValid);

    const LE::GridCell *getGridCell(int  x, int y) const;

    bool createBall(sf::Vector2i gridPosition, LE::BallColor ballColor);

    LE::Ball *getBall(sf::Vector2i position);
    const LE::Ball *getBall(sf::Vector2i position) const;

    LE::GridCell &getGridCell(sf::Vector2i position);
    const LE::GridCell &getGridCell(sf::Vector2i position) const;

    // 获取特定的下预览球颜色
    const std::vector<LE::BallColor>& getNextColors() const;
    
    // 生成随机颜色（内部使用，但也公开以便测试）
    static LE::BallColor generateRandomColor();
    
    // 检查棋盘是否已满
    bool isBoardFull() const;
    
    // 清除所有球（用于重开游戏）
    void clearAllBalls();
    
    // ═══════════════════════════════════════════════════════════
    // 存读档支持
    // ═══════════════════════════════════════════════════════════
    
    // 获取棋盘状态 (用于存档)
    struct CellState
    {
        bool hasBall = false;
        LE::BallColor color = LE::BallColor::Red;
        bool isChameleon = false;
        LE::BallColor mimicColor = LE::BallColor::Red;
    };
    using BoardState = std::array<std::array<CellState, 9>, 9>;
    BoardState getBoardState() const;
    
    // 恢复棋盘状态 (用于读档)
    void setBoardState(const BoardState& state);
    
    // 设置预览球颜色
    void setNextColors(const std::vector<LE::BallColor>& colors);

private:
    sf::RectangleShape backGroundShape1{{static_cast<float>(LE::MAIN_AREA_WIDTH), static_cast<float>(LE::WINDOW_HEIGHT)}};
    sf::RectangleShape backGroundShape2{{static_cast<float>(LE::SIDEBAR_BG_WIDTH), static_cast<float>(LE::WINDOW_HEIGHT)}};
    sf::Color backGroundColor1;
    sf::Color backGroundColor2;
    std::array<std::array<LE::GridCell, 9>, 9> gridCells;
    std::array<std::array<std::unique_ptr<LE::Ball>, 9>, 9> balls;
    
    // 下一次生成的球颜色缓冲
    std::vector<LE::BallColor> m_nextColors;
    
    // 消除状态机
    EliminationPhase m_eliminationPhase = EliminationPhase::IDLE;
    float m_phaseTimer = 0.0f;
    
    // 分离的待消除列表
    std::vector<sf::Vector2i> m_lineElimination;  // 线球列表
    std::vector<sf::Vector2i> m_blobElimination;  // 块球列表 (纯块 + 散球)
    
    // 检测到的直线（用于彩虹球消除时的颜色锁定）
    std::vector<std::vector<sf::Vector2i>> m_detectedLines;
    
    // 待创建的计分弹窗数据 (在相应 FADING 阶段创建)
    std::vector<ScorePopup> m_pendingLinePopups;  // 线消分数弹窗
    
    // 线消指示器动画
    std::vector<LineIndicator> m_lineIndicators;
    
    // 连通块指示器动画
    std::vector<BlobIndicator> m_blobIndicators;
    
    // 计分弹窗动画
    std::vector<ScorePopup> m_scorePopups;
    
    // 辅助方法
    void drawLineIndicators(sf::RenderTarget &target) const;
    void drawBlobIndicators(sf::RenderTarget &target) const;
    void startLineFading();   // 开始线球消退动画
    void startBlobFading();   // 开始块球消退动画
    void updateFadingBalls(float deltaTime);  // 更新正在消失的球
    bool checkAllBallsFaded() const;  // 检查所有球是否已消失
    
public:
    // 绘制计分弹窗 (需要字体，所以在公共区域)
    void drawScorePopups(sf::RenderTarget &target, const sf::Font &font) const;
};

