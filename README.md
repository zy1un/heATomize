# heATomize

Godot 4.6 prototype for a heat-driven cluster elimination puzzle game.

The current build is a playable rules prototype: the player moves heat balls across a 9x9 board, new balls spawn after each move, heat updates propagate through adjacent balls, matching heat clusters clear, and aftershocks can trigger chained reactions.

## Current Status

Implemented:

- 9x9 board rendering and mouse hover feedback
- Heat ball scene with shader-based color styling
- Ball selection, deselection, and selected pulse feedback
- 4-neighbor pathfinding for legal movement
- Reachable path preview while hovering an empty target cell
- Animated movement along the chosen path
- Staggered spawn timing so new balls do not appear in a single visual burst
- Next-spawn heat preview for planning
- Turn flow after a successful player move
- Random spawn plan: `min(3, empty_cells)` new balls per turn
- New ball heat range: `1..3`
- Phase 1 heat update using simultaneous old-state evaluation
- Phase 2 same-heat cluster elimination
- Phase 3 aftershock heat increase and chained system cycles
- Spawn, heat-update, aftershock, and elimination feedback
- HUD scene with score, cleared count, chain stats, next preview, restart, and debug controls
- Headless smoke tests for rules, turns, and animation

Still rough:

- Feedback is functional but still needs polish and sound
- Chain reaction hand feel needs visual/playtesting verification

## Requirements

- Godot `4.6.x`
- Known local executable paths:

```powershell
D:\program\tools\godot\Godot_v4.6.1-stable_win64.exe
D:\program\tools\godot\Godot_v4.6.1-stable_win64_console.exe
```

Use the console executable for headless tests and agent automation because it reports terminal output and exit codes more reliably.

## Run

Open the project in Godot, or run the main scene from the command line:

```powershell
& "D:\program\tools\godot\Godot_v4.6.1-stable_win64.exe" --path "D:\program\heATomzie"
```

## Test

Run the smoke tests:

```powershell
.\scripts\dev\run_tests.ps1
```

This currently runs board-rule, turn-planning, and animation smoke tests. To call the board-rule test directly:

```powershell
& "D:\program\tools\godot\Godot_v4.6.1-stable_win64_console.exe" --headless --log-file "D:\program\heATomzie\.godot\agent-logs\board_rules_smoke_test.log" --path "D:\program\heATomzie" --script res://scripts/tests/board_rules_smoke_test.gd
```

The helper script writes Godot logs under `.godot/agent-logs/` so headless runs do not depend on the default user log directory.

## Debug Boards

While the game is running:

- `F5`: load a cascade-focused debug board
- `F6`: load a blocked-path debug board

## Project Layout

- `scenes/Main.tscn` main composition scene
- `scenes/board/` board scenes
- `scenes/pieces/` reusable piece scenes
- `scenes/ui/` HUD and interface scenes
- `scripts/core/` application entry scripts
- `scripts/board/` board coordination, rules, pathfinding, and turn logic
- `scripts/pieces/` heat ball visual behavior
- `scripts/ui/` HUD scripts
- `scripts/tests/` headless smoke tests
- `assets/` game assets, shaders, and fonts
- `docs/design/` rule and design notes
- `references/` source reference material from the previous implementation
- `todo.md` current implementation checklist
- `AGENTS.md` quick-start guide for coding agents

## Core Rules

Each completed player move runs this system loop:

1. Phase 1: update every ball's heat simultaneously from the old board state.
2. Phase 2: find same-heat 4-neighbor connected groups and clear groups meeting the heat threshold.
3. Phase 3: apply aftershock, raising orthogonal neighbors of cleared cells by `+1`, capped at `5`.
4. Repeat Phase 1-3 until there are no new eliminations.

Scoring:

- Each cleared ball is worth `100 * current_chain`.
- `Best Chain` records the largest chain depth reached in the current session.

Elimination thresholds:

| Heat | Threshold |
| ---- | --------- |
| 1 | 7 |
| 2 | 6 |
| 3 | 5 |
| 4 | 4 |
| 5 | 3 |

## Next Priorities

1. Polish feedback timing and visual style.
2. Add sound hooks for movement, spawn, elimination, and aftershock.
3. Add screenshot-based visual smoke checks.
4. Expand full-board scenario tests around cascades.
