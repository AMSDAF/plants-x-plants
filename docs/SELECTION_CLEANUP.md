# Auditoria de recursos: menu e seleção

## Escopo e rastreamento

A árvore de trabalho estava limpa no início. Foram conferidos os carregadores e
fallbacks C, caminhos montados com `TextFormat`, metadados gerados, CMake,
scripts de preparação, checks e documentação. Nenhum arquivo foi removido
apenas por não aparecer literalmente no C. A limpeza não redesenha a interface.

## Remoções confirmadas

21 arquivos, aproximadamente 17,2 MiB. A antiga floresta continuava carregada e
atualizada, mas seu desenho já havia sido substituído por `trainer_background`.
Os cartões e controles atuais usam formas raylib. Os atlas procedurais de
respiração dos retratos detalhados não eram mais carregados.

| Arquivo removido | Motivo |
|---|---|
| `src/forest_scene.c` | Código/metadados exclusivos da floresta aposentada. |
| `include/forest_scene.h` | Código/metadados exclusivos da floresta aposentada. |
| `include/selection_scene_data.h` | Código/metadados exclusivos da floresta aposentada. |
| `tools/prepare_selection_scene.py` | Gerador exclusivo da antiga floresta/moldura e seus sete derivados. |
| `assets/sprites/menu_buttons.png` | Botões de madeira substituídos por formas geométricas. |
| `assets/sprites/trainers/male_portrait_breath.png` | Respiração procedural antiga, sem consumidor; repousos atuais usam poses desenhadas. |
| `assets/sprites/trainers/female_portrait_breath.png` | Respiração procedural antiga, sem consumidor; repousos atuais usam poses desenhadas. |
| `assets/sprites/background_base.png` | Fonte exclusiva do gerador antigo; nenhum gerador ou tela atual depende dela. |
| `assets/sprites/tree_trunks.png` | Fonte exclusiva do gerador antigo; nenhum gerador ou tela atual depende dela. |
| `assets/sprites/portrait_frame.png` | Fonte exclusiva do gerador antigo; nenhum gerador ou tela atual depende dela. |
| `assets/sprites/clouds.png` | Fonte exclusiva do gerador antigo; nenhum gerador ou tela atual depende dela. |
| `assets/sprites/canopies.png` | Fonte exclusiva do gerador antigo; nenhum gerador ou tela atual depende dela. |
| `assets/sprites/ground_foliage.png` | Fonte exclusiva do gerador antigo; nenhum gerador ou tela atual depende dela. |
| `assets/sprites/leaf_particles.png` | Fonte exclusiva do gerador antigo; nenhum gerador ou tela atual depende dela. |
| `assets/sprites/selection/prepared/background_base.png` | Derivado exclusivo da floresta/moldura aposentada. |
| `assets/sprites/selection/prepared/tree_trunks.png` | Derivado exclusivo da floresta/moldura aposentada. |
| `assets/sprites/selection/prepared/portrait_frame.png` | Derivado exclusivo da floresta/moldura aposentada. |
| `assets/sprites/selection/prepared/clouds.png` | Derivado exclusivo da floresta/moldura aposentada. |
| `assets/sprites/selection/prepared/canopies.png` | Derivado exclusivo da floresta/moldura aposentada. |
| `assets/sprites/selection/prepared/ground_foliage.png` | Derivado exclusivo da floresta/moldura aposentada. |
| `assets/sprites/selection/prepared/leaf_particles.png` | Derivado exclusivo da floresta/moldura aposentada. |

Também saíram campos/carga/liberação da floresta, moldura e botões, o
parâmetro de textura de botões sem uso, a interpolação `emphasis` que não era
lida pelo desenho, e as entradas correspondentes do CMake. O gerador
`prepare_trainer_portraits.py` mantém somente a preparação estática: sem elipses,
máscaras, deformação ou atlas de respiração aposentados. Também saiu o modo
`--check-runtime` de `prepare_selection_portraits_v2.py`, cuja premissa de
ambos os retratos estáticos não corresponde mais aos dois repousos atuais.

## Preservações e dúvidas

- Menu: fonte `menu_backround_v2.png` (grafia fornecida), logo, mascote estático,
  atlas de repouso, fundos intermediários e atlas de grama/copas. A alternativa
  `menu_background_v2.png` é opcional; sua ausência não torna a fonte fornecida obsoleta.
- Seleção: fonte completa de fundo, base/atlas derivados, fonte do par v2,
  fontes de repouso, atlas/halos separados, retratos v2 estáticos e metadados.
- Fallbacks detalhados `trainers/{male,female}_portrait.png`, suas fontes
  `trainer_{male,female}_portrait.png` e o gerador estático continuam ativos.
  Os checks exercitam a carga tanto do derivado quanto da fonte em diretórios isolados.
- Sprites, folhas e fontes da exploração, inclusive experimento de seis poses,
  foram preservados. `trainer_male_idle.png` e `trainer_female_idle.png` não têm
  consumidor confirmado neste fluxo, mas pertencem ao material de movimento:
  ficam mantidos para uma auditoria própria da exploração, fora deste escopo.
- Pillow, NumPy e SciPy continuam necessários aos geradores ativos; nenhuma
  dependência externa pôde ser removida com segurança.
- `preview_selection_scene.py` continua útil para as 48 capturas atuais. O
  trecho de seis segundos é diagnóstico, não um ciclo completo da cena.
- Prévias, relatórios, caches, fixtures e binários ficam em `build/` ou
  `__pycache__/`, já ignorados. Nenhuma ampliação das regras de ignore é necessária.

## Verificação

A conferência de hashes cobre os 34 assets remanescentes, inclusive fontes e
fallbacks: todos devem continuar byte a byte idênticos. Os 24 caminhos PNG de
execução resolvem, além da alternativa opcional de grafia do fundo do menu.
As dimensões/retângulos dos atlas são exercitados pelos checks OpenGL.

```powershell
cmake --build build
cmake --build build --target menu_screen_check selection_scene_check trainer_background_check
.\build\menu_screen_check.exe
.\build\selection_scene_check.exe
.\build\trainer_background_check.exe
.\build\trainer_tools\Scripts\python tools/prepare_male_portrait_idle.py --runtime-preview
.\build\trainer_tools\Scripts\python tools/prepare_male_portrait_idle.py --character female --runtime-preview
.\build\trainer_tools\Scripts\python tools/preview_trainer_selection_background.py
git diff --check
```

A auditoria usa testes automatizados e capturas; não acrescenta aprovação
visual manual às animações anteriormente aprovadas pelo usuário. Diagnósticos
da limpeza ficam em `build/selection_cleanup/`. As alterações ficam sem commit.

Resultado executado: build e `git diff --check` passaram; `menu_screen_check`,
`selection_scene_check` e `trainer_background_check` passaram. As prévias dos
dois repousos, do fundo completo e o trecho da seleção foram regenerados com
sucesso. O gerador estático de fallback e o gerador v2 reproduziram seus PNGs
sem mudar nenhum dos 34 arquivos de assets remanescentes. Os fixtures verificam
os fallbacks detalhados preparado e original. O único arquivo novo não rastreado
da limpeza é este relatório; o índice permanece vazio.
