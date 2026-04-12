// ═══════════════════════════════════════════════════════════
// MapPathfinding.cpp - 寻路算法实现
// ═══════════════════════════════════════════════════════════
#include "board/Map.hpp"
#include "Utility.hpp"
#include <queue>
#include <algorithm>

// ═══════════════════════════════════════════════════════════
// BFS寻路算法实现
// ═══════════════════════════════════════════════════════════
bool Map::pathSeeking(sf::Vector2i start, sf::Vector2i end) const
{
    // 1. 基础检查
    if (!LE::isInGrids(start) || !LE::isInGrids(end))
        return false;
    
    if (start == end)
        return true; // 起点和终点相同

    // 如果终点有球，则无法移动到那里
    if (balls[end.x][end.y] != nullptr)
        return false;

    // 2. BFS 初始化
    std::queue<sf::Vector2i> q;
    q.push(start);

    // 访问标记数组 (9x9)
    bool visited[9][9] = {false};
    visited[start.x][start.y] = true;

    // 方向数组：上、下、左、右
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};

    // 3. 开始BFS搜索
    while (!q.empty())
    {
        sf::Vector2i current = q.front();
        q.pop();

        // 到达终点？
        if (current == end)
            return true;

        // 探索 4 个方向
        for (int i = 0; i < 4; ++i)
        {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];

            // 检查边界
            if (nx >= 0 && nx < 9 && ny >= 0 && ny < 9)
            {
                // 如果未访问过
                if (!visited[nx][ny])
                {
                    // 检查是否有障碍物（球）
                    bool isObstacle = (balls[nx][ny] != nullptr);
                    
                    // 如果不是障碍物，则加入队列
                    if (!isObstacle)
                    {
                        visited[nx][ny] = true;
                        q.push({nx, ny});
                    }
                }
            }
        }
    }

    // 队列空了还没找到终点，说明无路可走
    return false;
}

// ═══════════════════════════════════════════════════════════
// 移动球（包含寻路检查）
// ═══════════════════════════════════════════════════════════
bool Map::moveBall(sf::Vector2i start, sf::Vector2i end)
{
    // 1. 基础检查
    if (!LE::isInGrids(start) || !LE::isInGrids(end))
        return false;
        
    if (balls[start.x][start.y] == nullptr)
        return false; // 起点没有球
        
    if (balls[end.x][end.y] != nullptr)
        return false; // 终点已有球
        
    // 2. 寻路检查
    if (!pathSeeking(start, end))
        return false; // 没有可行路径
        
    // 3. 移动球
    // 转移球的所有权
    balls[end.x][end.y] = std::move(balls[start.x][start.y]);
    
    // 更新球的内部位置
    balls[end.x][end.y]->setGridPosition(end);
    
    // 移动后更新变色球状态
    updateChameleonBalls();
    
    return true;
}

// ═══════════════════════════════════════════════════════════
// 寻找完整路径（用于动画）
// ═══════════════════════════════════════════════════════════
std::vector<sf::Vector2i> Map::findPath(sf::Vector2i start, sf::Vector2i end) const
{
    std::vector<sf::Vector2i> path;
    
    // 1. 基础检查
    if (!LE::isInGrids(start) || !LE::isInGrids(end))
        return path; // 返回空路径
    
    if (start == end)
    {
        path.push_back(start);
        return path;
    }

    // 如果终点有球，则无法移动到那里
    if (balls[end.x][end.y] != nullptr)
        return path;

    // 2. BFS 初始化
    std::queue<sf::Vector2i> q;
    q.push(start);

    // 访问标记数组和父节点记录
    bool visited[9][9] = {false};
    sf::Vector2i parent[9][9];
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            parent[i][j] = {-1, -1};
    
    visited[start.x][start.y] = true;

    // 方向数组：上、下、左、右
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};

    bool found = false;

    // 3. 开始BFS搜索
    while (!q.empty() && !found)
    {
        sf::Vector2i current = q.front();
        q.pop();

        // 到达终点？
        if (current == end)
        {
            found = true;
            break;
        }

        // 探索 4 个方向
        for (int i = 0; i < 4; ++i)
        {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];

            // 检查边界
            if (nx >= 0 && nx < 9 && ny >= 0 && ny < 9)
            {
                // 如果未访问过
                if (!visited[nx][ny])
                {
                    // 检查是否有障碍物（球）
                    bool isObstacle = (balls[nx][ny] != nullptr);
                    
                    // 如果不是障碍物，则加入队列
                    if (!isObstacle)
                    {
                        visited[nx][ny] = true;
                        parent[nx][ny] = current;
                        q.push({nx, ny});
                    }
                }
            }
        }
    }

    // 4. 如果找到路径，重建路径
    if (found)
    {
        sf::Vector2i current = end;
        while (current != start)
        {
            path.push_back(current);
            current = parent[current.x][current.y];
        }
        path.push_back(start);
        
        // 反转路径（从起点到终点）
        std::reverse(path.begin(), path.end());
    }

    return path;
}

// ═══════════════════════════════════════════════════════════
// 启动球移动动画（非阻塞）
// ═══════════════════════════════════════════════════════════
void Map::startBallMove(sf::Vector2i from, sf::Vector2i to, const std::vector<sf::Vector2i>& path)
{
    if (balls[from.x][from.y] == nullptr) return;
    
    // 启动移动动画
    balls[from.x][from.y]->startMove(path);
    
    // 立即转移球的所有权到目标位置
    balls[to.x][to.y] = std::move(balls[from.x][from.y]);
}

// ═══════════════════════════════════════════════════════════
// 检查是否有球正在移动
// ═══════════════════════════════════════════════════════════
bool Map::isBallMoving() const
{
    for (int x = 0; x < 9; ++x)
    {
        for (int y = 0; y < 9; ++y)
        {
            if (balls[x][y] && balls[x][y]->isMoving())
                return true;
        }
    }
    return false;
}
