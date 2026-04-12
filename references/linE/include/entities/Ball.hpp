#pragma once
#include <SFML/Graphics.hpp>
#include "entities/BallTypes.hpp"

namespace LE
{
    class Ball
    {
    public:
        Ball(sf::Vector2i gridPosition, BallColor ballColor);
        Ball();
        
        // ═══════════════════════════════════════════════════════════
        // 颜色相关
        // ═══════════════════════════════════════════════════════════
        
        // 获取用于消除检测的颜色（如果是变色球，返回拟态颜色）
        BallColor getBallColor() const;
        // 获取真实的原始颜色
        BallColor getRealColor() const;
        // 设置颜色
        void setBallColor(BallColor ballColor);
        // 设置拟态颜色（触发动画）
        void setMimicColor(BallColor color);
        // 获取变色球当前显示颜色
        sf::Color getMimicDisplayColor() const;
        
        // ═══════════════════════════════════════════════════════════
        // 状态相关
        // ═══════════════════════════════════════════════════════════
        
        BallState getBallState() const;
        void setBallState(BallState ballState);
        bool isChameleon() const;
        bool isRainbow() const;  // 彩虹球判断
        sf::Vector2i getGridPosition() const;
        void setGridPosition(sf::Vector2i newPosition);
        void setPixelPosition(sf::Vector2f position);  // 直接设置像素位置
        
        // ═══════════════════════════════════════════════════════════
        // 选中与呼吸动画
        // ═══════════════════════════════════════════════════════════
        
        void setSelected(bool selected);
        bool isSelected() const;
        void updateBlink(float deltaTime);
        
        // ═══════════════════════════════════════════════════════════
        // 生长动画
        // ═══════════════════════════════════════════════════════════
        
        void updateSpawn(float deltaTime);
        void finishSpawn();  // 立即完成生成动画（用于预告栏等非棋盘球）
        bool isSpawning() const;
        
        // ═══════════════════════════════════════════════════════════
        // 消除动画 (呼吸式渐隐)
        // ═══════════════════════════════════════════════════════════
        
        void startEliminate();
        void updateEliminate(float deltaTime);
        bool isEliminating() const;
        float getEliminateAlpha() const; // 返回 0-255 的透明度
        
        // 彩虹球消除时的颜色锁定
        void lockEliminateColor(BallColor color);  // 锁定到指定颜色
        bool hasLockedColor() const;               // 是否已锁定颜色
        
        // ═══════════════════════════════════════════════════════════
        // 移动动画
        // ═══════════════════════════════════════════════════════════
        
        void startMove(const std::vector<sf::Vector2i>& path);
        void updateMove(float deltaTime);
        bool isMoving() const;
        sf::Vector2f getDisplayPosition() const;  // 获取当前显示像素位置
        const std::vector<sf::Vector2i>& getMovePath() const;  // 获取移动路径
        int getMovePathIndex() const;  // 获取当前路径索引
        
        // ═══════════════════════════════════════════════════════════
        // 愤怒跳动动画 (无法到达时)
        // ═══════════════════════════════════════════════════════════
        
        void startAngryBounce();
        void updateAngryBounce(float deltaTime);
        bool isAngryBouncing() const;
        
        // ═══════════════════════════════════════════════════════════
        // 绘制
        // ═══════════════════════════════════════════════════════════
        
        void draw(sf::RenderTarget &target) const;

    private:
        // 绘制辅助方法
        void drawChameleon(sf::RenderTarget &target) const;
        void drawRainbow(sf::RenderTarget &target) const;  // 彩虹球绘制
        void drawNormal(sf::RenderTarget &target) const;
        float getBreathScale() const;
        float getSpawnScale() const;
        sf::Color getRainbowColor() const;  // 获取当前渐变色
        
        // 成员变量
        sf::CircleShape shape;
        sf::Vector2i gridPosition;
        BallColor ballColor;
        BallColor m_mimicColor;
        BallState ballState;
        bool m_isSelected = false;
        float m_breathPhase = 0.f;
        float m_spawnProgress = 0.f;
        
        // 消除动画
        bool m_isEliminating = false;
        float m_eliminatePhase = 0.f;  // 呼吸相位
        float m_eliminateAlpha = 255.f; // 当前透明度
        
        // 移动动画
        bool m_isMoving = false;
        std::vector<sf::Vector2i> m_movePath;  // 移动路径
        int m_movePathIndex = 0;               // 当前路径段索引
        float m_moveProgress = 0.f;            // 当前段进度 (0-1)
        
        // 愤怒跳动动画
        bool m_isAngryBouncing = false;
        float m_angryBounceProgress = 0.f;     // 动画进度 (0-1)
        
        // 彩虹球渐变动画
        mutable float m_rainbowPhase = 0.f;    // 色相偏移 (0-1 循环)
        
        // 彩虹球消除时的颜色锁定
        bool m_hasLockedColor = false;         // 是否已锁定颜色
        BallColor m_lockedColor = BallColor::Red;  // 锁定的目标颜色
    };
}
