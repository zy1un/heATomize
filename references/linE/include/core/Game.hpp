#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "graphics/Color.hpp"
#include "core/Defaults.hpp"
#include "board/Map.hpp"
#include "ui/UIManager.hpp"

// ═══════════════════════════════════════════════════════════
// 游戏状态枚举
// ═══════════════════════════════════════════════════════════
enum class GameState
{
    PLAYING,    // 正常游戏中
    GAME_OVER   // 游戏结束
};

class Game
{
public:
    Game(unsigned int width, unsigned int height, const std::string& title);
    
    // Main Loop
    void run();

    // Event Handling
    void handleResize(unsigned int newWidth, unsigned int newHeight);

private:
   // Internal Logic
    void checkGameOver();     // 检查是否游戏结束
    void restartGame();       // 重新开始游戏
    
    // Input
    void handleMouseMove();
    void handleMouseClick();
    
    // Helper to process input for UIManager interactions
    void setupUIActions();
    
    // Game Over Logic
    // TODO: Move Game Over rendering to UIManager too?
    // For now we keep drawGameOver internal or delegate it.
    void drawGameOver(); 

private:
    sf::RenderWindow window;
    sf::Vector2f mousePosition;
    
    // Game Logic State
    GameState m_gameState = GameState::PLAYING;
    Map map{LE::COLORS[16], LE::COLORS[21]};
    
    // UI System (The new manager)
    LE::UIManager m_ui;
    
    // ------------------------------------------------
    // Logic Variables (Data)
    // ------------------------------------------------
    int m_score = 0;
    int m_comboWave = 0;
    float m_playTime = 0.f;
    std::vector<LE::BallColor> m_nextColors;
    
    // ------------------------------------------------
    // Input / Selection State
    // ------------------------------------------------
    sf::Vector2i currentHovered{0, 0};
    sf::Vector2i previousHovered{0, 0};
    
    // Ball Selection
    sf::Vector2i selectedBallPosition{-1, -1};
    bool isBallSelected = false;
    
    // Moving State
    bool m_waitingForMove = false;
    sf::Vector2i m_moveTarget{-1, -1};
    
    // Rainbow Placement State (Special Mode)
    // Managed by Game because it interacts with Map directly
    bool m_isPlacingRainbow = false;
    sf::CircleShape m_dragBall;
    void drawDragBall();

    // ------------------------------------------------
    // Persistence Helpers (called by UI via callbacks)
    // ------------------------------------------------
    bool saveGame(int slot);
    bool loadGame(int slot);
    
    void processScore(const EliminationResult& result, const std::string& context = "Wave");
};
