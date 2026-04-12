#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <memory>
#include <functional>
#include "board/GridCell.hpp"

namespace LE
{
    // ═══════════════════════════════════════════════════════════
    // GridSystem - 通用网格系统抽象基类
    // 提供可配置的网格布局、渲染和交互能力
    // ═══════════════════════════════════════════════════════════
    
    // 网格配置
    struct GridConfig
    {
        int cols = 9;                    // 列数
        int rows = 9;                    // 行数
        float cellSize = 50.f;           // 格子尺寸
        float cellGap = 4.f;             // 格子间距
        float offsetX = 0.f;             // 网格起始X偏移
        float offsetY = 0.f;             // 网格起始Y偏移
        sf::Color fillColor = sf::Color(60, 50, 45);      // 格子填充色
        sf::Color outlineColor = sf::Color(80, 70, 60);   // 格子边框色
        sf::Color hoverColor = sf::Color(90, 80, 70);     // 悬停高亮色
    };
    
    // 格子点击事件数据
    struct CellClickEvent
    {
        int col;
        int row;
        sf::Vector2f worldPos;
    };
    
    class GridSystem
    {
    public:
        GridSystem() = default;
        virtual ~GridSystem() = default;
        
        // 初始化网格（必须调用）
        virtual void init(const GridConfig& config);
        
        // 绘制网格（可重写以添加额外渲染）
        virtual void draw(sf::RenderTarget& target) const;
        
        // 更新（用于动画等）
        virtual void update(float deltaTime);
        
        // 处理鼠标移动（悬停效果）
        virtual void handleMouseMove(sf::Vector2f mousePos);
        
        // 处理点击，返回是否命中格子
        virtual bool handleClick(sf::Vector2f mousePos);
        
        // 设置格子点击回调
        void setOnCellClick(std::function<void(const CellClickEvent&)> callback);
        
        // 获取网格配置
        const GridConfig& getConfig() const { return m_config; }
        
        // 坐标转换
        sf::Vector2i pixelToGrid(sf::Vector2f pixelPos) const;
        sf::Vector2f gridToPixel(int col, int row) const;  // 返回格子中心
        
        // 检查坐标是否在网格内
        bool isInGrid(int col, int row) const;
        bool isInGrid(sf::Vector2i gridPos) const;
        
        // 获取格子（用于子类访问）
        GridCell* getCell(int col, int row);
        const GridCell* getCell(int col, int row) const;
        
        // 设置格子状态
        void setCellHighlight(int col, int row, bool highlight);
        void setCellColor(int col, int row, sf::Color fillColor, sf::Color outlineColor);
        
        // 获取网格尺寸
        int getCols() const { return m_config.cols; }
        int getRows() const { return m_config.rows; }
        
        // 获取网格区域边界
        sf::FloatRect getBounds() const;
        
    protected:
        GridConfig m_config;
        
        // 动态分配的格子数组（支持任意尺寸）
        std::vector<std::vector<std::unique_ptr<GridCell>>> m_cells;
        
        // 当前悬停的格子
        sf::Vector2i m_hoverCell = {-1, -1};
        
        // 回调
        std::function<void(const CellClickEvent&)> m_onCellClick;
        
        // 背景（可选）
        sf::RectangleShape m_background;
        bool m_hasBackground = false;
        
        // 辅助方法
        void createCells();
        void updateHoverState(sf::Vector2i newHover);
    };
}
