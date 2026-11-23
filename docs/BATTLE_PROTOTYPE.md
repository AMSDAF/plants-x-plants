# Relatório do protótipo de combate

Na campanha, interaja com um guardião para iniciar um encontro.
A equipe do prólogo é preservada e a batalha começa diretamente. Golpes lógicos são recompensas opcionais da placa perto do depósito.
A arena mostra a planta ativa, o HP das duas equipes, a ação preparada e o
resultado da última rodada. A interface usa os cenários próprios do depósito e
viveiro, oito sprites definitivas, Pixelify Sans e o menu Atacar/Trocar. Mochila
e Fugir aparecem bloqueados com aviso de implementação futura.
Detalhes e capturas em [BATTLE_UI_PREPARATION.md](BATTLE_UI_PREPARATION.md).

| Comando | Ação |
| --- | --- |
| **1**, **2** ou **3** | Usar um golpe; o escolhido para o buff só fica disponível quando a fórmula for verdadeira. |
| **Q/W/E/R** | Escolher uma das quatro posições da equipe para entrar em campo. |
| **TAB** | Abrir ou fechar o painel com os status e atributos atuais de todas as plantas. |
| **ENTER** no resultado | Retornar à estufa. |
| **ESC** | Retornar à estufa, inclusive durante a montagem ou a batalha. |

Os golpes, as trocas e os botões do resultado também aceitam cliques.
Use setas e ENTER/Espaço nos menus. ESC, Backspace ou Voltar fecham os submenus
voluntários; ESC na raiz mantém o retorno à estufa. A troca obrigatória abre
automaticamente as reservas. Os atalhos diretos 1/2/3 e Q/W/E/R foram preservados.

## Adquira um golpe lógico por planta

Interaja com a placa ao lado do depósito com **E**. O modal tem abas livres com
os nomes da equipe, sem uma sequência obrigatória. Cada integrante responde uma
pergunta própria (OU, E ou NÃO); a equipe inicial precisa de três acertos para
liberar os três editores. Erros permitem nova tentativa.

Depois do acerto, a própria aba mostra a escolha do golpe de dano, de duas
condições distintas e do operador E, OU ou XOR. **Confirmar configuração** salva
somente essa planta. A pergunta resolvida não aplica bônus nem bloqueio por si
só. Plantas sem configuração confirmada continuam com ataques normais.

O título e o rodapé permanecem fixos. Cada planta preserva seu rascunho ao trocar
de aba ou reabrir a placa, e configurações confirmadas podem ser editadas. ESC
fecha primeiro uma lista aberta; fora da lista, fecha o modal. Clique ou use
TAB/cima/baixo para navegar, esquerda/direita nas abas e operadores, e
ENTER/Espaço para selecionar. Os atalhos 1/2 respondem às perguntas.
Golpes de atributo e condições repetidas aparecem indisponíveis nas listas.

Acertos, rascunhos e confirmações duram a sessão atual e são zerados ao iniciar
outra campanha; não há salvamento em disco. O editor interno da batalha continua
somente como ferramenta de desenvolvimento. O motor oferece as regras abaixo.

As condições disponíveis são a planta que recebe o buff estar viva com até metade
do HP, a planta ativa rival estar viva com até metade do HP, o rival estar com
status (envenenado, queimado, Hospedeiro ou paralisado), o rival ser mais rápido
que a planta em campo, a planta do buff ter ataque maior que a ativa rival, ter
defesa maior que a ativa rival e a própria equipe ter uma reserva viva. A reserva
rival não é mais uma condição. Ataque, defesa e velocidade comparados são os
atuais, incluindo buffs e debuffs. Uma reserva é uma planta viva fora da posição
ativa.
Cada planta escolhe as próprias condições, operador e golpe que recebe o bônus.

| Operador | Regra | Bônus de poder |
| --- | --- | --- |
| **E** | As duas condições são verdadeiras. | +70 |
| **OU** | Pelo menos uma condição é verdadeira. | +45 |
| **XOR** | Exatamente uma condição é verdadeira. | +55 |

O OU oferece o menor bônus por ativar em mais combinações da tabela-verdade.
O E oferece o maior bônus por exigir ambas as condições. O XOR fica entre os
dois. O golpe mantém o próprio nome e precisão; a fórmula aumenta seu poder.
Na batalha, o golpe escolhido para receber o bônus só fica disponível quando
a fórmula daquela planta for verdadeira. Os outros golpes continuam disponíveis
com poder normal. Uma tentativa de usar o golpe bloqueado é rejeitada sem
gastar o turno.

Por exemplo, com **Esta planta tem até metade do HP E Rival com status**, o golpe
fica bloqueado com `F E F = F`. Se sua planta perder metade do HP ou mais e o
rival estiver envenenado, queimado, com Hospedeiro ou paralisado, a arena passa
a mostrar `V E V = V` e libera o golpe fortalecido. Os outros golpes continuam disponíveis pelo poder
base. O motor reavalia a expressão ao receber o comando, então uma fórmula
que deixou de ser verdadeira bloqueia apenas o golpe escolhido para o buff.

