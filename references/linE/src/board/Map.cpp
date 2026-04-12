#include <SFML/Graphics.hpp>
#include <array>
#include <memory>
#include <queue>
#include <algorithm>
#include <random>
#include <iostream>
#include <map>
#include <cmath>
#include <set>
#include "board/GridCell.hpp"
#include "board/Map.hpp"
#include "entities/Ball.hpp"
#include "entities/BallTypes.hpp"
#include "Utility.hpp"
#include "graphics/Color.hpp"
#include "core/Defaults.hpp"
#include "systems/Scoring.hpp"

// 使用 BallTypes.hpp 中的 ballColorToString
using LE::ballColorToString;

Map::Map(sf::Color backGroundColor1, sf::Color backGroundColor2) 
: backGroundColor1(backGroundColor1), backGroundColor2(backGroundColor2)
{
    // 初始化背景
    backGroundShape1.setFillColor(backGroundColor1);
    backGroundShape2.setFillColor(backGroundColor2);
    backGroundShape1.setPosition({0, 0});
    backGroundShape2.setPosition({static_cast<float>(LE::MAIN_AREA_WIDTH), 0});
    
    // 初始化网格
    for (int y = 0; y < 9; y++) {
        for (int x = 0; x < 9; x++) {
            gridCells[x][y] = LE::GridCell({x, y});
        }
    }
    
    // 生成初始球
    spawnRandomBalls(LE::INITIAL_BALL_COUNT);
    updateChameleonBalls();
    
    // 初始化下一次生成的球预览
    for (int i = 0; i < LE::PREVIEW_BALL_COUNT; ++i) {
        m_nextColors.push_back(generateRandomColor());
    }
}
// 从文件加载地图数据并创建GridCell对象
// void loadFromFile(const std::string& filename, const sf::Texture& tileTextureAtlas);

// void update(sf::Time deltaTime); // 如果地图有动画或交互，需要更新

// 批量绘制方法
void Map:: draw(sf::RenderTarget &target) const{
	
	target.draw(backGroundShape1);
	target.draw(backGroundShape2);
	for (int y = 0; y < 9; y++) {
		for (int x = 0; x < 9; x++) {
			gridCells[x][y].draw(target);
		}
	}
	for (int y = 0; y < 9; y++) {
		for (int x = 0; x < 9; x++) {
			if(balls[x][y])
				balls[x][y]->draw(target);
		}
	}
	
	// 绘制线消指示器 (在球的上层)
	drawLineIndicators(target);
	
	// 绘制连通块轮廓指示器
	drawBlobIndicators(target);
}

// 获取特定格子
LE::GridCell* Map:: getGridCell(int x, int y){
	return &gridCells[x][y];
}
const LE::GridCell* Map:: getGridCell(int x, int y) const{
	return &gridCells[x][y];
}

bool Map:: createBall(sf::Vector2i gridPosition,LE::BallColor ballColor){
	balls[gridPosition.x][gridPosition.y] = std::make_unique<LE::Ball>(gridPosition, ballColor);
	return true;
}

LE::Ball* Map:: getBall(sf::Vector2i position){
	if(position.x>=9||position.x<0||position.y>=9||position.y<0)
		return nullptr;
	return balls[position.x][position.y].get();
}
    const LE::Ball* Map:: getBall(sf::Vector2i position) const{
		return balls[position.x][position.y].get();
	}

    LE::GridCell& Map:: getGridCell(sf::Vector2i position){
		if(!LE :: isInGrids(position))
			throw std::out_of_range("Invalid grid position");
		return gridCells[position.x][position.y];
	}
    const LE::GridCell& Map:: getGridCell(sf::Vector2i position) const{
		if(position.x>=9||position.x<0||position.y>=9||position.y<0)
			throw std::out_of_range("Invalid grid position");
		return gridCells[position.x][position.y];
	}

// ═══════════════════════════════════════════════════════════
// 寻路相关函数已移至 MapPathfinding.cpp
// ═══════════════════════════════════════════════════════════


// 随机生成指定数量的球
int Map::spawnRandomBalls(int count)
{
    // 1. 收集所有空格子
    std::vector<sf::Vector2i> emptyCells;
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            if (balls[x][y] == nullptr)
            {
                emptyCells.push_back({x, y});
            }
        }
    }

    // 2. 如果没有空位，返回0
    if (emptyCells.empty())
        return 0;

    // 3. 随机打乱
    static std::random_device rd;
    static std::mt19937 g(rd());
    std::shuffle(emptyCells.begin(), emptyCells.end(), g);

    // 4. 生成球
    int spawned = 0;
    int limit = std::min((int)emptyCells.size(), count);
    
    for (int i = 0; i < limit; ++i)
    {
        sf::Vector2i pos = emptyCells[i];
        
        // 从预告队列获取颜色
        LE::BallColor color;
        if (!m_nextColors.empty())
        {
            color = m_nextColors.front();
            m_nextColors.erase(m_nextColors.begin());
        }
        else
        {
            color = generateRandomColor();
        }
        
        // 补充新球到预告队列
        m_nextColors.push_back(generateRandomColor());
        
        createBall(pos, color);
        std::cout << "  Spawned " << ballColorToString(color) << " at (" << pos.x << "," << pos.y << ")" << std::endl;
        spawned++;
    }
    
    // 生成新球后，更新变色球状态
    if (spawned > 0)
    {
        std::cout << "[Spawn] " << spawned << " balls spawned, updating chameleon balls..." << std::endl;
        updateChameleonBalls();
    }

    return spawned;
}

// ═══════════════════════════════════════════════════════════
// 辅助函数：计算位置优先级
// ═══════════════════════════════════════════════════════════
namespace {
    // 8方向偏移量
    constexpr int DX[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    constexpr int DY[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    
    // 计算位置优先级分数
    inline int calcPositionPriority(int x, int y, LE::BallColor color)
    {
        return (x + 9 * y) * 10 + static_cast<int>(color);
    }
}

// 更新所有变色球的状态（分层优先级系统）
void Map::updateChameleonBalls()
{
    // 收集所有变色球位置
    std::vector<sf::Vector2i> chameleonPositions;
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            LE::Ball* ball = balls[x][y].get();
            if (ball != nullptr && ball->getRealColor() == LE::BallColor::Chameleon)
            {
                chameleonPositions.push_back({x, y});
            }
        }
    }
    
