//------------------------------------------------------------------------------
// estrutura de dados para representar um grafo

#include "grafo.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// tamanho máximo de uma linha do arquivo de entrada
#define MAX_LINHA 2048

// nodos para lista encadeada e fila
struct nodo {
    struct nodo *ant;
    struct nodo *prox;
    unsigned int valor;  // índice do vértice
    unsigned int peso;   // peso da aresta que chega nele
};
typedef struct nodo nodo;

// estrutura auxiliar para heap
struct par {
    unsigned int peso;
    unsigned int vertice;
};
typedef struct par par;
// heap
struct heap {
    par *dados;
    int n;
};
typedef struct heap heap;

// metadados úteis para cálculo de cortes
// será usado num vetor, de forma que cortes[i] = metadados do vértice i
struct corte {
    unsigned int estado;
    unsigned int pai;  // se pai == vértice, é raiz
    unsigned int level;
    unsigned int lowerpoint;
};
typedef struct corte corte;

struct grafo {
    char *nome;
    char **nome_vertices;  // internamente, as funções tratam os vértices como índices de 0 até n-1
    unsigned int n;  // número de vértices
    unsigned int m;  // número de arestas
    nodo **lista_adj;  // lista de adjacência: vetor de listas (lista_adj[i] = índice dos vizinhos do vértice i)
    unsigned int *componente;  // comp[i] = número do componente do vértice i
};
typedef struct grafo grafo;

// PROTÓTIPOS
void adiciona_lista(nodo **lista, unsigned int valor, unsigned int peso);
unsigned int remove_lista(nodo **lista, unsigned int *peso);
void destroi_lista(nodo *lista);
unsigned int contem_string(char **vetor, unsigned int n, char *alvo);
char *preprocessa_linha(char *linha);
int tipo_linha(char *linha);
void pega_vertices(char *linha, char **v1, char **v2, unsigned int *peso);
int comp_str(const void *a, const void *b);
unsigned int max_dist(grafo *g, unsigned int r);
int comp_uint(const void *a, const void *b);
char *str_vetor(unsigned int *v, unsigned int n);
char *texto_vertices(unsigned int *v, unsigned int n, char **nome_vertices);
char *texto_aresta(char *v1, char *v2);
char *texto_arestas(char **arestas, unsigned int n);
void dfs(grafo *g, corte *metadados);
void dfs_rec(grafo *g, unsigned int r, corte *metadados);
void troca_par(par *a, par *b);
void insere_heap(heap *h, unsigned int v, unsigned int peso);
par deleta_heap(heap *h);

// FUNÇÕES DE LISTA (segue política FIFO) e VETORES DE STRINGS

// adiciona valor à lista (no final dela)
void adiciona_lista(nodo **lista, unsigned int valor, unsigned int peso) {
    // caso de borda: lista vazia
    if (!(*lista)) {
        *lista = malloc(sizeof(nodo));
        (*lista)->valor = valor;
        (*lista)->peso = peso;
        (*lista)->ant = (*lista);
        (*lista)->prox = (*lista);
        return;
    }
    // aloca nodo
    nodo *novo = malloc(sizeof(nodo));
    novo->valor = valor;
    novo->peso = peso;
    // insere no final da lista
    nodo *ultimo = (*lista)->ant;
    (*lista)->ant = novo;
    ultimo->prox = novo;
    novo->prox = (*lista);
    novo->ant = ultimo;
}

// remove valor na cabeça da lista e retorna ele
// assume que lista não está vazia
// se peso for NULL, só ignora
unsigned int remove_lista(nodo **lista, unsigned int *peso) {
    unsigned int temp;
    // caso de borda: lista de tamanho 1
    if ((*lista)->prox == (*lista)) {
        temp = (*lista)->valor;
        if (peso) *peso = (*lista)->peso;
        free(*lista);
        *lista = NULL;
        return temp;
    }
    // guardando valores para nao perder
    temp = (*lista)->valor;
    if (peso) *peso = (*lista)->peso;
    nodo *prox = (*lista)->prox;
    nodo *ant = (*lista)->ant;
    free((*lista));
    (*lista) = prox;
    prox->ant = ant;
    ant->prox = prox;
    return temp;
}

