# Experimento de vento na vegetacao real

A cena usa dez regioes extraidas de `menu_background.png` (1672 x 941).
A imagem original permanece intacta. O gerador cria um unico fundo reconstruido
e um atlas compartilhado; troncos, rochas, chao, galhos principais e clareira
nao recebem deformacao. A area central da UI permanece sem vegetacao animada.

## Regioes e movimento

Coordenadas e amplitudes em pixels da fonte. O ponto de fixacao fica imovel;
o deslocamento cresce na direcao da ponta. A amplitude indicada e o limite
do sinal antes do arredondamento por vizinho mais proximo.

| Regiao | Material | Fixacao | Ponta | Amplitude | Fase (rad) |
|---|---|---|---|---|---|
| left_fern | Fronde do plano intermediario esquerdo | (460, 291) | (511, 284) | 3,5 | 0,90 |
| right_fern | Fronde pequena junto as rochas da direita | (1296, 483) | (1301, 457) | 3,0 | 3,80 |
| left_canopy | Ponta de folha da copa esquerda | (218, 23) | (215, 56) | 5,0 | 0,00 |
| right_canopy | Folha pendente da copa direita | (1340, 43) | (1322, 100) | 5,5 | 3,30 |
| left_moss | Franja adjacente ao musgo aprovado | (273, 182) | (274, 220) | 4,5 | 0,35 |
| left | Musgo esquerdo aprovado | (314, 198) | (315, 251) | 4,5 | 0,00 |
| right | Musgo direito aprovado | (1370, 128) | (1371, 173) | 4,5 | 2,35 |
| upper_twig | Folha laranja na ponta do galho superior | (701, 78) | (734, 99) | 5,5 | 1,40 |
| left_twig | Folha e haste fina na lateral esquerda | (313, 288) | (374, 286) | 5,0 | 0,55 |
| right_twig | Ponta flexivel da folha lateral direita | (1460, 282) | (1418, 326) | 5,5 | 3,90 |

Ciclo compartilhado de **64 quadros a 12 FPS**: aproximadamente **5,33 s**.
As fases sugerem a passagem da brisa da esquerda para a direita. Folhas e
frondes combinam a onda lenta com harmonicos de frequencia dupla e tripla;
o musgo novo responde principalmente a onda lenta. Os dois musgos aprovados
conservam suas mascaras, amplitude, fase e periodo de 2,67 s, agora com o dobro
da frequencia de amostragem. Todos os sinais sao periodicos; nao se acrescenta
um quadro final repetindo o inicial.

O deslocamento de ponta no tamanho de exibicao fica geralmente em torno de
2 a 4 pixels; a amostragem discreta pode manter uma ponta no mesmo pixel por
alguns quadros perto de uma reversao. Nenhuma regiao usa translacao rigida
da cena inteira. As primeiras sete unidades ao longo do eixo de fixacao
ficam sem deformacao; no musgo aprovado, sao sete linhas.

## Atlas, coordenadas e memoria

`canopy_atlas.png` mede **2048 x 1792**, RGBA8: **14 MiB** sem mipmaps.
Cada regiao ocupa um bloco de oito colunas por oito linhas de quadros.
Os quadros variam de 48 a 96 pixels de largura e de 48 a 88 de altura.
Os dois musgos aprovados mantem seus quadros de 64 x 80 pixels.
`metadata.json` registra todos os tamanhos, selecoes, fases e origens no atlas.
O gerador escreve tambem `include/menu_wind_test_data.h`; nao editar esse
arquivo manualmente.

`background.png` usa RGB8 em 1672 x 941: aproximadamente **4,50 MiB**.
Total logico das texturas experimentais: **18,50 MiB**. Reservar cerca de
**20 MiB** se o driver armazenar o fundo RGB como RGBA. Isso exclui buffers
internos do driver e a UI existente (logo e botoes, cerca de 12 MiB).
O runtime nao carrega o fundo original, os ramos decorativos ou as sheets
antigas quando o conjunto experimental esta valido.

A ordem do descritor e a ordem de desenho: frondes distantes, copa, musgo,
pontas proximas. As mascaras e seus envelopes completos sao disjuntos;
qualquer intersecao faz a geracao falhar, em vez de renderizar vegetacao em
duplicidade. A reconstrucao considera a uniao de todas as mascaras numa unica
passagem. Somente os pixels removidos mudam no fundo experimental.

## Reproducao e previas

Dependencias apenas de desenvolvimento em `tools/menu_wind_requirements.txt`.
Ambiente validado com Python 3.14:

```powershell
python -m venv build/wind-venv
build/wind-venv/Scripts/python.exe -m pip install -r tools/menu_wind_requirements.txt
build/wind-venv/Scripts/python.exe tools/generate_menu_wind_test.py
```

O gerador verifica hash da fonte, pixels inalterados fora das mascaras,
recomposicao estatica identica a fonte, envelopes disjuntos, margem contra
clipping, fixacoes imoveis e periodicidade. Os recortes e as mascaras sao
diagnosticos reproduziveis gerados a partir do fundo original, nao entradas
necessarias nem assets de runtime. Ficam em `build/menu_wind_preview/`
(ou no diretorio indicado por `--preview-dir`). Em `assets/sprites/menu_wind_test/`
permanecem o fundo reconstruido, o atlas, os metadados e esta documentacao.
O atlas substitui as sheets antigas dos dois musgos.

Em `build/menu_wind_preview/`:
- `*_extracted.png` e `*_mask.png`: recortes transparentes e mascaras das dez regioes.
- `reconstruction_mask.png`: uniao das mascaras utilizadas na reconstrucao.
- `menu_wind.gif`: ciclo completo em 1280 x 720, com o sombreamento do fundo,
  sem logo, botoes ou particulas. A paleta GIF e reduzida; os PNGs mantem as cores.
- `scene_00.png`, `scene_16.png`, `scene_32.png`, `scene_48.png`, `scene_63.png`:
  pontos do ciclo em cores completas.
- `contact.png`: todas as regioes, diversas fases e a passagem 62, 63, 0, 1.
- `regions.png`: selecoes identificadas sobre a fonte.

## Integracao e limites

`MENU_WIND_TEST_ENABLED` em `src/menu_screen.c` permanece em **1**.
Mudar para **0** e recompilar restaura o fundo original e `menu_foliage.png`.
Tambem aceita `-DMENU_WIND_TEST_ENABLED=0`. Uma textura experimental ausente
ou com dimensoes incorretas descarta o conjunto inteiro e carrega a cena
original. O conjunto parcial e liberado imediatamente; o restante, no
encerramento.

Todas as camadas usam a mesma transformacao cover do fundo. O sombreamento
e aplicado uma vez, antes das particulas e UI. O tempo avanca no update e
nao reinicia nas transicoes; o desenho somente le estado.

A reconstrucao Telea de raio 4 aproxima texturas vizinhas nas silhuetas.
Nao recupera detalhes originalmente ocultos. As selecoes foram mantidas
pequenas para evitar grandes preenchimentos; ampliacoes podem revelar
suavizacao local e degraus nas bordas. As frondes distantes tem pouco contraste
e movimento menor por perspectiva. O teste distribui vento pela vegetacao
periferica, sem deformar a copa inteira nem os galhos estruturais.