## Equipes e turnos

O jogador escolhe três plantas no prólogo entre Trevo, Hera Venenosa, Pimenta,
Cacto, Orquídea e Planta Carnívora. Babosa e Girassol são os bosses do depósito e
do viveiro e não aparecem entre as opções iniciais. O catálogo contém somente
essas oito espécies. Os guardiões lutam sozinhos, sem reservas, contra as três plantas do jogador.
Babosa tem 275 HP e 45 ATQ; Girassol tem 240 HP e 65 ATQ. Defesa, velocidade,
golpes e habilidades foram preservados. O buff lógico é exclusivo do jogador:
os guardiões não recebem fórmula nem golpe fortalecido. Cada espécie usa
diretamente os atributos base do catálogo.
Cada espécie tem três golpes, incluindo buffs/debuffs de atributo quando disponíveis.

A montagem de reserva da interface de batalha continua disponível como alternativa
interna de duas a quatro plantas, usando somente as seis iniciais. Na campanha,
a equipe do prólogo é preservada. Nessa montagem, selecione uma vaga com **Q/W/E/R** ou clique nela. Depois selecione a espécie
com **setas** e **Espaço** ou clique no cartão. A primeira posição começa
ativa. Se escolher uma espécie já presente para uma vaga ocupada, as duas
trocam de lugar. Para adicionar em uma vaga livre, escolha uma espécie que
ainda esteja fora do grupo. **Backspace** remove uma reserva, mantendo
pelo menos duas plantas no grupo.

As ações são escolhidas ao mesmo tempo: você seleciona seu golpe ou sua troca
e a CPU planeja a resposta antes de qualquer dano. As duas ações são então
resolvidas pela velocidade das plantas ativas no início da rodada; em empate,
o jogador resolve primeiro. Se a ação mais rápida derrubar a planta adversária,
a ação lenta já escolhida é cancelada. Ao fim da rodada, a escolha volta ao
jogador. Ao retornar à exploração, sua equipe é
restaurada e sua fórmula é preservada para os encontros seguintes. Cada equipe comporta até quatro plantas.

1. Em uma rodada normal, cada lado pode atacar ou colocar em campo uma reserva
   viva. Tentar trocar pela planta já ativa, por uma derrotada ou por uma
   posição inexistente mantém o estado e não consome a rodada.
2. Quando um ataque derruba a planta ativa e ainda há reserva viva, o combate
   passa ao estado de substituição obrigatória. O lado que sofreu o nocaute
   só pode escolher uma reserva. Depois da escolha, esse mesmo lado participa
   da próxima rodada.
3. Quando uma equipe perde a última planta, o combate registra o vencedor
   imediatamente. A tela continua exibindo a última ação e oferece retorno
   à estufa. O motor rejeita ataques e trocas após o resultado.

## Dano e precisão

A precisão vem do catálogo da espécie. Um golpe com 100% de precisão acerta
sempre. Os demais usam um sorteio de 0 a 99 e acertam quando o número sorteado
é menor que a precisão. Um erro de precisão é uma ação válida: gasta o turno
e deixa o HP do alvo intacto.

O dano usa a fórmula sem nível `((poder × ataque ÷ defesa) ÷ 2 + 2) × multiplicadores`.
Cada golpe possui sua própria chance de crítico, indicada na arena. Um crítico
multiplica o dano por 1,5. O HP fica limitado a zero e a mensagem informa o dano
e se houve crítico. O poder usado inclui o bônus de fórmula quando as condições
são verdadeiras.

Cada espécie possui velocidade base, exibida na montagem da equipe. Ela define
qual das duas ações escolhidas é resolvida primeiro em cada rodada; em empate,
o jogador resolve primeiro. A prioridade dos golpes não altera essa ordem.
A ordem é recalculada a cada rodada com as velocidades vigentes.

Golpes de dano são classificados como **Físico** ou **À distância**. Golpes de
atributo exibem o aumento ou redução correspondente e não causam dano direto.

## Habilidades das espécies

- **Girassol (guardião):** recupera 30% da vida máxima a cada três turnos próprios; seu terceiro golpe aumenta o ataque em 1x, até 3x do valor inicial.
- **Babosa (guardião):** depois de receber dano de um golpe, recupera 25% do dano sofrido naquele golpe (arredondado para baixo).
- **Trevo:** dobra a chance crítica dos próprios golpes, limitada a 100%; seu terceiro golpe aumenta a velocidade em 0,5x, até 2x do valor inicial.
- **Hera Venenosa:** ao receber um golpe físico, tem 30% de chance de envenenar quem atacou; seu terceiro golpe aumenta a defesa em 0,5x, até 2,5x do valor inicial.
- **Pimenta:** gera um fruto a cada dois turnos próprios em campo. O terceiro golpe consome um fruto e tem 30% de chance de causar Queimadura; seu segundo golpe aumenta o ataque em 0,5x, até 2x do valor inicial. O contador aparece enquanto ela está ativa.
- **Cacto:** devolve 20% do dano recebido de golpes físicos; seu terceiro golpe reduz a defesa adversária em passos de 0,5x, até acumular 2x da defesa inicial.
- **Orquídea:** cada golpe que acerta tem 40% de chance de aplicar Hospedeiro; seu terceiro golpe aumenta a defesa em 0,5x, até 2,5x do valor inicial.
- **Planta Carnívora:** cada golpe que acerta tem 20% de chance de paralisar o alvo pela próxima ação; seu terceiro golpe aumenta o ataque em 0,5x, até 2x do valor inicial.