// libera toda a memória alocada pela lista
void destroi_lista(nodo *lista) {
    while (lista) {
        remove_lista(&lista, NULL);
    }
}

// verifica se string dada está no vetor
// retorna posição se estiver; n c.c
unsigned int contem_string(char **vetor, unsigned int n, char *alvo) {
    for (unsigned int i = 0; i < n; i++) {
        if (strcmp(vetor[i], alvo) == 0)
            return i;
    }
    return n;
}


// -------------------------------


// FUNÇÕES AUXILIARES DE STRING

// função de ordenação de string (para qsort)
int comp_str(const void *a, const void *b) {
    const char *s1 = *(const char * const *)a;
    const char *s2 = *(const char * const *)b;
    return strcmp(s1, s2);
}

// função de ordenação de unsigned int (para qsort)
int comp_uint(const void *a, const void *b) {
    unsigned int x = *(const unsigned int *)a;
    unsigned int y = *(const unsigned int *)b;
    if (x < y)
        return -1;
    else if (x == y)
        return 0;
    return 1;
}

// preprocessa uma linha, para ficar num formato mais simples
char *preprocessa_linha(char *linha) {
    // trocar '\n' por '\0'
    if (linha[strlen(linha) -1] == '\n')
        linha[strlen(linha) - 1] = '\0';

    // remover espaços no inicio
    char *novo_inicio;
    for (novo_inicio = linha; *novo_inicio == ' ' && *novo_inicio != '\0'; novo_inicio++);
    // caso de borda: linha ficou vazia
    if (*novo_inicio == '\0')
        return novo_inicio;
    // remover espaços no final
    // mesma coisa que encher de '\0'
    for (size_t i = strlen(novo_inicio)-1; i > 0; i--) {
        if (linha[i] == ' ')
            linha[i] = '\0';
        else
            break;
    }
    return novo_inicio;
}

// retorna tipo da linha:
//  0 -> comentário
//  1 -> nome (grafo ou vértice)
//  2 -> aresta
int tipo_linha(char *linha) {
    // comentário: começa com '//' ou é linha vazia
    if (strlen(linha) == 0 || strstr(linha, "//") == linha)
        return 0;
    // aresta: tem '--' em algum lugar
    if (strstr(linha, "--") != NULL)
        return 2;
    // se não for ennhum desses, é nome
    return 1;
}

// pega os vértices numa linha de aresta
// TODO: ADICIONAR PESOS
void pega_vertices(char *linha, char **v1, char **v2, unsigned int *peso) {
    // procura onde está o --
    char *local_sep = strstr(linha, "--");

    // ignora espaços entre primeiro vértice e "--"
    char *fim = local_sep - 1;
    while (*fim == ' ')
        fim--;
    // aloca memória para primeiro vértice e copia conteúdo
    size_t tam = strlen(linha) - strlen(fim) + 1;
    char *primeiro = malloc(tam+1);
    strncpy(primeiro, linha, tam);
    primeiro[tam] = '\0';
    *v1 = primeiro;

    // ignora espaços entre segundo vértice e "--"
    char *inicio = local_sep + 2;
    while (*inicio == ' ')
        inicio++;
    // procura o fim dessa string
    fim = inicio;
    while (*fim != ' ' && *fim != '\0')
        fim++;
    fim--;
    // aloca memória para segundo vértice e copia conteúdo
    tam = strlen(inicio) - strlen(fim) + 1;
    char *segundo = malloc(tam+1);
    strncpy(segundo, inicio, tam);
    segundo[tam] = '\0';
    *v2 = segundo;

    // pegar vértice, se existir
    // se não existir, assumir que é peso 1
    inicio = fim + 1;
    // ignorar espaços e tal
    while (*inicio == ' ' && *inicio != '\0')
        inicio++;
    if (*inicio == '\0') {
        *peso = 1;
        return;
    }
    sscanf(inicio, "%u", peso);
}

