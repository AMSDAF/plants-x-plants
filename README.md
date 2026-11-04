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

## Como executar no Windows

### Ferramentas necessárias

- Git.
- CMake 3.24 ou superior.
- GCC com suporte a C11, usando uma distribuição MinGW-w64, como w64devkit.
- Ninja.

O VS Code é opcional: os comandos abaixo podem ser executados pelo PowerShell.

As ferramentas devem estar disponíveis no PATH. Após instalar ou alterar o PATH, reabra o terminal e, caso esteja usando o VS Code, reinicie o editor.

Verifique a instalação:

```powershell
git --version
cmake --version
gcc --version
ninja --version
```

O ambiente inicial foi validado com:

| Ferramenta | Versão |
|---|---|
| Git | 2.55.0.windows.4 |
| CMake | 4.3.1 |
| GCC | 15.2.0 |
| Ninja | 1.13.2 |

Essas são as versões utilizadas na validação, não requisitos de versões exatas.

### Obter o projeto

```powershell
git clone https://github.com/AMSDAF/plants-x-plants.git
cd plants-x-plants
```

Caso já tenha clonado o repositório, entre na pasta existente.

### Configurar

Na raiz do projeto, execute:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc
```

O CMake baixa a raylib 5.5 automaticamente por meio do Git. Não é necessário instalar a biblioteca separadamente.

A primeira configuração exige conexão com a internet e pode demorar alguns minutos sem exibir novas mensagens.

Aguarde a conclusão:

```text
-- Configuring done
-- Generating done
-- Build files have been written to: ...
```

### Compilar

```powershell
cmake --build build
```

Execute novamente esse comando após alterar o código. O CMake recompila os arquivos necessários.

### Executar

A partir da raiz do projeto:

```powershell
.\build\plants_x_plants.exe
```

A versão atual apresenta uma janela com uma tela provisória.

Para sair, pressione ESC ou feche a janela pelo botão X.

### Arquivos gerados

A pasta `build/` contém arquivos temporários, dependências baixadas e o executável. Ela é ignorada pelo Git e não deve ser adicionada aos commits.

Os comandos de configuração não devem conter caminhos absolutos específicos do computador de um integrante.

### Problemas comuns

**Comando não reconhecido**

Confirme a instalação da ferramenta e sua presença no PATH. Depois, reabra o terminal ou reinicie o VS Code.

Para localizar o GCC utilizado:

```powershell
(Get-Command gcc).Source
```

**Configuração aparentemente parada**

Na primeira execução, o CMake pode estar baixando a raylib sem mostrar progresso. Aguarde o término ou uma mensagem de erro.

**Conflito de gerador ou troca de compilador**

Use outra pasta de build para criar uma configuração independente:

```powershell
cmake -S . -B build-clean -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc
cmake --build build-clean
.\build-clean\plants_x_plants.exe
```

**Aviso de compatibilidade do CMake na raylib**

O ambiente validado exibiu um aviso de depreciação vindo do CMakeLists.txt da raylib. Esse aviso não impediu a configuração nem a compilação.

**Avisos sobre arquivos-fonte vazios**

Nesta etapa, alguns módulos contêm apenas comentários e podem gerar avisos com `-Wpedantic`. Eles serão implementados nas próximas tarefas.