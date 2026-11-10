# Estufa Lógica

Abra **Campanha**, escolha o personagem, confirme o nome e explore a estufa.
Use **WASD** ou **setas** para mover, segure **Shift** para correr e pressione
**E** quando aparecer o marcador de interação. O cenário usa formas da raylib
e os sprites de treinador já presentes.

## Prédios e acesso ao viveiro

A câmera acompanha o personagem no exterior e se reposiciona ao atravessar
uma porta, usar o elevador ou voltar ao pátio após uma derrota. A casa de
manutenção, o depósito e o viveiro têm fachadas sólidas e interiores próprios.
O painel indica a região atual.

1. Entre no depósito e vença a Babosa para receber a **Chave da Manutenção**.
2. Na casa de manutenção, desative o alarme, registre a pista e abra a
   escotilha com a chave. A escotilha dá o primeiro acesso ao subsolo.
3. No subsolo, o distribuidor alimenta o elevador ou o refletor. O elevador
   vira um atalho após a primeira descida. Quando a energia vai para o
   refletor, a escotilha continua disponível para voltar.
4. A válvula da água e a segunda pista ficam do outro lado da ponte. A porta
   do viveiro abre quando `Água E Luz E NÃO Alarme` vale. Depois da primeira
   entrada, a porta permanece liberada.

## Ponte

Os dois canais ficam dentro da **Casa de Controle da Ponte**, ao sul do
pátio. O norte acrescenta **3** à pressão e o sul acrescenta **5**. O
dreno externo começa aberto e retira **2** unidades, com mínimo de zero.
A ponte exige `(Norte XOR Sul) E (Pressão = 3)`.

| Canal único aberto | Dreno | Pressão | Ponte |
| --- | --- | --- | --- |
| Norte | Fechado | 3 | Estendida |
| Norte | Aberto | 1 | Recolhida |
| Sul | Fechado | 5 | Recolhida |
| Sul | Aberto | 3 | Estendida |

O painel mostra o XOR e a pressão separadamente. Com os dois canais
abertos ou os dois fechados, a travessia fica bloqueada.

## Guardiões e central de relés

Interaja com um guardião para lutar na arena. Monte o grupo e o Golpe
Lógico no primeiro encontro. A equipe recupera o HP entre lutas e mantém
a composição escolhida. Uma derrota ou recuo leva o jogador de volta ao
pátio, preservando o progresso do mapa.

Vencer o Girassol no viveiro libera a central. Uma regra pede a pista dos
canais e outra pede a pista da cabine do alarme. O painel tem oito relés e
nove restrições simultâneas. Elas combinam XOR, equivalência, implicação e
uma contagem exata de relés ligados. Há uma única configuração válida
entre as 256 possíveis.

Use **1–8** ou clique para alternar um relé. A central mostra uma regra por
vez. **Enter** valida a configuração até a regra atual e avança quando todas
essas regras forem verdadeiras. Se uma tentativa falhar, a central mostra
a primeira regra violada. As nove precisam valer juntas na última etapa.
Use **seta esquerda** para rever a regra anterior e **Esc** para fechar
a central. A etapa, a configuração e as tentativas são preservadas ao sair
do painel. Depois de resolver, **Enter** ou **Esc** retorna ao menu.

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
