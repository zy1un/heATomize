#include <SFML/Graphics.hpp>
#include "entities/Ball.hpp"
#include "graphics/Color.hpp"
#include "core/Defaults.hpp"
#include "systems/Animation.hpp"
#include <cmath>

using namespace LE;

// ═══════════════════════════════════════════════════════════
// 构造函数
// ═══════════════════════════════════════════════════════════

Ball::Ball(sf::Vector2i gridPosition, BallColor ballColor) 
    : gridPosition(gridPosition), ballColor(ballColor), m_mimicColor(ballColor), 
      ballState(BallState::Emerging), shape(BALL_RADIUS)
{
    shape.setOrigin({BALL_RADIUS, BALL_RADIUS});
    shape.setPosition({static_cast<float>(FIRST_GRID_CENTER + GRID_LENGTH * gridPosition.x), 
                       static_cast<float>(FIRST_GRID_CENTER + GRID_LENGTH * gridPosition.y)});
    shape.setOutlineThickness(BALL_BORDER);
    
    // 设置初始颜色（draw() 会覆盖变色球和彩虹球的颜色，但普通球需要这里设置）
    if (ballColor != BallColor::Chameleon && ballColor != BallColor::Rainbow)
    {
        shape.setFillColor(COLORS[2*static_cast<int>(ballColor)]);
        shape.setOutlineColor(COLORS[2*static_cast<int>(ballColor) + 1]);
    }
}

Ball::Ball(){}

// ═══════════════════════════════════════════════════════════
// 颜色相关
// ═══════════════════════════════════════════════════════════

BallColor Ball::getBallColor() const
{
    return m_mimicColor;
}

BallColor Ball::getRealColor() const
{
    return ballColor;
}

void Ball::setBallColor(BallColor newColor)
{
    this->ballColor = newColor;
    this->m_mimicColor = newColor;
    shape.setFillColor(COLORS[2*static_cast<int>(newColor)]);
    shape.setOutlineColor(COLORS[2*static_cast<int>(newColor) + 1]);
}

void Ball::setMimicColor(BallColor color)
{
    if (m_mimicColor != color)
    {
        sf::Color oldDisplayColor = getMimicDisplayColor();
        this->m_mimicColor = color;
        sf::Color newDisplayColor = getMimicDisplayColor();
        
        Animation::startColorChange(this, oldDisplayColor, newDisplayColor);
    }
}

sf::Color Ball::getMimicDisplayColor() const
{
    if (m_mimicColor == BallColor::Chameleon)
    {
        return Chameleon::FILL_NORMAL;
    }
    else
    {
        sf::Color base = COLORS[2*static_cast<int>(m_mimicColor)];
        return sf::Color(base.r, base.g, base.b, Chameleon::DYED_ALPHA);
    }
}

// ═══════════════════════════════════════════════════════════
// 状态相关
// ═══════════════════════════════════════════════════════════

BallState Ball::getBallState() const
{
    return ballState;
}

void Ball::setBallState(BallState ballState)
{
    this->ballState = ballState;
}

bool Ball::isChameleon() const
{
    return ballColor == BallColor::Chameleon;
}

bool Ball::isRainbow() const
{
    return ballColor == BallColor::Rainbow;
}

sf::Vector2i Ball::getGridPosition() const
{
    return gridPosition;
}

void Ball::setGridPosition(sf::Vector2i newPosition)
{
    this->gridPosition = newPosition;
    shape.setPosition({static_cast<float>(FIRST_GRID_CENTER + GRID_LENGTH * gridPosition.x), 
                       static_cast<float>(FIRST_GRID_CENTER + GRID_LENGTH * gridPosition.y)});
}

void Ball::setPixelPosition(sf::Vector2f position)
{
    shape.setPosition(position);
}

// ═══════════════════════════════════════════════════════════
// 选中与呼吸动画
// ═══════════════════════════════════════════════════════════

