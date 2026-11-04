# Backlog — Plants x Plants

O controle das tarefas é realizado pelo GitHub Issues e pelo quadro do GitHub Projects.

- [Quadro Kanban](https://github.com/users/AMSDAF/projects/2)
- [Issues do projeto](https://github.com/AMSDAF/plants-x-plants/issues)
- [Arquitetura](ARCHITECTURE.md)

Este documento registra o escopo da entrega e as regras de organização. Status, responsáveis e critérios de aceite são mantidos nas issues, evitando duplicação.

## Entrega — Protótipo jogável

Data prevista: **10/11/2026**.

O protótipo deve permitir abrir o jogo no Windows, escolher um modo, concluir uma batalha e iniciar outra partida.

### Escopo

- Modos contra o computador e versus local.
- Equipes predefinidas com duas plantas por lado.
- Capacidade máxima de quatro plantas por equipe.
- Duas espécies: cacto e girassol.
- Dois golpes de dano por espécie.
- Uma planta ativa por equipe.
- Combate com turnos alternados.
- Troca voluntária consumindo o turno.
- Substituição obrigatória após uma planta ser derrotada.
- Após a substituição, o turno pertence à equipe que perdeu a planta.
- Vitória ao derrotar todas as plantas adversárias.
- Interface com vida, turno, golpes e mensagens.
- IA simples, com escolha de golpes válidos e substituição de plantas.
- Tela de resultado, reinício e retorno ao menu.
- Visual provisório com formas geométricas ou sprites simples.
- Executável para Windows com os recursos necessários.

### Fora desta entrega

As funcionalidades abaixo continuam previstas para o jogo, mas não fazem parte do primeiro protótipo:

- Campanha e rotas alternativas.
- Biomas e regras de arena.
- Recrutamento.
- Evolução das plantas.
- Seleção personalizada de equipes.
- Efeitos especiais de golpes.
- Salvamento de progresso.
- Áudio e arte final.

## Épicos

| Identificador | Épico | Issue |
|---|---|---|
| EPIC-01 | Fundação executável | [#1](https://github.com/AMSDAF/plants-x-plants/issues/1) |
| EPIC-02 | Plantas e equipes | [#2](https://github.com/AMSDAF/plants-x-plants/issues/2) |
| EPIC-03 | Combate por turnos | [#3](https://github.com/AMSDAF/plants-x-plants/issues/3) |
| EPIC-04 | Interface jogável | [#4](https://github.com/AMSDAF/plants-x-plants/issues/4) |
| EPIC-05 | Modos e fluxo da partida | [#5](https://github.com/AMSDAF/plants-x-plants/issues/5) |
| EPIC-06 | Validação e entrega | [#6](https://github.com/AMSDAF/plants-x-plants/issues/6) |

Cada épico possui um checklist com links para suas tarefas.

Os identificadores `PVP-001` a `PVP-018` são referências internas e foram mantidos após a definição do nome Plants x Plants. Eles não correspondem à numeração automática das issues.

## Uso do quadro

| Coluna | Significado |
|---|---|
| Backlog | Tarefa planejada, ainda não liberada para execução. |
| Ready | Tarefa com dependências concluídas e pronta para ser assumida. |
| In Progress | Tarefa em desenvolvimento, com responsável definido. |
| Done | Tarefa concluída e validada conforme os critérios de aceite. |

Caso as colunas ainda tenham outros nomes, esta é a organização proposta para o quadro.

## Como assumir uma tarefa

1. Ler a descrição, os arquivos envolvidos e os critérios de aceite.
2. Conferir se as dependências foram concluídas.
3. Atribuir a issue ao próprio usuário e mover para In Progress.
4. Criar uma branch a partir da main atualizada.
5. Implementar apenas o escopo da tarefa.
6. Compilar e validar o comportamento alterado.
7. Abrir um pull request, vinculando a issue.
8. Após revisão e merge, fechar a issue e conferir sua posição em Done.

Impedimentos devem ser registrados na própria issue. Uma tarefa bloqueada não deve ser marcada como concluída.

## Convenções de desenvolvimento

Exemplo de branch:

`feat/pvp-008-battle-attacks`

Exemplo de commit:

`feat: implement turn-based attacks`

No corpo do pull request, usar `Closes #14` para vincular e fechar a issue correspondente após o merge na branch padrão.

Antes de alterar estruturas compartilhadas ou assinaturas de funções, avisar os integrantes que trabalham nos módulos afetados.

## Critério de conclusão

Uma tarefa está concluída quando:

- Todos os critérios de aceite foram atendidos.
- O projeto compila com a alteração.
- O comportamento afetado foi validado.
- A documentação foi atualizada, quando necessário.
- A alteração foi revisada e integrada à main.

Criar o arquivo ou escrever parte do código não significa concluir a tarefa.

## Ordem de execução

A base executável e as estruturas de dados devem ser implementadas primeiro, seguidas pelas regras de combate.

Com as estruturas e funções do combate definidas, interface e modos de jogo podem avançar em paralelo.

A prioridade é obter uma batalha completa o quanto antes. Melhorias visuais entram depois que o fluxo principal estiver funcionando.

Meta interna: ter uma partida completa até domingo, **08/11**, e reservar segunda-feira, **09/11**, para correções, empacotamento e teste da apresentação.