# Elementos extraídos do Complexo Botânico

Material de trabalho para melhorar a arte e, posteriormente, remontar o cenário
em camadas. Fonte: `assets/maps/botanical_courtyard.png`, com 1672 × 941 pixels.
O fundo original, os recursos em uso e as colisões não foram alterados.

## Conteúdo

- `cutouts/`: 32 PNGs em resolução nativa, com máscaras poligonais transparentes.
- `references/`: os mesmos recortes retangulares, preservando o contexto original.
- `masks/`: máscaras em preto e branco; branco mantém o pixel.
- `terrain/`: seis amostras de piso, grama, água, vitórias-régias, borda e muro.
- `contact_sheet.png`: catálogo visual numerado, com o nome de cada elemento.
- `source_overview.png`: localização dos 32 elementos no cenário original.
- `manifest.json`: nomes, grupos, posições, dimensões e máscaras dos recortes.

Prédios: depósito, casa de manutenção, casa de controle da ponte, viveiro e
estufa menor. Também há pergolado, dois portões, cerca, árvores, arbustos,
floreiras, canteiros, placas e bancos.

A ponte estendida já possui um recurso independente no projeto:
`assets/maps/botanical_bridge.png`. Ela não aparece estendida no fundo usado
como fonte e não foi recriada aqui.

## Limites dos recortes

São recortes dos pixels originais, sem geração por IA, ampliação ou melhoria
artificial. As máscaras são **aproximadas**, destinadas à edição da arte:
podem conter pequenos restos de piso/grama e precisam de acabamento nas bordas.
As referências retangulares permitem recuperar detalhes que a máscara cortou.

Objetos sobrepostos permanecem agrupados quando não há informação para
separá-los integralmente: por exemplo, ferramentas/caixas junto ao depósito,
tubos da estufa menor e a árvore central com seu canteiro. Alguns bancos
também têm recortes individuais, além de aparecerem no conjunto maior.
As partes escondidas não foram reconstruídas. As amostras de terreno não
foram preparadas para repetição contínua sem emendas.

## Como melhorar e substituir depois

1. Use `references/<nome>.png` como referência e `cutouts/<nome>.png` como
   ponto de partida; finalize a máscara ou redesenhe o elemento.
2. Preserve a perspectiva, a direção da luz e as proporções do cenário.
3. Guarde os arquivos melhorados em outra pasta. Mantenha o nome de cada
   elemento para relacioná-lo ao manifesto; não sobrescreva esta extração.
4. Se aumentar a resolução, registre o fator de escala. Para remontar o mapa,
   mantenha o enquadramento da tela transparente ou registre a mudança de pivô.
5. Integre os elementos ao renderizador em uma etapa posterior e defina as
   colisões pela base física dos objetos, separadamente da silhueta visual.

`source_rect` usa `[x, y, largura, altura]` na imagem original, com origem no
canto superior esquerdo. `polygon_local` usa coordenadas do recorte.
`anchor_local` é somente o centro inferior da tela do recorte, **não** uma
posição dos pés ou um pivô físico aprovado. `collision: null` significa que
nenhuma colisão foi definida. O jogo ainda não carrega estes arquivos.

## Refazer a extração

A partir da raiz, usando Python com Pillow (o ambiente de preparação já usado
pelo projeto atende ao requisito):

```powershell
.\build\trainer_tools\Scripts\python.exe tools/extract_courtyard_assets.py
```

O comando regenera os derivados desta pasta, preservando este README.
Para inspecionar outra saída sem sobrescrever os recortes de trabalho:

```powershell
.\build\trainer_tools\Scripts\python.exe tools/extract_courtyard_assets.py --output build/courtyard_extraction_preview
```

O script verifica o tamanho da fonte, a preservação dos pixels RGB e a
presença de transparência nas máscaras. O manifesto registra o SHA-256 da
fonte para identificar a versão usada. Python/Pillow não são necessários
para jogar.