void Ball::setSelected(bool selected)
{
    m_isSelected = selected;
    if (!selected)
        m_breathPhase = 0.f;
}

bool Ball::isSelected() const
{
    return m_isSelected;
}

void Ball::updateBlink(float deltaTime)
{
    if (m_isSelected)
    {
        // 呼吸动画：相位以 2π 为周期
        constexpr float TWO_PI = 6.283185f;
        m_breathPhase += deltaTime * BREATH_SPEED * TWO_PI;
        if (m_breathPhase > TWO_PI)
            m_breathPhase -= TWO_PI;
    }
}

// 计算呼吸缩放值（使用通用缓动函数）
float Ball::getBreathScale() const
{
    float t = Easing::breathe(m_breathPhase);
    return Easing::mapRange(t, SELECTED_SCALE_MIN, SELECTED_SCALE_MAX);
}

// ═══════════════════════════════════════════════════════════
// 生长动画
// ═══════════════════════════════════════════════════════════

void Ball::updateSpawn(float deltaTime)
{
    if (ballState == BallState::Emerging)
    {
        m_spawnProgress += deltaTime / SPAWN_DURATION;
        if (m_spawnProgress >= 1.0f)
        {
            m_spawnProgress = 1.0f;
            ballState = BallState::Sleep; // 生长完成
        }
    }
}

void Ball::finishSpawn()
{
    m_spawnProgress = 1.0f;
    ballState = BallState::Sleep;
}

bool Ball::isSpawning() const
{
    return ballState == BallState::Emerging;
}

// 计算生长缩放值（使用通用缓动函数）
float Ball::getSpawnScale() const
{
    if (m_spawnProgress >= 1.0f)
        return 1.0f;
    
    // 使用缓出弹跳曲线，减小过冲幅度以获得更平滑的效果
    float scale = Easing::easeOutBack(m_spawnProgress, 1.2f);
    return std::max(0.0f, scale);
}

// ═══════════════════════════════════════════════════════════
// 消除动画
// ═══════════════════════════════════════════════════════════

void Ball::startEliminate()
{
    m_isEliminating = true;
    m_eliminatePhase = 0.f;
    m_eliminateAlpha = 255.f;
}

void Ball::updateEliminate(float deltaTime)
{
    if (!m_isEliminating) return;
    
    // 呼吸速度 (降低为 2.0，更平缓)
    constexpr float ELIMINATE_BREATH_SPEED = 2.0f;
    constexpr float TWO_PI = 6.283185f;
    constexpr float FADE_SPEED = 200.f; // 每秒减少的透明度
    
    // 更新呼吸相位
    m_eliminatePhase += deltaTime * ELIMINATE_BREATH_SPEED * TWO_PI;
    if (m_eliminatePhase > TWO_PI)
        m_eliminatePhase -= TWO_PI;
    
    // 同时渐隐
    m_eliminateAlpha -= deltaTime * FADE_SPEED;
    if (m_eliminateAlpha < 0.f)
        m_eliminateAlpha = 0.f;
}

bool Ball::isEliminating() const
{
    return m_isEliminating;
}

float Ball::getEliminateAlpha() const
{
    // 叠加呼吸效果到透明度上 (幅度减小，0.8 ~ 1.0 的波动)
    float breathMod = Easing::breathe(m_eliminatePhase);
    float alpha = m_eliminateAlpha * (0.8f + 0.2f * breathMod);
    return alpha;
}

void Ball::lockEliminateColor(BallColor color)
{
    m_hasLockedColor = true;
    m_lockedColor = color;
}

bool Ball::hasLockedColor() const
{
    return m_hasLockedColor;
}

// ═══════════════════════════════════════════════════════════
// 绘制
// ═══════════════════════════════════════════════════════════

void Ball::draw(sf::RenderTarget &target) const
{
    if (ballColor == BallColor::Rainbow)
    {
        drawRainbow(target);
    }
    else if (ballColor == BallColor::Chameleon)
    {
        drawChameleon(target);
    }
    else
    {
        drawNormal(target);
    }
}

