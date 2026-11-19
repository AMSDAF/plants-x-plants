# Diálogo botânico

A apresentação segue o conceito aprovado: madeira marrom, detalhes de folhas,
fundo creme, texto verde à esquerda e retrato com plaquinha de nome à direita.
A caixa usa coordenadas lógicas (32, 504, 1216, 200), preservando uma margem
inferior de 16 pixels. Placas usam a mesma moldura, sem coluna de retrato.
Os demais painéis e o aviso compacto de interação continuam independentes.

## Nomes e personalidades definidos

Nomes e personalidades aprovados para a revisão das falas:

| Nome | Profissão | Personalidade |
|---|---|---|
| Lia | Jardineira | Acolhedora, observadora e com humor tranquilo. Explica com exemplos simples, sem infantilizar o jogador. |
| Bento | Técnico | Prático, direto e com humor seco. Reclama dos equipamentos, mas tem paciência para ajudar. |
| Íris | Pesquisadora | Curiosa e entusiasmada. Pode se alongar numa explicação, percebe e volta ao essencial. Convida o jogador a descobrir as respostas. |

Na primeira conversa, cada personagem deve se apresentar e usar o nome escolhido
pelo jogador. Nos encontros seguintes, deve cumprimentá-lo brevemente e comentar
a situação atual da missão, sem repetir a apresentação. A plaquinha deve mostrar
o nome do personagem em vez da profissão.

Nomes, apresentações e cumprimentos de reencontro estão integrados. Cada NPC
se apresenta uma vez por campanha, no primeiro contato, usando o nome da sessão.
As apresentações abaixo definem o tom; `{nome}` representa o nome do jogador:

- **Lia:** “Você deve ser {nome}! Eu sou a Lia. Cuido desse jardim — e ele anda me dando trabalho por três. Ainda bem que você chegou.”
- **Bento:** “Ah, você é {nome}? Sou o Bento. Se alguma coisa aqui emperra, me chamam. Hoje resolveram emperrar todas juntas.”
- **Íris:** “{nome}, certo? A Lia comentou que você vinha! Sou a Íris. Estudo as plantas daqui. Você precisa ver o que descobri sobre o Girassol… mas vamos por partes.”

A apresentação ocupa uma página adicional antes das orientações existentes;
a conversa comporta até três páginas. Reencontros usam uma saudação breve
antes da pista atual. Íris só menciona que Lia comentou sobre o jogador quando
o primeiro contato com Lia já ocorreu. Sem nome na sessão de teste, usa
“visitante”. Os buffers de texto pertencem ao mundo; o desenho só os consulta.

Nomes, apresentações, orientações, placas e agradecimentos estão integrados e
foram validados no percurso completo da missão.

## Pedidos de ajuda e agradecimentos

As orientações dos NPCs seguem pedido, ajuda conforme o progresso e reconhecimento:

- Lia pede ajuda para recuperar o refletor, reconhece a obtenção da chave e orienta
  sobre escotilha, energia, alarme e água. Agradece pela reabertura do viveiro e
  encaminha o jogador a Íris.
- Bento pede ajuda para liberar a ponte. Reage de modo diferente a nenhum canal,
  dois canais ou um canal com pressão incorreta. Quando a ponte abre, comemora e
  agradece; depois aponta a válvula, o refletor, o alarme ou o viveiro, conforme
  o que falta. O agradecimento não se repete a cada conversa.
- Íris convida o jogador a investigar a central, pede ajuda com o Girassol e as
  pistas, agradece pela liberação do painel e reconhece as regras já validadas.
  Ao concluir a central, um diálogo de agradecimento com o nome do jogador
  aparece antes da tela de conclusão. E/Enter revela e avança normalmente;
  Esc pode fechá-lo sem desfazer a vitória.

Pedidos e dicas não alteram as regras dos desafios. Apresentações e saudações
continuam usando o nome da sessão. As referências evitam depender só da cor
antiga dos telhados: usam os prédios, a ponte, o canal e os controles atuais.
O check do HUD percorre 768 combinações de progresso e contato, conferindo
que cada página cabe em três linhas, inclusive com nomes largos de 12 letras.
A revisão das placas e bilhetes está concluída, conforme as decisões abaixo.

## Placas e bilhetes revisados

| Leitura | Decisão | Motivo |
|---|---|---|
| Placa do jardim | Manter e encurtar | Orienta quem chega: localiza depósito e manutenção em relação ao pátio e indica Lia como primeiro contato. |
| Aviso da ponte | Remover | Repetia o pedido e as dicas de Bento e o painel de condições da ponte. Interação, marcador e desenho extra foram retirados. |
| Placa dos canais | Manter e esclarecer | É a pista obrigatória da regra 3 da central: `E = (B XOR F)`. Explica que E liga quando B e F diferem, e desliga nos demais casos. |
| Bilhete da manutenção | Manter e corrigir a leitura | É a pista obrigatória da regra 8: `SE D, ENTÃO C`. Agora abre o conteúdo ao interagir e permite releitura; antes registrava sem mostrar o texto e ficava bloqueado após a coleta. |

As duas pistas continuam registradas nos mesmos campos de progresso. Reler não
acrescenta outra pista nem modifica água, energia, alarme ou regras da central.
O nome “cabine do alarme” foi substituído por “casa de manutenção” nas instruções,
para corresponder à sala atual. As três leituras cabem em duas linhas na área
reservada a placas e não dependem da cor antiga de um telhado.

