# HUD creme e tipografia legível

Implementa a direção escolhida em [HUD_REDESIGN_PLAN.md](HUD_REDESIGN_PLAN.md):
painéis creme com texto verde-escuro, interação no centro inferior e seta
apontando para o alvo. A referência visual é
`assets/ui/hud_concepts/cream_arrow_v1.png`.

## Comportamento

O objetivo e o local ficam no canto superior esquerdo; as pistas, no superior
direito. O rodapé mostra um único painel, centralizado em x = 640, com margem
inferior de 24 pixels lógicos. Sua largura varia entre 400 e 640; a altura é
76 ou 104, conforme a quebra de linha. A tecla E aparece somente se a ação
estiver disponível. Bloqueios e pistas registradas mostram seu estado e a
explicação, sem sugerir uma ação indisponível.

A seta tem 20 × 26 pixels lógicos, com preenchimento creme e contorno escuro.
Sua âncora visual é convertida pela câmera, mas seu tamanho não acompanha o
zoom do mundo. Portas, painel, escotilha e NPCs usam deslocamentos visuais
próprios, sem alterar as coordenadas físicas da interação. Não há texto de
interação junto ao alvo, círculo amarelo nem marca desenhada no chão.

Sem alvo, o painel e a seta somem. Diálogo, central de relés e resultado
substituem o HUD de exploração. As condições da ponte e do viveiro também
usam painéis creme. O rodapé permanente de controles e as etiquetas F6/F8
foram retirados da cena; os atalhos continuam funcionando. Não foi criado
o menu de ajuda por Tab sugerido em uma das alternativas visuais.

## Responsabilidades

- `UiTheme`, pertencente a `Game`, carrega Regular e Semibold uma vez e libera
  os recursos no encerramento. É passado explicitamente às funções de desenho.
- `ui_theme.c` centraliza fonte, medidas, painéis e quebra de linha UTF-8.
- `exploration_hud.c` calcula a geometria e desenha objetivo, pistas, painel
  inferior e seta a partir de dados somente de leitura.
- `world_interaction_view` fornece contexto, mensagem, estado e âncoras.
  Reutiliza `nearby_object`, incluindo alcance e prioridade existentes.
- `interaction_state` é compartilhado pela apresentação e por `world_interact`,
  mantendo a tecla E e a instrução ligadas ao mesmo alvo e disponibilidade.
- `world.c` mantém regras, portas, colisões e progressão. O desenho não executa
  ações, avança relógios ou modifica o estado do jogo.

Textos e contadores são desenhados em tempo de execução. As imagens dos
conceitos não são coladas na tela nem usadas como textura de texto.

## Fonte

O jogo usa Atkinson Hyperlegible Next Regular e Semibold. Os TTFs estáticos,
a origem e a licença SIL Open Font License estão em `assets/fonts/`.
O atlas de 96 pixels tem acentos latinos e a pontuação usada na interface,
com filtro bilinear. Menu, seleção, entrada de nome, exploração, diálogos,
central e batalha utilizam os mesmos auxiliares de fonte e medição.
Logos, cenários e personagens foram preservados.

Na ausência de um peso, usa o outro; na ausência de ambos, usa a fonte padrão
da raylib. Não há alteração da fonte padrão interna nem estado global mutável
para compartilhar a tipografia. O canvas continua renderizando na resolução
física da tela, com coordenadas lógicas de 1280 × 720.

## Verificação

Execute a partir da raiz:

```powershell
cmake --build build --target plants_x_plants exploration_hud_check name_entry_check game_canvas_check
.\build\exploration_hud_check.exe
.\build\name_entry_check.exe
.\build\game_canvas_check.exe
```

`exploration_hud_check` cobre fontes e acentos reais, estado disponível,
bloqueado e registrado, ausência de alvo, suspensão por diálogo/central,
seleção consistente com E, bloqueio sem efeitos, centralização, dimensão da
seta independente do zoom, retorno à fonte de reserva e desenho imutável.
Captura a interface real em 720p, 1080p e 1440p em `build/hud_preview/`.

As capturas incluem o menu, a seleção, o nome, o exterior e os estados da
escotilha. `name_entry_check` percorre a campanha e verifica foco, acentos,
diálogos, portas, colisões, ponte, guardiões e conclusão. `game_canvas_check`
verifica resolução, mouse, recorte e a regressão da borda horizontal.

Os alvos existentes `versus_screen_check`, `selection_scene_check` e
`menu_screen_check` também foram adaptados para os parâmetros explícitos
de tipografia. As funções que desenham isoladamente podem receber `NULL`
para usar a fonte de reserva; `game_draw` sempre recebe os recursos de `Game`.