void Ball::drawChameleon(sf::RenderTarget &target) const
{
    sf::CircleShape ballShape = shape;
    float spawnScale = getSpawnScale();
    
    // 获取当前透明度（正常为255，消除时渐变）
    uint8_t alpha = 255;
    if (m_isEliminating)
    {
        alpha = static_cast<uint8_t>(getEliminateAlpha());
    }
    
    // 获取显示位置（考虑移动动画）
    sf::Vector2f displayPos = getDisplayPosition();
    ballShape.setPosition(displayPos);
    
    if (m_isSelected)
    {
        // 呼吸效果：平滑缩放
        float breathScale = getBreathScale();
        float finalScale = breathScale * spawnScale;
        
        // 选中高亮：已染色用深色，未染色用银灰
        sf::Color fillColor = (m_mimicColor == BallColor::Chameleon) 
            ? Chameleon::FILL_SELECTED 
            : COLORS[2*static_cast<int>(m_mimicColor) + 1];
            
        // 应用透明度
        fillColor.a = std::min(fillColor.a, alpha);
        
        sf::Color outlineColor = Chameleon::OUTLINE_SELECTED;
        outlineColor.a = std::min(outlineColor.a, alpha);

        ballShape.setFillColor(fillColor);
        ballShape.setOutlineColor(outlineColor);
        ballShape.setOutlineThickness(BALL_BORDER + BALL_BORDER_SELECTED);
        ballShape.setScale({finalScale, finalScale});
    }
    else
    {
        // 非选中：检查动画状态
        ChameleonAnimState* animState = Animation::getAnimState(const_cast<Ball*>(this));
        
        sf::Color displayColor;
        float animScale = 1.0f;
        
        if (animState && animState->isAnimating)
        {
            displayColor = Animation::lerpColor(animState->fromColor, animState->toColor, animState->progress);
            animScale = Animation::getAnimScale(animState->progress);
        }
        else
        {
            displayColor = getMimicDisplayColor();
        }
        
        // 应用透明度
        displayColor.a = std::min(displayColor.a, alpha);
        
        sf::Color outlineColor = Chameleon::OUTLINE_NORMAL;
        outlineColor.a = std::min(outlineColor.a, alpha);
        
        float finalScale = animScale * spawnScale;
        ballShape.setFillColor(displayColor);
        ballShape.setScale({finalScale, finalScale});
        ballShape.setOutlineColor(outlineColor);
        ballShape.setOutlineThickness(BALL_BORDER + 1);
    }
    
    target.draw(ballShape);
}

void Ball::drawNormal(sf::RenderTarget &target) const
{
    float spawnScale = getSpawnScale();
    
    // 获取当前透明度
    uint8_t alpha = 255;
    if (m_isEliminating)
    {
        alpha = static_cast<uint8_t>(getEliminateAlpha());
    }

    // 获取显示位置（考虑移动动画）
    sf::Vector2f displayPos = getDisplayPosition();

    if (m_isSelected)
    {
        // 呼吸效果：平滑缩放
        float breathScale = getBreathScale();
        float finalScale = breathScale * spawnScale;
        sf::CircleShape ballShape = shape;
        
        // 选中高亮：反色
        sf::Color fillColor = COLORS[2*static_cast<int>(ballColor) + 1];
        fillColor.a = std::min(fillColor.a, alpha);
        
        sf::Color outlineColor = COLORS[2*static_cast<int>(ballColor)];
        outlineColor.a = std::min(outlineColor.a, alpha);
        
        ballShape.setPosition(displayPos);
        ballShape.setFillColor(fillColor);
        ballShape.setOutlineColor(outlineColor);
        ballShape.setOutlineThickness(BALL_BORDER + BALL_BORDER_SELECTED);
        ballShape.setScale({finalScale, finalScale});
        target.draw(ballShape);
    }
    else
    {
        sf::CircleShape ballShape = shape;
        
        // 应用透明度（需要重新设置颜色）
        sf::Color fillColor = COLORS[2*static_cast<int>(ballColor)];
        fillColor.a = std::min(fillColor.a, alpha);
        
        sf::Color outlineColor = COLORS[2*static_cast<int>(ballColor) + 1];
        outlineColor.a = std::min(outlineColor.a, alpha);
        
        ballShape.setPosition(displayPos);
        ballShape.setFillColor(fillColor);
        ballShape.setOutlineColor(outlineColor);
        
        // 非选中：仅应用生长缩放
        if (spawnScale < 1.0f)
        {
            ballShape.setScale({spawnScale, spawnScale});
        }
        
        target.draw(ballShape);
    }
}

