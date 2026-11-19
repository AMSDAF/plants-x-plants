# Estufa Lógica

Pressione **ENTER** na tela inicial, escolha o personagem, confirme o nome e explore a estufa.
Use **WASD** ou **setas** para mover, segure **Shift** para correr e pressione
**E** quando aparecer o marcador de interação.

## Prédios e acesso ao viveiro

A câmera acompanha o personagem no exterior e se reposiciona ao atravessar
uma porta, usar o elevador ou voltar ao pátio após uma derrota. A casa de
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
objetivo atual e as pistas reunidas. Ao se aproximar de um objeto, a ação
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
3. No subsolo, o distribuidor alimenta o elevador ou o refletor. O elevador
   vira um atalho após a primeira descida. Quando a energia vai para o
   refletor, a escotilha continua disponível para voltar.
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

Vencer o Girassol no viveiro libera a central. Uma regra pede a pista dos
canais e outra pede a pista do bilhete na casa de manutenção. O painel tem oito relés e
nove restrições simultâneas. Elas combinam XOR, equivalência, implicação e
uma contagem exata de relés ligados. Há uma única configuração válida
entre as 256 possíveis.

Use **1–8** ou clique para alternar um relé. A central mostra uma regra por
vez. **Enter** valida a configuração até a regra atual e avança quando todas
essas regras forem verdadeiras. Se uma tentativa falhar, a central mostra
a primeira regra violada. As nove precisam valer juntas na última etapa.
Use **seta esquerda** para rever a regra anterior e **Esc** para fechar
a central. A etapa, a configuração e as tentativas são preservadas ao sair
do painel. Depois de resolver, Íris agradece em uma conversa com o nome do jogador.
Ao fechar essa conversa, a tela de conclusão permite voltar ao menu com
**Enter** ou **Esc**.

## Implementação e teste

`world.c` mantém a região ativa, as transições por proximidade, a colisão
das fachadas e da ponte e as regras de acesso. A central valida as nove
restrições sobre a configuração atual
dos relés antes de concluir a missão. O desenho consulta o estado. `game.c`
coordena a câmera e as transições entre exploração e combate, e aplica a
recompensa do encontro depois de uma vitória.

No Windows, execute a partir da raiz:

```powershell
cmake --build build --target plants_x_plants name_entry_check
.\build\name_entry_check.exe
```

O teste percorre a missão depois de confirmar o nome. Verifica a colisão
das fachadas e paredes, as oito combinações da ponte e suas duas soluções,
a escotilha e o elevador antes e depois da chave, o retorno
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
`aloe_guard.png` e `sunflower_guard.png` em `assets/sprites/`, são referenciados
pelo código de interiores, mas não acompanharam o commit remoto integrado.
Enquanto estiverem ausentes, o jogo desenha as salas e os guardiões com as
formas de reserva. A manutenção continua usando sua ilustração existente.

A validação usa caminhos livres diante dos móveis sólidos e preserva os testes
de bloqueio, acesso aos controles, vitória/derrota e progressão dos encontros.
