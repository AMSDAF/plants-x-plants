# Teste provisório dos treinadores

O menu Campanha abre a seleção de Explorador/Exploradora e uma área de movimento.
Este teste não implementa a campanha completa nem altera os módulos de combate.

## Fontes e preparação

As duas fontes `assets/sprites/trainer_male_sheet.png` e
`assets/sprites/trainer_female_sheet.png` medem **1254 × 1254 pixels**, com canal
alfa entre 0 e 255. Não são atlas com células de 96 × 96: as divisões proporcionais
têm 313 ou 314 pixels, com margens e posições diferentes dentro de cada célula.

`tools/prepare_trainers.py` usa Pillow, NumPy e SciPy, listados em
`tools/trainer_requirements.txt`. Os comandos de instalação e execução estão no
[README](../README.md). O script lê as fontes sem sobrescrevê-las e:

1. Divide as quatro linhas/colunas proporcionalmente às dimensões reais.
2. Identifica o componente principal com alfa maior que 32 e conserva até dois
   pixels de borda ao redor dele, removendo componentes isolados e ruído distante.
3. Remove manchas vermelhas/verdes saturadas na faixa superior do chapéu. Essa
   limpeza localizada não se aplica às plantas, roupas e acessórios mais abaixo.
4. Alinha a base visível ao chão e projeta o centro horizontal do chapéu até os
   pés. Usar apenas a bota mais baixa fazia o corpo saltar lateralmente, sobretudo
   na vista de costas. A mochila não participa do cálculo desse eixo.
5. Copia os pixels **sem redimensionamento por quadro** para células transparentes
   de 320 × 320, com pivô em (160, 304). Os dois atlas finais têm 1280 × 1280.

`assets/sprites/trainers/male.png` e `female.png` são os recursos aprovados de execução.
Em `build/trainer_preview/`, o script gera duas folhas de contato, oito GIFs
direcionais e relatórios JSON com dimensões, alfa, recortes, pontos de apoio,
quantidade de pixels removidos, sequências e SHA-256 das fontes. As prévias são
artefatos locais ignorados pelo Git.

No jogo, todos os quadros usam escala 0,30 e filtro pontual: a altura visível fica
aproximadamente entre 83 e 89 pixels. A célula desenhada mede 96 × 96, incluindo
transparência; proporções e assimetrias dos acessórios são preservadas.

## Sequências e defeitos da arte

As colunas abaixo começam em **1**; cada posição dura 140 ms. Nenhum personagem
é espelhado para fabricar a passada oposta.

| Personagem | Sul/frente | Oeste/esquerda | Leste/direita | Norte/costas |
|---|---|---|---|---|
| Explorador | 1–2–4–2 | 1–2–1–4 | 1–2–1–4 | 1–2–4–2 |
| Exploradora | 1–2–1–2 | 1–2–1–4 | 1–2–1–4 | 1–2–4–2 |
| Parado, ambos | 2 | 2 | 4 | 2 |

- **Laterais, ambos:** as colunas 1 e 3 são quase duplicadas da mesma passada
  aberta; 2 e 4 são poses próximas de recolhimento. Reproduzir 1–2–3–4 sugeriria
  repetir a mesma perna à frente. O ciclo escolhido evita a terceira coluna
  redundante, mas ainda parece um passo repetido, sem alternância convincente.
- **Frente:** 1 e 3 também são muito próximas. O Explorador aproveita a variação
  da coluna 4. Na Exploradora, a quarta pose é cerca de 15 pixels de fonte mais
  baixa que a primeira, causando uma contração perceptível. Ela é mantida no
  atlas de inspeção, mas excluída da reprodução; a coluna 2 serve como repouso
  mais neutro e consistente em escala. Seu ciclo frontal continua incompleto.
- **Costas:** há variações de braços e botas, mas as colunas 1 e 3 repetem uma
  silhueta muito próxima. A sequência aproveita 2 e 4; não foi validada como um
  ciclo anatomicamente correto de duas passadas alternadas.
- O pivô remove deslocamentos de enquadramento, mas não corrige diferenças
  desenhadas de altura, proporção, chapéu, mãos e pernas. Há pequena oscilação
  residual da silhueta, sem corte nas células ou mudança da escala aplicada.