// cria uma string a partir de um vetor de unsigned int
// formato: v[0] v[1] v[2] ... v[n-1]
char *str_vetor(unsigned int *v, unsigned int n) {
    // tamanho da string
    size_t len = 0;
    // número com maior quantia de dígitos
    size_t max_digitos = 0;
    for (unsigned int i = 0; i < n; i++) {
        // divindo valor por 10 para ver quantos dígitos tem
        unsigned int x = v[i];
        size_t num_digitos = 0;
        do {
            x /= 10;
            num_digitos++;
        } while (x > 0);
        len += num_digitos;
        if (num_digitos > max_digitos)
            max_digitos = num_digitos;
        // + 1 para caracter de espaço (ou \0 se for o último)
        len++;
    }
    // espaço para armezanar string de cada número
    char *str_num = malloc(max_digitos+1);
    char *resposta = malloc(len);
    size_t p = 0;
    for (unsigned int i = 0; i < n; i++) {
        sprintf(str_num, "%u", v[i]);
        strcpy(&(resposta[p]), str_num);
        p += strlen(str_num);
        resposta[p] = ' ';
        p++;
    }
    resposta[len-1] = '\0';
    free(str_num);
    return resposta;
}

// cria o texto dos vértices
// usado em vertices_corte
char *texto_vertices(unsigned int *v, unsigned int n, char **nome_vertices) {
    // guardando nome dos vértices de corte e ordenando vértices
    char **vertices_ordenado = malloc(n * sizeof(char *));
    for (unsigned int i = 0; i < n; i++) {
        vertices_ordenado[i] = malloc(strlen(nome_vertices[v[i]]) + 1);
        strcpy(vertices_ordenado[i], nome_vertices[v[i]]);
    }
    qsort(vertices_ordenado, n, sizeof(char*), comp_str);
    // primeiro, precisamos ver quanto espaço temos que alocar
    size_t tam_string = 0;
    for (unsigned int i = 0; i < n; i++)
        tam_string += strlen(vertices_ordenado[i]) + 1;  // + 1 para espaço ou \0
    char *s = malloc(tam_string);
    // escrevendo cada uma das strings
    size_t pos = 0;
    for (unsigned int i = 0; i < n; i++) {
        strcpy(s+pos, vertices_ordenado[i]);
        pos += strlen(vertices_ordenado[i]);
        s[pos] = ' ';
        pos++;
    }
    s[pos-1] = '\0';
    for (unsigned int i = 0; i < n; i++)
        free(vertices_ordenado[i]);
    free(vertices_ordenado);
    return s;
}

char *texto_aresta(char *v1, char *v2) {
    // primeiro, alocar espaço
    size_t tam = strlen(v1) + strlen(v2) + 2;  // +2 para espaço no meio e \0
    char *s = malloc(tam);
    char *temp;
    // ver qual é maior
    if (strcmp(v1, v2) > 0) {
        temp = v1;
        v1 = v2;
        v2 = temp;
    }
    strcpy(s, v1);
    strcpy(s+strlen(v1), " ");
    strcpy(s+strlen(v1)+1, v2);
    s[tam-1] = '\0';
    return s;
}

char *texto_arestas(char **arestas, unsigned int n) {
    // primeiro, ordenar arestas
    qsort(arestas, n, sizeof(char *), comp_str);
    // alocar espaço para elas
    size_t tam_string = 0;
    for (unsigned int i = 0; i < n; i++)
        tam_string += strlen(arestas[i]) + 1;  // + 1 para espaço ou \0
    char *s = malloc(tam_string);
    // escrevendo cada uma das strings
    size_t pos = 0;
    for (unsigned int i = 0; i < n; i++) {
        strcpy(s+pos, arestas[i]);
        pos += strlen(arestas[i]);
        s[pos] = ' ';
        pos++;
    }
    s[pos-1] = '\0';
    return s;
}

// --------------------------------