// ═══════════════════════════════════════════════════════════
// 移动动画
// ═══════════════════════════════════════════════════════════

void Ball::startMove(const std::vector<sf::Vector2i>& path)
{
    if (path.size() < 2) return;
    
    m_isMoving = true;
    m_movePath = path;
    m_movePathIndex = 0;
    m_moveProgress = 0.f;
}

void Ball::updateMove(float deltaTime)
{
    if (!m_isMoving || m_movePath.size() < 2) return;
    
    // 每格移动的时间
    m_moveProgress += deltaTime / MOVE_SPEED_PER_CELL;
    
    while (m_moveProgress >= 1.0f && m_movePathIndex < static_cast<int>(m_movePath.size()) - 2)
    {
        m_moveProgress -= 1.0f;
        m_movePathIndex++;
    }
    
    // 到达终点
    if (m_movePathIndex >= static_cast<int>(m_movePath.size()) - 2 && m_moveProgress >= 1.0f)
    {
        m_moveProgress = 1.0f;
        m_isMoving = false;
        
        // 更新最终位置
        sf::Vector2i finalPos = m_movePath.back();
        gridPosition = finalPos;
        shape.setPosition({static_cast<float>(FIRST_GRID_CENTER + GRID_LENGTH * finalPos.x),
                           static_cast<float>(FIRST_GRID_CENTER + GRID_LENGTH * finalPos.y)});
        m_movePath.clear();
    }
}

bool Ball::isMoving() const
{
    return m_isMoving;
}

sf::Vector2f Ball::getDisplayPosition() const
{
    // 计算愤怒跳动的 Y 偏移
    float angryBounceOffset = 0.0f;
    if (m_isAngryBouncing)
    {
        // 两次跳跃: 0-0.5 第一跳, 0.5-1.0 第二跳
        float t = m_angryBounceProgress;
        float bounceT;
        float height;
        
        if (t < 0.5f)
        {
            // 第一跳 (较高)
            bounceT = t * 2.0f;  // 0-1
            height = ANGRY_BOUNCE_HEIGHT;
        }
        else
        {
            // 第二跳 (较低)
            bounceT = (t - 0.5f) * 2.0f;  // 0-1
            height = ANGRY_BOUNCE_HEIGHT * 0.5f;
        }
        
        // 抛物线: 4 * t * (1 - t) 在 t=0.5 时达到最大值 1
        angryBounceOffset = -height * 4.0f * bounceT * (1.0f - bounceT);
    }
    
    if (!m_isMoving || m_movePath.size() < 2)
    {
        // 不在移动，返回shape的位置（加上跳动偏移）
        sf::Vector2f basePos = shape.getPosition();
        return {basePos.x, basePos.y + angryBounceOffset};
    }
    
    // 获取当前段的起点和终点
    int idx = std::min(m_movePathIndex, static_cast<int>(m_movePath.size()) - 2);
    sf::Vector2i from = m_movePath[idx];
    sf::Vector2i to = m_movePath[idx + 1];
    
    // 平滑插值 (使用 ease-out 曲线)
    float t = m_moveProgress;
    float eased = 1.0f - (1.0f - t) * (1.0f - t);  // ease-out-quad
    
    float fromX = static_cast<float>(FIRST_GRID_CENTER + GRID_LENGTH * from.x);
    float fromY = static_cast<float>(FIRST_GRID_CENTER + GRID_LENGTH * from.y);
    float toX = static_cast<float>(FIRST_GRID_CENTER + GRID_LENGTH * to.x);
    float toY = static_cast<float>(FIRST_GRID_CENTER + GRID_LENGTH * to.y);
    
    return {fromX + (toX - fromX) * eased, fromY + (toY - fromY) * eased};
}

