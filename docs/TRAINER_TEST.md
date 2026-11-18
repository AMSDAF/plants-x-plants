# Treinadores da campanha

## Recursos mantidos

Somente quatro PNGs de personagem permanecem em `assets/sprites/trainers/`:

| Arquivo | Uso |
|---|---|
| `male_botanical.png` | Explorador na exploração. |
| `female_botanical.png` | Exploradora na exploração. |
| `male_portrait_idle.png` | Repouso animado masculino na seleção e entrada de nome. |
| `female_portrait_idle.png` | Repouso animado feminino na seleção e entrada de nome. |

Os atlas botânicos têm 768 × 1024 pixels: três poses por direção em células
de 256 × 256. As linhas são sul, oeste, leste e norte; pés na linha 240.
A escala de 0,23 e o filtro pontual preservam o tamanho aparente, inclusive
nos interiores com zoom. Os ciclos e o movimento da exploração foram mantidos.
Sem atlas válido, um marcador geométrico preserva a possibilidade de jogar.

Os retratos usam quatro poses, tempos e limites corporais próprios; consulte
[TRAINER_SELECTION.md](TRAINER_SELECTION.md). As fontes e variantes legadas,
retratos estáticos, halos separados e experimento F6 foram removidos.
Os quatro PNGs mantidos não foram reprocessados.

## Preparação opcional

`tools/prepare_botanical_trainers.py` aceita duas folhas externas para refazer
os atlas botânicos. As fontes não acompanham o repositório:

```powershell
.\build\trainer_tools\Scripts\python.exe tools/prepare_botanical_trainers.py folha_masculina.png folha_feminina.png
```

As folhas devem ter três poses por linha, na ordem frente, costas, esquerda e
direita. Pillow, NumPy e SciPy estão em `tools/trainer_requirements.txt`.
Não há gerador de retratos de repouso com fontes incluídas no repositório.

## Verificação

```powershell
cmake --build build --target selection_scene_check name_entry_check
.\build\selection_scene_check.exe
.\build\name_entry_check.exe
```

O check de seleção verifica movimento nas quatro direções, parada, proporção
e tamanho aparente nos interiores para ambos os treinadores, além dos retratos.

Validação executada nesta limpeza: build do jogo e dos dois checks concluído;
`selection_scene_check` e `name_entry_check` passaram. Os hashes dos quatro
PNGs foram preservados e as capturas reais mostram quatro poses distintas
para cada retrato. Não houve alteração das regras de combate.
