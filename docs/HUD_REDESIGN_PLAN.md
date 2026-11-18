# Plano do novo HUD e da tipografia

Status: direção escolhida e implementada. Este documento preserva o plano
e a exploração visual. A execução e as verificações estão descritas em
[HUD_IMPLEMENTATION.md](HUD_IMPLEMENTATION.md).

## Direção escolhida

O usuário escolheu o HUD creme da segunda proposta, com uma seta apontando
para baixo como indicador de alvo. A prévia combinada está em
`assets/ui/hud_concepts/cream_arrow_v1.png`.

Preservar os painéis creme, o texto verde-escuro e o painel de interação
centralizado no rodapé, com contexto e ação em duas linhas e tecla E à
esquerda. Substituir os cantos da segunda proposta por uma única seta pequena,
com haste curta e ponta triangular, preenchimento creme e contorno escuro.
A seta fica acima do alvo, sem texto, círculo, losango ou marca no chão.

Essa revisão combina as preferências visuais solicitadas e orienta a
implementação. O registro da edição fica em
`assets/ui/hud_concepts/cream_arrow_prompt.json`.

## Objetivo e escopo

Mover a instrução de interação para o centro inferior da tela, substituir o
círculo amarelo por uma marca discreta e melhorar a legibilidade dos textos
do jogo. Preservar cenários, personagens, movimento, colisões, alcance e
regras das interações. O desenho consulta o estado; não executa ações.

## Diagnóstico do estado atual

`src/world.c` calcula a posição de `draw_interaction_hint` junto ao objeto.
O marcador de `draw_interaction_marker` desenha círculos amarelos somente nos
interiores. O rodapé também contém uma legenda permanente de controles.
Isso distribui mensagens entre o alvo, os cantos e a parte inferior da tela.

`assets/fonts/` ainda não tem uma fonte própria. As chamadas `DrawText` e
`MeasureText` utilizam a fonte padrão da raylib. A estrutura existente de
`game_canvas` já permite desenhar o texto na resolução física do viewport.

## Composição proposta

Manter localização e objetivo no canto superior esquerdo, em um bloco compacto.
O objetivo é o texto principal; o nome do local tem menos destaque. Manter
as pistas no canto superior direito. Não adicionar inventário, minimapa,
barras de vida ou outros indicadores sem função na exploração atual.

Reservar o centro inferior para uma única instrução contextual. Em coordenadas
lógicas de 1280 × 720, usar margem inferior inicial de 24 pixels e largura
conforme o texto, entre 320 e 640 pixels. O centro do painel permanece em
x = 640. Altura inicial de 56–64 pixels para uma linha ou até 88 para duas.
Medir o texto com a fonte real, quebrando frases longas sem cortá-las.

O painel deve manter alto contraste sobre o piso, a água e a vegetação. A
paleta parte de verde-escuro `#183B30`, creme `#F5EBCD` e verde-sálvia como
apoio. Contornos discretos e pouca sombra; evitar molduras grandes que
disputem atenção com o cenário. A cor final depende da proposta escolhida.

Retirar a legenda longa de controles do rodapé normal. Os atalhos existentes
continuam funcionando. A prévia 1 sugere uma ajuda por Tab, mas essa tecla
não existe hoje na exploração: sua eventual inclusão é uma decisão separada,
não um requisito do HUD. As etiquetas permanentes de diagnóstico F6/F8 devem
ficar fora do HUD principal; preservar suas funções de teste.

## Estados da interação

| Situação | Centro inferior | Marca do alvo |
| --- | --- | --- |
| Sem objeto no alcance | Oculto | Oculta |
| Ação disponível | Tecla E e verbo, como “Descer pela escotilha” | Marca neutra clara |
| Ação bloqueada | Motivo, como “A escotilha está trancada”, sem tecla E ativa | Marca com símbolo de bloqueio, sem depender só da cor |
| Informação já obtida | Estado curto, como “Pista registrada”; oferecer E apenas se houver releitura | Marca de informação/conclusão |
| Diálogo, central ou resultado aberto | Prompt de exploração oculto | Oculta |

Reutilizar a seleção de `nearby_object` e o alcance atual. O objeto indicado
deve ser exatamente o que receberá E. Usar a mesma decisão de disponibilidade
da ação para a tecla e para o painel; não deduzir disponibilidade pela cor ou
por um prefixo de texto. Não introduzir outro seletor de alvo no renderizador.

A mudança de mensagem deve ser imediata ao mudar de alvo. Uma entrada/saída
visual curta, de aproximadamente 0,12 s, pode suavizar a troca sem atrasar a
ação nem usar esperas bloqueantes. Atualizar essa animação fora do desenho.

## Substituição do círculo amarelo

Usar somente uma marca pequena sobre o alvo selecionado. O texto e a tecla
ficam exclusivamente no rodapé. As três imagens exploram formas diferentes;
a escolhida será refinada antes da implementação.

Como referência de escala, limitar a marca a cerca de 12–18 pixels lógicos,
com contraste claro/escuro e deslocamento visual fixo acima do objeto. Ela
não deve cercar o corpo do jogador, pintar o chão nem formar um anel.
Usar a mesma linguagem nos interiores e no exterior, resolvendo a diferença
atual entre as duas regiões.

Converter a âncora do mundo para a tela e desenhar a marca depois do cenário
e personagens, com tamanho independente do zoom. Usar uma âncora visual
própria quando o ponto de interação estiver na base de um objeto alto. Não
mover a posição física da interação para posicionar o marcador. Não exibir
marca quando o alvo estiver fora da tela, em outra sala ou com o HUD suspenso.

## Fonte proposta

