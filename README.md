# Trabalho de Implementação - Grafos
## Alunos
Eloiza Sthefanny Rocha da Silva Cardoso (GRR20235165)

Roberto Sprengel Minozzo Tomchak (GRR20232369)

## Estrutura de Dados Usada
Para representar um grafo, foi escolhido a estrutura em forma de lista de adjência, onde lista_adj é um vetor de listas, de forma que lista_adj[i] = vizinhos do vértice i, na forma (j, w), onde {i, j} é uma aresta de G e w é o peso dessa aresta (1 se seu peso não foi definido.

Internamente, os vértices são nomeados de 0, 1, ..., n-1, onde n é o número de vértices. Para armazenar o nome real do vértice, foi utilizado um vetor de strings nome_vertices, de forma que nome_vertices[i] = nome do vértice i.

Além disso, alguns metadados úteis foram salvos, sendo eles n (número de vértices), m (número de arestas) e componente (componente[i] = número do componente do vértice i).

## Limitações
A principal limitação desse trabalho é o alto custo computacional para ler o grafo, que possui complexidade O(n^2), já que para cada vértice numa linha de aresta, é preciso checar se ele já existia, além de que leitura de arquivos costuma ser uma operação lenta.

Para fazer um teste de estresse, foi testado o arquivo de teste (teste.c) com o grafo de todas as paavras da língua portuguesa, mas limitado a apenas as palavras que começam com a letra "a". O tempo total foi de 1min8seg, com os principais gargalos sendo a função de leitura do grafo e do cálculo de diâmetros, que também possui alta complexidade.

Fora isso, a biblioteca realiza diversas alocações de memória, o que pode fazer um uso intensivo de memória que pode ser prejudicial em arquiteturas pequenas. Entretanto, esse problema não foi observado nos testes realizados.
