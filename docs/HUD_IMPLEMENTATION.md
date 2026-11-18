# HUD creme e tipografia legível

Implementa a direção escolhida em [HUD_REDESIGN_PLAN.md](HUD_REDESIGN_PLAN.md):
painéis creme com texto verde-escuro, interação no centro inferior e seta
apontando para o alvo. As imagens de conceito e seus prompts foram removidos
após a integração; a direção visual está registrada no plano.

## Comportamento

O objetivo e o local ficam no canto superior esquerdo; as pistas, no superior
direito. O rodapé mostra um único painel, centralizado em x = 640, com margem
inferior de 24 pixels lógicos. Sua largura varia entre 400 e 640; a altura é
76 ou 104, conforme a quebra de linha. A tecla E aparece somente se a ação
estiver disponível. Bloqueios e pistas registradas mostram seu estado e a
explicação, sem sugerir uma ação indisponível.

A seta usa o PNG transparente `assets/ui/interaction_chevron_pixel.png`, gerado
com a ferramenta integrada de imagens. É um chevron para baixo, sem haste,
em pixel art, com preenchimento marfim, detalhe dourado e contorno escuro,
em 18 × 12 pixels lógicos. O prompt está em
`assets/ui/interaction_chevron_pixel_prompt.json`.
Ela oscila suavemente 4 pixels na vertical, em ciclos de 1,4 segundo.
Na importação, a arte é amostrada na grade 18 × 12, com paleta opaca e
contorno de um pixel. A filtragem é pontual, sem blur ou halo. A posição usa
pixels inteiros; o enquadramento reserva todo o percurso antes da
oscilação, evitando que a seta suma e reapareça perto da borda da tela.
O relógio avança na atualização da exploração; o desenho apenas consulta
seu valor. Sem a textura, desenha um chevron de reserva, também sem haste.
Sua âncora visual é convertida pela câmera, mas seu tamanho não acompanha o
zoom do mundo. Cada objeto tem uma âncora no centro superior da sua arte,
separada das coordenadas físicas de alcance e colisão. Quando o personagem
ocupa a área da seta, sua âncora sobe gradualmente, preservando o alinhamento
horizontal com o objeto. Não há texto de interação junto ao alvo, círculo
amarelo nem marca desenhada no chão.

Sem alvo, o painel e a seta somem. Diálogo, central de relés e resultado
substituem o HUD de exploração. As condições da ponte e do viveiro também
usam painéis creme. O rodapé permanente de controles e as etiquetas F6/F8
foram retirados da cena; os atalhos continuam funcionando. Não foi criado
o menu de ajuda por Tab sugerido em uma das alternativas visuais.

## Responsabilidades

- `UiTheme`, pertencente a `Game`, carrega os dois pesos de Atkinson e o chevron uma
  vez e libera os recursos no encerramento. Guarda também o relógio visual
  do marcador e é passado explicitamente às funções de desenho.
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

O HUD de exploração, os diálogos e a central usam Atkinson Hyperlegible Next,
priorizando a leitura.
As molduras creme têm contorno duplo, cantos em degraus e sombra sólida;
a tecla E também usa uma caixa quadrada. Não há arredondamento suave.
As demais telas usam Atkinson Hyperlegible Next Regular e Semibold. Os TTFs estáticos,
a origem e a licença SIL Open Font License estão em `assets/fonts/`.
O atlas de 96 pixels tem acentos latinos e a pontuação usada na interface,
com filtro bilinear. Todas as telas utilizam os mesmos auxiliares de fonte
e medição, com a mesma fonte legível também na exploração.
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

Os alvos existentes `selection_scene_check` e
`menu_screen_check` também foram adaptados para os parâmetros explícitos
de tipografia. As funções que desenham isoladamente podem receber `NULL`
para usar a fonte de reserva; `game_draw` sempre recebe os recursos de `Game`.
