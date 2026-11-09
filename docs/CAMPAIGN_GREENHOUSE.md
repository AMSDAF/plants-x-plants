# Estufa Lógica

Abra **Campanha**, escolha o personagem, confirme o nome e explore a estufa.
Use **WASD** ou **setas** para mover e **E** para interagir com o objeto mais
próximo. O cenário usa formas da raylib e os sprites de treinador já presentes.

## Comporta

A válvula alterna **Água**, o refletor alterna **Luz** e pegar a chave torna
**Chave** verdadeira durante a missão. A regra é `Água E (Luz OU Chave)`.
A água é obrigatória. Com a água ligada, qualquer uma destas combinações abre
a comporta:

- Ligar o refletor.
- Pegar a chave no desvio.
- Ligar o refletor e pegar a chave.

O painel substitui cada proposição por **V** ou **F**. Por exemplo,
`V E (F OU V) = V` mostra uma solução com água e chave.

## Ponte

Cada controle alterna o seu canal. A ponte estende quando exatamente um
dos canais está aberto. O painel usa `Norte XOR Sul`.

| Norte | Sul | Ponte |
| --- | --- | --- |
| F | F | Recolhida |
| F | V | Estendida |
| V | F | Estendida |
| V | V | Recolhida |

Atravesse a ponte e pressione **E** na placa de saída para concluir. **Enter**
ou **Esc** retorna ao menu na tela de conclusão. Abrir a campanha novamente
reinicia os objetos da estufa.

## Implementação e teste

`world.c` calcula as duas regras a partir do estado dos objetos. As mesmas
funções determinam o resultado mostrado, a abertura desenhada e a colisão.
A caixa dos pés só atravessa dentro do vão quando a fórmula correspondente
é verdadeira. O desenho consulta o estado e `game.c` coordena as transições.

No Windows, execute a partir da raiz:

```powershell
cmake --build build --target plants_x_plants name_entry_check
.\build\name_entry_check.exe
```

O teste percorre a missão depois de confirmar o nome. Verifica as tabelas
de verdade completas, as duas formas de abrir a comporta, as colisões,
o fechamento da ponte com os dois canais abertos e a conclusão pela saída.
As capturas ficam em `build/name_entry_preview/`.
