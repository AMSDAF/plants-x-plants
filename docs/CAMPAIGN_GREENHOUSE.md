# Estufa Lógica

Abra **Campanha**, escolha o personagem, confirme o nome e explore a estufa.
Use **WASD** ou **setas** para mover e **E** para interagir com o objeto mais
próximo. O cenário usa formas da raylib e os sprites de treinador já presentes.

## Mapa e comporta

A câmera acompanha o personagem entre o pátio, o subsolo, o depósito, os
canais e o viveiro. A cabine do alarme fica ao sul do pátio e a válvula
fica do outro lado da ponte, ao norte dos canais. A comporta do viveiro
avalia `Água E (Luz OU Chave) E NÃO Alarme`.

Para satisfazer o **OU**, há duas rotas. O distribuidor no subsolo pode
alimentar o refletor em vez do elevador. Nesse caso, use o corredor ao sul
do pátio para voltar. A alternativa é vencer a Babosa no depósito e
receber a chave de manutenção. Depois da primeira travessia, a comporta
fica travada aberta para permitir o retorno.

## Ponte

Cada controle alterna o seu canal. A ponte estende quando exatamente um
dos canais está aberto. O painel usa `Norte XOR Sul`.

| Norte | Sul | Ponte |
| --- | --- | --- |
| F | F | Recolhida |
| F | V | Estendida |
| V | F | Estendida |
| V | V | Recolhida |

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

`world.c` calcula as regras da comporta e da ponte a partir do estado dos objetos. As mesmas
funções determinam o resultado mostrado, a abertura desenhada e a colisão.
A caixa dos pés só atravessa dentro do vão quando a fórmula correspondente
é verdadeira. A central valida as nove restrições sobre a configuração atual
dos relés antes de concluir a missão. O desenho consulta o estado. `game.c`
coordena a câmera e as transições entre exploração e combate, e aplica a
recompensa do encontro depois de uma vitória.

No Windows, execute a partir da raiz:

```powershell
cmake --build build --target plants_x_plants name_entry_check
.\build\name_entry_check.exe
```

O teste percorre a missão pela rota da chave depois de confirmar o nome.
Verifica as tabelas de verdade, os guardiões, o retorno com progresso, a
distribuição de energia, a coleta das pistas, as colisões e a solução única
entre as 256 configurações da central. Também verifica o avanço guiado,
o retorno a uma regra quebrada, a rejeição na última etapa e a conclusão.
As capturas ficam em `build/name_entry_preview/`.