// FUNÇÕES AUXILIARES DE GRAFO

// retorna a maior distância no grafo a partir do vértice r
unsigned int max_dist(grafo *g, unsigned int r) {
    // estratégia: dijktra's

    // distancias a partir de r
    unsigned int *distancia = malloc(sizeof(unsigned int) * g->n);
    for (unsigned int i = 0; i < g->n; i++)
        distancia[i] = 4294967295;  // "infinito"
    // vértices que já foram processados
    unsigned int *processado = malloc(sizeof(unsigned int) * g->n);
    for (unsigned int i = 0; i < g->n; i++)
        processado[i] = 0;

    // fila prioritária
    heap *h = malloc(sizeof(heap));
    h->dados = malloc(sizeof(par) * g->n);
    h->n = 0;

    // insere raiz e começa dijktra's
    insere_heap(h, r, 0);
    distancia[r] = 0;
    while (h->n) {
        par temp = deleta_heap(h);
        unsigned int v = temp.vertice;
        if (processado[v])
            continue;
        processado[v] = 1;
        // processa vizinhos
        nodo *vizinhos = g->lista_adj[v];
        // caso de borda: vértice isolado
        if (!vizinhos)
            break;
        nodo *atual = vizinhos;
        do {
            unsigned int u = atual->valor;
            unsigned int peso_u = atual->peso;
            // se melhora distancia, atualizar
            if (distancia[v]+peso_u < distancia[u]) {
                distancia[u] = distancia[v] + peso_u;
                insere_heap(h, u, distancia[u]);
            }
            atual = atual->prox;
        } while(atual != vizinhos);
    }
    // retorna maior distancia
    unsigned int max_dist = 0;
    for (unsigned int i = 0; i < g->n; i++)
    // se um vértice não foi processado é porque ele não ta no componente
        if (distancia[i] > max_dist && processado[i] && i != r)
            max_dist = distancia[i];
    free(distancia);
    free(processado);
    free(h->dados);
    free(h);
    return max_dist;
}

// realiza uma busca em profundidade, populando os metadados para cálculos de corte
void dfs(grafo *g, corte *metadados) {
    for (unsigned int i = 0; i < g->n; i++) {
        metadados[i].estado = 0;
        metadados[i].pai = i;
    }
    // passar por todos os vértices, caso tenha vários componentes
    for (unsigned int v = 0; v < g->n; v++) {
        // se não foi processado, é novo componente
        if (metadados[v].estado)
            continue;
        // é raiz
        metadados[v].level = 0;
        metadados[v].lowerpoint = 0;
        dfs_rec(g, v, metadados);
    }
}

// auxiliar de dfs, faz uma busca em profundidade a partir de r
void dfs_rec(grafo *g, unsigned int r, corte *metadados) {
    metadados[r].estado = 1;
    // iterando nos vizinhos
    nodo *atual = g->lista_adj[r];
    // caso de borda: vértice isolado
    if (!atual) {
        metadados[r].estado = 2;
        return;
    }
    do {
        unsigned int w = atual->valor;
        if (metadados[w].estado == 1 && w != metadados[r].pai && metadados[w].level < metadados[r].lowerpoint) {
            metadados[r].lowerpoint = metadados[w].level;
        }
        else if (metadados[w].estado == 0) {
            metadados[w].pai = r;
            // avançando uma unidade na profundidade
            metadados[w].level = metadados[r].level + 1;
            // por enquanto, l = L
            metadados[w].lowerpoint = metadados[w].level;
            // recursivamente busca nos vizinhos
            dfs_rec(g, w, metadados);
            if (metadados[w].lowerpoint < metadados[r].lowerpoint)
                metadados[r].lowerpoint = metadados[w].lowerpoint;
        }
        // próximo vizinho
        atual = atual->prox;
    } while(atual != g->lista_adj[r]);
    metadados[r].estado = 2;
}

// --------------------------------