    if (chameleonPositions.empty())
        return;
    
    // 临时颜色数组 [9][9]
    LE::BallColor tempColors[9][9];
    for (int x = 0; x < 9; ++x)
        for (int y = 0; y < 9; ++y)
            tempColors[x][y] = LE::BallColor::Chameleon;
    
    // 迭代计算目标颜色
    bool changed;
    int iteration = 0;
    const int MAX_ITERATIONS = LE::MAX_CHAMELEON_ITERATIONS;
    
    do {
        changed = false;
        iteration++;
        
        for (const auto& pos : chameleonPositions)
        {
            // 分层优先级：原色球 > 变色球
            std::map<LE::BallColor, int> originalScores;
            std::map<LE::BallColor, int> chameleonScores;
            
            for (int i = 0; i < 8; ++i)
            {
                int nx = pos.x + DX[i];
                int ny = pos.y + DY[i];
                
                if (LE::isInGrids(sf::Vector2i(nx, ny)))
                {
                    LE::Ball* neighbor = balls[nx][ny].get();
                    if (neighbor != nullptr)
                    {
                        if (neighbor->getRealColor() == LE::BallColor::Chameleon)
                        {
                            LE::BallColor tempColor = tempColors[nx][ny];
                            if (tempColor != LE::BallColor::Chameleon && tempColor != LE::BallColor::Rainbow)
                            {
                                chameleonScores[tempColor] += calcPositionPriority(nx, ny, tempColor);
                            }
                        }
                        else if (neighbor->getRealColor() != LE::BallColor::Rainbow)
                        {
                            // 彩虹球不参与变色判定，跳过
                            LE::BallColor color = neighbor->getBallColor();
                            originalScores[color] += calcPositionPriority(nx, ny, color);
                        }
                    }
                }
            }
            
            // ═══════════════════════════════════════════════════════════
            // 决策逻辑：原色球绝对优先
            // ═══════════════════════════════════════════════════════════
            LE::BallColor targetColor = LE::BallColor::Chameleon;
            int maxScore = 0;
            
            if (!originalScores.empty())
            {
                // 有原色球邻居：只看原色球
                for (const auto& pair : originalScores)
                {
                    if (pair.second > maxScore)
                    {
                        maxScore = pair.second;
                        targetColor = pair.first;
                    }
                }
            }
            else if (!chameleonScores.empty())
            {
                // 只有变色球邻居：看变色球（链式传播）
                for (const auto& pair : chameleonScores)
                {
                    if (pair.second > maxScore)
                    {
                        maxScore = pair.second;
                        targetColor = pair.first;
                    }
                }
            }
            
            // 更新临时数组
            if (tempColors[pos.x][pos.y] != targetColor)
            {
                tempColors[pos.x][pos.y] = targetColor;
                changed = true;
            }
        }
    } while (changed && iteration < MAX_ITERATIONS);
    
    // ═══════════════════════════════════════════════════════════
    // 应用阶段：只更新真正改变的球
    // ═══════════════════════════════════════════════════════════
    for (const auto& pos : chameleonPositions)
    {
        LE::Ball* ball = balls[pos.x][pos.y].get();
        if (!ball) continue;
        
        LE::BallColor currentColor = ball->getBallColor();
        LE::BallColor targetColor = tempColors[pos.x][pos.y];
        
        if (currentColor != targetColor)
        {
            ball->setMimicColor(targetColor);
        }
    }
}