const std::vector<sf::Vector2i>& Ball::getMovePath() const
{
    return m_movePath;
}

int Ball::getMovePathIndex() const
{
    return m_movePathIndex;
}

// ═══════════════════════════════════════════════════════════
// 愤怒跳动动画
// ═══════════════════════════════════════════════════════════

void Ball::startAngryBounce()
{
    m_isAngryBouncing = true;
    m_angryBounceProgress = 0.f;
}

void Ball::updateAngryBounce(float deltaTime)
{
    if (!m_isAngryBouncing) return;
    
    m_angryBounceProgress += deltaTime / ANGRY_BOUNCE_DURATION;
    
    if (m_angryBounceProgress >= 1.0f)
    {
        m_angryBounceProgress = 1.0f;
        m_isAngryBouncing = false;
    }
}

bool Ball::isAngryBouncing() const
{
    return m_isAngryBouncing;
}

// ═══════════════════════════════════════════════════════════
// 彩虹球相关
// ═══════════════════════════════════════════════════════════

// 预定义的彩虹色序列（7色彩虹）
static const sf::Color RAINBOW_PALETTE[] = {
    sf::Color(231, 76, 60),    // 红 #E74C3C
    sf::Color(230, 126, 34),   // 橙 #E67E22
    sf::Color(241, 196, 15),   // 黄 #F1C40F
    sf::Color(46, 204, 113),   // 绿 #2ECC71
    sf::Color(0, 188, 212),    // 青 #00BCD4
    sf::Color(52, 152, 219),   // 蓝 #3498DB
    sf::Color(155, 89, 182),   // 紫 #9B59B6
};
static const int RAINBOW_PALETTE_SIZE = 7;

// 平滑插值两个颜色
static sf::Color lerpColor(const sf::Color& a, const sf::Color& b, float t)
{
    return sf::Color(
        static_cast<uint8_t>(a.r + (b.r - a.r) * t),
        static_cast<uint8_t>(a.g + (b.g - a.g) * t),
        static_cast<uint8_t>(a.b + (b.b - a.b) * t),
        static_cast<uint8_t>(a.a + (b.a - a.a) * t)
    );
}