// FUNÇÕES DE HEAP
void troca_par(par *a, par *b) {
    par c = *a;
    *a = *b;
    *b = c;
}
void insere_heap(heap *h, unsigned int v, unsigned int peso) {
    int i = h->n++;
    par p;
    p.vertice = v;
    p.peso = peso;
    h->dados[i] = p;

    while (i > 0) {
        int pai = (i - 1) / 2;
        if (h->dados[i].peso >= h->dados[pai].peso) break;
        troca_par(&h->dados[i], &h->dados[pai]);
        i = pai;
    }
}

par deleta_heap(heap *h) {
    if (h->n == 0) {
        h->n--;
        return h->dados[0];
    }
    par topo = h->dados[0];
    h->dados[0] = h->dados[--h->n];

    int i = 0;
    while (1) {
        int esq = 2 * i + 1;
        int dir = 2 * i + 2;
        int menor = i;

        if (esq < h->n && h->dados[esq].peso < h->dados[menor].peso)
            menor = esq;
        if (dir < h->n && h->dados[dir].peso < h->dados[menor].peso)
            menor = dir;

        if (menor == i) break;
        troca_par(&h->dados[i], &h->dados[menor]);
        i = menor;
    }
    return topo;
}

// -----------------------------------------------------------------------------


//------------------------------------------------------------------------------
// lê um grafo de f e o devolve
//
// um grafo é representado num arquivo por uma "string" que é o nome
// do grafo seguido de uma lista de vértices e arestas
//
// o nome do grafo, cada vértice e cada aresta aparecem numa linha por si só
// cada linha tem no máximo 2047 caracteres
// linhas iniciando por // são consideradas comentários e são ignoradas
//
// um vértice é representado por uma "string"
//
// uma aresta é representada por uma linha
// xxx -- yyy ppp
//
// onde xxx e yyy são nomes de vértices e ppp (opcional) é um int indicando o peso da aresta
//
// se um vértice faz parte de uma aresta, não é necessário nomeá-lo individualmente em uma linha
//
// a função supõe que a entrada está corretamente construída e não faz nenhuma checagem 
// caso a entrada não esteja corretamente construída, o comportamento da função é indefinido
//
// abaixo, a título de exemplo, a representação de um grafo com quatro
// vértices e dois componentes, um K_3 e um K_1
/*

// o nome do grafo
triângulo_com_vértice

// uma lista com três arestas e seus pesos
um -- dois 12
dois -- quatro 24
quatro -- um 41

// um vértice isolado
três

*/

