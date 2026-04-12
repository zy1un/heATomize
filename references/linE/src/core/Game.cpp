#include <iostream>
#include <cmath>
#include "core/Game.hpp"
#include "graphics/Color.hpp"
#include "graphics/BallRenderer.hpp"
#include "core/Defaults.hpp"
#include "Utility.hpp"
#include "systems/Animation.hpp"
#include "systems/Scoring.hpp"
#include "systems/SaveSystem.hpp"
#include "entities/BallTypes.hpp"

using namespace LE;

Game::Game(unsigned int width, unsigned int height, const std::string &title)
    : window(sf::RenderWindow(sf::VideoMode({width, height}), title, 
             sf::Style::Default))
{
    setupUIActions();
    
    // Attempt auto-load
    int lastSlot = LE::SaveSystem::getLastSlot();
    if (lastSlot >= 0)
    {
        if (loadGame(lastSlot))
        {
            std::cout << "[Game] Auto-loaded last save from slot " << lastSlot << std::endl;
        }
    }
}


void Game::setupUIActions()
{
    LE::UIActions actions;
    
    actions.onShopOpen = [this]() {
        m_ui.setShopVisible(true);
        std::cout << "[UI] Shop Opened" << std::endl;
    };
    
    actions.onSave = [this]() {
        m_ui.showSlotPanel(true);
    };
    
    actions.onLoad = [this]() {
        m_ui.showSlotPanel(false);
    };
    
    actions.onReset = [this]() {
        restartGame();
    };
    
    actions.onExit = [this]() {
        window.close();
    };
    
    actions.onSaveSlot = [this](int slot) {
        return saveGame(slot);
    };
    
    actions.onLoadSlot = [this](int slot) {
        return loadGame(slot);
    };
    
    actions.onShopPurchase = [this](LE::ItemType item) {
        if (item == LE::ItemType::RAINBOW_PLACE) {
            if (m_score >= LE::RAINBOW_PLACE_PRICE) {
                m_score -= LE::RAINBOW_PLACE_PRICE;
                m_isPlacingRainbow = true;
                map.setHoveredCell({-1, -1}, false);
                m_ui.setShopVisible(false);
                m_ui.updateScore(m_score);
                std::cout << "[Shop] Rainbow Ball purchased. Place it on board." << std::endl;
            } else {
                std::cout << "[Shop] Not enough score!" << std::endl;
            }
        }
        else if (item == LE::ItemType::RAINBOW_PREVIEW) {
             if (m_score >= LE::RAINBOW_PREVIEW_PRICE) {
                 m_score -= LE::RAINBOW_PREVIEW_PRICE;
                 std::vector<LE::BallColor> rainbowNext(LE::PREVIEW_BALL_COUNT, LE::BallColor::Rainbow);
                 map.setNextColors(rainbowNext);
                 m_ui.updateScore(m_score);
                 m_ui.updatePreview(map.getNextColors());
                 std::cout << "[Shop] Rainbow Preview purchased." << std::endl;
             }
        }
    };

    m_ui.init(actions);
    m_ui.updateScore(m_score);
    m_ui.updatePreview(map.getNextColors());
}

bool Game::saveGame(int slot)
{
    using namespace LE;
    // 构建存档数据
    GameSaveData data;
    data.version = 1;
    data.timestamp = SaveSystem::getCurrentTimestamp();
    data.score = m_score;
    data.playTimeSeconds = static_cast<int>(m_playTime);
    
    // 获取预览球颜色
    data.previewColors = map.getNextColors();
    
    // 获取棋盘状态 (转换类型)
    auto boardState = map.getBoardState();
    
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            const auto& src = boardState[x][y];
            auto& dst = data.board[x][y];
            dst.hasBall = src.hasBall;
            dst.color = src.color;
            dst.isChameleon = src.isChameleon;
            dst.mimicColor = src.mimicColor;
        }
    }
    
    bool success = SaveSystem::save(data, slot);
    if (success)
    {
        SaveSystem::saveLastSlot(slot);
        std::cout << "[Game] Saved to slot " << slot << std::endl;
    }
    else
    {
        std::cout << "[Game] Save failed!" << std::endl;
    }
    
    return success;
}

