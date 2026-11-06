# Tela inicial e menu em pixel art

## Composição e recursos

O fluxo existente em `menu_screen` desenha uma composição fixa de 1280 × 720. O mascote agora possui repouso discreto, descrito em [MENU_MASCOT_IDLE.md](MENU_MASCOT_IDLE.md); o painel e os textos das opções mantêm posição e tamanho. O vento localizado da grama e das copas é descrito em [MENU_GRASS_WIND.md](MENU_GRASS_WIND.md). O logo flutua ±4 pixels em ciclo de três segundos; o aviso fica visível por 0,8 s e oculto por 0,4 s, sem mudar de posição. A seta pulsa entre aproximadamente 90–110% em um segundo, com centro fixo e dimensões arredondadas na grade de pixels. Um relógio visual com delta time mantém os ciclos contínuos entre título/menu, inclusive durante bloqueios de entrada. Essas animações não alteram hitboxes, seleção, disponibilidade ou ativação. O painel usa primitivas de desenho: fundo creme, borda verde em degraus e cursor triangular. As opções continuam Campanha, Versus local desabilitado e Sair.

As fontes permanecem intactas:

| Arquivo fornecido | Dimensão | Limites visíveis, alfa acima de 32 |
|---|---|---|
| `assets/sprites/menu_backround_v2.png` | 1672 × 941 | imagem opaca completa |
| `assets/sprites/logo_v2.png` | 1639 × 960 | x=76, y=28, largura=1481, altura=900 |
| `assets/sprites/menu_mascot.png` | 1312 × 1199 | x=218, y=101, largura=968, altura=994 |

O carregador prefere a grafia solicitada `menu_background_v2.png`, caso exista, e aceita `menu_backround_v2.png`, o nome efetivamente fornecido. Logo e mascote estático não exigem derivados ou limpeza de cores. O vento usa fundos derivados reconstruídos e atlas de grama/copas extraídos da fonte, carregados como pares indivisíveis; falha nas copas conserva árvores estáticas sobre o fundo da grama. O repouso usa um atlas separado, preparado somente a partir do mascote original. A carga calcula o recorte de alfa uma vez e descarta a imagem de CPU após criar a textura. Todos os recursos do menu usam `TEXTURE_FILTER_POINT`.

Na tela inicial, o logo tem altura visível de 324 pixels, tem posição central em y=28 e fica centralizado em x=640. O mascote tem altura de 240 pixels e vai de y=374 a y=614, sobre a clareira, com sombra pequena em degraus. O aviso é desenhado em código próximo à base. No menu, o logo passa instantaneamente para altura 244; o mascote conserva posição e escala. O painel fica à direita, separado do mascote. Escalas são uniformes; o fundo cobre a janela mantendo proporção.

## Entrada e integração

ENTER abre o menu, que aguarda a soltura das entradas antes de aceitar outra ação. ESC retorna à tela inicial. Setas pulam Versus local e alternam as opções disponíveis, com retorno ao começo/fim. Mouse seleciona ao mover e ativa somente ao pressionar e soltar na mesma opção. Soltar fora cancela. A confirmação por teclado conserva o feedback breve de cor de 0,12 s. Não há espera bloqueante.

Campanha continua entrando na seleção de treinadores por `game.c`, e a seleção entra na exploração existente. A proteção de entrada entre essas telas permanece. Os cartões e controles da seleção são formas raylib; seu fundo e retratos permanecem independentes do menu. A antiga textura de botões de madeira foi removida por não participar mais do desenho.

## Verificação

O alvo opcional usa o jogo e a raylib reais, com contexto OpenGL e eventos automatizados:

```powershell
cmake --build build --target menu_screen_check
.\build\menu_screen_check.exe
```

As capturas ficam em `build/menu_v2_preview/`, `build/menu_mascot_preview/` e `build/menu_grass_preview/`. A verificação isola o movimento do mascote congelando o relógio da grama; também cobre as 32 fases da vegetação, regiões permitidas, fallback indivisível, cenário/pés/sombra fixos, relógio contínuo entre título/menu, desenho sem mutação, ENTER mantido, ESC, navegação com setas, opção desabilitada, clique, cancelamento ao soltar fora, Campanha por teclado/mouse, seleção, exploração e retorno, Sair por teclado/mouse, fallback sem texturas e liberação de recursos.

Validação da etapa inicial estática em 05/11/2026: compilação e eventos automatizados passaram. Capturas OpenGL foram inspecionadas para proporção, transparência, nitidez pontual, nomes, painel e recortes. A captura da seleção foi comparada com a composição anterior: igualdade exata de pixels após a transferência dos botões. O teste atualizado compara capturas antes/depois de 30 quadros, permitindo movimento apenas na área do mascote acima dos pés. Não houve teste manual por uma pessoa.

`cmake --build build`, `git diff --check` e a regressão `selection_scene_check` também passaram, incluindo retorno pelo botão Voltar, reentrada e exploração com ambos os treinadores. A tela inicial foi observada na janela do executável. A injeção externa de ENTER não abriu o menu nessa janela; os testes de interação e as capturas do menu usaram eventos raylib e `game_draw` reais no verificador. Os textos inferiores têm margem adicional para reduzir sobreposição da barra de tarefas.

## Manutenção dos recursos

O menu mantém as fontes v2, a imagem aprovada do mascote, seu atlas de repouso e os derivados atuais da grama e das copas. A arte e o experimento de vento da apresentação anterior foram removidos após verificação de referências em código, CMake, ferramentas e documentação. Os recursos compartilhados usados pela seleção e pela exploração permanecem disponíveis.

A preparação atual do mascote usa Pillow, NumPy e SciPy em `tools/trainer_requirements.txt`, também necessários aos outros preparadores ativos. Não há dependência de OpenCV. Contatos, máscaras, GIFs e fixtures de teste ficam exclusivamente em `build/`, ignorado pelo Git. O vento atual é independente do experimento removido: usa apenas vegetação extraída do fundo v2 e as mesmas dependências de preparação.

As fases visuais da interface podem ser inspecionadas nas capturas OpenGL em `build/menu_ui_preview/`, geradas pelo mesmo verificador. As comparações isoladas de mascote/vento fixam o relógio da interface.
