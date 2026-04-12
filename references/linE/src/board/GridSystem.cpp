// ═══════════════════════════════════════════════════════════
// GridSystem.cpp - 通用网格系统实现
// ═══════════════════════════════════════════════════════════
#include "board/GridSystem.hpp"
#include <cmath>

namespace LE
{
    // ═══════════════════════════════════════════════════════════
    // 初始化
    // ═══════════════════════════════════════════════════════════
    void GridSystem::init(const GridConfig& config)
    {
        m_config = config;
        createCells();
    }
    
    void GridSystem::createCells()
    {
        // 清空现有格子
        m_cells.clear();
        m_cells.resize(m_config.cols);
        
        float cellStep = m_config.cellSize + m_config.cellGap;
        
        for (int col = 0; col < m_config.cols; ++col)
        {
            m_cells[col].resize(m_config.rows);
            
            for (int row = 0; row < m_config.rows; ++row)
            {
                // 计算格子中心位置
                float centerX = m_config.offsetX + col * cellStep + m_config.cellSize / 2.f;
                float centerY = m_config.offsetY + row * cellStep + m_config.cellSize / 2.f;
                
                // 创建格子
                auto cell = std::make_unique<GridCell>();
                
                // 设置自定义尺寸、位置和颜色
                cell->setSize(m_config.cellSize);
                cell->setPosition({centerX, centerY});
                cell->setFillColor(m_config.fillColor);
                cell->setOutlineColor(m_config.outlineColor);
                
                m_cells[col][row] = std::move(cell);
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // 绘制
    // ═══════════════════════════════════════════════════════════
    void GridSystem::draw(sf::RenderTarget& target) const
    {
        // 绘制背景（如果有）
        if (m_hasBackground)
        {
            target.draw(m_background);
        }
        
        // 绘制所有格子
        for (int col = 0; col < m_config.cols; ++col)
        {
            for (int row = 0; row < m_config.rows; ++row)
            {
                if (m_cells[col][row])
                {
                    m_cells[col][row]->draw(target);
                }
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // 更新
    // ═══════════════════════════════════════════════════════════
    void GridSystem::update(float deltaTime)
    {
        // 基类默认不做任何事，子类可重写
        (void)deltaTime;
    }
    
    // ═══════════════════════════════════════════════════════════
    // 鼠标交互
    // ═══════════════════════════════════════════════════════════
    void GridSystem::handleMouseMove(sf::Vector2f mousePos)
    {
        sf::Vector2i gridPos = pixelToGrid(mousePos);
        
        if (isInGrid(gridPos))
        {
            updateHoverState(gridPos);
        }
        else
        {
            updateHoverState({-1, -1});
        }
    }
    
    bool GridSystem::handleClick(sf::Vector2f mousePos)
    {
        sf::Vector2i gridPos = pixelToGrid(mousePos);
        
        if (isInGrid(gridPos))
        {
            if (m_onCellClick)
            {
                CellClickEvent event;
                event.col = gridPos.x;
                event.row = gridPos.y;
                event.worldPos = gridToPixel(gridPos.x, gridPos.y);
                m_onCellClick(event);
            }
            return true;
        }
        return false;
    }
    
    void GridSystem::setOnCellClick(std::function<void(const CellClickEvent&)> callback)
    {
        m_onCellClick = callback;
    }
    
    void GridSystem::updateHoverState(sf::Vector2i newHover)
    {
        if (m_hoverCell == newHover) return;
        
        // 取消旧的悬停
        if (isInGrid(m_hoverCell) && m_cells[m_hoverCell.x][m_hoverCell.y])
        {
            m_cells[m_hoverCell.x][m_hoverCell.y]->setFillColor(m_config.fillColor);
        }
        
        // 设置新的悬停
        m_hoverCell = newHover;
        if (isInGrid(m_hoverCell) && m_cells[m_hoverCell.x][m_hoverCell.y])
        {
            m_cells[m_hoverCell.x][m_hoverCell.y]->setFillColor(m_config.hoverColor);
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // 坐标转换
    // ═══════════════════════════════════════════════════════════
    sf::Vector2i GridSystem::pixelToGrid(sf::Vector2f pixelPos) const
    {
        float cellStep = m_config.cellSize + m_config.cellGap;
        
        float relX = pixelPos.x - m_config.offsetX;
        float relY = pixelPos.y - m_config.offsetY;
        
        if (relX < 0 || relY < 0) return {-1, -1};
        
        int col = static_cast<int>(relX / cellStep);
        int row = static_cast<int>(relY / cellStep);
        
        // 检查是否在格子内（排除间距区域）
        float cellX = relX - col * cellStep;
        float cellY = relY - row * cellStep;
        
        if (cellX > m_config.cellSize || cellY > m_config.cellSize)
        {
            return {-1, -1};  // 点在间距区域
        }
        
        if (col >= m_config.cols || row >= m_config.rows)
        {
            return {-1, -1};
        }
        
        return {col, row};
    }
    
    sf::Vector2f GridSystem::gridToPixel(int col, int row) const
    {
        float cellStep = m_config.cellSize + m_config.cellGap;
        float centerX = m_config.offsetX + col * cellStep + m_config.cellSize / 2.f;
        float centerY = m_config.offsetY + row * cellStep + m_config.cellSize / 2.f;
        return {centerX, centerY};
    }
    
    // ═══════════════════════════════════════════════════════════
    // 边界检查
    // ═══════════════════════════════════════════════════════════
    bool GridSystem::isInGrid(int col, int row) const
    {
        return col >= 0 && col < m_config.cols && row >= 0 && row < m_config.rows;
    }
    
    bool GridSystem::isInGrid(sf::Vector2i gridPos) const
    {
        return isInGrid(gridPos.x, gridPos.y);
    }
    
    // ═══════════════════════════════════════════════════════════
    // 格子访问
    // ═══════════════════════════════════════════════════════════
    GridCell* GridSystem::getCell(int col, int row)
    {
        if (!isInGrid(col, row)) return nullptr;
        return m_cells[col][row].get();
    }
    
    const GridCell* GridSystem::getCell(int col, int row) const
    {
        if (!isInGrid(col, row)) return nullptr;
        return m_cells[col][row].get();
    }
    
    void GridSystem::setCellHighlight(int col, int row, bool highlight)
    {
        if (auto* cell = getCell(col, row))
        {
            cell->setSelected(highlight);
        }
    }
    
    void GridSystem::setCellColor(int col, int row, sf::Color fillColor, sf::Color outlineColor)
    {
        if (auto* cell = getCell(col, row))
        {
            cell->setFillColor(fillColor);
            cell->setOutlineColor(outlineColor);
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // 边界获取
    // ═══════════════════════════════════════════════════════════
    sf::FloatRect GridSystem::getBounds() const
    {
        float cellStep = m_config.cellSize + m_config.cellGap;
        float width = m_config.cols * cellStep - m_config.cellGap;
        float height = m_config.rows * cellStep - m_config.cellGap;
        
        return sf::FloatRect(
            {m_config.offsetX, m_config.offsetY},
            {width, height}
        );
    }
}
