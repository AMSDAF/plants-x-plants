# Seleção de exploradores

## Composição atual

**Campanha** abre “Escolha seu personagem” sobre o fundo animado aprovado.
Cartões iguais de 336 × 480, controles e título são formas raylib, sem textura
da folha de referência. A seleção muda apenas borda, cantos laranja e cursor;
hitboxes e relógio do fundo permanecem estáveis. “Homem” e “Mulher” reutilizam
os retratos simplificados com filtro pontual, proporção preservada, altura
corporal de 360 pixels e pés em y=526. Ambos têm repouso em quatro poses, com relógios independentes. Os controles e F7 permanecem disponíveis. A confirmação abre a
[entrada de nome](CAMPAIGN_NAME_ENTRY.md) antes da exploração; voltar preserva
a escolha, o rascunho e os relógios dos retratos/fundo. `selection_scene_check` salva ambos os
estados em `build/selection_preview/selected_male.png` e `selected_female.png`.
A auditoria de recursos e as remoções estão em [SELECTION_CLEANUP.md](SELECTION_CLEANUP.md).

## Retratos simplificados v2

`tools/prepare_selection_portraits_v2.py` divide a fonte RGBA de 1536 × 1024
em duas metades completas, com 24 pixels extras de margem. Gera dois PNGs
816 × 1072 e `include/selection_portrait_v2_data.h`, sem remover pixels nem
alterar cores/alfa. O maior componente de alfa ≥128 mede somente o corpo;
nenhuma máscara é aplicada à arte. A fonte tem alfa 0–254; o halo marrom amplo
está principalmente em RGB sob alfa zero e, por isso, quase não aparece no
cartão creme. Não houve recorte retangular visível que exigisse suavização.
Diagnósticos ficam em `build/selection_portrait_v2_preview/`. A reprodução e
os fallbacks são verificados por `selection_scene_check` e pelas prévias dos
repousos descritas abaixo. As fontes e atlas detalhados antigos permanecem
no projeto, mas seus atlas não são carregados pela seleção atual.

## Repouso masculino

`tools/prepare_male_portrait_idle.py` preserva a fonte RGBA
`male_portrait_idle_source.png` (1199 × 1312) e mede cada quadrante separadamente.
Alinha as solas e um pivô horizontal comum, sem redimensionar poses. O atlas
`male_portrait_idle.png` contém quatro células 352 × 656. A parte inferior do
neutro é reutilizada a partir da linha 372, junto ao cinto, mantendo pernas e
botas idênticas. A escala comum é 360/586; não há deformação, rotação ou salto
vertical do corpo inteiro. `male_portrait_idle_halo.png` preserva separadamente
o halo do retrato v2, retirando somente seu corpo opaco; essa camada fica fixa.

