# Estufa Lógica

Pressione **ENTER** na tela inicial, escolha o personagem, confirme o nome e explore a estufa.
Use **WASD** ou **setas** para mover, segure **Shift** para correr e pressione
**E** quando aparecer o marcador de interação.

## Prédios e acesso ao viveiro

A câmera acompanha o personagem no exterior e se reposiciona ao atravessar
uma porta, atravessar a escotilha ou voltar ao pátio após uma derrota. A casa de
manutenção, o depósito e o viveiro têm fachadas sólidas e interiores próprios.
Os interiores mantêm 720 × 420 unidades lógicas, centralizados com margens
e zoom 1, sem ampliação para preencher a tela, inclusive no modo tela cheia.
O exterior usa a ilustração do Complexo Botânico em seu tamanho nativo.
Os quatro quadrantes melhorados compõem a versão completa de maior resolução,
aplicada sobre a base preservada. **F8** alterna a comparação do mapa inteiro;
**F11** permite observá-la em tela cheia na resolução física da tela.
Consulte [COURTYARD_DETAIL.md](COURTYARD_DETAIL.md) para resolução e validação.
Os limites de circulação acompanham os contornos do piso. O jogador começa
do lado interno do portão sul e pode caminhar na diagonal e deslizar junto
às bordas. Os NPCs e objetos interativos são desenhados sobre o cenário.
No exterior, o cenário ocupa toda a área do jogo. O HUD indica o setor, o
objetivo atual. As pistas são ajudas opcionais, sem contador de coleta. Ao se aproximar de um objeto, a ação
aparece no centro inferior da tela, acompanhada da tecla E somente quando
está disponível. Uma seta creme aponta para o alvo, tanto no exterior como
nos interiores. Bloqueios e pistas registradas mostram uma explicação no
mesmo painel, sem círculo amarelo ou texto junto ao objeto.

A casa de manutenção usa a ilustração `maintenance_house_interior.png`,
com colisões nas bases dos móveis. O painel do alarme, a pista e a escotilha
seguem as posições desse interior; o percurso contorna a prateleira central.
Essa arte interna é carregada separadamente do mapa exterior melhorado.

Leia as placas com **E** e fale com Lia, a jardineira no pátio, Bento, o técnico
perto da ponte, e Íris, a pesquisadora no viveiro. As falas acompanham o progresso na
campanha: pedidos de ajuda dão lugar a agradecimentos e dicas da etapa seguinte.
Na caixa de diálogo, **E** ou **Enter** completa o texto em animação; outro toque
avança ou encerra a conversa. **Esc** fecha a caixa.

1. Entre no depósito e vença a Babosa para receber a **Chave da Manutenção**.
2. Na casa de manutenção, desative o alarme, registre a pista e abra a
   escotilha com a chave. A escotilha dá o primeiro acesso ao subsolo.
3. No subsolo, ligue o refletor no painel de energia e retorne pela escotilha.
   A escotilha conecta o subsolo à casa de manutenção em ambos os sentidos,
   independentemente do estado do refletor.
4. A válvula da água e a segunda pista ficam do outro lado da ponte. A porta
   do viveiro abre quando `Água E Luz E NÃO Alarme` vale. Depois da primeira
   entrada, a porta permanece liberada.

## Placas e pistas

A placa de entrada orienta até Lia e indica depósito e manutenção em relação ao
pátio. O aviso extra junto à ponte foi removido: Bento e o painel de condições
já explicam os canais e a pressão.

A placa dos canais revela a regra 3 (`E = B XOR F`) e o bilhete da manutenção
revela a regra 8 (`SE D, ENTÃO C`). As duas leituras explicam o significado da
regra, registram a pista e continuam disponíveis para releitura.

## Ponte

Os dois canais ficam dentro da **Casa de Controle da Ponte**, junto à
margem oeste. O norte acrescenta **3** à pressão e o sul acrescenta **5**. O
dreno externo começa aberto e retira **2** unidades, com mínimo de zero.
A ponte exige `(Norte XOR Sul) E (Pressão = 3)`.

| Canal único aberto | Dreno | Pressão | Ponte |
| --- | --- | --- | --- |
| Norte | Fechado | 3 | Estendida |
| Norte | Aberto | 1 | Recolhida |
| Sul | Fechado | 5 | Recolhida |
| Sul | Aberto | 3 | Estendida |