// 更新动画状态 (六阶段交错状态机)
bool Map::update(float deltaTime)
{
    // 始终更新球的生长动画
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            if (balls[x][y])
            {
                balls[x][y]->updateSpawn(deltaTime);
                balls[x][y]->updateMove(deltaTime);  // 更新移动动画
                balls[x][y]->updateAngryBounce(deltaTime);  // 更新愤怒跳动
                
                // 路径高亮：如果球正在移动，高亮已经过的格子
                if (balls[x][y]->isMoving())
                {
                    const auto& path = balls[x][y]->getMovePath();
                    int currentIdx = balls[x][y]->getMovePathIndex();
                    
                    // 高亮从起点到当前位置的格子
                    for (int i = 0; i <= currentIdx + 1 && i < static_cast<int>(path.size()); ++i)
                    {
                        const auto& pos = path[i];
                        if (pos.x >= 0 && pos.x < 9 && pos.y >= 0 && pos.y < 9)
                        {
                            gridCells[pos.x][pos.y].setSelected(true);
                        }
                    }
                }
            }
        }
    }
    
    // 如果没有球在移动，清除所有路径高亮
    if (!isBallMoving())
    {
        for (int x = 0; x < 9; ++x)
        {
            for (int y = 0; y < 9; ++y)
            {
                gridCells[x][y].setSelected(false);
            }
        }
    }
    
    // 更新计分弹窗动画
    for (auto& popup : m_scorePopups)
    {
        popup.progress += deltaTime / LE::SCORE_POPUP_DURATION;
    }
    // 移除已完成的弹窗
    m_scorePopups.erase(
        std::remove_if(m_scorePopups.begin(), m_scorePopups.end(),
            [](const ScorePopup& p) { return p.progress >= 1.0f; }),
        m_scorePopups.end());
    
    // 根据当前消除阶段处理
    switch (m_eliminationPhase)
    {
    case EliminationPhase::IDLE:
        return false;
        
    // ═══════════════════════════════════════════════════════════
    // 阶段1：线标记 - 线指示器出现，线球不变
    // ═══════════════════════════════════════════════════════════
    case EliminationPhase::LINE_MARKING:
    {
        // 更新线消指示器动画
        for (auto& indicator : m_lineIndicators)
        {
            if (!indicator.finished)
            {
                indicator.progress += deltaTime / LE::LINE_INDICATOR_DURATION;
                if (indicator.progress >= 1.0f)
                {
                    indicator.progress = 1.0f;
                    indicator.finished = true;
                }
            }
        }
        
        m_phaseTimer += deltaTime;
        if (m_phaseTimer >= LE::LINE_MARKING_DURATION)
        {
            // 进入线消退阶段
            startLineFading();
            m_eliminationPhase = EliminationPhase::LINE_FADING;
            m_phaseTimer = 0.0f;
        }
        return false;
    }
    
    // ═══════════════════════════════════════════════════════════
    // 阶段2：线消退 - 线球渐隐 + 线指示器渐隐
    // ═══════════════════════════════════════════════════════════
    case EliminationPhase::LINE_FADING:
    {
        // 更新所有正在消失的球
        updateFadingBalls(deltaTime);
        
        // 更新线指示器渐隐
        for (auto& indicator : m_lineIndicators)
        {
            indicator.fadeOutProgress += deltaTime / LE::LINE_FADING_DURATION;
            if (indicator.fadeOutProgress > 1.0f)
                indicator.fadeOutProgress = 1.0f;
        }
        
        m_phaseTimer += deltaTime;
        if (m_phaseTimer >= LE::LINE_FADING_DURATION)
        {
            // 检查是否有块消除
            if (!m_blobElimination.empty() || !m_blobIndicators.empty())
            {
                // 有块消除，进入块标记阶段
                m_eliminationPhase = EliminationPhase::BLOB_MARKING;
                m_phaseTimer = 0.0f;
            }
            else
            {
                // 无块消除，等待线球完全消失后进入清理
                if (checkAllBallsFaded())
                {
                    m_eliminationPhase = EliminationPhase::CLEANUP;
                }
            }
        }
        return false;
    }
    
    // ═══════════════════════════════════════════════════════════
    // 阶段3：块标记 - 块指示器出现，块球不变（线球继续消失）
    // ═══════════════════════════════════════════════════════════
    case EliminationPhase::BLOB_MARKING:
    {
        // 线球继续消失
        updateFadingBalls(deltaTime);
        
        // 更新连通块指示器动画
        for (auto& indicator : m_blobIndicators)
        {
            if (!indicator.finished)
            {
                indicator.progress += deltaTime / LE::BLOB_INDICATOR_DURATION;
                if (indicator.progress >= 1.0f)
                {
                    indicator.progress = 1.0f;
                    indicator.finished = true;
                }
            }
        }
        
        m_phaseTimer += deltaTime;
        if (m_phaseTimer >= LE::BLOB_MARKING_DURATION)
        {
            // 进入块消退阶段
            startBlobFading();
            m_eliminationPhase = EliminationPhase::BLOB_FADING;
            m_phaseTimer = 0.0f;
        }
        return false;
    }
    
    // ═══════════════════════════════════════════════════════════
    // 阶段4：块消退 - 块球渐隐 + 块指示器渐隐
    // ═══════════════════════════════════════════════════════════
    case EliminationPhase::BLOB_FADING:
    {
        // 更新所有正在消失的球
        updateFadingBalls(deltaTime);
        
        // 更新块指示器渐隐
        for (auto& indicator : m_blobIndicators)
        {
            indicator.fadeOutProgress += deltaTime / LE::BLOB_FADING_DURATION;
            if (indicator.fadeOutProgress > 1.0f)
                indicator.fadeOutProgress = 1.0f;
        }
        
        m_phaseTimer += deltaTime;
        if (checkAllBallsFaded() || m_phaseTimer >= LE::BLOB_FADING_DURATION)
        {
            m_eliminationPhase = EliminationPhase::CLEANUP;
        }
        return false;
    }
    
    // ═══════════════════════════════════════════════════════════
    // 阶段5：清理 - 删除所有球，触发连锁检测
    // ═══════════════════════════════════════════════════════════
    case EliminationPhase::CLEANUP:
    {
        int totalRemoved = static_cast<int>(m_lineElimination.size() + m_blobElimination.size());
        std::cout << "[Eliminate] Removing " << totalRemoved << " balls ("
                  << m_lineElimination.size() << " line, "
                  << m_blobElimination.size() << " blob)." << std::endl;
        
        // 删除线球
        for (const auto& pos : m_lineElimination)
        {
            balls[pos.x][pos.y] = nullptr;
        }
        // 删除块球
        for (const auto& pos : m_blobElimination)
        {
            balls[pos.x][pos.y] = nullptr;
        }
        
        m_lineElimination.clear();
        m_blobElimination.clear();
        m_lineIndicators.clear();
        m_blobIndicators.clear();
        m_eliminationPhase = EliminationPhase::IDLE;
        m_phaseTimer = 0.0f;
        return true;  // 触发连锁检测
    }
    }
    
    return false;
}

// 开始线球消退动画 + 激活线消分数弹窗
void Map::startLineFading()
{
    // 为彩虹球确定锁定颜色：查找同一消除组中非彩虹球的颜色
    // 遍历每条检测到的直线
    for (const auto& line : m_detectedLines)
    {
        // 找到这条线的主色（第一个非彩虹球的颜色）
        LE::BallColor dominantColor = LE::BallColor::Rainbow;
        for (const auto& pos : line)
        {
            if (balls[pos.x][pos.y] && !balls[pos.x][pos.y]->isRainbow())
            {
                dominantColor = balls[pos.x][pos.y]->getBallColor();
                break;
            }
        }
        
        // 为这条线上的所有彩虹球锁定主色
        if (dominantColor != LE::BallColor::Rainbow)
        {
            for (const auto& pos : line)
            {
                if (balls[pos.x][pos.y] && balls[pos.x][pos.y]->isRainbow())
                {
                    balls[pos.x][pos.y]->lockEliminateColor(dominantColor);
                }
            }
        }
    }
    
    // 开始消除动画
    for (const auto& pos : m_lineElimination)
    {
        if (balls[pos.x][pos.y])
        {
            balls[pos.x][pos.y]->startEliminate();
        }
    }
    
    // 激活待创建的线消分数弹窗
    for (const auto& popup : m_pendingLinePopups)
    {
        m_scorePopups.push_back(popup);
    }
    m_pendingLinePopups.clear();
}

