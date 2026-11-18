# Seleção de exploradores

A seleção e a entrada de nome utilizam diretamente os atlas
`assets/sprites/trainers/male_portrait_idle.png` e `female_portrait_idle.png`.
Não há prioridade para retratos estáticos nem dependência de halos separados.
Os PNGs são preservados sem alteração.

Cada atlas contém quatro poses: neutro, inspiração, piscar e relaxado.
Os limites corporais dos headers `male_portrait_idle_data.h` e
`female_portrait_idle_data.h` determinam a escala uniforme e os pés em y=526,
com altura corporal de 360 pixels no cartão normal. O masculino usa células
352 × 656 e ciclo de 4,70 s; o feminino, células 384 × 704 e ciclo de 5,30 s,
com fase inicial de 2,20 s. Os relógios continuam durante bloqueios de entrada.

Cartões de 336 × 480, bordas, cantos e controles são desenhados em código.
Setas/A-D e mouse escolhem o personagem; ENTER/Confirmar abre a entrada de nome;
ESC/Voltar retorna à tela inicial. F7 mostra somente o cenário. Essas entradas permanecem iguais.
Um atlas ausente ou de dimensão inválida exibe “Retrato indisponivel” no cartão.

Somente os quatro PNGs de personagem descritos em [TRAINER_TEST.md](TRAINER_TEST.md)
são mantidos. Fontes e variantes anteriores, halos e seus geradores exclusivos
foram removidos; os retratos atuais não são regeneráveis com fontes do repositório.
O fundo da seleção permanece independente desses recursos.

## Validação

```powershell
cmake --build build --target selection_scene_check name_entry_check
.\build\selection_scene_check.exe
.\build\name_entry_check.exe
```

O check de seleção cobre dimensões, falha parcial do carregador, limites de poses,
retorno dos ciclos, teclado/mouse, transições, desenho imutável, exploração com
os dois personagens e descarregamento. Capturas de cada pose ficam em
`build/male_portrait_idle_preview/` e `build/female_portrait_idle_preview/`.
As capturas da seleção ficam em `build/selection_preview/`.

Validação executada nesta limpeza: build do jogo e dos dois checks concluído;
`selection_scene_check` e `name_entry_check` passaram. Os hashes dos quatro
PNGs foram preservados e as capturas reais mostram quatro poses distintas
para cada retrato. Não houve alteração das regras de combate.
