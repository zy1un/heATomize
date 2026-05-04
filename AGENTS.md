# heATomize Agent 指南

这份文件是给在本仓库中工作的编码 agent 使用的快速上下文。

## 项目

`heATomize` 是一个 Godot 4.6 益智游戏原型，围绕热量、移动、簇消除和余震连锁反应展开。

游戏目前已经有可玩的核心循环，但仍处于原型阶段。优先做小而可测试的改动，并保持当前模块边界。

## 环境

- 工作区：`D:\program\heATomize`
- 引擎：Godot `4.6.x`
- 已知 Godot 可执行文件：

```powershell
D:\program\tools\godot\Godot_v4.6.1-stable_win64.exe
D:\program\tools\godot\Godot_v4.6.1-stable_win64_console.exe
```

使用 console 可执行文件运行 headless 测试和自动化。普通可执行文件用于启动编辑器或游戏窗口。

运行 smoke tests：

```powershell
.\scripts\dev\run_tests.ps1
```

该脚本会运行规则、回合规划和动画 smoke tests。

直接运行 Godot 命令：

```powershell
& "D:\program\tools\godot\Godot_v4.6.1-stable_win64_console.exe" --headless --log-file "D:\program\heATomize\.godot\agent-logs\board_rules_smoke_test.log" --path "D:\program\heATomize" --script res://scripts/tests/board_rules_smoke_test.gd
```

辅助脚本会把日志写入 `.godot/agent-logs/`，避免默认用户日志目录导致失败。

## 架构地图

- `scenes/Main.tscn`：主组合场景。
- `scenes/board/Board.tscn`：棋盘场景。
- `scenes/pieces/HeatBall.tscn`：可复用热球场景。
- `scenes/ui/HUD.tscn`：HUD 场景。
- `scripts/core/main.gd`：顶层启动脚本和场景接线。
- `scripts/board/board.gd`：棋盘协调、输入处理、绘制、球节点同步、计分，以及应用规则结果。
- `scripts/board/board_pathfinding.gd`：4 邻接寻路、可达性和合法移动检查。
- `scripts/board/board_rules.gd`：纯棋盘规则：热量更新、消除组、余震更新。
- `scripts/board/board_turns.gd`：新球生成规划。
- `scripts/pieces/heat_ball.gd`：球的视觉配置和选中脉冲行为。
- `scripts/ui/hud.gd`：HUD 显示和 UI 信号。
- `scripts/tests/board_rules_smoke_test.gd`：headless 规则 smoke tests。
- `scripts/tests/board_turns_smoke_test.gd`：headless 回合规划 smoke tests。
- `scripts/tests/animation_smoke_test.gd`：headless 场景/反馈 smoke tests。
- `docs/design/`：设计和规则参考。
- `references/linE/`：旧 C++ 实现参考；只用于指导，不作为生产代码。
- `todo.md`：当前 checklist 和进度追踪。

## 当前游戏流程

玩家移动一个球之后：

1. 在空格中生成最多 3 个新球。
2. 阶段 1：基于旧棋盘值同时计算所有热量变化。
3. 阶段 2：查找相同热量的 4 邻接连通组，并消除达到阈值的组。
4. 阶段 3：余震让被消除格子的正交邻居 `+1`，最高封顶到热量 `5`。
5. 重复阶段 1-3，直到没有新的消除。
6. 当没有任何球能到达任何空格时，检查游戏结束。

棋盘维护下一次生成热量预览。生成位置是随机的，但预览的热量值会按顺序消费。

## 调试棋盘

运行时快捷键：

- `F5`：聚焦 cascade 的调试棋盘。
- `F6`：阻塞路径调试棋盘。

这些棋盘刻意保持确定性，并应继续适合快速视觉检查。

消除阈值：

- 热量 `1`：7 格
- 热量 `2`：6 格
- 热量 `3`：5 格
- 热量 `4`：4 格
- 热量 `5`：3 格

## Worktree 约定

**默认使用 worktree 创建功能分支。** 不要在主仓库中直接 checkout 功能分支。始终通过 `git worktree add` 在同级目录创建独立工作目录，确保主仓库始终停留在 master 上，不会因分支切换污染工作区。

流程：
1. `git worktree add D:\program\heATomize-<branch-name> -b feature/<branch-name>`
2. 在 worktree 中开发和提交
3. 合并到 master 后，先验证合并结果，再删除 worktree 和分支

**合并前不要删除 worktree 或分支。** 只有在合并结果经过验证（至少确认无重复声明、无编译错误）后才能清理。

为并行实验创建 git worktree 时，始终在对应 worktree 的 `project.godot` 中修改 `config/name`，加入分支标识，方便 Godot 项目管理器区分窗口。例如：

```
config/name="heATomize (v1-classic)"
config/name="heATomize (v2-tween)"
```

Worktree 目录应当放在主项目的同级目录中，例如 `D:\program\heATomize-v1-classic`，不要嵌套在主项目内部。

当多个 agent 可能并行做同一部分时，worktree、分支名和 `config/name` 都必须带上 agent 署名，避免互相覆盖候选方案。例如：

```
D:\program\heATomize-codex-shockwave
D:\program\heATomize-claude-shockwave
codex/shockwave-v2
claude/shockwave-v2
config/name="heATomize (codex-shockwave-v2)"
config/name="heATomize (claude-shockwave-v2)"
```

如果发现已有 worktree 属于其他 agent，不要复用或继续修改；除非用户明确指定，否则新建带自己署名的 worktree 和分支。

## 开发规则

- 尽可能把规则逻辑保留在 `board_rules.gd`。
- 把场景、输入和视觉同步保留在 `board.gd`。
- 把 HUD 展示和按钮逻辑保留在 `hud.gd`。
- 把移动可达性保留在 `board_pathfinding.gd`。
- 把随机生成规划保留在 `board_turns.gd`。
- 修改规则时添加或更新 headless tests。
- 除非是纯视觉行为，不要把玩法逻辑埋进视觉脚本。
- 除非用户明确要求，不要用大范围重构替换当前模块划分。
- 把现有未提交文件视为用户/项目工作。不要回滚它们。

## 有用的可观测性

Agent 可能看不到 Godot 窗口，因此使用：

- 命令退出码
- Godot 终端输出
- 游戏流程里的 `print()` 日志
- headless smoke tests
- `board.gd` 发出的 HUD snapshots
- 可选的已保存截图，用于视觉检查

## 建议的下一步工作

1. 打磨反馈时序和视觉风格。
2. 为移动、生成、消除和余震添加音效 hook。
3. 添加基于截图的视觉 smoke checks。
4. 扩展围绕 cascade 的满棋盘场景测试。