## Recursos

- `assets/ui/dialogue_frame.png`: moldura quadrada de 1254 × 1254, com madeira
  e folhas nos cantos, sem falas ou nomes impressos.
- `assets/sprites/npcs/gardener_portrait.png`: retrato RGBA de 1254 × 1254,
  com fundo transparente. O alfa visível determina o recorte; o busto é
  enquadrado sem deformar o rosto e sem cortar o chapéu.
- `assets/ui/dialogue_skin.png`: arte RGBA horizontal de 1933 × 813 com moldura,
  divisor com rebites, janela de retrato com cantos ornamentais e plaquinha
  decorada. A região útil atual `(0, 223, 1933, 319)` ocupa a barra de 1216 × 200.
  O exterior e os cantos recortados são transparentes. `UiTheme` calcula
  a região útil pelo alfa ao carregar, sem coordenadas de recorte no desenho.
- `assets/ui/dialogue_assets_prompts.json`: prompts, referências e região útil.

Os PNGs foram gerados com a ferramenta integrada `image_gen` a partir
do conceito aprovado. Não há dependência de Python para jogar. As imagens
originais geradas são usadas diretamente, com filtro pontual.

Conversas de personagens usam a arte horizontal pronta, preservando a proporção
original dos enfeites. Retrato, nome, falas, controles e indicador continuam
independentes da imagem. A janela e a plaquinha usam coordenadas medidas na
arte; o retrato inteiro cabe na janela sem cobrir os cantos ornamentais.

Placas e a reserva para ausência da arte horizontal usam a moldura quadrada,
dividida em nove regiões durante o desenho: as faixas externas
usam 16% da dimensão da fonte e os cantos ocupam 44 pixels lógicos. Isso
preserva cantos e folhas; as faixas centrais das bordas são repetidas na mesma escala dos cantos,
sem esticar os veios da madeira. O centro usa o creme da própria textura.

Jardineira, Técnico e Pesquisadora têm retratos próprios. Os dois últimos
foram gerados usando a Jardineira como referência de estilo e os sprites
correspondentes em `botanical_tiles.png` como referência de identidade.
Os arquivos são `assets/sprites/npcs/technician_portrait.png` e
`assets/sprites/npcs/researcher_portrait.png`, ambos com transparência.
O alfa visível determina o enquadramento de todos os retratos.
Um retrato ausente usa somente a inicial do respectivo personagem.
Sem a textura da moldura, o jogo desenha bordas de madeira e verde com
formas raylib e mantém a conversa.

## Responsabilidades e entrada

`UiTheme` carrega e libera fonte pixel, molduras e retrato junto dos demais recursos da
interface. `dialogue_ui.c` desenha a apresentação, consultando os recursos
somente para leitura. `world.c` mantém os nomes, até três páginas por conversa,
a escolha das falas conforme o progresso, o registro dos primeiros contatos
e o tratamento das teclas. `game.c` fornece o nome da sessão ao interagir.

- A fala aparece a 40 codepoints UTF-8 por segundo, acumulando o tempo entre quadros.
- E ou Enter durante a animação: revela a página inteira, sem avançar.
- Com a fala completa, outro toque em E ou Enter avança ou fecha a conversa.
- Cada página e conversa nova reinicia a animação. Manter a tecla pressionada
  não repete a ação.
- ESC: fecha a conversa.
- O triângulo de continuação aparece quando a página termina e há outra página.

Nome, falas e instruções usam Pixelify Sans, incluindo os acentos.
O atlas de 28 pixels preserva a cobertura dos traços e usa filtro pontual; falas e nomes usam
28 pixels lógicos, controles usam 20. Atkinson continua nas demais telas
e serve de reserva se a fonte pixel estiver ausente.
Não há texto embutido nos PNGs. O quadro reserva espaço separado para os
controles; as falas quebram linhas dentro da área disponível. A quebra considera a fala
completa antes da revelação, mantendo as palavras fixas. A renderização nunca
altera o relógio ou o progresso da animação. O personagem
não anda enquanto a conversa está aberta, conforme o fluxo existente.

## Verificação

```powershell
cmake --build build --target plants_x_plants exploration_hud_check name_entry_check
.\build\exploration_hud_check.exe
.\build\name_entry_check.exe
```

O check do HUD captura o diálogo da Jardineira em 720p, 1080p e 1440p, a
última página, os demais personagens, placas, texto longo e ausência de
retrato, moldura e fontes. Cada captura confere que desenhar não altera
`Game`. Também confere carga e liberação das texturas e a reserva sem a arte horizontal. As capturas ficam em
`build/hud_preview/dialogue_*.png`. O check de campanha cobre as entradas,
avanço, revelação por tempo, salto por tecla, tecla mantida, fechamento,
bloqueio de movimento e conversas durante a missão.

Validação executada: build do jogo, `exploration_hud_check` e
`name_entry_check` passaram. Foram inspecionadas capturas reais da Jardineira,
texto longo e identificação de reserva, incluindo a ausência individual dos três retratos. Nenhuma alteração foi feita nas regras
de combate. Os comandos de diálogo distinguem revelação e avanço.
