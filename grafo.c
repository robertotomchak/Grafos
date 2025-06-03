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
    unsigned int valor;  // guardará índices de vértices
};
typedef struct nodo nodo;

// armazena uma aresta (auxiliar para aresta_corte)
struct aresta {
    int v1;
    int v2;
};
typedef struct aresta aresta;

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
void adiciona_lista(nodo **lista, unsigned int valor);
unsigned int remove_lista(nodo **lista);
void destroi_lista(nodo *lista);
unsigned int contem_string(char **vetor, unsigned int n, char *alvo);
int tipo_linha(char *linha);
void pega_vertices(char *linha, char **v1, char **v2);
int comp_str(const void *a, const void *b);
unsigned int max_dist(grafo *g, unsigned int r);
int comp_uint(const void *a, const void *b);
char *str_vetor(unsigned int *v, unsigned int n);

// FUNÇÕES DE LISTA (segue política FIFO) e VETORES DE STRINGS

// adiciona valor à lista (no final dela)
void adiciona_lista(nodo **lista, unsigned int valor) {
    // caso de borda: lista vazia
    if (!(*lista)) {
        *lista = malloc(sizeof(nodo));
        (*lista)->valor = valor;
        (*lista)->ant = (*lista);
        (*lista)->prox = (*lista);
        return;
    }
    // aloca nodo
    nodo *novo = malloc(sizeof(nodo));
    novo->valor = valor;
    // insere no final da lista
    nodo *ultimo = (*lista)->ant;
    (*lista)->ant = novo;
    ultimo->prox = novo;
    novo->prox = (*lista);
    novo->ant = ultimo;
}

