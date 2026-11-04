# Instructions for coding agents

## Project overview

Plants x Plants is an academic turn-based battle game written in C using raylib, targeting Windows.

Read these files before making changes:

- `README.md`: game scope and execution instructions.
- `docs/ARCHITECTURE.md`: module responsibilities.
- `docs/BACKLOG.md`: task workflow and prototype scope.

Task details, dependencies, and acceptance criteria are maintained in GitHub Issues.

## Scope and workflow

- Implement the requested task and its acceptance criteria.
- Check dependencies before starting implementation.
- Do not implement future features unless explicitly requested.
- If the request is limited to planning, documentation, or scaffolding, do not add gameplay implementation.
- Keep changes focused and avoid unrelated refactoring.
- Preserve existing user changes.
- Do not commit, push, merge, or close issues unless requested.
- Report what changed, how it was validated, and any remaining limitations.

## Language and style

- Use English for identifiers, filenames, branch names, and commit messages.
- Use Portuguese for comments, documentation, and game text.
- Use Conventional Commits.
- Use C11.
- Prefer simple structs, enums, functions, and fixed-size arrays.
- Avoid unnecessary abstractions and dynamic allocation.
- Use four spaces for indentation.
- Keep a brief responsibility comment at the top of each `.c` and `.h` file.

## Architecture

- Keep `main.c` small: window initialization, main loop, and shutdown.
- Use `game.c` to coordinate screens and game flow.
- Keep plant definitions and team operations in `plant`.
- Centralize species attributes and moves in `plant_data`.
- Keep combat rules and action validation in `battle`.
- Let `ai` choose actions and submit them through the combat API.
- Keep battle input and rendering in `battle_screen`.
- Do not introduce raylib dependencies into `plant`, `plant_data`, `battle`, or `ai`.
- Do not calculate damage or modify plant health directly in the interface or AI.
- Rendering functions must not modify gameplay state.
- Avoid mutable global variables; pass state explicitly.
- Never include `.c` files.
- Use include guards in headers.
- Declare file-local helper functions as `static`.

## Combat rules for the prototype

- Teams support up to four plants; prototype teams start with two.
- Only one plant per team is active.
- Turns alternate after valid actions.
- Voluntary switching consumes the turn.
- A defeated plant must be replaced when another living plant is available.
- After a forced replacement, the team that lost its plant takes its turn.
- Invalid actions must not change battle state or consume a turn.
- End the battle when one team has no living plants.
- Reject combat actions after the battle ends.
- Human players and AI must use the same combat rules.

## Prototype boundaries

The first prototype includes:

- Local versus and computer opponent.
- Predefined teams.
- Cactus and sunflower species.
- Two damage moves per species.
- Basic battle interface.
- Victory, restart, and return to menu.
- Simple AI and provisional visuals.

Campaign, biomes, recruitment, evolution, save files, special move effects,
and final art are outside this delivery unless explicitly requested.

## Build and validation

- Keep CMake as the project build configuration.
- Update CMake when adding source files.
- Follow the build instructions in README.md.
- Never claim a build or test passed unless it was actually executed.
- If validation cannot be performed, state the limitation.
- Validate the behavior affected by the change.
- For combat changes, check invalid actions, switching, defeat, and victory as applicable.
- Keep the window responsive; do not use blocking waits for player input or AI delays.
- Do not version build outputs, executables, or local machine paths.

## Dependencies and assets

- Do not add dependencies without a concrete need.
- Keep assets under their corresponding directories in `assets/`.
- Do not rely on absolute paths from a developer's computer.
- Update documentation when setup or module responsibilities change.