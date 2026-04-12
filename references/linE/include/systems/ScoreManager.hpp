#pragma once
#include <string>
#include <functional>
#include <SFML/Graphics.hpp>
#include "board/EliminationTypes.hpp"
#include "systems/Scoring.hpp"

namespace LE
{
    // ═══════════════════════════════════════════════════════════
    // 分数管理器 - 统一管理游戏分数和金币
    // ═══════════════════════════════════════════════════════════
    class ScoreManager
    {
    public:
        ScoreManager() = default;
        
        // 初始化UI（需要字体）
        void init(const sf::Font& font);
        
        // 处理消除结果并返回获得的分数
        int processElimination(const EliminationResult& result, int comboWave, const std::string& context = "Wave");
        
        // 分数操作
        int getScore() const { return m_score; }
        void setScore(int score) { m_score = score; updateDisplay(); }
        void addScore(int amount) { m_score += amount; updateDisplay(); }
        bool spendScore(int amount);
        
        // 金币 = 分数（简化模型）
        int getCoins() const { return m_score; }
        
        // 绘制分数UI
        void draw(sf::RenderWindow& window) const;
        
        // 连锁波次管理
        int getComboWave() const { return m_comboWave; }
        void setComboWave(int wave) { m_comboWave = wave; }
        void incrementComboWave() { m_comboWave++; }
        void resetComboWave() { m_comboWave = 0; }
        
        // 分数变化回调（用于通知商店等系统）
        void setOnScoreChanged(std::function<void(int)> callback) { m_onScoreChanged = callback; }
        
    private:
        void updateDisplay();
        
        int m_score = 0;
        int m_comboWave = 0;
        
        // UI 元素
        const sf::Font* m_font = nullptr;
        std::unique_ptr<sf::Text> m_scoreTitle;
        std::unique_ptr<sf::Text> m_scoreValue;
        bool m_initialized = false;
        
        // 回调
        std::function<void(int)> m_onScoreChanged;
    };
}
