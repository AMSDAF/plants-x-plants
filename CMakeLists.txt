# CMakeLists.txt
# Configura a compilação em C11 e integra a biblioteca raylib.
# Baixa uma versão fixa da dependência durante a configuração inicial.

cmake_minimum_required(VERSION 3.24)

project(plants_x_plants VERSION 0.1.0 LANGUAGES C)

# O prefix localizado do /showIncludes é detectado incorretamente pelo CMake
# neste Windows em português, fazendo o Ninja ignorar dependências de headers.
if(MSVC AND CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_CL_SHOWINCLUDES_PREFIX "Observação: incluindo arquivo: ")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)

# Não compilar os exemplos da raylib.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(raylib)

set(BATTLE_SOURCES
    src/plant.c
    src/plant_data.c
    src/battle.c
    src/ai.c
    src/battle_screen.c
    src/battle_fx.c
    src/relay_ui.c
    src/result_screen.c
    src/logic_setup_screen.c
    src/battle_screen_assets.c
)

add_executable(plants_x_plants
    src/main.c

    src/game.c
    ${BATTLE_SOURCES}
    src/player.c
    src/world.c
    src/trainer_select_screen.c
    src/selection_ui.c
    src/name_entry_screen.c
    src/trainer_background.c
    src/menu_screen.c
)

