# CMakeLists.txt
# Configura a compilação em C11 e integra a biblioteca raylib.
# Baixa uma versão fixa da dependência durante a configuração inicial.

cmake_minimum_required(VERSION 3.24)

project(plants_x_plants VERSION 0.1.0 LANGUAGES C)

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

add_executable(plants_x_plants
    src/main.c
    src/game.c
    src/plant.c
    src/plant_data.c
    src/battle.c
    src/ai.c
    src/battle_screen.c
    src/player.c
    src/world.c
    src/trainer_select_screen.c
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
    src/game.c
    src/menu_screen.c
    src/trainer_select_screen.c
    src/trainer_background.c
    src/player.c
    src/world.c
)
target_include_directories(selection_scene_check PRIVATE include)
target_link_libraries(selection_scene_check PRIVATE raylib)

# Verificação opcional da composição, mascote e navegação do menu.
add_executable(menu_screen_check EXCLUDE_FROM_ALL
    tools/check_menu_screen.c
    src/game.c
    src/menu_screen.c
    src/trainer_select_screen.c
    src/trainer_background.c
    src/player.c
    src/world.c
)
target_include_directories(menu_screen_check PRIVATE include)
target_link_libraries(menu_screen_check PRIVATE raylib)

add_executable(trainer_background_check EXCLUDE_FROM_ALL
    tools/check_trainer_background.c
    src/game.c src/menu_screen.c src/trainer_select_screen.c
    src/trainer_background.c src/player.c src/world.c
)
target_include_directories(trainer_background_check PRIVATE include)
target_link_libraries(trainer_background_check PRIVATE raylib)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(trainer_background_check PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(selection_scene_check PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(menu_screen_check PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(plants_x_plants PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