- Restam algumas franjas coloridas conectadas ao contorno nas fontes; remover
  automaticamente toda cor semelhante também apagaria detalhes da personagem.

Para um ciclo definitivo, é necessária **nova arte com passadas opostas distintas,
poses intermediárias e um repouso consistente**, mantendo proporções e acessórios
em todas as vistas. Este teste não representa uma validação de atlas de produção.

## Validação nesta implementação

- `cmake --build build` e `git diff --check` concluídos com sucesso.
- Inspeção das fontes, folhas normalizadas e quadros decodificados dos oito GIFs:
  sem cortes nas células, alinhamento ao chão consistente; defeitos acima observados.
- Verificação sem janela dos módulos reais de jogador/mundo com entrada simulada:
  ambos os personagens, quatro direções, deslocamento por delta time, ausência de
  diagonais, prioridade, repouso, direção mantida e bloqueio nos quatro limites.
- Verificação sem janela do fluxo real de `game.c` e da seleção, com menu/entrada/
  renderização simulados: teclado, clique, ENTER mantido, ESC, reentrada, cancelamento
  de ativação pendente e desenho do marcador sem modificar o jogador.
- A execução gráfica abriu e a seleção com os dois sprites foi inspecionada.
  O usuário interrompeu computer-use com ESC físico antes dos testes de movimento.
  Movimento contínuo, colisões, parada, retorno e reentrada **ainda precisam de
  validação manual na janela**. Os testes simulados não substituem essa inspeção.

O executável deve ser iniciado na raiz do projeto. Atlas ausente ou com dimensões
incorretas produz aviso no console e marcador jogável, sem impedir seleção ou movimento.

## Experimento reversível: seis poses masculinas

### Extração e tamanho

Fonte: `assets/sprites/trainer_male_walk6_source.png`, **1536 × 1024**, RGBA,
seis colunas de 256 pixels e quatro linhas de 256 pixels, na ordem sul/oeste/leste/norte.
O RGB contém fundo marrom e gradientes. A inspeção do alfa revelou uma máscara
parcial já existente: faixa 0–254, mediana zero, corpo predominantemente em 252–253
e resíduos quase transparentes ao redor. Portanto, não foi necessário classificar
a cor marrom para separar botas, calças e mochila do fundo.

Execute `tools/prepare_trainer_walk6.py` com o mesmo ambiente e dependências da
preparação aprovada. Este script separado não executa nem modifica o preparo anterior.
Ele mantém o maior componente com alfa >32, preserva dois pixels de suporte de borda,
elimina alfa ≤8 e remapeia 8–245 para 0–255. Não usa limiar global de cor, espelhamento,
nem redesenha detalhes. RGB de pixels totalmente transparentes fica zerado no resultado.

A altura mediana do atlas aprovado determina uma **única escala de 1,23828125**
para as 24 poses novas. Todos os recortes de 256 × 256 são ampliados igualmente para
317 × 317, com filtro pontual, e alinhados em células transparentes de 320 × 320,
pivô (160, 304). O PNG experimental mede **1920 × 1280**. Não há esticamento por pose.
No jogo, mantém-se a escala de desenho 0,30: alturas visíveis de **79,8–85,5 pixels**,
próximas aos 83,7–87,0 do masculino aprovado. As poses frontais/de costas mais
encolhidas continuam menores; não se tenta compensar isso com escala individual.

O recorte é **utilizável para comparação local**. Foram inspecionadas as 24 máscaras
e composições em cinza claro, azul e fundo escuro: sem áreas marrons grandes,
névoa evidente ou apagamento visível de botas, roupa, pele, chapéu e mochila.
Existe uma borda escura/marrom fina em alguns contornos, herdada da pintura; a
limpeza do alfa não é um processo completo de remoção de cor do fundo nas bordas.
Não se declara uma extração perfeita para arte final.

### Avaliação do movimento

Mantida a ordem **1–2–3–4–5–6** em todas as direções. Não foi encontrada uma
reordenação claramente melhor que preserve a intenção das poses. Os GIFs repetem
o ciclo em 0,56 s: a quantidade maior de quadros não aumenta a duração.

