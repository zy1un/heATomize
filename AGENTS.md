# heATomzie Agent Guide

This file is the quick-start context for coding agents working on this repository.

## Project

`heATomzie` is a Godot 4.6 puzzle prototype about heat, movement, cluster elimination, and aftershock chain reactions.

The game currently has a playable core loop, but it is still a prototype. Prefer small, testable changes that preserve the current module boundaries.

## Environment

- Workspace: `D:\program\heATomzie`
- Engine: Godot `4.6.x`
- Known Godot executables:

```powershell
D:\program\tools\godot\Godot_v4.6.1-stable_win64.exe
D:\program\tools\godot\Godot_v4.6.1-stable_win64_console.exe
```

Use the console executable for headless tests and automation. The regular executable is for launching the editor/game window.

Run smoke tests:

```powershell
.\scripts\dev\run_tests.ps1
```

This runs rules, turn-planning, and animation smoke tests.

Direct Godot command:

```powershell
& "D:\program\tools\godot\Godot_v4.6.1-stable_win64_console.exe" --headless --log-file "D:\program\heATomzie\.godot\agent-logs\board_rules_smoke_test.log" --path "D:\program\heATomzie" --script res://scripts/tests/board_rules_smoke_test.gd
```

The helper script writes logs to `.godot/agent-logs/` to avoid failures from the default user log directory.

## Architecture Map

- `scenes/Main.tscn`: main composition scene.
- `scenes/board/Board.tscn`: board scene.
- `scenes/pieces/HeatBall.tscn`: reusable heat ball scene.
- `scenes/ui/HUD.tscn`: HUD scene.
- `scripts/core/main.gd`: top-level boot script and scene wiring.
- `scripts/board/board.gd`: board coordination, input handling, drawing, ball-node synchronization, scoring, and applying rule results.
- `scripts/board/board_pathfinding.gd`: 4-neighbor pathfinding, reachability, and legal-move checks.
- `scripts/board/board_rules.gd`: pure board rules: heat updates, elimination groups, aftershock updates.
- `scripts/board/board_turns.gd`: new-ball spawn planning.
- `scripts/pieces/heat_ball.gd`: ball visual configuration and selected pulse behavior.
- `scripts/ui/hud.gd`: HUD display and UI signals.
- `scripts/tests/board_rules_smoke_test.gd`: headless rule smoke tests.
- `scripts/tests/board_turns_smoke_test.gd`: headless turn-planning smoke tests.
- `scripts/tests/animation_smoke_test.gd`: headless scene/feedback smoke tests.
- `docs/design/`: design/rule references.
- `references/linE/`: old C++ implementation references; use for guidance, not as production code.
- `todo.md`: current checklist and progress tracker.

## Current Game Flow

After the player moves one ball:

1. Spawn up to 3 new balls in empty cells.
2. Phase 1: compute all heat changes simultaneously from old board values.
3. Phase 2: find same-heat 4-neighbor connected groups and eliminate groups that meet thresholds.
4. Phase 3: aftershock raises orthogonal neighbors of eliminated cells by `+1`, capped at heat `5`.
5. Repeat Phase 1-3 until there are no new eliminations.
6. Check game over when no ball can reach any empty cell.

The board maintains a next-spawn heat preview. Spawn positions are random, but previewed heat values are consumed in order.

## Debug Boards

Runtime shortcuts:

- `F5`: cascade-focused debug board.
- `F6`: blocked-path debug board.

These boards are intentionally deterministic and should stay useful for quick visual checks.

Elimination thresholds:

- heat `1`: 7 cells
- heat `2`: 6 cells
- heat `3`: 5 cells
- heat `4`: 4 cells
- heat `5`: 3 cells

## Development Rules

- Keep rules logic in `board_rules.gd` whenever possible.
- Keep scene/input/visual synchronization in `board.gd`.
- Keep HUD presentation and buttons in `hud.gd`.
- Keep movement reachability in `board_pathfinding.gd`.
- Keep random spawn planning in `board_turns.gd`.
- Add or update headless tests for rule changes.
- Do not bury gameplay logic inside visual scripts unless it is purely visual behavior.
- Do not replace the current module split with a broad refactor unless the user explicitly asks for it.
- Treat existing uncommitted files as user/project work. Do not revert them.

## Useful Observability

Godot may not be visible to an agent, so use:

- command exit codes
- Godot terminal output
- `print()` logs from game flow
- headless smoke tests
- HUD snapshots emitted from `board.gd`
- optional saved screenshots for visual checks

## Suggested Next Work

1. Polish feedback timing and visual style.
2. Add sound hooks for movement, spawn, elimination, and aftershock.
3. Add screenshot-based visual smoke checks.
4. Expand full-board scenario tests around cascades.