grafo *le_grafo(FILE *f) {
    // buffer para ler linhas
    char buffer[MAX_LINHA];
    char *linha;
    grafo *g = malloc(sizeof(grafo));

    // ler nome do grafo
    while (fgets(buffer, MAX_LINHA, f)) {
        linha = preprocessa_linha(buffer);
        // se for nome, é nome do grafo
        if (tipo_linha(linha) == 1) {
            g->nome = malloc(strlen(linha)+1);
            strcpy(g->nome, linha);
            break;
        }
    }

    // ler resto do grafo
    // guardar nome dos vértices, para depois ler arestas
    g->nome_vertices = malloc(sizeof(char *));
    g->lista_adj = malloc(sizeof(nodo *));
    g->lista_adj[0] = NULL;
    g->m = 0;
    g->n = 0;
    // capacidade para fazer reallocs mais eficientemente
    unsigned int capacidade = 1;
    while (fgets(buffer, MAX_LINHA, f)) {
        linha = preprocessa_linha(buffer);
        int tipo = tipo_linha(linha);
        // ignorar comentários
        // se for nome, só adicionar vértice
        if (tipo == 1) {
            char *nome = malloc(strlen(linha)+1);
            strcpy(nome, linha);
            nome[strlen(linha)] = '\0';
            g->n++;
            // aumenta espaço se necessário
            if (g->n > capacidade) {
                capacidade *= 2;
                g->nome_vertices = realloc(g->nome_vertices, sizeof(char *) * capacidade);
                g->lista_adj = realloc(g->lista_adj, sizeof(nodo *) * capacidade);
            }
            g->nome_vertices[g->n - 1] = nome;
            g->lista_adj[g->n - 1] = NULL;
        }
        // se for aresta, adicionar ambos vértices
        else if (tipo == 2) {
            char *v1, *v2;
            unsigned int peso;
            pega_vertices(linha, &v1, &v2, &peso);
            g->m++;
            unsigned int i, j;
            i = contem_string(g->nome_vertices, g->n, v1);
            j = contem_string(g->nome_vertices, g->n, v2);
            unsigned int tam_atual = g->n;  // se i == tam_atual, não estava na lista (j idem)
            // se vértices não estiverem nos vértices já guardados, adiciona-los
            if (i == tam_atual) {
                g->n++;
                if (g->n > capacidade) {
                    capacidade *= 2;
                    g->nome_vertices = realloc(g->nome_vertices, sizeof(char *) * capacidade);
                    g->lista_adj = realloc(g->lista_adj, sizeof(nodo *) * capacidade);
                }
                g->nome_vertices[g->n - 1] = v1;
                g->lista_adj[g->n - 1] = NULL;
                // posição desse vértice na lista
                i = g->n - 1;
            }
            else
                free(v1);
            if (j == tam_atual) {
                g->n++;
                if (g->n > capacidade) {
                    capacidade *= 2;
                    g->nome_vertices = realloc(g->nome_vertices, sizeof(char *) * capacidade);
                    g->lista_adj = realloc(g->lista_adj, sizeof(nodo *) * capacidade);
                }
                g->nome_vertices[g->n - 1] = v2;
                g->lista_adj[g->n - 1] = NULL;
                // posição desse vértice na lista
                j = g->n - 1;
            }
            else
                free(v2);
            // adicionando aresta em ambos vértices
            adiciona_lista(&(g->lista_adj[i]), j, peso);
            adiciona_lista(&(g->lista_adj[j]), i, peso);
        }
    }

    // espaço para componentes (= 0 indica que não foi calculado)
    g->componente = malloc(sizeof(unsigned int) * g->n);
    for (unsigned int i = 0; i < g->n; i++)
        g->componente[i] = 0;
    return g;
}

//------------------------------------------------------------------------------
// desaloca toda a estrutura de dados alocada em g
//
// devolve 1 em caso de sucesso e 0 em caso de erro

unsigned int destroi_grafo(grafo *g) {
    // libera nome
    free(g->nome);
    // libera lista de adjacência
    for (unsigned int i = 0; i < g->n; i++)
        destroi_lista(g->lista_adj[i]);
    free(g->lista_adj);
    // libera nome dos vértices
    for (unsigned int i = 0; i < g->n; i++)
        free(g->nome_vertices[i]);
    free(g->nome_vertices);
    // libera números dos componentes
    free(g->componente);
    // finalmente, libera o grafo
    free(g);
    return 1;
}

//------------------------------------------------------------------------------
// devolve o nome de g

char *nome(grafo *g) {
    return g->nome;
}

//------------------------------------------------------------------------------
// devolve 1 se g é bipartido e 0 caso contrário

