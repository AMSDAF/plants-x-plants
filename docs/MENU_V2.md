# Tela inicial em pixel art

ENTER na tela inicial abre diretamente a seleção de personagem. Não há painel
de opções nem escolha de modo. ESC na seleção ou exploração retorna à tela
inicial; ESC na tela inicial encerra. Após cada transição, o jogo aguarda a
soltura de teclas e mouse, mantendo a janela e as animações responsivas.

## Composição e recursos

A composição usa coordenadas lógicas de 1280 × 720 e renderização na resolução
física do viewport. O fundo mantém proporção, com cobertura da tela; logo e
mascote usam limites visíveis do alfa, escala uniforme e filtro pontual.
O logo flutua ±4 pixels em três segundos; o aviso aparece durante 0,8 s e
some durante 0,4 s. O mascote usa oito quadros a 6 FPS, com pés e sombra fixos.
Os relógios continuam durante bloqueios de entrada.

O carregador prefere `menu_background_v2.png`, quando disponível, e aceita
`menu_backround_v2.png`, a grafia da fonte fornecida. Logo, mascote estático,
atlas de repouso e atlas de vegetação são preservados. Não há fonte de arte
nova nem reprocessamento dos PNGs nesta mudança de fluxo.

A grama e as copas usam pares de fundo reconstruído e atlas. Falha da grama
restaura o fundo completo; falha das copas conserva árvores estáticas e grama
animada. Atlas de mascote ausente ou inválido usa a imagem estática. A carga
valida dimensões e cada textura é liberada no encerramento.

Consulte [MENU_MASCOT_IDLE.md](MENU_MASCOT_IDLE.md) e
[MENU_GRASS_WIND.md](MENU_GRASS_WIND.md) para os limites dos recursos.

## Verificação

```powershell
cmake --build build --target menu_screen_check
.\build\menu_screen_check.exe
```

O check usa o jogo, OpenGL e eventos raylib reais. Cobre entrada direta na
seleção, ENTER mantido sem confirmar o personagem, retorno por ESC sem fechar
por entrada herdada, exploração e reentrada, encerramento por ESC na tela
inicial, ausência dos antigos cliques de opções, desenho imutável e liberação.
Também compara as oito fases do mascote e as 32 fases do vento, verificando
pés fixos, mudanças apenas nas regiões previstas e falhas parciais dos atlas.

Capturas ficam em `build/menu_v2_preview/`, `build/menu_mascot_preview/`,
`build/menu_grass_preview/`, `build/menu_tree_preview/` e `build/menu_ui_preview/`.
Os geradores mantêm Pillow, NumPy e SciPy em `tools/trainer_requirements.txt`;
Python é opcional para jogar. Prévias e fixtures permanecem ignorados pelo Git.