Os golpes de atributo são ações normais e consomem o turno, sem causar dano.
Eles acumulam os aumentos a partir do atributo inicial. O golpe do Cacto usa
`defesa atual × (defesa atual / (defesa atual + debuff))`. Ao trocar a planta,
seus aumentos e reduções temporários desaparecem. Os nomes dos golpes de
atributo são provisórios e podem ser ajustados depois.

## Status

Cada planta comporta até um status de cada tipo; efeitos diferentes coexistem,
mas `Plant_AddStatus` rejeita a segunda aplicação do mesmo tipo. Os tipos
disponíveis são Veneno, Queimadura, Sorte, Paralisado, Hospedeiro e buffs de
velocidade, ataque, defesa e precisão. Ao fim do turno da planta ativa, Veneno
causa dano igual a 1/8 da vida máxima e Queimadura causa 1/16. Hospedeiro causa
1/16 da vida máxima em dano e cura a planta ativa adversária pela quantidade de
vida efetivamente perdida; o efeito permanece na planta afetada mesmo quando ela
volta à reserva e só termina quando a planta que o aplicou é derrotada.
Paralisado bloqueia ataque e troca e consome a próxima ação da planta
afetada, independentemente de qual lado esteja jogando; então o efeito acaba.
A aplicação guarda duração em turnos e um valor:
`PLANT_STATUS_LUCK` usa 2,0 para dobrar a chance crítica quando esse efeito for
conectado ao cálculo de combate; status de buff guardam o multiplicador do
atributo. Duração 0 significa permanente, e `Plant_TickStatuses` reduz durações
temporárias e remove as expiradas. `Team_Restore` limpa todos os status.

`Battle_ApplyStatus` aplica um status a uma planta viva durante uma batalha em
andamento. Os golpes ainda não aplicam status automaticamente. Sorte e os
status de buff ainda aguardam conexão com as regras correspondentes.

## Computador e interface

Os guardiões não usam o buff lógico: todos os golpes de dano da CPU têm poder
normal e nenhum fica bloqueado por fórmula. A CPU usa um golpe de atributo
quando sua planta tem mais de 70% de HP, o adversário ativo tem mais de 30% de
HP e ainda há espaço até o limite. Caso contrário, escolhe o golpe de dano com
maior produto de poder por precisão. Para repor uma planta derrotada, escolhe
a primeira reserva válida na ordem da equipe. A CPU só troca quando a
substituição é obrigatória.

Cada coisa que acontece na rodada ganha dois segundos de tela. Primeiro aparece
o anúncio da ação que vai resolver, como “Planta Carnívora vai usar Morder...”.
Depois o motor executa apenas aquela etapa, e cada frase gerada aparece
separadamente: dano, cura da Babosa, veneno, fruto, paralisia ou troca. Só então
vem o anúncio da próxima ação, na ordem de velocidade. No beat da mensagem da
ação, a planta que agiu avança e volta se usou golpe físico, pula se usou golpe
de status e fica parada se usou golpe à distância. A animação usa o próprio
timer do beat, sem novas regras de combate. Uma barra mostra o tempo
restante, e a janela continua responsiva durante a espera. Se a batalha terminar,
a fila é descartada na hora: a mensagem final fica visível e ENTER retorna sem
esperar o restante do intervalo.
A interface envia as duas ações à mesma API de combate usada pela CPU. O motor
valida as ações e modifica o estado; a tela apenas divide as mensagens e controla
o intervalo. O desenho não modifica o estado, e `game.c` coordena entrada e saída
dos encontros.

## Como validar

Execute os comandos na raiz do projeto.

```powershell
cmake --build build --target plants_x_plants battle_check name_entry_check menu_screen_check
.\build\battle_check.exe
.\build\name_entry_check.exe
.\build\menu_screen_check.exe
```

`battle_check` verifica atributos das oito espécies, as seis iniciais sem bosses,
os golpes e as tabelas-verdade, além de ações inválidas,
trocas, derrota, substituição pela IA, vitória e restauração. `name_entry_check`
percorre a campanha com guardiões e progresso. `menu_screen_check` confere a
entrada direta na seleção, teclas mantidas, retorno à tela inicial e encerramento.
O modo de batalha independente e seus recursos exclusivos foram removidos.

O balanceamento dos guardiões é conferido por `boss_balance_check`; metodologia
e comparação em [BOSS_BALANCE.md](BOSS_BALANCE.md).