Propor **Atkinson Hyperlegible Next**, Regular para textos e Semibold para
ações e títulos. A família foi desenvolvida com foco em distinção de caracteres
e legibilidade; o projeto distribui a fonte sob SIL Open Font License 1.1.
Fontes: [Braille Institute](https://www.brailleinstitute.org/freefont/) e
[repositório oficial Google Fonts](https://github.com/googlefonts/atkinson-hyperlegible-next).

Na implementação, obter TTFs estáticos e incluir a licença junto aos arquivos
em `assets/fonts/`. Conferir acentos portugueses, números, pontuação e símbolos
realmente usados pelo jogo. As letras das prévias geradas são ilustrativas:
não comprovam a aparência da fonte TTF nem substituem um teste com a fonte real.

Escala inicial, em pixels lógicos:

| Uso | Tamanho inicial |
| --- | --- |
| Ação principal e objetivo | 22–24 |
| Contexto e motivo de bloqueio | 18–20 |
| Local e contador de pistas | 18 |
| Diálogo | 24, entrelinha aproximada de 1,3 |
| Títulos de tela | 28–36 |

Meta de contraste para textos de leitura: pelo menos 4,5:1 entre texto e fundo
efetivo do painel. Não compensar letras pequenas com contornos grossos.

Carregar as fontes uma vez, com atlas de resolução suficiente para o viewport,
e usar `DrawTextEx` e `MeasureTextEx` de maneira consistente. Preservar o
canvas nativo: evitar gerar textos pequenos em uma imagem 720p para ampliá-la.
Prever retorno à fonte padrão se o arquivo falhar, com aviso de diagnóstico.
Não alterar a fonte padrão interna da raylib nem criar globais mutáveis.

## Plano de implementação

1. Escolher uma das três prévias e refinar marcador, contraste e dimensões.
2. Preparar tipografia compartilhada e seus recursos em `ui_theme.h/.c`, com
   estado de recursos pertencente a `Game` e passado explicitamente às telas.
   Obter fonte e licença; validar a leitura real antes de migrar todas as telas.
3. Preparar em `world.c` uma descrição somente de leitura da interação:
   alvo, âncora, ação, disponibilidade e motivo. Reutilizar o seletor e as
   regras existentes. Manter os identificadores privados de objetos no módulo.
4. Criar `exploration_hud.h/.c` para compor objetivo, pistas, painel inferior
   e marcador, recebendo dados de apresentação e recursos explicitamente.
   Conservar os fluxos de diálogo e central; não calcular regras no HUD.
5. Migrar a fonte de menu, seleção, entrada de nome, exploração, diálogos,
   central e batalha, ajustando medidas e quebras de linha. Preservar logos
   e textos que façam parte da arte, sem redesenhá-los como fonte de interface.
6. Retirar o tooltip junto ao objeto e o círculo antigo. Atualizar CMake
   para os novos arquivos `.c` e documentar os recursos e atalhos finais.
7. Executar as verificações abaixo e comparar capturas reais com a prévia
   escolhida antes de considerar o HUD pronto.

As imagens geradas são referências de composição, não telas para colar no jogo.
Desenhar textos, teclas, contadores e estados dinamicamente. Gerar separadamente
apenas ornamentos/ícones necessários após a escolha visual; não gravar texto
ou números dentro de imagens de painel. Fundos redimensionáveis devem manter
bordas e margens estáveis, sem esticar letras ou ornamentos.

## Verificação e critérios de aceite

- Instrução sempre no centro inferior, inclusive perto das bordas e em salas.
- Nenhum texto de interação junto ao objeto; nenhum círculo amarelo restante.
- Marca e tecla E correspondem ao mesmo alvo, inclusive com dois objetos próximos.
- Ação bloqueada informa a causa e não sugere E como ação disponível.
- Diálogo, central, batalha e resultado não mostram prompts sobrepostos.
- Textos longos e com acentos cabem; testar `ç`, `ã`, `õ`, `á`, `é`, `ô` e
  distinção de `I`, `l`, `1`, `O` e `0`.
- Nome, objetivo, pistas, regras e golpes usam a fonte compartilhada.
- Validar 1280 × 720, 1920 × 1080, 2560 × 1440, tela cheia, barras e HiDPI.
- Mouse, foco e recorte do campo de nome continuam corretos.
- Repetir o percurso de campanha, a ponte, portas, escotilha e NPCs.
- Capturar estados disponível, bloqueado, registrado e sem alvo em exterior
  e interior; confirmar que desenhar não muda o estado de jogo.
- Ausência da fonte ou de ícone não impede jogar; imagens atuais são preservadas.

## Prévias e limites desta entrega

As três imagens estão em `assets/ui/hud_concepts/concept_01.png`,
`concept_02.png` e `concept_03.png`, na ordem em que foram apresentadas.
`reference_maintenance.png` guarda a captura usada; `prompts.json` registra
os prompts e o uso da ferramenta integrada de edição de imagens.

Todas simulam a escotilha já desbloqueada e a ação disponível. A captura de
referência mostra outro estado da missão; o cenário serve de base visual.
As imagens são conceitos e podem variar enquadramento, texto ou detalhes do
cenário. As duas primeiras têm 1672 × 941 e a terceira 1717 × 916; nenhuma
foi esticada para simular uma captura exata. A implementação deverá respeitar
as dimensões lógicas do jogo, não copiar coordenadas dessas imagens.

As imagens desta pasta continuam sendo conceitos; a versão integrada desenha
componentes e textos dinamicamente. Consulte HUD_IMPLEMENTATION.md para os
recursos de execução e os testes implementados após a escolha visual.
