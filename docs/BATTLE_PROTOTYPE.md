# Relatório do protótipo de combate

No menu, abra **Versus local**, escolha **VS CPU** ou **VS PLAYER** e confirme.
Monte a equipe e o Golpe Lógico para entrar na arena.
A arena mostra a planta ativa, o HP das duas equipes, de quem é o turno e o
resultado da última ação. As plantas são desenhadas com formas da raylib.

| Comando | Ação |
| --- | --- |
| **1** ou **2** | Usar um dos dois golpes da planta ativa. |
| **3** | Usar o Golpe Lógico se a fórmula estiver verdadeira. |
| **Q/W/E/R** | Escolher uma das quatro posições da equipe para entrar em campo. |
| **ENTER** no resultado | Recomeçar no mesmo modo com as duas equipes restauradas. |
| **M** ou **ESC** no resultado | Retornar ao menu. ESC também funciona durante a partida. |

Os golpes, as trocas e os botões do resultado também aceitam cliques.

## Monte seu próprio golpe

Antes da luta, cada jogador humano combina duas condições distintas. Use
**Q/W** para mudar as condições, **E** para mudar o operador e **Enter** para
confirmar. Os mesmos controles estão disponíveis por clique. A fórmula vale
para a equipe inteira, acompanha a planta ativa e é preservada no reinício.
Para montar outra fórmula, volte ao menu e inicie uma nova partida.

As condições disponíveis são ter uma reserva viva, o rival ter uma reserva
viva, a planta ativa estar viva com até metade do HP e a planta ativa rival
estar viva com até metade do HP. Uma reserva é uma planta viva fora da posição ativa.

| Operador | Regra | Poder |
| --- | --- | --- |
| **E** | As duas condições são verdadeiras. | 60 |
| **OU** | Pelo menos uma condição é verdadeira. | 45 |
| **XOR** | Exatamente uma condição é verdadeira. | 55 |

O OU oferece o menor poder por ativar em mais combinações da tabela-verdade.
O E oferece o maior poder por exigir ambas as condições. O XOR fica entre os
dois. Todos têm 100% de precisão e seguem a fórmula de dano dos demais golpes.

Por exemplo, com **Sua ativa tem até metade do HP E O adversário tem reserva
viva**, o golpe começa bloqueado com `F E V = F`. Se sua planta perder
metade do HP ou mais e o rival ainda tiver reserva, a arena passa a mostrar
`V E V = V` e libera o golpe. O motor reavalia a expressão ao receber o
comando, de modo que tentar usar um golpe bloqueado preserva HP e turno.

## Equipes e turnos

Cada jogador humano escolhe de duas a quatro plantas de nível 1 entre Cacto,
Girassol, Babosa, Rosa, Hera, Bambu, Samambaia, Lavanda, Orquídea, Lótus,
Hortelã e Trevo. A CPU sorteia espécies distintas e monta um grupo do mesmo
tamanho. Cada espécie tem atributos e dois golpes próprios.

Selecione uma vaga com **Q/W/E/R** ou clique nela. Depois selecione a espécie
com **setas** e **Espaço** ou clique no cartão. A primeira posição começa
ativa. Se escolher uma espécie já presente para uma vaga ocupada, as duas
trocam de lugar. Para adicionar em uma vaga livre, escolha uma espécie que
ainda esteja fora do grupo. **Backspace** remove uma reserva, mantendo
pelo menos duas plantas no grupo.

O Jogador 1 faz a primeira ação. Reiniciar restaura o HP e a ordem inicial
das equipes escolhidas. Cada equipe comporta até quatro plantas.

1. Em um turno normal, a equipe pode atacar ou colocar em campo uma reserva viva.
   Qualquer uma dessas ações passa o turno ao adversário. Tentar trocar pela
   planta já ativa, por uma derrotada ou por uma posição inexistente mantém
   o estado e o turno.
2. Quando um ataque derruba a planta ativa e ainda há reserva viva, o combate
   passa ao estado de substituição obrigatória. O lado que sofreu o nocaute
   só pode escolher uma reserva. Depois da escolha, esse mesmo lado faz seu
   turno normal.
3. Quando uma equipe perde a última planta, o combate registra o vencedor
   imediatamente. A tela continua exibindo a última ação e oferece reinício
   ou retorno ao menu. O motor rejeita ataques e trocas após o resultado.

## Dano e precisão

A precisão vem do catálogo da espécie. Um golpe com 100% de precisão acerta
sempre. Os demais usam um sorteio de 0 a 99 e acertam quando o número sorteado
é menor que a precisão. Um erro de precisão é uma ação válida: gasta o turno
e deixa o HP do alvo intacto.

O motor preserva a fórmula `máx(5, poder + ataque − defesa / 2)`, com divisão
inteira. O HP fica limitado a zero e a mensagem informa quanto HP o alvo
perdeu. Os atributos e poderes do catálogo foram preservados. Com os valores
atuais, **Espinho Afiado** continua sendo a opção mais forte e mais precisa
do Cacto.

## Computador e interface

A CPU usa uma fórmula fixa de **E** entre HP baixo da própria planta ativa e
HP baixo da planta ativa rival. Quando sua fórmula é verdadeira, o golpe
personalizado entra na mesma comparação que os demais. A CPU escolhe o golpe
com maior produto de poder por precisão. Esse critério
é uma aproximação simples e ignora o HP do alvo. Para repor uma planta
derrotada, escolhe a primeira reserva válida na ordem da equipe. A CPU só
troca quando a substituição é obrigatória.

A tela acumula o tempo dos quadros antes de cada ação da CPU. Isso dá tempo
para ler as mensagens enquanto a janela continua desenhando e aceitando ESC.
A interface e a CPU chamam as mesmas funções de ataque e troca. O motor
valida a ação e modifica o estado. O desenho apenas consulta esse estado,
e `game.c` coordena entrada, reinício e saída da arena.

## Como validar

Execute os comandos na raiz do projeto.

```powershell
cmake --build build --target plants_x_plants versus_screen_check
.\build\versus_screen_check.exe
```

A verificação injeta eventos de teclado e mouse pelo fluxo real do jogo.
Ela cobre partidas em ambos os modos, substituições, troca voluntária, erro de
precisão, rejeição de ações inválidas, vitória, reinício e retorno ao menu.
Também verifica as 12 combinações das tabelas-verdade, a montagem independente,
o bloqueio da fórmula falsa e o uso do golpe por jogador e CPU.
Na montagem da equipe, a verificação cobre adicionar e remover reservas,
evitar duplicatas, iniciar com quatro plantas contra a CPU e usar a quarta
posição em combate.
As capturas da arena ficam em `build/versus_preview/`.

A compilação do jogo e a verificação automatizada do Versus passaram.
Um teste isolado do motor também confirmou a
vitória do segundo lado e verificou que golpes e trocas com índices inválidos
deixam o estado intacto.
