# Seleção de personagem e nome — interface botânica

Estado: **assets integrados nas duas telas**.

A direção aprovada está em `assets/ui/trainer_setup/approved_reference_v1.png`.
As duas telas seguem a seleção de plantas: madeira alaranjada, folhas verdes,
pergaminho creme, Pixelify Sans e fundo animado com escurecimento uniforme.

## Peças novas

| Arquivo | Uso |
|---|---|
| `portrait_normal_v1.png` | Cartão vertical vazio, creme, sem nome ou retrato gravado. Serve aos dois personagens e à tela do nome. |
| `portrait_selected_v1.png` | Mesmo cartão com interior verde-claro e contorno dourado incorporado. |
| `name_panel_v1.png` | Painel horizontal vazio para rótulo, campo, contador e instrução. |
| `asset_manifest_v1.json` | Dimensões, transparência, recortes e hashes dos arquivos. |
| `layout_spec_v1.json` | Posições das telas em 1280 × 720, tipografia, estados e comportamento a preservar. |
| `generation_prompts_v1.json` | Prompts completos das três gerações, realizadas com ImageGen integrado. |
| `validation_v1.json` | Conferência dos arquivos e dos recortes. |

Os originais gerados foram preservados, sem edição ou redimensionamento.
Normal e selecionado têm um pixel de diferença na altura do canvas da geração.
Por isso, usar o **mesmo recorte explícito e a mesma origem** do manifesto em
ambos: não reenquadrar cada estado pelo seu alfa. O contorno dourado está
incluído no recorte; resíduos quase transparentes externos ficam de fora.
O enquadramento preserva a proporção. O registro dos estados foi conferido
nas capturas reais das duas seleções.

## Reaproveitamento

Não gerar novamente os botões, a seta, a fonte, o cenário ou os personagens.
Reutilizar `footer_button_v1.png` e `cursor_v1.png` da seleção de plantas, com os
recortes já registrados no seu manifesto. Os botões continuam em nove partes.
Usar os atlas animados `male_portrait_idle.png` e `female_portrait_idle.png`
atuais, preservando proporção, altura e pés alinhados.

Campo de nome, cursor de edição, placa do nome do personagem e marcadores de
progresso são formas simples desenhadas pela interface. Não precisam de novas
texturas. Todos os textos devem continuar dinâmicos, em Pixelify Sans, com
atlas no tamanho físico necessário para evitar a perda de nitidez já corrigida
na seleção do time. Os textos sintetizados na referência não são fonte.

## Montagem no jogo

O JSON propõe cartões lado a lado na seleção e cartão à esquerda com painel à
direita no nome. Títulos e rodapés seguem a altura e os botões atuais do prólogo.
As coordenadas traduzem a imagem aprovada e foram conferidas no render real.
A barrinha reúne seis etapas: personagem, nome, carta, aceite, equipe e viagem.
O mesmo desenho atende às duas telas e ao prólogo. Os segmentos laranja incluem
a etapa atual; ao retornar, o indicador acompanha a tela, sem reiniciar na carta.

`TrainerSelectAssets.ui` possui as cinco texturas (três novas e duas reutilizadas)
e sete atlas Pixelify nos tamanhos lógicos 20, 22, 24, 28, 30, 36 e 72. Os recursos
são carregados e liberados junto da seleção. `game_update` atualiza os atlas para
a resolução física, fora do desenho; tamanhos iguais reutilizam os atlas.
As molduras preservam a proporção e o recorte; os botões usam nove partes.
`selection_ui` compartilha o cabeçalho, botões, textos e molduras entre as telas.
Sem uma textura, oferece formas geométricas; sem atlas próprio, usa a fonte de
reserva da interface. A prévia F7 continua mostrando o fundo original.

Foram preservados seleção por teclado/mouse, confirmação separada, bloqueio de entradas
herdadas, ESC/Voltar, UTF-8 com até 12 codepoints, TAB/SHIFT+TAB e transição
reversível entre personagem e nome. Confirmar nome inválido continua bloqueado.
Textos longos no campo deslocam horizontalmente para manter o cursor visível.
As regras da campanha, plantas e combate continuam nos módulos existentes.

## Validação

As três artes foram inspecionadas visualmente. Dimensões, modo RGBA, alfa externo,
centro opaco, limites dos recortes, hashes e referências a recursos existentes
foram verificados. O relatório JSON registra os resultados.

Os alvos de verificação usam os módulos reais, eventos raylib e capturas OpenGL:

```powershell
cmake --build build --target plants_x_plants intro_check selection_scene_check name_entry_check
.\build\intro_check.exe
.\build\selection_scene_check.exe
.\build\name_entry_check.exe --setup-only
```

`--setup-only` verifica nome e transições sem repetir a campanha da estufa.
As capturas em `build/intro_preview/` incluem ambas as seleções, nome, 720p,
1080p e ausência de molduras. O alvo do nome verifica UTF-8, acentos, limite de
12 caracteres, campo vazio, Backspace, TAB/SHIFT+TAB, cliques, rascunho e as duas
direções da transição. O desenho é conferido como somente de leitura.
O alvo da seleção cobre navegação, confirmação, voltar, F7, entrada mantida,
relógios de animação e liberação dos recursos.