// remove valor na cabeça da lista e retorna ele
// assume que lista não está vazia
unsigned int remove_lista(nodo **lista) {
    unsigned int temp;
    // caso de borda: lista de tamanho 1
    if ((*lista)->prox == (*lista)) {
        temp = (*lista)->valor;
        free(*lista);
        *lista = NULL;
        return temp;
    }
    // guardando valores para nao perder
    temp = (*lista)->valor;
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
        remove_lista(&lista);
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
// queremos ordenar de forma decrescente
int comp_uint(const void *a, const void *b) {
    unsigned int x = *(const unsigned int *)a;
    unsigned int y = *(const unsigned int *)b;
    if (x < y)
        return 1;
    else if (x == y)
        return 0;
    return -1;
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
void pega_vertices(char *linha, char **v1, char **v2) {
    // procura onde stá o --
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
    // ignora qualquer coisa depois do segundo vértice
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

// --------------------------------

// FUNÇÕES AUXILIARES DE GRAFO

// retorna a maior distância no grafo a partir do vértice r
unsigned int max_dist(grafo *g, unsigned int r) {
    // estratégia: usar busca em largura e guardar distâncias

    // distancias a partir de r
    unsigned int *distancia = malloc(sizeof(unsigned int) * g->n);
    for (unsigned int i = 0; i < g->n; i++)
        distancia[i] = 0;
    // vértices que já foram processados
    unsigned int *processado = malloc(sizeof(unsigned int) * g->n);
    for (unsigned int i = 0; i < g->n; i++)
        processado[i] = 0;

    // fila para fazer busca em largura
    nodo *fila = NULL;

    // insere raiz e começa BFS
    adiciona_lista(&fila, r);
    processado[r] = 1;
    while (fila) {
        unsigned int v = remove_lista(&fila);
        // processa vizinhos
        nodo *vizinhos = g->lista_adj[v];
        // caso de borda: vértice isolado
        if (!vizinhos)
            break;
        nodo *atual = vizinhos;
        do {
            unsigned int u = atual->valor;
            // incrementar distância
            if (!processado[u]) {
                // agora, ta processado
                processado[u] = 1;
                distancia[u] += distancia[v] + 1;
                adiciona_lista(&fila, u);
            }
            // avança para próximo vizinho
            atual = atual->prox;
        } while(atual != vizinhos);
    }
    // retorna maior distancia
    unsigned int max_dist = 0;
    for (unsigned int i = 0; i < g->n; i++)
        if (distancia[i] > max_dist)
            max_dist = distancia[i];
    return max_dist;
}

// --------------------------------


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
    char linha[MAX_LINHA];
    grafo *g = malloc(sizeof(grafo));

    // ler nome do grafo
    while (fgets(linha, MAX_LINHA, f)) {
        // trocar '\n' por '\0'
        if (linha[strlen(linha) -1] == '\n')
            linha[strlen(linha) - 1] = '\0';
        // se for nome, é nome do grafo
        if (tipo_linha(linha) == 1) {
            g->nome = malloc(strlen(linha)+1);
            strcpy(g->nome, linha);
            break;
        }
    }

    // guardar nome dos vértices, para depois ler arestas
    g->nome_vertices = NULL;
    g->n = 0;
    while (fgets(linha, MAX_LINHA, f)) {
        // trocar '\n' por '\0'
        if (linha[strlen(linha) -1] == '\n')
            linha[strlen(linha) - 1] = '\0';
        int tipo = tipo_linha(linha);
        // ignorar comentários
        // se for nome, só adicionar vértice
        if (tipo == 1) {
            char *nome = malloc(strlen(linha)+1);
            strcpy(nome, linha);
            nome[strlen(linha)] = '\0';
            g->n++;
            g->nome_vertices = realloc(g->nome_vertices, sizeof(char *) * g->n);
            g->nome_vertices[g->n - 1] = nome;
        }
        // se for aresta, adicionar ambos vértices
        else if (tipo == 2) {
            char *v1, *v2;
            pega_vertices(linha, &v1, &v2);
            // se vértices não estiverem nos vértices já guardados, adiciona-los
            if (contem_string(g->nome_vertices, g->n, v1) == g->n) {
                g->n++;
                g->nome_vertices = realloc(g->nome_vertices, sizeof(char *) * g->n);
                g->nome_vertices[g->n - 1] = v1;
            }
            else
                free(v1);
            if (contem_string(g->nome_vertices, g->n, v2) == g->n) {
                g->n++;
                g->nome_vertices = realloc(g->nome_vertices, sizeof(char *) * g->n);
                g->nome_vertices[g->n - 1] = v2;
            }
            else
                free(v2);
        }
    }
    // ordenar vértices por ordem alfabética (por conveniência)
    qsort(g->nome_vertices, g->n, sizeof(char *), comp_str);

    // agora, adicionar arestas
    g->lista_adj = malloc(sizeof(nodo *) * g->n);
    // zerando listas
    for (unsigned int i = 0; i < g->n; i++)
        g->lista_adj[i] = NULL;
    g->m = 0;
    // recomecar leitura do arquivo e procurar arestas
    rewind(f);
    g->m = 0;
    while (fgets(linha, MAX_LINHA, f)) {
        // trocar '\n' por '\0'
        if (linha[strlen(linha) -1] == '\n')
            linha[strlen(linha) - 1] = '\0';
        // so ler arestas
        if (tipo_linha(linha) != 2)
            continue;
        g->m++;
        char *v1, *v2;
        pega_vertices(linha, &v1, &v2);
        unsigned int i = contem_string(g->nome_vertices, g->n, v1);
        unsigned int j = contem_string(g->nome_vertices, g->n, v2);
        // adiciona arestas nos dois sentidos
        adiciona_lista(&(g->lista_adj[i]), j);
        adiciona_lista(&(g->lista_adj[j]), i);
        free(v1);
        free(v2);
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
        adiciona_lista(&fila, i);
        while (fila) {
            unsigned int v = remove_lista(&fila);
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
                    adiciona_lista(&fila, u);
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
        adiciona_lista(&fila, i);
        while (fila) {
            unsigned int v = remove_lista(&fila);
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
                    adiciona_lista(&fila, u);
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
        unsigned int comp = g->componente[r];
        unsigned int dist = max_dist(g, r);
        if (dist > diametro[comp])
            diametro[comp] = dist;
    }
    // ordenando diametros em ordem nao decrescente
    qsort(diametro, c, sizeof(unsigned int), comp_uint);
    printf("DIAMETROS: ");
    for (unsigned i = 0; i < c; i++)
        printf("%u ", diametro[i]);
    printf("\n");
    // criando string disso
    char *str_diametro = str_vetor(diametro, c);
    return str_diametro;
}

//------------------------------------------------------------------------------
// devolve uma "string" com os nomes dos vértices de corte de g em
// ordem alfabética, separados por brancos

char *vertices_corte(grafo *g);

//------------------------------------------------------------------------------
// devolve uma "string" com as arestas de corte de g em ordem alfabética, separadas por brancos
// cada aresta é o par de nomes de seus vértices em ordem alfabética, separadas por brancos
//
// por exemplo, se as arestas de corte são {z, a}, {x, b} e {y, c}, a resposta será a string
// "a z b x c y"

char *arestas_corte(grafo *g);
