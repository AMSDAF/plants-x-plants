# Central de relés — pacote industrial

Estado: **v0 integrada ao jogo**.

A referência aprovada está em `assets/ui/relay_panel/approved_reference_v1.png`.
Preserva composição, fonte, cards claros e cursor dos menus existentes, com aço
azulado e parafusos de cobre no lugar da madeira. O desafio continua em três
etapas independentes. A interface industrial substitui o painel provisório.

## Artes prontas

Todos os arquivos ficam em `assets/ui/relay_panel/`:

| Arquivo | Uso |
|---|---|
| `panel_frame_v1.png` | Moldura metálica vazia com centro marfim: modal, objetivo e cards, em nove partes. |
| `button_frame_v1.png` | Botões Voltar/Validar e placas de etapa; cantos preservados em nove partes. |
| `switch_on_v1.png` | Interruptor verde em cima e lâmpada verde, no mesmo sprite. |
| `switch_off_v1.png` | Interruptor vermelho embaixo e lâmpada vermelha, no mesmo sprite. |

O pacote foi gerado com o **ImageGen integrado**. Os prompts completos estão em
`generation_prompts_v1.json`. PNGs originais foram copiados sem edição,
redimensionamento ou limpeza destrutiva. A referência é uma quinta imagem,
destinada à consulta e não ao carregamento da interface.

`asset_manifest_v1.json` registra dimensões, modo, SHA-256, recortes de desenho
e insets. Há resíduos suaves de alfa baixo nas margens: os recortes foram
medidos com alfa >= 128 e excluem a área externa. Não recalcular pelo primeiro
pixel de alfa quase nulo. Os dois controles usam **o mesmo recorte fixo**,
`(542,303,429,409)`, e devem compartilhar destino e escala para manter o registro.
Usar filtro POINT e preservar a proporção dos controles.

## Composição integrada

`layout_spec_v1.json` registra uma composição lógica de 1280 × 720. Registra
modal, título, objetivo, etapa, cards e rodapé, conferidos no render real.
Manter os três cards nas mesmas posições em todas as etapas.
As placas superiores são indicadores de progresso, sem navegação para trás.

Para molduras, dividir centro e bordas: preservar a escala dos cantos e os
parafusos; estender somente as faixas retas e o centro. Aplicar verde-claro ao
centro ligado, âmbar à etapa atual e contorno de foco por cima, conservando o
aço. O botão neutro é compartilhado pelos estados; não há PNG com rótulo fixo.

Reutilizar Pixelify Sans e sua licença de `assets/fonts/`, o cursor âmbar de
`assets/ui/plant_selection/cursor_v1.png` e o cenário atual do viveiro escurecido.
Títulos, A/B/C, fórmulas, dicas, estado, teclas e tentativas são desenhados
dinamicamente com os atlas físicos. Não carregar outra imagem de fundo, não
gravar texto nos PNGs e não gerar um card diferente para cada letra.

`Game.relay_assets` possui as quatro texturas, carregadas/liberadas por
`relay_ui_load/unload`. `relay_ui.c` compõe as molduras em nove partes e os
controles, com fallback geométrico se faltar uma textura. A referência não é
carregada em tempo de execução. Fontes e cursor são emprestados de
`SelectionUiAssets`; os atlas são atualizados ao redimensionar com o painel aberto.

`world` continua validando os três desafios e controla o foco e a entrada.
`world_relay_bounds` e os limites dos botões são compartilhados entre entrada
e desenho. Os três cards ficam visíveis e interativos desde a primeira etapa. Clique ou
use 1/2/3 para alternar; esquerda/direita move o cursor e Espaço alterna o foco.
ENTER valida e ESC/Voltar fecha. As placas de etapa são somente indicadores.
O desenho não modifica o mundo ou o progresso.

## Validação

As quatro artes foram inspecionadas visualmente. O validador opcional verifica
arquivos, hashes, alfa, centro das molduras, recortes, insets, registro comum
dos estados, fonte/licença, cursor e posições de um a três cards.

```powershell
.\build\trainer_tools\Scripts\python.exe tools/validate_relay_ui_assets.py
```

`relay_ui_check` usa a mesma entrada, desenhos e recursos do jogo. Verifica
mouse/teclado, relés indisponíveis, erro sem retroceder etapa, retomada, conclusão,
720p/1080p/4:3, recursos ausentes e desenho imutável. As capturas ficam em
`build/relay_ui_preview/`. `name_entry_check` percorre a campanha completa e
confere a central dentro do viveiro e a liberação de seus recursos.

```powershell
cmake --build build --target plants_x_plants relay_ui_check name_entry_check
.\build\relay_ui_check.exe
.\build\name_entry_check.exe
```