Perto da ponte, do dreno ou dentro da casa de controle, um cartão mostra
o XOR e a pressão separadamente. Com os dois canais abertos ou os dois
fechados, a travessia fica bloqueada. Um cartão semelhante mostra água,
luz e alarme ao se aproximar do viveiro.

Quando a ponte estende, o jogo desenha o recorte correspondente sobre o
canal e libera a travessia.

## Guardiões e central de relés

Interaja com um guardião para lutar na arena. Monte o grupo e o Golpe
Lógico no primeiro encontro. A equipe recupera o HP entre lutas e mantém
a composição escolhida. Uma derrota ou recuo leva o jogador de volta ao
pátio, preservando o progresso do mapa.

Vencer o Girassol no viveiro libera a central. O desafio possui três etapas
lineares e independentes, com dificuldade gradual:

1. **A:** ligar o primeiro relé.
2. **A E B:** ligar os dois relés.
3. **(A OU B) E NÃO C:** ligar A ou B (ou ambos) e manter C desligado.

A, B e C aparecem e podem ser alternados desde a primeira etapa; somente os
relés citados na expressão atual influenciam a validação. Use **1–3** ou clique para
alternar e **Enter** para validar. Ao acertar, a próxima etapa começa com seus
relés desligados; as regras anteriores não precisam continuar valendo. Errar
mantém a etapa atual e mostra uma dica, sem punição ou limite de tentativas.
Não é preciso voltar às etapas já concluídas. **ESC** fecha o painel e preserva
a etapa, os ajustes atuais e as tentativas durante a sessão.

A placa dos canais explica OU e o bilhete da manutenção explica NÃO; ambos são
dicas opcionais e não bloqueiam a central. Na última etapa, as configurações válidas são A, B ou A+B ligados,
sempre com C desligado.

Depois de resolver, Íris agradece em uma conversa com o nome do jogador.
Ao fechar essa conversa, a tela de conclusão permite voltar ao menu com
**Enter** ou **Esc**.

## Implementação e teste

`world.c` mantém a região ativa, as transições por proximidade, a colisão
das fachadas e da ponte e as regras de acesso. A central valida apenas a etapa atual dos três desafios antes de concluir a missão. O desenho consulta o estado. `game.c`
coordena a câmera e as transições entre exploração e combate, e aplica a
recompensa do encontro depois de uma vitória.

No Windows, execute a partir da raiz:

```powershell
cmake --build build --target plants_x_plants name_entry_check
.\build\name_entry_check.exe
```

O teste percorre a missão depois de confirmar o nome. Verifica a colisão
das fachadas e paredes, as oito combinações da ponte e suas duas soluções,
a escotilha antes e depois da chave, o retorno
com progresso após derrota, o acesso ao viveiro e a coleta de pistas.
Também valida a solução única entre as 256 configurações da central, o
avanço guiado, o retorno a uma regra quebrada e a conclusão.
As capturas ficam em `build/name_entry_preview/`.

## Integração dos interiores

O commit de interiores introduziu colisores e posições próprios para cada sala,
com personagens em escala visual 0,36 e enquadramento reduzido. O exterior
conserva a caixa dos pés de 16 × 8 pixels; os interiores usam 24 × 14 pixels.
Os marcadores acompanham as posições atualizadas dos controles, pistas e guardiões.

Os recursos opcionais `depot_interior.png`, `basement_interior.png`,
`bridge_control_interior.png` e `nursery_interior.png` em `assets/maps/`, e
`sunflower_guard.png` em `assets/sprites/`, são referenciados pelo código de
interiores, mas não acompanharam o commit remoto integrado. A Babosa usa
`assets/sprites/plants/aloe_v2.png`, a mesma arte da arena. Enquanto algum
recurso estiver ausente, o jogo desenha a sala ou o guardião com a forma de
reserva. A manutenção continua usando sua ilustração existente.

A validação usa caminhos livres diante dos móveis sólidos e preserva os testes
de bloqueio, acesso aos controles, vitória/derrota e progressão dos encontros.

## Telas de resultado