unsigned int bipartido(grafo *g) {
    // estratégia: colocar vértices até acabar ou dar conflito
    // vetor de cores (0 -> sem cor; 1 ou 2 -> cores)
    int *cores = malloc(sizeof(int) * g->n);
    for (unsigned int i = 0; i < g->n; i++)
        cores[i] = 0;
    // vetor que indica se vértice já foi processado
    int *processado = malloc(sizeof(int) * g->n);
    for (unsigned int i = 0; i < g->n; i++)
        processado[i] = 0;
    // fila para fazer busca em largura
    nodo *fila = NULL;
    // processar todos os nodos
    for (unsigned int i = 0; i < g->n; i++) {
        // nodo já foi processado -> seguir em frente
        if (processado[i])
            continue;
        // senão, pintar de uma cor qualquer
        cores[i] = 1;
        // colocar nodo na fila e processar em BFS
        adiciona_lista(&fila, i, 0);
        while (fila) {
            unsigned int v = remove_lista(&fila, NULL);
            // agora ta sendo processado
            processado[v] = 1;
            // vamos tentar colorir os vizinhos diferente
            nodo *vizinhos = g->lista_adj[v];
            // caso de borda: vértice isolado
            if (!vizinhos)
                continue;
            nodo *atual = vizinhos;
            do {
                unsigned int u = atual->valor;
                // se não tinha cor, pintar da cor oposta
                if (cores[u] == 0)
                    cores[u] = cores[v] == 1? 2: 1;
                // se já tinha cor e é a mesma de v, contradição!
                else if (cores[u] == cores[v]) {
                    free(cores);
                    free(processado);
                    destroi_lista(fila);
                    return 0;
                }
                // adicionar nodo na fila se não foi processado
                if (!processado[u])
                    adiciona_lista(&fila, u, 0);
                // avança para próximo vizinho
                atual = atual->prox;
            } while(atual != vizinhos);
        }
    }
    // se não deu problema em nenhum momento, é bipartido
    free(cores);
    free(processado);
    destroi_lista(fila);
    return 1;
}

//------------------------------------------------------------------------------
// devolve o número de vértices em g

unsigned int n_vertices(grafo *g) {
    return g->n;
}

//------------------------------------------------------------------------------
// devolve o número de arestas em g

unsigned int n_arestas(grafo *g) {
    return g->m;
}

//------------------------------------------------------------------------------
// devolve o número de componentes em g

unsigned int n_componentes(grafo *g) {
    // estratégia: númerar vértices conforme o componente que pertencem
    // zerando novamente caso ja tenha sido calculado
    // é interessante recalcular caso g tenha sido alterado  
    for (unsigned int i = 0; i < g->n; i++)
        g->componente[i] = 0;
    // fila para fazer busca em largura
    nodo *fila = NULL;
    // contador de componentes
    unsigned int c = 0;
    // processar todos os vértices
    for (unsigned int i = 0; i < g->n; i++) {
        // vértice já está num componente -> seguir em frente
        if (g->componente[i])
            continue;
        // senão, está num novo componente
        c++;
        g->componente[i] = c;
        // colocar vértice na fila e processar em BFS
        adiciona_lista(&fila, i, 0);
        while (fila) {
            unsigned int v = remove_lista(&fila, NULL);
            // vizinhos estão no mesmo componente
            nodo *vizinhos = g->lista_adj[v];
            // caso de borda: vértice isolado
            if (!vizinhos)
                break;
            nodo *atual = vizinhos;
            do {
                unsigned int u = atual->valor;
                // definir componente se necessário e colocar na fila
                if (!g->componente[u]) {
                    g->componente[u] = c;
                    adiciona_lista(&fila, u, 0);
                }
                // avança para próximo vizinho
                atual = atual->prox;
            } while(atual != vizinhos);
        }
    }
    return c;
}

//------------------------------------------------------------------------------
// devolve uma "string" com os diâmetros dos componentes de g separados por brancos
// em ordem não decrescente

char *diametros(grafo *g) {
    // estratégia: para cada componente, calcular a distância máxima começando em r
    // pegar a maior distância de todas essas

    // primeiro, definir componentes e armazenar distâncias
    unsigned int c = n_componentes(g);
    // diametro[i] = diametro do componente i
    unsigned int *diametro = malloc(sizeof(unsigned int) * c);
    // por enquanto, o melhor diametro é zero
    for (unsigned i = 0; i < c; i++)
        diametro[i] = 0;
    // vamos andar pelos vértices, usar como raiz e ver sua distancia máxima
    for (unsigned int r = 0; r < g->n; r++) {
        unsigned int comp = g->componente[r] - 1;
        unsigned int dist = max_dist(g, r);
        if (dist > diametro[comp])
            diametro[comp] = dist;
    }
    // ordenando diametros em ordem nao decrescente
    qsort(diametro, c, sizeof(unsigned int), comp_uint);
    // criando string disso
    char *str_diametro = str_vetor(diametro, c);
    free(diametro);
    return str_diametro;
}