bool Game::loadGame(int slot)
{
    using namespace LE;
    
    auto dataOpt = SaveSystem::load(slot);
    if (!dataOpt)
    {
        std::cout << "[Game] Load failed: slot " << slot << " not found" << std::endl;
        return false;
    }
    
    const auto& data = *dataOpt;
    
    m_score = data.score;
    m_playTime = static_cast<float>(data.playTimeSeconds);
    m_comboWave = 0;
    
    Map::BoardState boardState;
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            const auto& src = data.board[x][y];
            auto& dst = boardState[x][y];
            dst.hasBall = src.hasBall;
            dst.color = src.color;
            dst.isChameleon = src.isChameleon;
            dst.mimicColor = src.mimicColor;
        }
    }
    map.setBoardState(boardState);
    map.setNextColors(data.previewColors);
    
    m_gameState = GameState::PLAYING;
    isBallSelected = false;
    selectedBallPosition = {-1, -1};
    m_waitingForMove = false;
    m_moveTarget = {-1, -1};
    m_isPlacingRainbow = false;
    map.setHoveredCell({-1, -1}, false);
    
    // Refresh UI
    m_ui.updateScore(m_score);
    m_ui.updatePreview(map.getNextColors());
    
    std::cout << "[Game] Loaded from slot " << slot << std::endl;
    return true;
}

void Game::drawDragBall() {
    // ... logic ...
    sf::Vector2f pos = mousePosition;
    m_dragBall.setRadius(GRID_LENGTH / 2.f - 5.f);
    m_dragBall.setOrigin({m_dragBall.getRadius(), m_dragBall.getRadius()});
    m_dragBall.setPosition(pos);
    
    // Rainbow animation from renderer?
    // Just simple color for drag
    m_dragBall.setFillColor(sf::Color(255, 255, 255, 128)); // Ghostly
    window.draw(m_dragBall);
}

void Game::checkGameOver()
{
    if (map.isBoardFull())
    {
        m_gameState = GameState::GAME_OVER;
        std::cout << "[Game Over] Final Score: " << m_score << std::endl;
    }
}

void Game::restartGame()
{
    map.clearAllBalls();
    m_score = 0;
    m_comboWave = 0;
    isBallSelected = false;
    selectedBallPosition = {-1, -1};
    m_waitingForMove = false;
    m_moveTarget = {-1, -1};
    m_isPlacingRainbow = false;
    map.setHoveredCell({-1, -1}, false);
    map.spawnRandomBalls(LE::INITIAL_BALL_COUNT);
    map.updateChameleonBalls();
    m_gameState = GameState::PLAYING;
    m_ui.updateScore(m_score);
    m_ui.updatePreview(map.getNextColors());
    std::cout << "[Game] Restarted!" << std::endl;
}