// 平滑缓动函数（smoothstep）- 创造呼吸感
static float smoothstep(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

sf::Color Ball::getRainbowColor() const
{
    // 使用平滑的呼吸感渐变
    // m_rainbowPhase 在 0-1 之间循环，代表在彩虹色轮上的位置
    
    // 计算当前位于哪两个颜色之间
    float scaledPhase = m_rainbowPhase * RAINBOW_PALETTE_SIZE;
    int colorIndex = static_cast<int>(scaledPhase) % RAINBOW_PALETTE_SIZE;
    int nextIndex = (colorIndex + 1) % RAINBOW_PALETTE_SIZE;
    
    // 计算两个颜色之间的插值比例，使用平滑曲线
    float localT = scaledPhase - static_cast<int>(scaledPhase);
    float smoothT = smoothstep(localT);
    
    // 插值得到当前颜色
    return lerpColor(RAINBOW_PALETTE[colorIndex], RAINBOW_PALETTE[nextIndex], smoothT);
}

void Ball::drawRainbow(sf::RenderTarget &target) const
{
    // ═══════════════════════════════════════════════════════════
    // 超慢呼吸感渐变：约30秒完成一个彩虹周期
    // ═══════════════════════════════════════════════════════════
    constexpr float RAINBOW_SPEED = 0.033f;   // 每秒完成 1/30 周期 = 30秒一个完整彩虹循环
    constexpr float ASSUMED_DT = 0.016f;      // 假设约60fps
    
    m_rainbowPhase += ASSUMED_DT * RAINBOW_SPEED;
    if (m_rainbowPhase >= 1.0f)
        m_rainbowPhase -= 1.0f;
    
    sf::CircleShape ballShape = shape;
    float spawnScale = getSpawnScale();
    
    // 获取当前透明度
    uint8_t alpha = 255;
    if (m_isEliminating)
    {
        alpha = static_cast<uint8_t>(getEliminateAlpha());
    }
    
    // 获取显示位置
    sf::Vector2f displayPos = getDisplayPosition();
    ballShape.setPosition(displayPos);
    
    // 获取当前显示颜色
    // 如果颜色已锁定（消除时），使用锁定的颜色；否则使用渐变色
    sf::Color rainbowColor;
    if (m_hasLockedColor)
    {
        // 使用锁定的纯色（从调色板获取）
        rainbowColor = RAINBOW_PALETTE[static_cast<int>(m_lockedColor) % RAINBOW_PALETTE_SIZE];
    }
    else
    {
        rainbowColor = getRainbowColor();
    }
    rainbowColor.a = alpha;
    
    // 描边用稍深的颜色（30%暗度）
    sf::Color outlineColor(
        static_cast<uint8_t>(rainbowColor.r * 0.7f),
        static_cast<uint8_t>(rainbowColor.g * 0.7f),
        static_cast<uint8_t>(rainbowColor.b * 0.7f),
        alpha
    );
    
    if (m_isSelected)
    {
        // 选中状态：呼吸缩放 + 更亮的颜色
        float breathScale = getBreathScale();
        float finalScale = breathScale * spawnScale;
        
        // 选中时颜色稍微提亮
        sf::Color brightColor(
            static_cast<uint8_t>(std::min(255, rainbowColor.r + 30)),
            static_cast<uint8_t>(std::min(255, rainbowColor.g + 30)),
            static_cast<uint8_t>(std::min(255, rainbowColor.b + 30)),
            alpha
        );
        
        ballShape.setFillColor(brightColor);
        ballShape.setOutlineColor(sf::Color(255, 255, 255, alpha));  // 白色描边
        ballShape.setOutlineThickness(BALL_BORDER + BALL_BORDER_SELECTED);
        ballShape.setScale({finalScale, finalScale});
    }
    else
    {
        ballShape.setFillColor(rainbowColor);
        ballShape.setOutlineColor(outlineColor);
        ballShape.setOutlineThickness(BALL_BORDER + 1);
        
        if (spawnScale < 1.0f)
        {
            ballShape.setScale({spawnScale, spawnScale});
        }
    }
    
    target.draw(ballShape);
    
    // ═══════════════════════════════════════════════════════════
    // 中心星芒效果：轻柔的呼吸脉冲（独立周期，约4秒）
    // ═══════════════════════════════════════════════════════════
    sf::CircleShape star(4.f);
    star.setOrigin({4.f, 4.f});
    star.setPosition(displayPos);
    star.setFillColor(sf::Color(255, 255, 255, static_cast<uint8_t>(alpha * 0.4f)));
    
    // 使用独立的慢速呼吸周期（约4秒）
    // 彩虹30秒周期，星芒约4秒周期 (30/7.5 ≈ 4)
    float breathPhase = m_rainbowPhase * 7.5f;
    float breathT = 0.5f + 0.5f * std::sin(breathPhase * 6.283185f);  // 0-1 呼吸
    float starScale = spawnScale * (0.7f + 0.3f * smoothstep(breathT));  // 柔和的缩放
    star.setScale({starScale, starScale});
    target.draw(star);
}