Tempos finais: neutro **3,80 s**, inspiração **0,35 s**, piscar **0,10 s** e
relaxado **0,45 s**; ciclo de **4,70 s**. A pausa neutra inicial de 1,80 s foi
ampliada para reduzir o piscar de cerca de 22 para 13 vezes por minuto. As
quatro poses ainda têm pequenas diferenças de rosto/camisa, sem interpolação.
O relógio avança inclusive sem seleção masculina, durante bloqueios e na prévia
F7. Falha no atlas ou halo usa o retrato estático existente. Os recursos da
exploração não mudam.

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_male_portrait_idle.py
cmake --build build
cmake --build build --target selection_scene_check
.\build\selection_scene_check.exe
.\build\trainer_tools\Scripts\python tools/prepare_male_portrait_idle.py --runtime-preview
git diff --check
```

O preparador usa Pillow, NumPy e SciPy já disponíveis. Metadados ficam em
`include/male_portrait_idle_data.h`; medidas, máscaras, contatos de emenda,
GIFs e capturas OpenGL ficam em `build/male_portrait_idle_preview/`. O GIF
`runtime_card_loop.gif` usa o cartão creme real; os outros relógios ficam
congelados somente nas capturas para isolar a emenda do repouso. Verificações
comparam pernas/botas, mulher, começo/fim e fallback, além dos limites dos
tempos e continuidade do relógio. A revisão técnica usa capturas ampliadas.

**Aprovação visual manual:** o usuário aprovou o repouso masculino antes deste
commit, conforme informado na solicitação de registro. Essa aprovação se limita
à animação masculina; as demais mudanças mantêm os registros de validação
automatizada e inspeção de capturas descritos nesta documentação.

## Repouso feminino

A mesma ferramenta, com `--character female`, prepara a fonte corrigida
`female_portrait_idle_source.png` (1094 × 1437). Mede os quatro quadrantes e
alinha as solas em y=672, com eixo horizontal comum x=259 na fonte. As células
384 × 704 usam escala única 360/619 e pés em y=526. A junção na linha 370,
na camisa acima do cinto, reutiliza a parte inferior neutra para estabilizar
calça, botas e antebraço. Trança, mão na alça e planta continuam na arte desenhada.
O halo v2 é separado em `female_portrait_idle_halo.png` e permanece imóvel.

Durações: **4,40 / 0,35 / 0,10 / 0,45 s**, ciclo **5,30 s**, fase inicial
**2,20 s**. Trocar/hover nos cartões não reinicia relógios. O fallback feminino
usa o retrato estático, sem interromper o homem. Atlas, halo, metadados e tempos
masculinos aprovados permanecem idênticos.

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_male_portrait_idle.py --character female
cmake --build build
cmake --build build --target selection_scene_check
.\build\selection_scene_check.exe
.\build\trainer_tools\Scripts\python tools/prepare_male_portrait_idle.py --character female --runtime-preview
git diff --check
```

Capturas, contatos e GIFs ficam em `build/female_portrait_idle_preview/`.
`runtime_card_loop.gif` isola a mulher no cartão real, com outros relógios
congelados nas capturas. `both_portraits_loop.gif` combina as 16 poses pareadas
capturadas em OpenGL e respeita ambos os tempos e o deslocamento inicial;
o período conjunto completo é 249,10 s. Fundo/cursor ficam congelados somente
nessa prévia para testar o fechamento. Os testes verificam pernas e botas fixas,
limites temporais, continuidade, fallback e fechamento dos dois ciclos.
Permanecem pequenas mudanças de posição/detalhes no rosto e na camisa entre as
poses fornecidas; não foram usadas deformações ou suavização. A revisão técnica
feminina usou capturas e GIFs decodificados.

**Aprovação visual manual:** o usuário aprovou o repouso feminino e solicitou
seu commit. Esse registro complementa a aprovação masculina anterior, sem
estender aprovação manual às demais mudanças da seleção. A animação masculina
e seus tempos aprovados permanecem preservados.

## Fallbacks preservados

Sem atlas/halo de repouso válido, cada personagem usa o retrato v2 estático.
Sem esse derivado, usa `assets/sprites/trainers/male_portrait.png` ou
`female_portrait.png` (320 × 480). Se também faltar, tenta a fonte detalhada
`assets/sprites/trainer_male_portrait.png` ou `trainer_female_portrait.png`.
`tools/prepare_trainer_portraits.py` regenera apenas esses dois fallbacks
estáticos, sem máscaras ou atlas de respiração. Seus diagnósticos ficam em
`build/portrait_preview/`. As fontes detalhadas são necessárias para essa cadeia.

A floresta separada em camadas e a interface de madeira foram substituídas
pelo fundo animado atual e pelas formas geométricas. Seus recursos, módulo e
gerador antigos foram retirados após verificar execução, CMake, scripts e testes.
`tools/preview_selection_scene.py` continua montando o trecho de seis segundos
capturado por `selection_scene_check`; não representa um ciclo completo de
fundo ou dos dois personagens.