void Game::drawGameOver()
{
    using namespace LE;
    const float centerX = static_cast<float>(WINDOW_WIDTH) / 2.f;

    sf::RectangleShape overlay({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    overlay.setFillColor(sf::Color(0, 0, 0, GAME_OVER_OVERLAY_ALPHA));
    window.draw(overlay);
    
    // "GAME OVER" 标题
    sf::Text titleText(m_ui.getFont());
    titleText.setString("GAME OVER");
    titleText.setCharacterSize(GAME_OVER_TITLE_SIZE + 8);
    titleText.setFillColor(sf::Color(255, 214, 78));
    titleText.setStyle(sf::Text::Bold);
    titleText.setLetterSpacing(1.1f);
    titleText.setOutlineColor(COLORS[16]);
    titleText.setOutlineThickness(3.4f);
    
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin(sf::Vector2f(titleBounds.position.x + titleBounds.size.x / 2.f,
                         titleBounds.position.y + titleBounds.size.y / 2.f));
    titleText.setPosition(sf::Vector2f(centerX, GAME_OVER_TITLE_Y));
    window.draw(titleText);
    
    // 最终分数
    sf::Text scoreText(m_ui.getFont());
    scoreText.setString("Final Score: " + std::to_string(m_score));
    scoreText.setCharacterSize(GAME_OVER_SCORE_SIZE + 2);
    scoreText.setFillColor(sf::Color(242, 236, 226));
    scoreText.setStyle(sf::Text::Bold);
    scoreText.setOutlineColor(sf::Color(18, 16, 14, 170));
    scoreText.setOutlineThickness(1.2f);
    
    sf::FloatRect scoreBounds = scoreText.getLocalBounds();
    scoreText.setOrigin(sf::Vector2f(scoreBounds.position.x + scoreBounds.size.x / 2.f,
                         scoreBounds.position.y + scoreBounds.size.y / 2.f));
    scoreText.setPosition(sf::Vector2f(centerX, GAME_OVER_SCORE_Y));
    window.draw(scoreText);
    
    // "Click to Restart" 提示
    sf::Text restartText(m_ui.getFont());
    restartText.setString("Click anywhere to restart");
    restartText.setCharacterSize(GAME_OVER_HINT_SIZE);
    restartText.setFillColor(sf::Color(214, 206, 196));
    restartText.setStyle(sf::Text::Bold);
    
    sf::FloatRect restartBounds = restartText.getLocalBounds();
    restartText.setOrigin(sf::Vector2f(restartBounds.position.x + restartBounds.size.x / 2.f,
                           restartBounds.position.y + restartBounds.size.y / 2.f));
    restartText.setPosition(sf::Vector2f(centerX, GAME_OVER_HINT_Y));
    window.draw(restartText);
}


void Game::run()
{
    sf::Clock clock;
    while (window.isOpen())
    {
        sf::Time dt = clock.restart();
        float deltaTime = dt.asSeconds();

        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                // Delegate to UI Exit confirmation
                m_ui.showExitConfirm();
            }
            
            if (event->is<sf::Event::Resized>())
            {
                const auto* resized = event->getIf<sf::Event::Resized>();
                handleResize(resized->size.x, resized->size.y);
            }
            
            if (event->is<sf::Event::MouseMoved>())
            {
                sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
                mousePosition = window.mapPixelToCoords(pixelPos);
                
                // Let UI handle hover first
                if (!m_ui.handleMouseMove(mousePosition))
                {
                    // If UI didn't consume, Game handles it
                    handleMouseMove();
                }
            }
            
            if (event->is<sf::Event::MouseButtonPressed>())
            {
                const auto* mb = event->getIf<sf::Event::MouseButtonPressed>();
                if (mb && mb->button == sf::Mouse::Button::Left)
                {
                    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
                    mousePosition = window.mapPixelToCoords(pixelPos);
                    
                    // Let UI handle click first
                    if (!m_ui.handleClick(mousePosition))
                    {
                        // Check if animating
                        if (!map.isAnimating())
                        {
                            handleMouseClick();
                        }
                    }
                }
            }
            
            if (event->is<sf::Event::KeyPressed>())
            {
                const auto* keyEvent = event->getIf<sf::Event::KeyPressed>();
                if (keyEvent)
                {
                     // Hotkeys
                     if (keyEvent->code == sf::Keyboard::Key::F5) saveGame(0);
                     else if (keyEvent->code == sf::Keyboard::Key::F9) loadGame(0);
                     else if (keyEvent->code == sf::Keyboard::Key::Num1 && keyEvent->control) saveGame(0);
                     else if (keyEvent->code == sf::Keyboard::Key::Num2 && keyEvent->control) saveGame(1);
                     else if (keyEvent->code == sf::Keyboard::Key::Num3 && keyEvent->control) saveGame(2);
                     else if (keyEvent->code == sf::Keyboard::Key::Num1 && keyEvent->alt) loadGame(0);
                     else if (keyEvent->code == sf::Keyboard::Key::Num2 && keyEvent->alt) loadGame(1);
                     else if (keyEvent->code == sf::Keyboard::Key::Num3 && keyEvent->alt) loadGame(2);
                }
            }
        }
        
        // Update Logic
        if (m_gameState == GameState::PLAYING)
        {
            m_playTime += deltaTime;
        }

        // Map Update
        if (map.update(deltaTime))
        {
            map.updateChameleonBalls();
            m_comboWave++;
            auto chainResult = map.eliminate();
            if (chainResult.hasElimination)
            {
                processScore(chainResult, "Chain");
            }
            else
            {
                m_comboWave = 0;
            }
        }
        
        // Move Animation Completion
        if (m_waitingForMove && !map.isBallMoving())
        {
            m_waitingForMove = false;
            map.updateChameleonBalls();
            m_comboWave = 1;
            auto result = map.eliminate();
            
            if (result.hasElimination)
            {
                processScore(result, "Wave");
            }
            else
            {
                m_comboWave = 0;
                int spawned = map.spawnRandomBalls(LE::SPAWN_BALL_COUNT);
                if (spawned > 0)
                {
                    map.updateChameleonBalls();
                    m_comboWave = 1;
                    auto spawnResult = map.eliminate();
                    if (spawnResult.hasElimination) processScore(spawnResult, "Spawn");
                    else m_comboWave = 0;
                    
                    // Sync preview with UI after spawn
                    m_ui.updatePreview(map.getNextColors());
                }
                checkGameOver();
            }
        }
        
        // System Updates
        LE::Animation::update(deltaTime);
        LE::BallRenderer::updateRainbowPhase(deltaTime);
        
        if (isBallSelected)
        {
             if (auto* b = map.getBall(selectedBallPosition)) b->updateBlink(deltaTime);
        }
        
        // UI Interaction Updates
        m_ui.update(deltaTime, mousePosition);

        // Dragging Logic
        if (m_isPlacingRainbow)
        {
            if (isInGrids(mousePosition))
            {
                sf::Vector2i gridPos = pixelToGrid(mousePosition);
                map.setHoveredCell(gridPos, map.getBall(gridPos) == nullptr);
            }
            else
            {
                map.setHoveredCell({-1, -1}, false);
            }
        }

        // Render
        window.clear();

        map.draw(window);
        map.drawScorePopups(window, m_ui.getFont()); 
        
        m_ui.draw(window);
        if (m_gameState == GameState::GAME_OVER) drawGameOver();
        
        // Overlays
        m_ui.drawOverlay(window);
        if (m_isPlacingRainbow) drawDragBall();

        window.display();
    }
}

