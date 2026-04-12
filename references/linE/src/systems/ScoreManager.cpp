#include "systems/ScoreManager.hpp"
#include "core/Defaults.hpp"
#include "graphics/Color.hpp"
#include <iostream>

namespace LE
{
    void ScoreManager::init(const sf::Font& font)
    {
        m_font = &font;
        
        // 创建分数标题
        m_scoreTitle = std::make_unique<sf::Text>(*m_font);
        m_scoreTitle->setString("SCORE");
        m_scoreTitle->setCharacterSize(18);
        m_scoreTitle->setFillColor(COLORS[18]);
        m_scoreTitle->setStyle(sf::Text::Bold);
        
        // 创建分数数值
        m_scoreValue = std::make_unique<sf::Text>(*m_font);
        m_scoreValue->setString("0");
        m_scoreValue->setCharacterSize(32);
        m_scoreValue->setFillColor(COLORS[21]);
        m_scoreValue->setStyle(sf::Text::Bold);
        
        m_initialized = true;
        updateDisplay();
    }
    
    int ScoreManager::processElimination(const EliminationResult& result, int comboWave, const std::string& context)
    {
        if (!result.hasElimination) return 0;
        
        m_comboWave = comboWave;
        
        // 使用Scoring命名空间计算分数
        auto breakdown = Scoring::calculateScore(
            result.lineLengths,
            result.blobSizes,
            result.leftoverBalls,
            m_comboWave
        );
        
        int gained = breakdown.finalScore;
        m_score += gained;
        
        // 日志输出
        std::cout << "[Score] " << context << " " << m_comboWave << ": ";
        if (!breakdown.details.empty())
        {
            std::cout << breakdown.details;
            if (breakdown.multiplier > 1)
            {
                std::cout << " x" << breakdown.multiplier;
            }
        }
        std::cout << " = +" << gained << " (Total: " << m_score << ")" << std::endl;
        
        updateDisplay();
        
        // 触发回调
        if (m_onScoreChanged)
        {
            m_onScoreChanged(m_score);
        }
        
        return gained;
    }
    
    bool ScoreManager::spendScore(int amount)
    {
        if (m_score >= amount)
        {
            m_score -= amount;
            updateDisplay();
            
            if (m_onScoreChanged)
            {
                m_onScoreChanged(m_score);
            }
            return true;
        }
        return false;
    }
    
    void ScoreManager::updateDisplay()
    {
        if (m_initialized && m_scoreValue)
        {
            m_scoreValue->setString(std::to_string(m_score));
        }
    }
    
    void ScoreManager::draw(sf::RenderWindow& window) const
    {
        if (!m_initialized) return;
        
        if (m_scoreTitle) window.draw(*m_scoreTitle);
        if (m_scoreValue) window.draw(*m_scoreValue);
    }
}