// 开始块球消退动画 + 创建块球分数弹窗 (+2 each)
void Map::startBlobFading()
{
    // 辅助函数：网格坐标 -> 像素坐标 (球心位置)
    auto gridToPixel = [](const sf::Vector2i& gridPos) -> sf::Vector2f {
        float x = static_cast<float>(LE::FIRST_GRID_CENTER + gridPos.x * LE::GRID_LENGTH);
        float y = static_cast<float>(LE::FIRST_GRID_CENTER + gridPos.y * LE::GRID_LENGTH);
        return {x, y};
    };
    
    for (const auto& pos : m_blobElimination)
    {
        if (balls[pos.x][pos.y])
        {
            balls[pos.x][pos.y]->startEliminate();
            
            // 为每个块球创建 +2 弹窗
            ScorePopup popup;
            popup.position = gridToPixel(pos);
            popup.score = 2;
            popup.progress = 0.0f;
            
            // 获取球的颜色
            LE::BallColor ballColor = balls[pos.x][pos.y]->getBallColor();
            if (ballColor != LE::BallColor::Chameleon)
            {
                int colorIdx = static_cast<int>(ballColor) * 2 + 1;
                popup.color = LE::COLORS[colorIdx];
            }
            else
            {
                popup.color = LE::Chameleon::OUTLINE_NORMAL;
            }
            
            m_scorePopups.push_back(popup);
        }
    }
}

// 更新所有正在消失的球
void Map::updateFadingBalls(float deltaTime)
{
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            if (balls[x][y])
            {
                balls[x][y]->updateEliminate(deltaTime);
            }
        }
    }
}

// 检查所有待消除球是否已完全消失
bool Map::checkAllBallsFaded() const
{
    for (const auto& pos : m_lineElimination)
    {
        if (balls[pos.x][pos.y] && balls[pos.x][pos.y]->getEliminateAlpha() > 5.f)
        {
            return false;
        }
    }
    for (const auto& pos : m_blobElimination)
    {
        if (balls[pos.x][pos.y] && balls[pos.x][pos.y]->getEliminateAlpha() > 5.f)
        {
            return false;
        }
    }
    return true;
}