| Direção | Observação |
|---|---|
| Sul | Há indicação visual de pernas opostas nas poses de avanço e de sola levantada, sobretudo entre 1/4 e 3/5. Porém, 1/2 são próximas, as fases não são uniformes e 6 tem silhueta mais baixa. A alternância é mais sugerida que no ciclo antigo, sem garantir uma caminhada correta. |
| Oeste | 1/4 repetem uma passada aberta muito semelhante; 3/6 repetem joelho elevado. 2/5 têm deslocamentos diferentes, mas não estabelecem inequivocamente uma segunda passada oposta. A mudança de braço também é irregular. |
| Leste | 1/4 e 3/6 têm o mesmo problema de repetição. A passagem para a pose de joelho alto é brusca, com aspecto de corrida ou marcha exagerada. Não há alternância convincente em um ciclo completo. |
| Norte | 1/4 são muito próximas. Há variação das solas e dos braços, mas 3 e principalmente 6 encolhem a silhueta e fazem chapéu/mochila oscilar verticalmente. Não foi confirmado um ciclo completo de duas passadas corretas. |

As transições 2→3, 3→4 e 6→1 evidenciam saltos de postura. As solas não cortam
a célula e a base visível permanece em y=304, mas a anatomia desenhada muda: alinhar
os pés não elimina essa oscilação. Há mais poses distintas e alguma melhora na
variedade frontal; **não há melhora global convincente na fluidez da caminhada**.
Seis quadros não corrigem os passos repetidos nem as descontinuidades de desenho.
O atlas aprovado permanece como padrão; para substituí-lo, recomenda-se nova arte
com duas passadas opostas e poses intermediárias consistentes, principalmente laterais.

### Comparação em execução e reversão

- **F6 na exploração com Explorador:** alterna quatro/seis poses; o texto superior
  indica a opção efetiva. A escolha volta ao padrão aprovado ao entrar novamente.
- Repouso e seleção sempre usam o atlas aprovado; a implementação feminina é mantida.
- O atlas opcional é carregado uma vez e liberado no encerramento. Se ausente ou
  com dimensões inválidas, a caminhada volta automaticamente ao aprovado.
- `PLAYER_MALE_WALK_CYCLE_SECONDS` em `include/player.h` configura a duração total
  masculina (padrão 0,56 s, valor positivo), independentemente do número de poses.
  Cada quadro experimental dura aproximadamente 93,3 ms; cada fase aprovada, 140 ms.
  O ciclo feminino continua em 0,56 s com as mesmas poses e tempos anteriores.
- Velocidade de 190 px/s, colisão, controles e transições são preservados. F6 não
  reinicia posição nem fase. Para reverter visualmente, pressione F6 ou reentre;
  remover somente `male_walk6.png` também mantém o comportamento aprovado.

### Artefatos e verificações do experimento

O script grava somente `assets/sprites/trainers/male_walk6.png` para execução.
Em `build/trainer_preview/walk6/`, ficam `masks.png`, `source_alpha.png`, `contact.png`,
`on_light.png`, `on_dark_blue.png`, `inspection.json`, quatro GIFs direcionais e
quatro GIFs `*_comparison.gif` (aprovado à esquerda; experimento à direita).
O relatório registra hash, cortes, escala e posições; verificou-se que todos os
PNGs preexistentes, incluindo fonte nova, atlas aprovados e idles avulsos, mantêm
os SHA-256 anteriores ao experimento.

A validação gráfica usou um programa temporário com **OpenGL real e os mesmos
`player_draw`/`world_draw` do jogo**, capturando 12 fases sincronizadas em quatro
direções. As imagens `runtime_00.png` a `runtime_11.png` e o GIF
`runtime_comparison.gif` mostram o A/B no chão de exploração e tamanho de execução.
Foram inspecionados esses quadros, os GIFs decodificados e as máscaras. Isso valida
a renderização real das duas opções, mas não equivale a uma sessão manual de
movimento e F6 na janela principal.

Os testes com entrada/desenho simulados verificaram F6, as 24 posições do atlas,
repouso aprovado, renderização feminina inalterada, fallback ausente/inválido,
desenho sem mutação e liberação de texturas. Também foram repetidas as verificações
de movimento, limites, seleção e transições do teste anterior. Preparação,
`cmake --build build` e `git diff --check` executados com sucesso.
