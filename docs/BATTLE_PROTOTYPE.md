# Relatório do protótipo de combate

Na campanha, interaja com um guardião para iniciar um encontro.
No primeiro encontro, monte a equipe e o Golpe Lógico antes de lutar.
A arena mostra a planta ativa, o HP das duas equipes, de quem é o turno e o
resultado da última ação. As plantas são desenhadas com formas da raylib.

| Comando | Ação |
| --- | --- |
| **1** ou **2** | Usar um dos dois golpes da planta ativa. |
| **3** | Usar o Golpe Lógico se a fórmula estiver verdadeira. |
| **Q/W/E/R** | Escolher uma das quatro posições da equipe para entrar em campo. |
| **ENTER** no resultado | Retornar à estufa. |
| **ESC** | Retornar à estufa, inclusive durante a montagem ou a batalha. |

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

O jogador escolhe de duas a quatro plantas entre Cacto,
Girassol, Babosa, Rosa, Hera, Bambu, Samambaia, Lavanda, Orquídea, Lótus,
Hortelã e Trevo. O guardião usa espécies predefinidas e monta um grupo do mesmo
tamanho. Cada espécie usa diretamente os atributos base do catálogo e tem dois golpes próprios.

Selecione uma vaga com **Q/W/E/R** ou clique nela. Depois selecione a espécie
com **setas** e **Espaço** ou clique no cartão. A primeira posição começa
ativa. Se escolher uma espécie já presente para uma vaga ocupada, as duas
trocam de lugar. Para adicionar em uma vaga livre, escolha uma espécie que
ainda esteja fora do grupo. **Backspace** remove uma reserva, mantendo
pelo menos duas plantas no grupo.

O jogador faz a primeira ação. Ao retornar à exploração, sua equipe é
restaurada e sua fórmula é preservada para os encontros seguintes. Cada equipe comporta até quatro plantas.

1. Em um turno normal, a equipe pode atacar ou colocar em campo uma reserva viva.
   Qualquer uma dessas ações passa o turno ao adversário. Tentar trocar pela
   planta já ativa, por uma derrotada ou por uma posição inexistente mantém
   o estado e o turno.
2. Quando um ataque derruba a planta ativa e ainda há reserva viva, o combate
   passa ao estado de substituição obrigatória. O lado que sofreu o nocaute
   só pode escolher uma reserva. Depois da escolha, esse mesmo lado faz seu
   turno normal.
3. Quando uma equipe perde a última planta, o combate registra o vencedor
   imediatamente. A tela continua exibindo a última ação e oferece retorno
   à estufa. O motor rejeita ataques e trocas após o resultado.

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

Cada guardião usa uma fórmula fixa de **E**, combinando uma condição de HP
com a existência de reservas na própria equipe. Quando sua fórmula é verdadeira, o golpe
personalizado entra na mesma comparação que os demais. A CPU escolhe o golpe
com maior produto de poder por precisão. Esse critério
é uma aproximação simples e ignora o HP do alvo. Para repor uma planta
derrotada, escolhe a primeira reserva válida na ordem da equipe. A CPU só
troca quando a substituição é obrigatória.

A tela acumula o tempo dos quadros antes de cada ação da CPU. Isso dá tempo
para ler as mensagens enquanto a janela continua desenhando e aceitando ESC.
A interface e a CPU chamam as mesmas funções de ataque e troca. O motor
valida a ação e modifica o estado. O desenho apenas consulta esse estado,
e `game.c` coordena entrada e saída dos encontros.

## Como validar

Execute os comandos na raiz do projeto.

```powershell
cmake --build build --target plants_x_plants battle_check name_entry_check menu_screen_check
.\build\battle_check.exe
.\build\name_entry_check.exe
.\build\menu_screen_check.exe
```

`battle_check` verifica atributos das 12 espécies, tabelas-verdade, ações inválidas,
trocas, derrota, substituição pela IA, vitória e restauração. `name_entry_check`
percorre a campanha com guardiões e progresso. `menu_screen_check` confere a
entrada direta na seleção, teclas mantidas, retorno à tela inicial e encerramento.
O modo de batalha independente e seus recursos exclusivos foram removidos.