// 消除检测 (返回详细结果用于计分)
// 逻辑：1.先检测直线 2.线球标记为"已消费" 3.检测连通块(排除线球) 4.计算散球
EliminationResult Map::eliminate()
{
    EliminationResult result;
    
    // 只有在空闲状态才能开始新的消除
    if (m_eliminationPhase != EliminationPhase::IDLE) return result;

    // 分离的待消除列表
    std::vector<sf::Vector2i> lineToEliminate;  // 线球
    std::vector<sf::Vector2i> blobToEliminate;  // 块球 (纯块 + 散球)
    
    std::vector<std::vector<sf::Vector2i>> detectedLines;
    std::vector<std::vector<sf::Vector2i>> detectedBlobs;  // 连通块区域（用于轮廓动画）
    
    // 标记哪些球属于线消（用于后续排除）
    bool isLineBall[9][9] = {false};

    // ═══════════════════════════════════════════════════════════
    // 阶段1：直线检测（5+同色球成线，8邻域）
    // ═══════════════════════════════════════════════════════════
    
    auto checkDirection = [&](int x, int y, int dx, int dy) {
        if (balls[x][y] == nullptr) return;
        
        LE::BallColor startColor = balls[x][y]->getBallColor();
        // 变色球（Chameleon）跳过（它使用拟态颜色，由getBallColor返回）
        // 但如果是纯 Chameleon 类型（未拟态），跳过
        if (startColor == LE::BallColor::Chameleon) return;
        
        // 彩虹球作为起点时，需要往后看确定主色
        bool startIsRainbow = balls[x][y]->isRainbow();
        LE::BallColor dominantColor = startColor;
        
        // ═══════════════════════════════════════════════════════════
        // 关键修复：检查反方向是否有兼容的球
        // 如果反方向有兼容球，说明当前位置不是线的起点，跳过
        // ═══════════════════════════════════════════════════════════
        int prevX = x - dx;
        int prevY = y - dy;
        if (prevX >= 0 && prevX < 9 && prevY >= 0 && prevY < 9 &&
            balls[prevX][prevY] != nullptr)
        {
            LE::BallColor prevColor = balls[prevX][prevY]->getBallColor();
            bool prevIsRainbow = balls[prevX][prevY]->isRainbow();
            
            // 兼容性检查：彩虹球与任意颜色兼容，或颜色相同
            bool compatible = prevIsRainbow || startIsRainbow || (prevColor == dominantColor);
            if (compatible && prevColor != LE::BallColor::Chameleon)
            {
                // 反方向有兼容球，当前不是线的起点，跳过
                return;
            }
        }

        std::vector<sf::Vector2i> line;
        line.push_back({x, y});

        int nx = x + dx;
        int ny = y + dy;

        while (nx >= 0 && nx < 9 && ny >= 0 && ny < 9 && 
               balls[nx][ny] != nullptr)
        {
            LE::BallColor nextColor = balls[nx][ny]->getBallColor();
            if (nextColor == LE::BallColor::Chameleon) break;  // 未拟态的变色球中断
            
            bool nextIsRainbow = balls[nx][ny]->isRainbow();
            
            // 彩虹球万能匹配
            if (nextIsRainbow)
            {
                line.push_back({nx, ny});
                nx += dx;
                ny += dy;
                continue;
            }
            
            // 确定主色（第一个非彩虹球的颜色）
            if (dominantColor == LE::BallColor::Rainbow)
            {
                dominantColor = nextColor;
            }
            
            // 检查颜色匹配：必须与主色相同
            // 注意：即使起点是彩虹球，一旦主色确定，后续球必须匹配主色
            if (nextColor == dominantColor)
            {
                line.push_back({nx, ny});
                nx += dx;
                ny += dy;
            }
            else
            {
                break;
            }
        }
        
        // 如果整条线都是彩虹球，也算有效消除
        if (line.size() >= 5)
        {
            detectedLines.push_back(line);
            for (const auto& pos : line)
            {
                isLineBall[pos.x][pos.y] = true;
                lineToEliminate.push_back(pos);
            }
        }
    };

    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            if (balls[x][y] == nullptr) continue;
            checkDirection(x, y, 1, 0);
            checkDirection(x, y, 0, 1);
            checkDirection(x, y, 1, 1);
            checkDirection(x, y, -1, 1);
        }
    }

    // ═══════════════════════════════════════════════════════════
    // 阶段2：连通块检测（4邻域）
    // ═══════════════════════════════════════════════════════════
    
    bool visited[9][9] = {false};
    
    auto findConnectedRegion = [&](int startX, int startY) -> std::vector<sf::Vector2i> {
        std::vector<sf::Vector2i> region;
        std::queue<sf::Vector2i> q;
        
        LE::BallColor startColor = balls[startX][startY]->getBallColor();
        bool startIsRainbow = balls[startX][startY]->isRainbow();
        
        // 纯未拟态的变色球跳过
        if (startColor == LE::BallColor::Chameleon) {
            visited[startX][startY] = true;
            return region;
        }
        
        // 确定主色（彩虹球需要往后看）
        LE::BallColor dominantColor = startIsRainbow ? LE::BallColor::Rainbow : startColor;
        
        q.push({startX, startY});
        visited[startX][startY] = true;
        
        const int dx[] = {0, 0, -1, 1};
        const int dy[] = {-1, 1, 0, 0};
        
        while (!q.empty()) {
            sf::Vector2i curr = q.front();
            q.pop();
            region.push_back(curr);
            
            // 如果当前球不是彩虹球，用它确定主色
            if (dominantColor == LE::BallColor::Rainbow && balls[curr.x][curr.y] && !balls[curr.x][curr.y]->isRainbow())
            {
                dominantColor = balls[curr.x][curr.y]->getBallColor();
            }
            
            for (int i = 0; i < 4; ++i) {
                int nx = curr.x + dx[i];
                int ny = curr.y + dy[i];
                
                if (nx >= 0 && nx < 9 && ny >= 0 && ny < 9 &&
                    !visited[nx][ny] && 
                    balls[nx][ny] != nullptr)
                {
                    LE::BallColor nextColor = balls[nx][ny]->getBallColor();
                    if (nextColor == LE::BallColor::Chameleon) continue;  // 跳过未拟态变色球
                    
                    bool nextIsRainbow = balls[nx][ny]->isRainbow();
                    bool currIsRainbow = balls[curr.x][curr.y]->isRainbow();
                    
                    // 兼容性检查：
                    // 1. 当前球或下一个球是彩虹球 -> 总是兼容
                    // 2. 主色还未确定(Rainbow) -> 总是兼容(会在后续确定主色)
                    // 3. 主色已确定 -> 必须与主色相同
                    bool compatible = false;
                    if (nextIsRainbow || currIsRainbow)
                    {
                        // 彩虹球与任何颜色兼容
                        compatible = true;
                    }
                    else if (dominantColor == LE::BallColor::Rainbow)
                    {
                        // 主色未确定，这个非彩虹球将成为主色的候选
                        compatible = true;
                    }
                    else
                    {
                        // 主色已确定，必须严格匹配
                        compatible = (nextColor == dominantColor);
                    }
                    
                    if (compatible)
                    {
                        visited[nx][ny] = true;
                        q.push({nx, ny});
                    }
                }
            }
        }
        return region;
    };
    
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            if (!visited[x][y] && balls[x][y] != nullptr)
            {
                auto region = findConnectedRegion(x, y);
                
                // 检查该连通区域是否包含线球
                bool regionHasLineBall = false;
                std::vector<sf::Vector2i> nonLineBalls;
                
                for (const auto& pos : region)
                {
                    if (isLineBall[pos.x][pos.y])
                        regionHasLineBall = true;
                    else
                        nonLineBalls.push_back(pos);
                }
                
                if (regionHasLineBall)
                {
                    // 该区域包含线球：只有当整个区域>=7时，非线球部分才作为散球消除
                    if (!nonLineBalls.empty() && region.size() >= 7)
                    {
                        result.leftoverBalls += static_cast<int>(nonLineBalls.size());
                        detectedBlobs.push_back(region);  // 保存整个区域用于轮廓动画（连通块判定成功）
                        for (const auto& pos : nonLineBalls)
                            blobToEliminate.push_back(pos);
                    }
                }
                else
                {
                    // 该区域不包含线球：按正常连通块规则判定（需满足7阈值）
                    if (region.size() >= 7)
                    {
                        result.blobSizes.push_back(static_cast<int>(region.size()));
                        detectedBlobs.push_back(region);  // 保存区域用于轮廓动画
                        for (const auto& pos : region)
                            blobToEliminate.push_back(pos);
                    }
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════
    // 阶段3：构建结果 + 去重 + 启动消除动画
    // ═══════════════════════════════════════════════════════════
    
    bool hasElimination = !lineToEliminate.empty() || !blobToEliminate.empty();
    
    if (hasElimination)
    {
        result.hasElimination = true;
        
        // 记录每条线的长度
        for (const auto& line : detectedLines)
            result.lineLengths.push_back(static_cast<int>(line.size()));
        
        // 去重 lineToEliminate
        auto sortByPos = [](const sf::Vector2i& a, const sf::Vector2i& b) {
            if (a.x != b.x) return a.x < b.x;
            return a.y < b.y;
        };
        auto equalPos = [](const sf::Vector2i& a, const sf::Vector2i& b) {
            return a.x == b.x && a.y == b.y;
        };
        
        std::sort(lineToEliminate.begin(), lineToEliminate.end(), sortByPos);
        lineToEliminate.erase(std::unique(lineToEliminate.begin(), lineToEliminate.end(), equalPos), lineToEliminate.end());
        
        // 去重 blobToEliminate
        std::sort(blobToEliminate.begin(), blobToEliminate.end(), sortByPos);
        blobToEliminate.erase(std::unique(blobToEliminate.begin(), blobToEliminate.end(), equalPos), blobToEliminate.end());
        
        result.totalBalls = static_cast<int>(lineToEliminate.size() + blobToEliminate.size());

        // ═══════════════════════════════════════════════════════════
        // 创建线消指示器 + 分数弹窗数据
        // ═══════════════════════════════════════════════════════════
        m_lineIndicators.clear();
        m_pendingLinePopups.clear();
        
        // 辅助函数：网格坐标 -> 像素坐标 (球心位置)
        auto gridToPixel = [](const sf::Vector2i& gridPos) -> sf::Vector2f {
            float x = static_cast<float>(LE::FIRST_GRID_CENTER + gridPos.x * LE::GRID_LENGTH);
            float y = static_cast<float>(LE::FIRST_GRID_CENTER + gridPos.y * LE::GRID_LENGTH);
            return {x, y};
        };
        
        for (const auto& line : detectedLines)
        {
            if (line.size() >= 2)
            {
                LineIndicator indicator;
                indicator.startPos = line.front();
                indicator.endPos = line.back();
                indicator.progress = 0.0f;
                indicator.finished = false;
                
                sf::Color lineColor = sf::Color::White;
                LE::Ball* ball = balls[line.front().x][line.front().y].get();
                if (ball)
                {
                    LE::BallColor ballColor = ball->getBallColor();
                    if (ballColor != LE::BallColor::Chameleon)
                    {
                        int colorIdx = static_cast<int>(ballColor) * 2 + 1;
                        lineColor = LE::COLORS[colorIdx];
                    }
                    else
                    {
                        lineColor = LE::Chameleon::OUTLINE_NORMAL;
                    }
                }
                indicator.color = lineColor;
                m_lineIndicators.push_back(indicator);
                
                // 创建线消分数弹窗数据
                ScorePopup popup;
                sf::Vector2f startPixel = gridToPixel(line.front());
                sf::Vector2f endPixel = gridToPixel(line.back());
                popup.position = (startPixel + endPixel) * 0.5f;  // 线中心
                popup.score = LE::Scoring::getLineScore(static_cast<int>(line.size()));
                popup.color = lineColor;
                popup.progress = 0.0f;
                m_pendingLinePopups.push_back(popup);
            }
        }

        // ═══════════════════════════════════════════════════════════
        // 创建连通块指示器 (为每个连通块创建一个轮廓指示器)
        // ═══════════════════════════════════════════════════════════
        m_blobIndicators.clear();
        for (const auto& blob : detectedBlobs)
        {
            if (!blob.empty())
            {
                BlobIndicator indicator;
                indicator.cells = blob;
                indicator.progress = 0.0f;
                indicator.finished = false;
                
                LE::Ball* ball = balls[blob.front().x][blob.front().y].get();
                if (ball)
                {
                    LE::BallColor ballColor = ball->getBallColor();
                    if (ballColor != LE::BallColor::Chameleon)
                    {
                        int colorIdx = static_cast<int>(ballColor) * 2 + 1;
                        indicator.color = LE::COLORS[colorIdx];
                    }
                    else
                    {
                        indicator.color = LE::Chameleon::OUTLINE_NORMAL;
                    }
                }
                else
                {
                    indicator.color = sf::Color::White;
                }
                
                m_blobIndicators.push_back(indicator);
            }
        }

        // 设置分离的待消除列表
        m_lineElimination = lineToEliminate;
        m_blobElimination = blobToEliminate;
        m_detectedLines = detectedLines;  // 保存检测到的直线（用于彩虹球颜色锁定）
        m_phaseTimer = 0.0f;
        
        // 决定初始阶段：有线消则从 LINE_MARKING 开始，否则从 BLOB_MARKING 开始
        if (!m_lineElimination.empty())
        {
            m_eliminationPhase = EliminationPhase::LINE_MARKING;
        }
        else
        {
            m_eliminationPhase = EliminationPhase::BLOB_MARKING;
        }
        
        std::cout << "[Eliminate] " << result.lineLengths.size() << " lines (" 
                  << m_lineElimination.size() << " balls), "
                  << result.blobSizes.size() << " blobs (" 
                  << m_blobElimination.size() << " balls), "
                  << result.leftoverBalls << " leftover, "
                  << result.totalBalls << " total." << std::endl;
    }

    return result;
}

const std::vector<LE::BallColor>& Map::getNextColors() const
{
    return m_nextColors;
}

LE::BallColor Map::generateRandomColor()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 100);

    // 5% 概率生成变色球
    if (dis(gen) < 5)
    {
        return LE::BallColor::Chameleon;
    }
    else
    {
        // 0-6 代表7种普通颜色
        static std::uniform_int_distribution<> colorDis(0, 6);
        return static_cast<LE::BallColor>(colorDis(gen));
    }
}