target_include_directories(plants_x_plants PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(plants_x_plants PRIVATE raylib)

# Verificação gráfica opcional; não participa da compilação normal do jogo.
add_executable(selection_scene_check EXCLUDE_FROM_ALL
    tools/check_selection_scene.c
    ${BATTLE_SOURCES}

    src/game.c
    src/menu_screen.c
    src/trainer_select_screen.c
    src/selection_ui.c
    src/name_entry_screen.c
    src/trainer_background.c
    src/player.c
    src/world.c
)
target_include_directories(selection_scene_check PRIVATE include)
target_link_libraries(selection_scene_check PRIVATE raylib)

# Verificação opcional da composição, mascote e navegação do menu.
add_executable(menu_screen_check EXCLUDE_FROM_ALL
    tools/check_menu_screen.c
    ${BATTLE_SOURCES}

    src/game.c
    src/menu_screen.c
    src/trainer_select_screen.c
    src/selection_ui.c
    src/name_entry_screen.c
    src/trainer_background.c
    src/player.c
    src/world.c
)
target_include_directories(menu_screen_check PRIVATE include)
target_link_libraries(menu_screen_check PRIVATE raylib)

add_executable(trainer_background_check EXCLUDE_FROM_ALL
    tools/check_trainer_background.c
    ${BATTLE_SOURCES}

    src/game.c src/menu_screen.c src/trainer_select_screen.c src/selection_ui.c src/name_entry_screen.c
    src/trainer_background.c src/player.c src/world.c
)
target_include_directories(trainer_background_check PRIVATE include)
target_link_libraries(trainer_background_check PRIVATE raylib)

if(WIN32)
    add_executable(name_entry_check EXCLUDE_FROM_ALL
        tools/check_name_entry.c tools/check_name_entry_input.c
        ${BATTLE_SOURCES}

        src/game.c src/menu_screen.c src/trainer_select_screen.c src/trainer_background.c
        src/name_entry_screen.c src/selection_ui.c src/player.c src/world.c)
    target_include_directories(name_entry_check PRIVATE include)
    target_link_libraries(name_entry_check PRIVATE raylib)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    if(TARGET name_entry_check)
        target_compile_options(name_entry_check PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    target_compile_options(trainer_background_check PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(selection_scene_check PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(menu_screen_check PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(plants_x_plants PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# O mesmo canvas atende à janela real e aos alvos de verificação da interface.
foreach(view_target plants_x_plants selection_scene_check menu_screen_check
        trainer_background_check name_entry_check)
    if(TARGET ${view_target})
        target_sources(${view_target} PRIVATE src/game_canvas.c)
    endif()
endforeach()

add_executable(exploration_hud_check EXCLUDE_FROM_ALL
    tools/check_exploration_hud.c ${BATTLE_SOURCES}
    src/game.c src/menu_screen.c src/trainer_select_screen.c
    src/selection_ui.c src/name_entry_screen.c src/trainer_background.c
    src/player.c src/world.c src/game_canvas.c src/ui_theme.c src/exploration_hud.c src/dialogue_ui.c)
target_include_directories(exploration_hud_check PRIVATE include)
target_link_libraries(exploration_hud_check PRIVATE raylib)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(exploration_hud_check PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(game_canvas_check EXCLUDE_FROM_ALL
    tools/check_game_canvas.c src/game_canvas.c src/world.c)
target_include_directories(game_canvas_check PRIVATE include)
target_link_libraries(game_canvas_check PRIVATE raylib)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(game_canvas_check PRIVATE -Wall -Wextra -Wpedantic)
endif()

foreach(ui_target plants_x_plants selection_scene_check menu_screen_check
        trainer_background_check name_entry_check game_canvas_check)
    if(TARGET ${ui_target})
        target_sources(${ui_target} PRIVATE src/ui_theme.c src/exploration_hud.c src/dialogue_ui.c)
    endif()
endforeach()

# Inclui o prólogo nos alvos que utilizam o coordenador de telas.
foreach(intro_target plants_x_plants selection_scene_check menu_screen_check
        trainer_background_check name_entry_check exploration_hud_check)
    if(TARGET ${intro_target})
        target_sources(${intro_target} PRIVATE src/intro_screen.c)
    endif()
endforeach()

# Áudio acompanha todos os alvos que usam o coordenador de telas.
foreach(audio_target plants_x_plants selection_scene_check menu_screen_check
        trainer_background_check name_entry_check exploration_hud_check)
    if(TARGET ${audio_target})
        target_sources(${audio_target} PRIVATE src/audio.c src/game_audio.c)
    endif()
endforeach()

# A abertura usa os mesmos módulos do executável principal.
get_target_property(INTRO_CHECK_SOURCES plants_x_plants SOURCES)
list(REMOVE_ITEM INTRO_CHECK_SOURCES src/main.c)
add_executable(intro_check EXCLUDE_FROM_ALL tools/check_intro.c ${INTRO_CHECK_SOURCES})
target_include_directories(intro_check PRIVATE include)
target_link_libraries(intro_check PRIVATE raylib)

# Arena real: menus, ações, recursos e capturas em resolução física.
add_executable(battle_screen_check EXCLUDE_FROM_ALL tools/check_battle_screen.c ${INTRO_CHECK_SOURCES})
target_include_directories(battle_screen_check PRIVATE include)
target_link_libraries(battle_screen_check PRIVATE raylib)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(battle_screen_check PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(intro_check PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Regras compartilhadas da campanha, sem janela nem dependência da raylib.
add_executable(battle_check EXCLUDE_FROM_ALL
    tools/check_battle.c src/plant.c src/plant_data.c src/battle.c src/ai.c)
target_include_directories(battle_check PRIVATE include)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(battle_check PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Central industrial com recursos reais e capturas OpenGL.
add_executable(relay_ui_check EXCLUDE_FROM_ALL tools/check_relay_ui.c ${INTRO_CHECK_SOURCES})
target_include_directories(relay_ui_check PRIVATE include)
target_link_libraries(relay_ui_check PRIVATE raylib)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(relay_ui_check PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Balanceamento reproduzível de bosses solo, sem janela nem golpe lógico do jogador.
add_executable(boss_balance_check EXCLUDE_FROM_ALL tools/check_boss_balance.c
    src/plant.c src/plant_data.c src/battle.c src/ai.c)
target_include_directories(boss_balance_check PRIVATE include)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(boss_balance_check PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Auditoria opcional de balanceamento, sem janela nem mudanças nos atributos.
add_executable(balance_audit_check EXCLUDE_FROM_ALL tools/check_balance_audit.c
    src/plant.c src/plant_data.c src/battle.c src/ai.c)
target_include_directories(balance_audit_check PRIVATE include)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(balance_audit_check PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Resultados da campanha: navegação, revanche e capturas reais.
add_executable(result_screen_check EXCLUDE_FROM_ALL tools/check_result_screen.c ${INTRO_CHECK_SOURCES})
target_include_directories(result_screen_check PRIVATE include)
target_link_libraries(result_screen_check PRIVATE raylib)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(result_screen_check PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Efeitos e áudio dos golpes, usando o motor real e capturas OpenGL.
add_executable(battle_fx_check EXCLUDE_FROM_ALL tools/check_battle_fx.c ${INTRO_CHECK_SOURCES})
target_include_directories(battle_fx_check PRIVATE include)
target_link_libraries(battle_fx_check PRIVATE raylib)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(battle_fx_check PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Descoberta das interações sem carregar todos os recursos da campanha.
add_executable(discovery_markers_check EXCLUDE_FROM_ALL tools/check_discovery_markers.c
    src/world.c src/game_canvas.c src/ui_theme.c src/exploration_hud.c src/dialogue_ui.c)
target_include_directories(discovery_markers_check PRIVATE include)
target_link_libraries(discovery_markers_check PRIVATE raylib)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(discovery_markers_check PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Fluxo manual das mensagens e proteção contra confirmação acidental.
add_executable(battle_flow_check EXCLUDE_FROM_ALL tools/check_battle_flow.c ${INTRO_CHECK_SOURCES})
target_include_directories(battle_flow_check PRIVATE include)
target_link_libraries(battle_flow_check PRIVATE raylib)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(battle_flow_check PRIVATE -Wall -Wextra -Wpedantic)
endif()

# UI do desafio lógico com entrada real, rolagem e capturas nas resoluções da campanha.
add_executable(logic_setup_ui_check EXCLUDE_FROM_ALL tools/check_logic_setup_ui.c ${INTRO_CHECK_SOURCES})
target_include_directories(logic_setup_ui_check PRIVATE include)
target_link_libraries(logic_setup_ui_check PRIVATE raylib)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(logic_setup_ui_check PRIVATE -Wall -Wextra -Wpedantic)
endif()
