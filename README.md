# Plants x Plants 🌱⚔️

Jogo de batalhas por turnos entre plantas, inspirado no sistema de combate de Pokémon e desenvolvido como projeto acadêmico em **C com raylib**, para **Windows**.

Monte sua equipe, evolua suas plantas e escolha seu caminho por diferentes biomas até enfrentar o boss.

## Status

Em planejamento. As funcionalidades abaixo representam o escopo previsto.

## Modos de jogo

### Campanha

O jogador começa com uma planta no estágio de mudinha e constrói sua equipe durante a jornada, recrutando plantas que enfrentou.

A campanha inclui um tutorial e três arenas, escolhidas entre rotas alternativas.

### Versus local

Dois jogadores montam suas equipes e se enfrentam no mesmo computador, alternando suas ações por turno. Não é necessário concluir a campanha para jogar esse modo.

## Sistema de combate

- Cada equipe pode ter até quatro plantas.
- Apenas uma planta de cada equipe fica ativa por vez.
- No turno, o jogador pode usar um golpe ou trocar a planta ativa.
- A troca voluntária consome o turno.
- Quando uma planta é derrotada, o jogador escolhe outra disponível.
- Vence quem derrotar todas as plantas da equipe adversária.

## Progressão da campanha

| Etapa | Progressão |
|---|---|
| Início | Escolha de uma planta inicial no estágio de mudinha. |
| Tutorial | Após a primeira luta, recrutamento de uma das plantas enfrentadas. Uma segunda luta apresenta a troca de plantas. |
| Primeira arena | Escolha de rota, recrutamento da terceira planta e evolução da equipe para o segundo estágio após a vitória. |
| Segunda arena | Nova escolha de rota, recrutamento da quarta planta e evolução da equipe para o estágio final após a vitória. |
| Arena final | Batalha contra o boss da rota escolhida, com a equipe completa. |

A vida da equipe é recuperada entre arenas.

## Arenas e rotas

Existem seis arenas, distribuídas em três etapas, com duas opções de bioma por etapa.

O jogador enfrenta apenas uma arena de cada etapa. Assim, cada campanha possui três arenas, além do tutorial.

Cada bioma terá adversários próprios e uma regra de combate simples. A escolha da rota também influencia quais plantas estarão disponíveis para recrutamento.

Os biomas e seus efeitos ainda serão definidos.

## Recrutamento e evolução

Após a primeira luta do tutorial e as vitórias nas duas primeiras arenas, o jogador escolhe uma das plantas enfrentadas para integrar sua equipe.

Cada espécie possui três estágios evolutivos:

1. Mudinha.
2. Planta jovem.
3. Forma final.

A evolução acontece após a primeira e a segunda arena, sem sistema de experiência ou níveis individuais. Plantas recrutadas entram no estágio atual da equipe.

As evoluções são fixas por espécie e não dependem do bioma escolhido.

## Tecnologias

- **Linguagem:** C.
- **Biblioteca gráfica:** raylib.
- **Plataforma inicial:** Windows.
- **Controle de versão:** Git e GitHub.

## Convenções

- Código e identificadores em inglês.
- Textos do jogo em português.
- Commits em inglês, seguindo Conventional Commits.

## Como executar

As instruções de instalação, compilação e execução serão adicionadas após a configuração inicial do projeto.