// ═══════════════════════════════════════════════════════════
// 检查棋盘是否已满
// ═══════════════════════════════════════════════════════════
bool Map::isBoardFull() const
{
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            if (balls[x][y] == nullptr)
            {
                return false;
            }
        }
    }
    return true;
}

// ═══════════════════════════════════════════════════════════
// 清除所有球（用于重开游戏）
// ═══════════════════════════════════════════════════════════
void Map::clearAllBalls()
{
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            balls[x][y] = nullptr;
        }
    }
    
    // 清除所有动画状态
    m_eliminationPhase = EliminationPhase::IDLE;
    m_phaseTimer = 0.0f;
    m_lineElimination.clear();
    m_blobElimination.clear();
    m_lineIndicators.clear();
    m_blobIndicators.clear();
    m_scorePopups.clear();
    m_pendingLinePopups.clear();
    
    // 重新生成预览球颜色
    m_nextColors.clear();
    for (int i = 0; i < LE::PREVIEW_BALL_COUNT; ++i)
    {
        m_nextColors.push_back(generateRandomColor());
    }
}

// isBallMoving 和 startBallMove 已移至 MapPathfinding.cpp

// ═══════════════════════════════════════════════════════════
// 绘制线消指示器 (Line Indicator Drawing)
// ═══════════════════════════════════════════════════════════
void Map::drawLineIndicators(sf::RenderTarget &target) const
{
    if (m_lineIndicators.empty()) return;
    
    // 辅助函数：网格坐标 -> 像素坐标 (球心位置)
    auto gridToPixel = [](const sf::Vector2i& gridPos) -> sf::Vector2f {
        float x = static_cast<float>(LE::FIRST_GRID_CENTER + gridPos.x * LE::GRID_LENGTH);
        float y = static_cast<float>(LE::FIRST_GRID_CENTER + gridPos.y * LE::GRID_LENGTH);
        return {x, y};
    };
    
    // 缓动函数：easeOutQuad (快速启动，缓慢结束)
    auto easeOutQuad = [](float t) -> float {
        return 1.0f - (1.0f - t) * (1.0f - t);
    };
    
    for (const auto& indicator : m_lineIndicators)
    {
        // 计算起点和终点的像素坐标
        sf::Vector2f startPixel = gridToPixel(indicator.startPos);
        sf::Vector2f endPixel = gridToPixel(indicator.endPos);
        
        // 计算中点
        sf::Vector2f midPoint = (startPixel + endPixel) * 0.5f;
        
        // 应用缓动后的进度
        float easedProgress = easeOutQuad(indicator.progress);
        
        // 从中点向两端生长
        sf::Vector2f currentStart = midPoint + (startPixel - midPoint) * easedProgress;
        sf::Vector2f currentEnd = midPoint + (endPixel - midPoint) * easedProgress;
        
        // 计算线的方向向量和垂直向量（用于粗线绘制）
        sf::Vector2f direction = currentEnd - currentStart;
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        
        if (length < 0.01f) continue; // 避免除以零
        
        // 归一化方向向量
        sf::Vector2f normalizedDir = direction / length;
        
        // 垂直向量 (用于线条宽度)
        sf::Vector2f perpendicular(-normalizedDir.y, normalizedDir.x);
        float halfThickness = LE::LINE_INDICATOR_THICKNESS * 0.5f;
        
        // 使用四边形绘制粗线条
        sf::VertexArray line(sf::PrimitiveType::TriangleStrip, 4);
        
        // 计算透明度：渐入 * (1 - 渐隐)
        float fadeOutEased = easeOutQuad(indicator.fadeOutProgress);
        float alpha = LE::LINE_INDICATOR_ALPHA * easedProgress * (1.0f - fadeOutEased);
        sf::Color lineColor = indicator.color;
        lineColor.a = static_cast<uint8_t>(alpha);
        
        // 四个顶点
        line[0].position = currentStart - perpendicular * halfThickness;
        line[0].color = lineColor;
        
        line[1].position = currentStart + perpendicular * halfThickness;
        line[1].color = lineColor;
        
        line[2].position = currentEnd - perpendicular * halfThickness;
        line[2].color = lineColor;
        
        line[3].position = currentEnd + perpendicular * halfThickness;
        line[3].color = lineColor;
        
        target.draw(line);
        
        // 绘制圆形端点（让线条更平滑）
        float endRadius = halfThickness;
        sf::CircleShape startCap(endRadius);
        startCap.setOrigin({endRadius, endRadius});
        startCap.setPosition(currentStart);
        startCap.setFillColor(lineColor);
        target.draw(startCap);
        
        sf::CircleShape endCap(endRadius);
        endCap.setOrigin({endRadius, endRadius});
        endCap.setPosition(currentEnd);
        endCap.setFillColor(lineColor);
        target.draw(endCap);
    }
}

