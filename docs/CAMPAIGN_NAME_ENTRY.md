# Nome da campanha

A confirmação de personagem abre “Como você se chama?”. O nome começa vazio;
ESC/Voltar retorna à seleção sem perder o rascunho ou a escolha. Somente uma
confirmação válida grava nome aparado e personagem em `Game.campaign` e inicia
a exploração. Não há persistência em disco.

`name_entry_screen` cuida da edição, foco e composição; `game` apenas coordena
as ações. `selection_ui` compartilha paleta e bordas em degraus. O cartão usa
os limites reais da seleção e chega a (172,140,308,440), escalando retrato e
halo juntos, uniformemente. A transição dura 0,60 s, com curva suave sem
ultrapassagem: elementos antigos desaparecem nos primeiros 0,15 s; a nova
interface sobe 12 pixels e aparece entre 0,30 e 0,60 s. O cartão móvel fica à
frente do formulário. A volta percorre os mesmos extremos. Fundo e repousos
continuam nos relógios existentes; fontes, atlas e tempos aprovados não mudam.
O rótulo Homem/Mulher e a seta desaparecem nos primeiros 0,15 s da ida e
reaparecem nos últimos 0,15 s da volta; permanecem visíveis na seleção.

O usuário aprovou manualmente a transição de entrada de nome. Essa aprovação
não implica teste manual das demais funcionalidades.

O campo aceita letras Unicode, dígitos decimais, espaços, apóstrofo e hífen,
com até 12 codepoints em 49 bytes UTF-8. Espaços Unicode são normalizados;
controles e símbolos são rejeitados. Backspace remove um codepoint completo,
com atraso de 0,40 s e repetição de 0,055 s. O contador mede caracteres, não
bytes. A fonte pixel existente cobre os acentos portugueses pré-compostos
(incluindo ç, ã e ô); outros alfabetos usam o glifo substituto da raylib.
Sequências com acentos combinantes não são compostas automaticamente.
`tools/prepare_name_unicode.py` regenera a tabela de categorias L/Nd usando
`unicodedata` do Python, sem dependência de execução.

TAB/SHIFT+TAB percorre campo, Voltar e Confirmar. Mouse define o foco; botões
ativam na soltura dentro da mesma área. ENTER no campo confirma; nos botões
ativa o botão focado. Nomes vazios ou só com espaços mostram aviso no painel.
Durante a transição, texto enfileirado é descartado e controles ficam bloqueados;
ao terminar é necessário soltar teclas/mouse antes de nova ativação. A/D e
setas não alteram o personagem durante a edição.

## Verificação reproduzível

A partir da raiz, no Windows com OpenGL disponível:

```powershell
cmake --build build
cmake --build build --target name_entry_check selection_scene_check menu_screen_check trainer_background_check
.\build\name_entry_check.exe
.\build\selection_scene_check.exe
.\build\menu_screen_check.exe
.\build\trainer_background_check.exe
.\build\trainer_tools\Scripts\python tools/preview_name_entry.py
git diff --check
```

`name_entry_check` usa eventos raylib e WM_CHAR enviados à própria janela de
teste. Cobre Antônio/João, 12 caracteres, UTF-8 de quatro bytes, remoção
multibyte/repetida, espaços, foco por teclado/mouse, entradas mantidas,
descarte de texto, retenção do rascunho e transferência para a sessão.
Captura ambos os personagens nos dois sentidos, verifica relógios contínuos
e desenho sem mutação. O montador compara os extremos com a seleção pixel
a pixel e gera GIFs, contatos e ampliações em `build/name_entry_preview/`.
São trechos de 0,60 s com pausas nos extremos; o reinício dos GIFs não representa
um ciclo fechado do cenário, cujo relógio continua avançando.

A inspeção visual das capturas OpenGL verifica enquadramento, acentos, halo,
recortes e sobreposição durante o movimento. Essa inspeção das capturas não
substitui um teste manual completo com teclado físico. A arte dos
repousos conserva as pequenas diferenças já documentadas em
[TRAINER_SELECTION.md](TRAINER_SELECTION.md).

Validação desta implementação: build, os quatro verificadores acima e
`git diff --check` executados com sucesso. Os dois layouts finais e os contatos
de ida/volta em 0,00/0,30/0,45/0,60 s foram inspecionados; sem recortes aparentes
ou salto nos extremos. Não houve teste manual com teclado físico.