Após concluir a central, o agradecimento de Íris permanece até o jogador encerrar
as páginas. Em seguida, a tela final apresenta as três companheiras, o nome do
jogador e as opções **Jogar novamente** (retorna à criação de personagem) e
**Menu principal**. A trilha `ending.ogg` continua nessa tela.

A derrota contra qualquer guardião abre uma tela dedicada com **Tentar novamente**
e **Voltar à vila**. A revanche repete o mesmo encontro com a equipe recuperada,
as fórmulas e os golpes lógicos confirmados. Voltar à vila mantém os objetivos,
as chaves e as etapas resolvidas; perder não derrota o guardião nem conclui a estufa.
A vinheta de derrota já existente toca sem música de fundo na tela de derrota.

Ambas reutilizam o fundo escurecido, a moldura botânica, os botões e as sprites da
abertura. A Pixelify usa os tamanhos já rasterizados para cada resolução. Setas,
A/D e TAB mudam o foco; ENTER/Espaço e clique confirmam; ESC escolhe a segunda ação.
Entradas mantidas na transição aguardam soltura para evitar confirmação acidental.

Verificação: `cmake --build build --target result_screen_check` e
`build/result_screen_check.exe`. O teste cobre revanche nos dois encontros,
equipe/fórmulas/progresso, espera pelo diálogo final, menu/reinício, teclado/mouse,
resoluções, recursos ausentes e desenho imutável. Capturas em `build/result_preview/`.

## Confirmação ao sair

ESC na exploração livre ou na raiz do combate abre **Sair da partida?**, com o
aviso **O progresso não será salvo**. **Continuar jogando** é a opção inicial;
ENTER confirma o foco, clique escolhe a ação e ESC cancela. **Sair para o menu**
retorna à tela inicial, sem salvar. A exploração e os temporizadores do combate
não avançam com o modal aberto. Diálogos, central e submenus conservam o ESC de
fechamento; ele só pede confirmação quando a ação abandonaria a partida.

Ao vencer a Babosa e retornar da batalha, a recompensa abre um aviso no mesmo
estilo das placas: **Chave do alçapão**. Ele confirma a vitória, a obtenção da
Chave da Manutenção e sua utilização no alçapão da casa de manutenção. A chave é
entregue imediatamente; fechar o aviso não altera a recompensa. O antigo texto
fixo desenhado no chão do depósito foi removido.

## Descoberta de interações

Objetos interativos visíveis recebem um brilho dourado pixelado com pulso suave,
mesmo à distância. Lia, Bento e Íris recebem um pequeno balão de conversa. Ao
entrar no alcance, o objeto selecionado usa apenas a seta e a instrução atuais,
sem duplicar o brilho. O alcance real de interação permanece igual.

Portas e controles reutilizáveis continuam destacados. Pistas já lidas e a placa
do jardim deixam de brilhar; ainda podem ser relidas de perto. Guardiões vencidos
não recebem marcador. A placa lógica continua marcada por permitir editar os
golpes. O brilho de um acesso bloqueado tem cor discreta, e a explicação do
bloqueio continua disponível de perto. Os ícones são ocultados em diálogos,
central, editor lógico e conclusão. Só aparecem na sala e no enquadramento atuais,
com tamanho fixo em pixels e sem indicadores presos às bordas.

Verificação: `cmake --build build --target discovery_markers_check` e
`build/discovery_markers_check.exe`. Capturas em `build/discovery_preview/`.

Os indicadores da Babosa e das alavancas ficam acima das artes, sem cobrir a planta
ou os manípulos. A pressão da ponte usa um quadro no canto superior direito da
tela; não fica mais sobre o manômetro. A placa lógica tem colisão no caminho,
permitindo aproximar-se para interagir e contornar pelos lados. O aviso da
escotilha bloqueada é apenas **Escotilha trancada**.

Os dois guardiões usam as sprites `*_v2.png` da arena na exploração, recortadas
pelo alfa e com filtro pontual. O Girassol tem 174 pixels de altura no mundo e a
Babosa mantém 190. Ambos repousam na mesma arte de terra da batalha, ajustada
à escala da sala, sem nomes desenhados no chão. A identificação fica no aviso de proximidade. O marcador do Girassol acompanha
sua nova altura. `WorldAssets` carrega e libera a textura de terra junto aos bosses;
a ausência da arte usa uma elipse de terra como reserva.
