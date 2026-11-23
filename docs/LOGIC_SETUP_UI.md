# Modal de golpes lógicos

A placa perto do depósito abre abas livres por integrante da equipe. O título,
as abas, o card da planta e o rodapé permanecem fixos; a área direita alterna
entre pergunta e editor. Não há progresso linear ou fórmula atribuída pelo jogo.

## Referências e recursos

- `assets/ui/logic_setup/approved_reference_v1.png`: editor aprovado.
- `assets/ui/logic_setup/dropdown_reference_v1.png`: lista expandida aprovada.
- Moldura externa: `UiTheme.interaction_frame`, montada em nove partes.
- Plantas: sprites de `IntroAssets`, com recortes alfa e proporção preservada.
- Voltar/confirmar e cursor: recursos existentes de `SelectionUiAssets`.
- Tipografia: atlas Pixelify na resolução física, também durante a exploração
  enquanto o modal está aberto.
- Campos, abas, listas, foco e seleção: molduras em degraus e camadas de cores,
  sem novos PNGs de controles. A lista é desenhada por último, sobre o conteúdo.

A composição usa coordenadas lógicas de 1280 × 720, com modal em
`(56,38,1168,654)`, abas em y=151, card em `(100,232,255,365)` e rodapé em y=632.
As listas de condição têm sete opções de 32 pixels; encerram acima do rodapé.

## Estados e confirmação

`World.logic_move_mask` registra perguntas resolvidas. `Game.logic_setup`
mantém rascunho de fórmula/ataque, erro da pergunta, confirmação e alterações
pendentes por planta. Trocar de aba ou fechar não descarta esses dados.
`CampaignSession` recebe somente configurações confirmadas. A batalha lê essas
configurações; o desenho não modifica o combate nem a vida das plantas.

O ataque deve ser de dano. As duas condições devem ser diferentes. As opções
inválidas ficam cinza e não podem ser escolhidas. A marca verde indica a opção
atual; a borda dourada/seta indica o foco. ESC cancela uma lista antes de fechar
o modal; clicar fora fecha a lista sem ativar o controle atrás dela.

## Verificação

`intro_check` verifica interação com a placa, início por outra aba, erros e
acertos, rascunhos independentes, confirmação individual, configuração pendente
fora do combate, condições duplicadas, golpes de atributo, mouse, teclado,
reabertura e aplicação em encontros seguintes. As capturas de pergunta, editor
e listas ficam em `build/intro_preview/logic_*.png`, incluindo 1080p.

```powershell
cmake --build build --target plants_x_plants intro_check battle_check battle_screen_check name_entry_check
.\build\intro_check.exe
.\build\battle_check.exe
.\build\battle_screen_check.exe
.\build\name_entry_check.exe
```