// ═══════════════════════════════════════════════════════════
// 绘制连通块轮廓指示器 (Blob Outline Drawing)
// ═══════════════════════════════════════════════════════════
void Map::drawBlobIndicators(sf::RenderTarget &target) const
{
    if (m_blobIndicators.empty()) return;
    
    // 辅助函数：网格坐标 -> 格子左上角像素坐标
    auto gridToTopLeft = [](const sf::Vector2i& gridPos) -> sf::Vector2f {
        float x = static_cast<float>(LE::WINDOW_BORDER + gridPos.x * LE::GRID_LENGTH);
        float y = static_cast<float>(LE::WINDOW_BORDER + gridPos.y * LE::GRID_LENGTH);
        return {x, y};
    };
    
    // 缓动函数：easeOutQuad
    auto easeOutQuad = [](float t) -> float {
        return 1.0f - (1.0f - t) * (1.0f - t);
    };
    
    const float cellSize = static_cast<float>(LE::GRID_LENGTH);
    const float thickness = LE::BLOB_INDICATOR_THICKNESS;
    const float inset = LE::BLOB_INDICATOR_INSET;  // 内边距，避免与深色边框重叠
    const float innerSize = cellSize - 2 * inset;  // 内部可用尺寸
    
    for (const auto& indicator : m_blobIndicators)
    {
        if (indicator.cells.empty()) continue;
        
        // 应用缓动后的进度
        float easedProgress = easeOutQuad(indicator.progress);
        
        // 计算透明度：渐入 * (1 - 渐隐)
        float fadeOutEased = easeOutQuad(indicator.fadeOutProgress);
        float alpha = LE::BLOB_INDICATOR_ALPHA * easedProgress * (1.0f - fadeOutEased);
        sf::Color outlineColor = indicator.color;
        outlineColor.a = static_cast<uint8_t>(alpha);
        
        // 创建一个集合用于快速查找某个格子是否在块内
        std::set<std::pair<int, int>> cellSet;
        for (const auto& cell : indicator.cells)
        {
            cellSet.insert({cell.x, cell.y});
        }
        
        // 遍历每个格子，绘制外边缘（在边缘内侧）
        for (const auto& cell : indicator.cells)
        {
            sf::Vector2f topLeft = gridToTopLeft(cell);
            
            // 检查4个方向的邻居，如果邻居不在块内，则绘制该边
            // 上边 (在格子顶部内侧)
            if (cellSet.find({cell.x, cell.y - 1}) == cellSet.end())
            {
                sf::RectangleShape edge({innerSize, thickness});
                edge.setPosition({topLeft.x + inset, topLeft.y + inset});
                edge.setFillColor(outlineColor);
                target.draw(edge);
            }
            
            // 下边 (在格子底部内侧)
            if (cellSet.find({cell.x, cell.y + 1}) == cellSet.end())
            {
                sf::RectangleShape edge({innerSize, thickness});
                edge.setPosition({topLeft.x + inset, topLeft.y + cellSize - inset - thickness});
                edge.setFillColor(outlineColor);
                target.draw(edge);
            }
            
            // 左边 (在格子左侧内侧)
            if (cellSet.find({cell.x - 1, cell.y}) == cellSet.end())
            {
                sf::RectangleShape edge({thickness, innerSize});
                edge.setPosition({topLeft.x + inset, topLeft.y + inset});
                edge.setFillColor(outlineColor);
                target.draw(edge);
            }
            
            // 右边 (在格子右侧内侧)
            if (cellSet.find({cell.x + 1, cell.y}) == cellSet.end())
            {
                sf::RectangleShape edge({thickness, innerSize});
                edge.setPosition({topLeft.x + cellSize - inset - thickness, topLeft.y + inset});
                edge.setFillColor(outlineColor);
                target.draw(edge);
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════
// 绘制计分弹窗 (Score Popup Drawing)
// ═══════════════════════════════════════════════════════════
void Map::drawScorePopups(sf::RenderTarget &target, const sf::Font &font) const
{
    if (m_scorePopups.empty()) return;
    
    for (const auto& popup : m_scorePopups)
    {
        // 缓动函数
        auto easeOutQuad = [](float t) -> float {
            return 1.0f - (1.0f - t) * (1.0f - t);
        };
        
        float easedProgress = easeOutQuad(popup.progress);
        
        // 计算上浮偏移
        float yOffset = LE::SCORE_POPUP_RISE * easedProgress;
        
        // 计算透明度：先快速出现，后渐隐
        // 0-0.3: 渐入, 0.3-1.0: 渐隐
        float alpha;
        if (popup.progress < 0.3f)
        {
            alpha = popup.progress / 0.3f;
        }
        else
        {
            alpha = 1.0f - (popup.progress - 0.3f) / 0.7f;
        }
        alpha = std::max(0.0f, std::min(1.0f, alpha));
        
        // 创建文本
        sf::Text text(font);
        text.setString("+" + std::to_string(popup.score));
        text.setCharacterSize(LE::SCORE_POPUP_FONT_SIZE);
        
        // 使用统一的金色，提高可识别度
        sf::Color textColor(LE::SCORE_POPUP_COLOR_R, 
                           LE::SCORE_POPUP_COLOR_G, 
                           LE::SCORE_POPUP_COLOR_B,
                           static_cast<uint8_t>(255 * alpha));
        text.setFillColor(textColor);
        
        // 添加深色描边，使用背景色提高对比度
        sf::Color outlineColor = LE::COLORS[16];  // Deep Mocha (48, 44, 40)
        outlineColor.a = static_cast<uint8_t>(255 * alpha);
        text.setOutlineColor(outlineColor);
        text.setOutlineThickness(2.0f);
        
        // 居中显示
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({bounds.position.x + bounds.size.x / 2.f, 
                        bounds.position.y + bounds.size.y / 2.f});
        text.setPosition({popup.position.x, popup.position.y - yOffset});
        
        target.draw(text);
    }
}

// ═══════════════════════════════════════════════════════════
// 存读档支持
// ═══════════════════════════════════════════════════════════

Map::BoardState Map::getBoardState() const
{
    BoardState state;
    
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            auto& cell = state[x][y];
            const auto& ball = balls[x][y];
            
            if (ball)
            {
                cell.hasBall = true;
                cell.color = ball->getRealColor();
                cell.isChameleon = ball->isChameleon();
                if (cell.isChameleon)
                {
                    cell.mimicColor = ball->getBallColor();
                }
            }
            else
            {
                cell.hasBall = false;
            }
        }
    }
    
    return state;
}

void Map::setBoardState(const BoardState& state)
{
    // 清除当前所有球
    clearAllBalls();
    
    // 根据状态创建球
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            const auto& cell = state[x][y];
            
            if (cell.hasBall)
            {
                sf::Vector2i pos{x, y};
                createBall(pos, cell.color);
                
                // 如果是变色球，设置拟态颜色
                if (cell.isChameleon && balls[x][y])
                {
                    balls[x][y]->setMimicColor(cell.mimicColor);
                }
            }
        }
    }
    
    // 更新变色球
    updateChameleonBalls();
}

void Map::setNextColors(const std::vector<LE::BallColor>& colors)
{
    m_nextColors = colors;
    
    // 确保有足够的预览球
    while (m_nextColors.size() < static_cast<size_t>(LE::PREVIEW_BALL_COUNT))
    {
        m_nextColors.push_back(generateRandomColor());
    }
}

// ═══════════════════════════════════════════════════════════
// 设置悬停格子（拖拽放置时使用）
// ═══════════════════════════════════════════════════════════
void Map::setHoveredCell(sf::Vector2i gridPos, bool isValid)
{
    // 先清除所有格子的选中状态
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            gridCells[x][y].setSelected(false);
        }
    }
    
    // 如果有效位置，激活目标格子
    if (gridPos.x >= 0 && gridPos.x < 9 && gridPos.y >= 0 && gridPos.y < 9)
    {
        gridCells[gridPos.x][gridPos.y].setSelected(true);
    }
}