//------------------------------------------------------------------------------
// devolve uma "string" com os nomes dos vértices de corte de g em
// ordem alfabética, separados por brancos

char *vertices_corte(grafo *g) {
    // coletando informações dos vértices
    corte *metadados = malloc(sizeof(corte) * g->n);
    dfs(g, metadados);
    // salva todos os vértices de corte
    unsigned int *cortes = NULL;
    unsigned conta_corte = 0;
    // analisando cada vértice
    for (unsigned int v = 0; v < g->n; v++) {
        // ignora vértices isolados
        if (!g->lista_adj[v])
            continue;
        unsigned int eh_corte = 0;
        // se for raiz, ver quantos filhos tem
        if (metadados[v].pai == v) {
            unsigned conta_filhos = 0;
            // loopar nos vizinhos...
            nodo *atual = g->lista_adj[v];
            do {
                unsigned int w = atual->valor;
                if (metadados[w].pai == v)
                    conta_filhos++;
                atual = atual->prox;
            } while(atual != g->lista_adj[v]);
            // se tiver dois ou mais filhos, é de corte
            if (conta_filhos >= 2)
                eh_corte = 1;
        }
        // se não for raiz, analisar lowerpoint
        else {
            // loopar nos vizinhos...
            nodo *atual = g->lista_adj[v];
            do {
                unsigned int w = atual->valor;
                // se l(w) >= L(v), é corte
                if (metadados[w].lowerpoint >= metadados[v].level)
                    eh_corte = 1;
                atual = atual->prox;
            } while(atual != g->lista_adj[v] && eh_corte == 0);
        }
        // se for de corte, adicionar
        if (eh_corte) {
            conta_corte++;
            cortes = realloc(cortes, conta_corte*sizeof(unsigned int));
            cortes[conta_corte-1] = v;
        }
    }
    char *s;
    // caso de borda: não tem vértices de corte
    if (conta_corte == 0) {
        s = malloc(1);
        s[0] = '\0';
    }
    else
        s = texto_vertices(cortes, conta_corte, g->nome_vertices);
    free(cortes);
    free(metadados);
    return s;
}

//------------------------------------------------------------------------------
// devolve uma "string" com as arestas de corte de g em ordem alfabética, separadas por brancos
// cada aresta é o par de nomes de seus vértices em ordem alfabética, separadas por brancos
//
// por exemplo, se as arestas de corte são {z, a}, {x, b} e {y, c}, a resposta será a string
// "a z b x c y"

char *arestas_corte(grafo *g) {
    // coletando informações dos vértices
    corte *metadados = malloc(sizeof(corte) * g->n);
    dfs(g, metadados);
    // salva as arestas, em formato de string ({a, b} -> "a b")
    char **cortes = NULL;
    unsigned int conta_corte = 0;
    // iterando nos vértices
    for (unsigned int v = 0; v < g->n; v++) {
        // caso de borda: vértice isolado obviamente não tem aresta de corte
        if (!g->lista_adj[v])
            continue;
        // vamos analisar seus filhos
        nodo *atual = g->lista_adj[v];
        do {
            unsigned int u = atual->valor;
            // se é filho e l(v) < L(u), é corte
            if (metadados[u].pai == v && metadados[u].lowerpoint > metadados[v].level) {
                char *temp = texto_aresta(g->nome_vertices[v], g->nome_vertices[u]);
                conta_corte++;
                cortes = realloc(cortes, conta_corte * sizeof(char *));
                cortes[conta_corte-1] = temp;
            }
            atual = atual->prox;
        } while(atual != g->lista_adj[v]);
    }
    // caso de borda: sem arestas de corte
    char *s;
    if (conta_corte == 0) {
        s = malloc(1);
        s[0] = '\0';
    }
    else
        s = texto_arestas(cortes, conta_corte);
    // libera textos temporários
    for (unsigned int i = 0; i < conta_corte; i++)
        free(cortes[i]);
    free(cortes);
    free(metadados);
    return s;
}