void Game::handleMouseMove()
{
    // Simplified: only map hover logic
    if (m_gameState == GameState::GAME_OVER) return;

    if (isInGrids(mousePosition))
    {
        sf::Vector2i newHovered = pixelToGrid(mousePosition);
        if (!isInGrids(newHovered)) return;
        
        currentHovered = newHovered;
        if (currentHovered != previousHovered)
        {
            if (isInGrids(previousHovered)) map.getGridCell(previousHovered).setSelected(false);
            map.getGridCell(currentHovered).setSelected(true);
            previousHovered = currentHovered;
        }
    }
    else
    {
        if (isInGrids(previousHovered)) map.getGridCell(previousHovered).setSelected(false);
        previousHovered = {-1, -1};
    }
}

void Game::handleMouseClick()
{
    if (m_gameState == GameState::GAME_OVER) { restartGame(); return; }
    
    // Rainbow Placement
    if (m_isPlacingRainbow)
    {
        if (isInGrids(mousePosition))
        {
            sf::Vector2i cell = pixelToGrid(mousePosition);
            if (!map.getBall(cell))
            {
                map.createBall(cell, LE::BallColor::Rainbow);
                m_isPlacingRainbow = false;
                map.setHoveredCell({-1, -1}, false);
                std::cout << "[Game] Rainbow Placed." << std::endl;
                
                auto elim = map.eliminate();
                if (elim.hasElimination) {
                    m_comboWave = 1;
                    processScore(elim, "Rainbow");
                }
            }
        }
        else
        {
            // Cancel placement
             m_isPlacingRainbow = false;
             m_score += LE::RAINBOW_PLACE_PRICE;
             m_ui.updateScore(m_score);
             std::cout << "[Game] Placement Cancelled." << std::endl;
        }
        return;
    }
    
    // Normal Interaction
    if (!isInGrids(mousePosition)) return;
    sf::Vector2i cell = pixelToGrid(mousePosition);
    
    // ... Logic for selection same as before ...
    if (!isBallSelected)
    {
        if (map.getBall(cell))
        {
            isBallSelected = true;
            selectedBallPosition = cell;
            map.getBall(cell)->setSelected(true);
        }
    }
    else
    {
        if (cell == selectedBallPosition)
        {
            if(auto* b = map.getBall(selectedBallPosition)) b->setSelected(false);
            isBallSelected = false;
            selectedBallPosition = {-1, -1};
        }
        else
        {
            if (map.getBall(cell))
            {
                if(auto* b = map.getBall(selectedBallPosition)) b->setSelected(false);
                if(auto* b = map.getBall(cell)) b->setSelected(true);
                selectedBallPosition = cell;
            }
            else
            {
                // Move
                auto path = map.findPath(selectedBallPosition, cell);
                if (!path.empty())
                {
                    if(auto* b = map.getBall(selectedBallPosition)) b->setSelected(false);
                    map.startBallMove(selectedBallPosition, cell, path);
                    m_waitingForMove = true;
                    m_moveTarget = cell;
                    isBallSelected = false;
                    selectedBallPosition = {-1, -1};
                }
                else
                {
                    if(auto* b = map.getBall(selectedBallPosition)) b->startAngryBounce();
                    std::cout << "Unreachable!" << std::endl;
                }
            }
        }
    }
}

void Game::handleResize(unsigned int newWidth, unsigned int newHeight)
{
    const float designWidth = 735.f;
    const float designHeight = 525.f;
    float scaleX = (float)newWidth / designWidth;
    float scaleY = (float)newHeight / designHeight;
    float scale = std::min(scaleX, scaleY);
    
    float viewportWidth = (designWidth * scale) / newWidth;
    float viewportHeight = (designHeight * scale) / newHeight;
    
    sf::View view(sf::FloatRect({0.f, 0.f}, {designWidth, designHeight}));
    view.setViewport(sf::FloatRect({(1.f - viewportWidth)/2.f, (1.f - viewportHeight)/2.f}, {viewportWidth, viewportHeight}));
    window.setView(view);
    
    m_ui.handleResize(newWidth, newHeight);
}

void Game::processScore(const EliminationResult& result, const std::string& context)
{
    auto score = Scoring::calculateScore(result.lineLengths, result.blobSizes, result.leftoverBalls, m_comboWave);
    m_score += score.finalScore;
    m_ui.updateScore(m_score);
    // Log logic...
}
