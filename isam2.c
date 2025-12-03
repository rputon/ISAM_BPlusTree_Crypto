/*
 * ============================================================================
 * SISTEMA DE GERENCIAMENTO DE DADOS DE JOIAS - VERSÃO REESTRUTURADA
 * Trabalho 2.1 - Estrutura de Dados 2
 *
 * Objetivo: Melhorar a legibilidade e modularidade do código original.
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

/* ============================================================================
 * 1. DEFINES E ESTRUTURAS DE DADOS GLOBAIS
 * ============================================================================ */

/* --- Caminhos de Arquivos --- */
#define ARQUIVO_PRODUTOS "../data/jewelryRegister.dat"
#define ARQUIVO_PEDIDOS "../data/orderHistory.dat"
#define ARQUIVO_INDICE_PRODUTOS "../data/jewelryIndex.dat"
#define ARQUIVO_INDICE_PEDIDOS "../data/orderIndex.dat"
#define ARQUIVO_CSV "../data/jewelry.csv"

/* --- Configurações Gerais --- */
#define FLAG_REMOVIDO '*'
#define MEMORY_LIMIT 10000
#define CSV_LINE_SIZE 200
#define LIMITE_MEMORIA 10000
#define LIMITE_RECONSTRUCAO 100
#define TAMANHO_BLOCO 100
#define GRAU_BTREE 100
#define TAMANHO_TABELA_HASH 50000
#define CHAVE_TRANSPOSICAO "UNCOPYRIGHTABLE"
#define TAMANHO_CHAVE 15
#define MAX_TREE_HEIGHT 256

/* --- Estruturas de Dados --- */

/* ==================== ESTRUTURA: PEDIDO ==================== */

typedef struct {
    char data[30];                  // Data/hora da compra (ex: "2019-01-15 14:30:00 UTC")
    long long int id_pedido;        // ID único do pedido (CHAVE)
    long long int id_produto;       // ID do produto comprado
    int quantidade;                 // Quantidade comprada
    long long int id_categoria;     // ID da categoria do produto
    char alias_categoria[30];       // Nome da categoria
    int id_marca;                   // ID da marca
    float preco_usd;                // Preço em dólares
    long long int id_usuario;       // ID do usuário comprador
    char genero_produto;            // Gênero: M/F/U
    char cor[10];                   // Cor do produto
    char metal[10];                 // Metal (ouro, prata, etc)
    char gema[25];                  // Gema (diamante, rubi, etc)
} PEDIDO;

/* ==================== ESTRUTURA: JOIA ==================== */

typedef struct {
    long long int id_produto;       // ID único do produto (CHAVE)
    long long int id_categoria;     // ID da categoria
    int id_marca;                   // ID da marca
    float preco_usd;                // Preço em dólares
    char genero_produto;            // Gênero: M/F/U
    char cor[10];                   // Cor
    char metal[10];                 // Metal
    char gema[25];                  // Gema
} JOIA;

/* ==================== ESTRUTURA: CATEGORIA ==================== */

typedef struct {
    long long int id_categoria;     // ID único da categoria (CHAVE)
    char alias_categoria[30];       // Nome da categoria
    int contagem_produtos;          // Quantidade de produtos únicos
    int total_vendas;               // Quantidade total de itens vendidos
    float receita_total;            // Receita total gerada
    struct CATEGORIA *proximo;      // Próximo nó (usado em listas encadeadas)
} CATEGORIA;

/* ==================== ESTRUTURA: ÍNDICE ==================== */

typedef struct {
    long long int id;               // Chave (id_pedido, id_produto ou id_categoria)
    long posicao;                   // Posição do registro no arquivo .dat (em bytes)
} INDICE;

/* ==================== ESTRUTURA: REGISTRO DE OVERFLOW ==================== */

typedef struct {
    PEDIDO registro;                // Pedido completo
    long pos_bloco_original;        // Posição do bloco onde deveria estar
    long proximo_overflow;          // Próximo overflow encadeado (-1 = fim)
} REGISTRO_OVERFLOW;

/* ============================================================================
 * MÓDULO 1: CARREGAMENTO CSV - External Merge Sort
 * Opção 1: Carregar dados do CSV (criar .dat)
 * ============================================================================ */

/* Funções do módulo CSV declaradas mais adiante */
int carregarDadosDoCSV(const char *csvPath, int indexGap);

/* ============================================================================
 * MÓDULOS 2-5: OPERAÇÕES BÁSICAS DE ARQUIVO
 * Opção 2: Mostrar primeiros registros
 * Opção 3: Buscar produto específico
 * Opção 4: Inserir novo registro
 * Opção 5: Remover registro
 * ============================================================================ */

/* Estruturas auxiliares para operações */
typedef struct NoCategoria {
    CATEGORIA dados;
    struct NoCategoria *proximo;
} NO_CATEGORIA;

typedef struct {
    long long int id_produto;
    int quantidade_total;
} VENDAS_PRODUTO;

typedef struct {
    int ano;
    int mes;
    int total_pedidos;
    int quantidade_total;
    float receita_total;
} VENDAS_MENSAIS;

/* Variável global para controle de remoções */
int contador_remocoes = 0;

/* ============================================================================
 * MÓDULOS 6-10: ÍNDICES EM MEMÓRIA
 * Opção 6: Carregar índices em memória
 * Opção 7: Buscar produto (Árvore B+)
 * Opção 8: Buscar pedidos por produto (Hash)
 * Opção 9: Estatísticas dos índices
 * Opção 10: Análise de colisões (Hash)
 * ============================================================================ */

/* Estrutura da Árvore B+ */
typedef struct NoBTree {
    int num_chaves;                     // Quantidade de chaves armazenadas no nó
    long long int chaves[GRAU_BTREE];  // Array de chaves (id_produto)
    long posicoes[GRAU_BTREE];         // Array de posições no arquivo
    struct NoBTree *filhos[GRAU_BTREE + 1]; // Ponteiros para filhos
    int eh_folha;                       // 1 se é folha, 0 se é nó interno
    struct NoBTree *proximo;            // Próximo nó folha (apenas para folhas em B+)
} NO_BTREE;

typedef struct {
    NO_BTREE *raiz;                     // Raiz da árvore
    int altura;                         // Altura da árvore
    int total_nos;                      // Total de nós na árvore
    int total_chaves;                   // Total de chaves armazenadas
} ARVORE_BTREE;

typedef struct EntradaHash {
    long long int id_produto;           // Chave de busca (produto)
    long long int id_pedido;            // ID do pedido que contém este produto
    long posicao_arquivo;               // Posição do pedido no arquivo
    struct EntradaHash *proximo;        // Próxima entrada (colisão)
} ENTRADA_HASH;


typedef struct {
    ENTRADA_HASH **entradas;            // Array de ponteiros para entradas
    int tamanho;                        // Tamanho da tabela
    int total_elementos;                // Total de elementos inseridos
    int total_colisoes;                 // Total de colisões detectadas
} TABELA_HASH;

/* Variáveis globais dos índices em memória */
ARVORE_BTREE *indice_produtos_memoria = NULL;
TABELA_HASH *indice_pedidos_memoria = NULL;

/* Funções dos índices declaradas mais adiante */
ARVORE_BTREE *criarArvoreBTree();
void destruirArvoreBTree(ARVORE_BTREE *arvore);
int inserirBTree(ARVORE_BTREE *arvore, long long int id_produto, long posicao);
int buscarBTree(ARVORE_BTREE *arvore, long long int id_produto, long *posicao);
ARVORE_BTREE *carregarIndiceBTreeDeArquivo(const char *nomeArquivo, double *tempo_criacao);
void imprimirEstatisticasBTree(ARVORE_BTREE *arvore);

TABELA_HASH *criarTabelaHash();
void destruirTabelaHash(TABELA_HASH *tabela);
int inserirHash(TABELA_HASH *tabela, long long int id_produto, long long int id_pedido, long posicao);
ENTRADA_HASH **buscarHash(TABELA_HASH *tabela, long long int id_produto, int *quantidade);
int removerHash(TABELA_HASH *tabela, long long int id_produto);
TABELA_HASH *carregarIndiceHashDeArquivo(const char *nomeArquivo, double *tempo_criacao);
void imprimirEstatisticasHash(TABELA_HASH *tabela);
void analisarColisoes(TABELA_HASH *tabela);

/* ============================================================================
 * MÓDULO 11: BENCHMARKS COMPLETOS
 * Opção 11: Executar benchmarks completos
 * ============================================================================ */

/* Estruturas de resultado para benchmarks */
typedef struct {
    double tempo_criacao_btree;
    double tempo_criacao_hash;
    int total_produtos;
    int total_pedidos;
} RESULTADO_CRIACAO;

typedef struct {
    long long int chave_busca;
    double tempo_arquivo;
    double tempo_memoria;
    int encontrado;
} RESULTADO_BUSCA;

typedef struct {
    int tipo_operacao;  // 0=inserção, 1=remoção
    double tempo_operacao;
    double tempo_recriacao_indice;
} RESULTADO_MODIFICACAO;

/* Funções de benchmark declaradas mais adiante */
RESULTADO_CRIACAO benchmarkCriacaoIndices(const char *arquivo_produtos, const char *arquivo_pedidos,
                                          ARVORE_BTREE **arvore, TABELA_HASH **tabela);
void executarBateriaBuscas(ARVORE_BTREE *arvore, TABELA_HASH *tabela,
                           const char *arquivo_produtos, const char *arquivo_pedidos);
void gerarRelatorioCompleto(const char *arquivo_produtos, const char *arquivo_pedidos);
void imprimirTabelaComparativa(RESULTADO_BUSCA *resultados, int quantidade);

/* ============================================================================
 * MÓDULOS 12-18: COMPRESSÃO E CRIPTOGRAFIA
 * Opção 12: Comprimir arquivo (Huffman)
 * Opção 13: Descomprimir arquivo (Huffman)
 * Opção 14: Criptografar arquivo (Transposição)
 * Opção 15: Descriptografar arquivo (Transposição)
 * Opção 16: Proteger arquivo (Comprimir + Criptografar)
 * Opção 17: Restaurar arquivo protegido
 * Opção 18: Verificar integridade
 * ============================================================================ */

/* Estruturas Huffman */
typedef struct NoHuffman {
    unsigned char byte;             // Byte representado
    int frequencia;                 // Frequência de ocorrência
    struct NoHuffman *esquerda;     // Filho esquerdo
    struct NoHuffman *direita;      // Filho direito
} NO_HUFFMAN;

/*
 * CODIGO_HUFFMAN - Código binário para um byte
 */
typedef struct {
    unsigned char bits[MAX_TREE_HEIGHT];  // Sequência de bits
    int tamanho;                          // Quantidade de bits
} CODIGO_HUFFMAN;

/*
 * TABELA_HUFFMAN - Tabela de códigos para todos os bytes
 */
typedef struct {
    CODIGO_HUFFMAN codigos[256];    // Código para cada byte possível
    int total_bytes_originais;      // Tamanho original
    int total_bytes_comprimidos;    // Tamanho após compressão
} TABELA_HUFFMAN;

/* Funções de compressão e criptografia declaradas mais adiante */
int comprimirArquivoHuffman(const char *arquivo_entrada, const char *arquivo_saida, double *taxa_compressao);
int descomprimirArquivoHuffman(const char *arquivo_entrada, const char *arquivo_saida);
int criptografarArquivo(const char *arquivo_entrada, const char *arquivo_saida);
int descriptografarArquivo(const char *arquivo_entrada, const char *arquivo_saida);
int comprimirECriptografarArquivo(const char *arquivo_original, const char *arquivo_seguro, double *taxa_compressao);
int descriptografarEDescomprimirArquivo(const char *arquivo_seguro, const char *arquivo_original);
void imprimirEstatisticasCompressao(const char *arquivo_original, const char *arquivo_comprimido);
int verificarIntegridadeArquivo(const char *arquivo1, const char *arquivo2);

/* ============================================================================
 * FUNÇÕES AUXILIARES GERAIS
 * ============================================================================ */

FILE *abrirArquivo(const char *nomeArquivo, const char *modo);
int pedidoRemovido(PEDIDO *pedido);
int comparadorPedidos(const void *a, const void *b);
int comparadorJoias(const void *a, const void *b);


/* ==================== UTILITÁRIOS ==================== */
FILE *abrirArquivo(const char *nomeArquivo, const char *modo);
int pedidoRemovido(PEDIDO *pedido);

void obterDataHoraUTC(char *buffer);

void obterDataHoraUTC(char *buffer) {
    time_t tempo_bruto;
    struct tm *info;

    time(&tempo_bruto);
    info = gmtime(&tempo_bruto);

    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", info);
    strcat(buffer, " UTC");
}

int comparadorPedidos(const void *a, const void *b);
int comparadorJoias(const void *a, const void *b);
int comparadorCategorias(const void *a, const void *b);
int comparadorVendasCategoria(const void *a, const void *b);
int comparadorVendasProduto(const void *a, const void *b);

int comparadorCategorias(const void *a, const void *b) {
    CATEGORIA *catA = (CATEGORIA *)a;
    CATEGORIA *catB = (CATEGORIA *)b;
    
    if (catA->id_categoria < catB->id_categoria)
        return -1;
    if (catA->id_categoria > catB->id_categoria)
        return 1;
    return 0;
}

int comparadorVendasCategoria(const void *a, const void *b) {
    CATEGORIA *catA = (CATEGORIA *)a;
    CATEGORIA *catB = (CATEGORIA *)b;
    
    // Ordem DECRESCENTE por total_vendas
    if (catA->total_vendas > catB->total_vendas)
        return -1;
    if (catA->total_vendas < catB->total_vendas)
        return 1;
    return 0;
}

int comparadorVendasProduto(const void *a, const void *b) {
    VENDAS_PRODUTO *vendaA = (VENDAS_PRODUTO *)a;
    VENDAS_PRODUTO *vendaB = (VENDAS_PRODUTO *)b;
    
    // Ordem DECRESCENTE por quantidade_total
    if (vendaA->quantidade_total > vendaB->quantidade_total)
        return -1;
    if (vendaA->quantidade_total < vendaB->quantidade_total)
        return 1;
    return 0;
}


/* ============================================================================
 * IMPLEMENTAÇÕES - MÓDULO 11: BENCHMARKS
 * ============================================================================ */

/* ==================== IMPLEMENTAÇÃO: Criação de Índices ==================== */

RESULTADO_CRIACAO benchmarkCriacaoIndices(
    const char *arquivo_produtos,
    const char *arquivo_pedidos,
    ARVORE_BTREE **arvore,
    TABELA_HASH **tabela
) {
    RESULTADO_CRIACAO resultado;
    memset(&resultado, 0, sizeof(RESULTADO_CRIACAO));
    
    printf("\n" "========================================\n");
    printf("BENCHMARK: Criação de Índices em Memória\n");
    printf("========================================\n\n");
    
    // Cria índice B+ para produtos
    printf("Criando índice B+ para produtos...\n");
    *arvore = carregarIndiceBTreeDeArquivo(arquivo_produtos, &resultado.tempo_criacao_btree);
    if (*arvore != NULL) {
        resultado.total_produtos = (*arvore)->total_chaves;
        imprimirEstatisticasBTree(*arvore);
    }
    
    printf("\n");
    
    // Cria índice hash para pedidos
    printf("Criando índice hash para pedidos...\n");
    *tabela = carregarIndiceHashDeArquivo(arquivo_pedidos, &resultado.tempo_criacao_hash);
    if (*tabela != NULL) {
        resultado.total_pedidos = (*tabela)->total_elementos;
        imprimirEstatisticasHash(*tabela);
    }
    
    printf("\n" "========================================\n");
    printf("RESUMO DA CRIAÇÃO:\n");
    printf("  Árvore B+:    %.4f segundos (%d produtos)\n", 
           resultado.tempo_criacao_btree, resultado.total_produtos);
    printf("  Tabela Hash:  %.4f segundos (%d pedidos)\n", 
           resultado.tempo_criacao_hash, resultado.total_pedidos);
    printf("========================================\n\n");
    
    return resultado;
}

/* ==================== BENCHMARK: CONSULTAS - PRODUTOS ==================== */

double benchmarkBuscaProdutoArquivo(
    const char *arquivo_produtos,
    const char *arquivo_indice,
    long long int id_produto
) {
    clock_t inicio = clock();
    
    // Abre arquivo de índice parcial
    FILE *indice = abrirArquivo(arquivo_indice, "rb");
    if (indice == NULL) {
        return -1.0;
    }
    
    // Conta quantos índices existem
    fseek(indice, 0, SEEK_END);
    long tamanho_indice = ftell(indice);
    int num_indices = tamanho_indice / sizeof(INDICE);
    fseek(indice, 0, SEEK_SET);
    
    // Busca binária no índice
    INDICE idx;
    int encontrado = 0;
    long posicao_inicio = 0;
    
    int esq = 0, dir = num_indices - 1;
    while (esq <= dir) {
        int meio = (esq + dir) / 2;
        fseek(indice, meio * sizeof(INDICE), SEEK_SET);
        fread(&idx, sizeof(INDICE), 1, indice);
        
        if (idx.id == id_produto) {
            posicao_inicio = idx.posicao;
            encontrado = 1;
            break;
        } else if (idx.id < id_produto) {
            posicao_inicio = idx.posicao;
            esq = meio + 1;
        } else {
            dir = meio - 1;
        }
    }
    
    fclose(indice);
    
    if (encontrado) {
        clock_t fim = clock();
        return (double)(fim - inicio) / CLOCKS_PER_SEC;
    }
    
    // Se não encontrou diretamente, faz busca sequencial no bloco
    FILE *arquivo = abrirArquivo(arquivo_produtos, "rb");
    if (arquivo == NULL) {
        return -1.0;
    }
    
    fseek(arquivo, posicao_inicio, SEEK_SET);
    JOIA joia;
    
    // Busca até 1000 registros (tamanho do bloco do índice parcial)
    for (int i = 0; i < 1000; i++) {
        if (fread(&joia, sizeof(JOIA), 1, arquivo) != 1) break;
        
        if (joia.id_produto == id_produto) {
            encontrado = 1;
            break;
        }
        
        if (joia.id_produto > id_produto) {
            break;
        }
    }
    
    fclose(arquivo);
    
    clock_t fim = clock();
    return (double)(fim - inicio) / CLOCKS_PER_SEC;
}

double benchmarkBuscaProdutoMemoria(
    ARVORE_BTREE *arvore,
    long long int id_produto
) {
    clock_t inicio = clock();
    
    long posicao;
    int encontrado = buscarBTree(arvore, id_produto, &posicao);
    
    clock_t fim = clock();
    
    (void)encontrado; // Evita warning
    return (double)(fim - inicio) / CLOCKS_PER_SEC;
}

/* ==================== BENCHMARK: CONSULTAS - PEDIDOS ==================== */

double benchmarkBuscaPedidosPorProdutoArquivo(
    const char *arquivo_pedidos,
    const char *arquivo_indice,
    long long int id_produto
) {
    clock_t inicio = clock();
    
    // Como não temos índice exaustivo implementado, simula busca sequencial
    FILE *arquivo = abrirArquivo(arquivo_pedidos, "rb");
    if (arquivo == NULL) {
        return -1.0;
    }
    
    (void)arquivo_indice; // Não usado nesta versão
    
    PEDIDO pedido;
    int count = 0;
    
    // Busca sequencial em todo o arquivo
    while (fread(&pedido, sizeof(PEDIDO), 1, arquivo) == 1) {
        if (!pedidoRemovido(&pedido) && pedido.id_produto == id_produto) {
            count++;
        }
    }
    
    fclose(arquivo);
    
    clock_t fim = clock();
    return (double)(fim - inicio) / CLOCKS_PER_SEC;
}

double benchmarkBuscaPedidosPorProdutoMemoria(
    TABELA_HASH *tabela,
    long long int id_produto
) {
    clock_t inicio = clock();
    
    int quantidade;
    ENTRADA_HASH **resultados = buscarHash(tabela, id_produto, &quantidade);
    
    if (resultados != NULL) {
        free(resultados);
    }
    
    clock_t fim = clock();
    return (double)(fim - inicio) / CLOCKS_PER_SEC;
}

/* ==================== BATERIA DE TESTES ==================== */

void executarBateriaBuscas(
    ARVORE_BTREE *arvore,
    TABELA_HASH *tabela,
    const char *arquivo_produtos,
    const char *arquivo_pedidos
) {
    printf("\n" "========================================\n");
    printf("BENCHMARK: Comparação de Buscas\n");
    printf("========================================\n\n");
    
    // Define IDs para testar (você pode ajustar esses valores)
    long long int ids_produtos[] = {
        4804056980595, 
        4804056986871, 
        4804189181293,
        4804195768013,
        4804197017293
    };
    int num_testes = 5;
    
    RESULTADO_BUSCA resultados_produtos[5];
    RESULTADO_BUSCA resultados_pedidos[5];
    
    printf("Testando busca de PRODUTOS (Árvore B+)...\n");
    for (int i = 0; i < num_testes; i++) {
        resultados_produtos[i].chave_busca = ids_produtos[i];
        
        // Busca em arquivo
        char indice_path[256];
        strcpy(indice_path, "../data/jewelryIndex.dat");
        resultados_produtos[i].tempo_arquivo = 
            benchmarkBuscaProdutoArquivo(arquivo_produtos, indice_path, ids_produtos[i]);
        
        // Busca em memória
        resultados_produtos[i].tempo_memoria = 
            benchmarkBuscaProdutoMemoria(arvore, ids_produtos[i]);
        
        resultados_produtos[i].encontrado = 1;
        
        printf("  Produto %lld: Arquivo=%.6fs, Memória=%.6fs (%.1fx mais rápido)\n",
               ids_produtos[i],
               resultados_produtos[i].tempo_arquivo,
               resultados_produtos[i].tempo_memoria,
               resultados_produtos[i].tempo_arquivo / resultados_produtos[i].tempo_memoria);
    }
    
    printf("\nTestando busca de PEDIDOS por produto (Hash)...\n");
    for (int i = 0; i < num_testes; i++) {
        resultados_pedidos[i].chave_busca = ids_produtos[i];
        
        // Busca em arquivo (sequencial)
        resultados_pedidos[i].tempo_arquivo = 
            benchmarkBuscaPedidosPorProdutoArquivo(arquivo_pedidos, NULL, ids_produtos[i]);
        
        // Busca em memória
        resultados_pedidos[i].tempo_memoria = 
            benchmarkBuscaPedidosPorProdutoMemoria(tabela, ids_produtos[i]);
        
        resultados_pedidos[i].encontrado = 1;
        
        printf("  Produto %lld: Arquivo=%.6fs, Memória=%.6fs (%.1fx mais rápido)\n",
               ids_produtos[i],
               resultados_pedidos[i].tempo_arquivo,
               resultados_pedidos[i].tempo_memoria,
               resultados_pedidos[i].tempo_arquivo / resultados_pedidos[i].tempo_memoria);
    }
    
    printf("\n");
    imprimirTabelaComparativa(resultados_produtos, num_testes);
}

void imprimirTabelaComparativa(RESULTADO_BUSCA *resultados, int quantidade) {
    printf("\n=== TABELA COMPARATIVA DE DESEMPENHO ===\n\n");
    printf("| %-15s | %-12s | %-12s | %-10s |\n", 
           "ID Produto", "Arquivo (s)", "Memória (s)", "Speedup");
    printf("|-----------------|--------------|--------------|------------|\n");
    
    for (int i = 0; i < quantidade; i++) {
        double speedup = resultados[i].tempo_arquivo / resultados[i].tempo_memoria;
        printf("| %-15lld | %12.6f | %12.6f | %9.1fx |\n",
               resultados[i].chave_busca,
               resultados[i].tempo_arquivo,
               resultados[i].tempo_memoria,
               speedup);
    }
    
    printf("\n");
}

/* ==================== BENCHMARK: MODIFICAÇÕES ==================== */

void benchmarkInsercaoComAtualizacao(
    ARVORE_BTREE *arvore,
    TABELA_HASH *tabela,
    const char *arquivo_produtos,
    const char *arquivo_pedidos,
    int quantidade
) {
    printf("\n" "========================================\n");
    printf("BENCHMARK: Inserção com Atualização de Índices\n");
    printf("========================================\n\n");
    
    (void)arquivo_produtos;
    (void)arquivo_pedidos;
    
    printf("Inserindo %d novos registros...\n", quantidade);
    
    clock_t inicio = clock();
    
    // Simula inserção de produtos na árvore B+
    for (int i = 0; i < quantidade; i++) {
        long long int novo_id = 9900000000000LL + i;
        long nova_posicao = 1000000 + (i * sizeof(JOIA));
        inserirBTree(arvore, novo_id, nova_posicao);
    }
    
    clock_t fim = clock();
    double tempo_btree = (double)(fim - inicio) / CLOCKS_PER_SEC;
    
    printf("Tempo para inserir na árvore B+: %.6f segundos\n", tempo_btree);
    printf("Tempo médio por inserção: %.6f segundos\n", tempo_btree / quantidade);
    
    inicio = clock();
    
    // Simula inserção de pedidos na hash
    for (int i = 0; i < quantidade; i++) {
        long long int novo_id_produto = 4804056000000LL + i;
        long long int novo_id_pedido = 9900000000LL + i;
        long nova_posicao = 2000000 + (i * sizeof(PEDIDO));
        inserirHash(tabela, novo_id_produto, novo_id_pedido, nova_posicao);
    }
    
    fim = clock();
    double tempo_hash = (double)(fim - inicio) / CLOCKS_PER_SEC;
    
    printf("Tempo para inserir na tabela hash: %.6f segundos\n", tempo_hash);
    printf("Tempo médio por inserção: %.6f segundos\n", tempo_hash / quantidade);
    
    printf("\n" "========================================\n\n");
}

void benchmarkRemocaoComAtualizacao(
    ARVORE_BTREE *arvore,
    TABELA_HASH *tabela,
    const char *arquivo_produtos,
    const char *arquivo_pedidos,
    int quantidade
) {
    printf("\n" "========================================\n");
    printf("BENCHMARK: Remoção com Atualização de Índices\n");
    printf("========================================\n\n");
    
    (void)arquivo_produtos;
    (void)arquivo_pedidos;
    (void)arvore;
    
    printf("Removendo %d registros...\n", quantidade);
    
    clock_t inicio = clock();
    
    // Simula remoção de pedidos da hash
    for (int i = 0; i < quantidade; i++) {
        long long int id_produto = 4804056000000LL + i;
        removerHash(tabela, id_produto);
    }
    
    clock_t fim = clock();
    double tempo_hash = (double)(fim - inicio) / CLOCKS_PER_SEC;
    
    printf("Tempo para remover da tabela hash: %.6f segundos\n", tempo_hash);
    printf("Tempo médio por remoção: %.6f segundos\n", tempo_hash / quantidade);
    
    printf("\nNOTA: Remoção da árvore B+ não implementada completamente.\n");
    printf("      Em sistema real, requer redistribuição e merge de nós.\n");
    
    printf("\n" "========================================\n\n");
}

/* ==================== RELATÓRIO COMPLETO ==================== */

void gerarRelatorioCompleto(
    const char *arquivo_produtos,
    const char *arquivo_pedidos
) {
    ARVORE_BTREE *arvore = NULL;
    TABELA_HASH *tabela = NULL;
    
    printf("\n");
    printf(";=========================================================;\n");
    printf("|  RELATÓRIO COMPLETO DE BENCHMARKS - TRABALHO 2          |\n");
    printf("|  Índices em Memória para Arquivos de Dados              |\n");
    printf(";=========================================================;\n");
    
    // 1. Criação dos índices
    benchmarkCriacaoIndices(arquivo_produtos, arquivo_pedidos, &arvore, &tabela);
    
    if (arvore == NULL || tabela == NULL) {
        printf("\nERRO: Não foi possível criar os índices.\n");
        if (arvore) destruirArvoreBTree(arvore);
        if (tabela) destruirTabelaHash(tabela);
        return;
    }
    
    // 2. Bateria de buscas
    executarBateriaBuscas(arvore, tabela, arquivo_produtos, arquivo_pedidos);
    
    // 3. Análise de colisões
    analisarColisoes(tabela);
    
    // 4. Testes de modificação
    benchmarkInsercaoComAtualizacao(arvore, tabela, arquivo_produtos, arquivo_pedidos, 100);
    benchmarkRemocaoComAtualizacao(arvore, tabela, arquivo_produtos, arquivo_pedidos, 50);
    
    // Limpeza
    destruirArvoreBTree(arvore);
    destruirTabelaHash(tabela);
    
    printf("\n");
    printf(";=========================================================;\n");
    printf("|  FIM DO RELATÓRIO                                        |\n");
    printf(";=========================================================;\n");
    printf("\n");
}

/* ============================================================================
 * IMPLEMENTAÇÕES - MÓDULOS 12-18: COMPRESSÃO E CRIPTOGRAFIA
 * ============================================================================ */

/* ==================== COMPRESSÃO HUFFMAN ==================== */

int comprimirArquivoHuffman(const char *arquivo_entrada, 
                            const char *arquivo_saida,
                            double *taxa_compressao);

int descomprimirArquivoHuffman(const char *arquivo_entrada,
                               const char *arquivo_saida);

NO_HUFFMAN *construirArvoreHuffman(int *frequencias);

void gerarCodigosHuffman(NO_HUFFMAN *raiz, TABELA_HUFFMAN *tabela);

void liberarArvoreHuffman(NO_HUFFMAN *raiz);

/* ==================== CRIPTOGRAFIA POR TRANSPOSIÇÃO ==================== */

unsigned char *criptografarTransposicao(unsigned char *dados, 
                                        size_t tamanho,
                                        const char *chave);


unsigned char *descriptografarTransposicao(unsigned char *dados,
                                           size_t tamanho,
                                           const char *chave);

int criptografarArquivo(const char *arquivo_entrada,
                        const char *arquivo_saida);


int descriptografarArquivo(const char *arquivo_entrada,
                           const char *arquivo_saida);

/* ==================== FUNÇÕES COMBINADAS ==================== */

int comprimirECriptografarArquivo(const char *arquivo_original,
                                  const char *arquivo_seguro,
                                  double *taxa_compressao);

int descriptografarEDescomprimirArquivo(const char *arquivo_seguro,
                                        const char *arquivo_original);

/* ==================== ESTATÍSTICAS ==================== */

void imprimirEstatisticasCompressao(const char *arquivo_original,
                                    const char *arquivo_comprimido);

int verificarIntegridadeArquivo(const char *arquivo1, const char *arquivo2);

/*
 * ========================================================================
 * IMPLEMENTAÇÃO DE COMPRESSÃO E CRIPTOGRAFIA
 * ========================================================================
 */

/* ==================== FUNÇÕES AUXILIARES HUFFMAN ==================== */

static NO_HUFFMAN *criarNoHuffman(unsigned char byte, int freq) {
    NO_HUFFMAN *no = (NO_HUFFMAN *)malloc(sizeof(NO_HUFFMAN));
    if (no == NULL) return NULL;
    
    no->byte = byte;
    no->frequencia = freq;
    no->esquerda = NULL;
    no->direita = NULL;
    
    return no;
}

static int compararFrequencia(const void *a, const void *b) {
    NO_HUFFMAN *noA = *(NO_HUFFMAN **)a;
    NO_HUFFMAN *noB = *(NO_HUFFMAN **)b;
    return noA->frequencia - noB->frequencia;
}

static void gerarCodigosRecursivo(NO_HUFFMAN *no, CODIGO_HUFFMAN *codigo_atual,
                                  TABELA_HUFFMAN *tabela) {
    if (no == NULL) return;
    
    // Se é folha, armazena o código
    if (no->esquerda == NULL && no->direita == NULL) {
        tabela->codigos[no->byte] = *codigo_atual;
        return;
    }
    
    // Vai para a esquerda (bit 0)
    if (no->esquerda != NULL) {
        CODIGO_HUFFMAN novo_codigo = *codigo_atual;
        novo_codigo.bits[novo_codigo.tamanho++] = 0;
        gerarCodigosRecursivo(no->esquerda, &novo_codigo, tabela);
    }
    
    // Vai para a direita (bit 1)
    if (no->direita != NULL) {
        CODIGO_HUFFMAN novo_codigo = *codigo_atual;
        novo_codigo.bits[novo_codigo.tamanho++] = 1;
        gerarCodigosRecursivo(no->direita, &novo_codigo, tabela);
    }
}

/* ==================== COMPRESSÃO HUFFMAN ==================== */

NO_HUFFMAN *construirArvoreHuffman(int *frequencias) {
    NO_HUFFMAN **fila = (NO_HUFFMAN **)malloc(256 * sizeof(NO_HUFFMAN *));
    int tamanho_fila = 0;
    
    // Cria nós folha para bytes com frequência > 0
    for (int i = 0; i < 256; i++) {
        if (frequencias[i] > 0) {
            fila[tamanho_fila++] = criarNoHuffman(i, frequencias[i]);
        }
    }
    
    if (tamanho_fila == 0) {
        free(fila);
        return NULL;
    }
    
    // Constrói árvore de baixo para cima
    while (tamanho_fila > 1) {
        // Ordena por frequência
        qsort(fila, tamanho_fila, sizeof(NO_HUFFMAN *), compararFrequencia);
        
        // Pega os dois menores
        NO_HUFFMAN *esq = fila[0];
        NO_HUFFMAN *dir = fila[1];
        
        // Cria nó pai
        NO_HUFFMAN *pai = criarNoHuffman(0, esq->frequencia + dir->frequencia);
        pai->esquerda = esq;
        pai->direita = dir;
        
        // Remove os dois menores e adiciona o pai
        fila[0] = pai;
        for (int i = 1; i < tamanho_fila - 1; i++) {
            fila[i] = fila[i + 1];
        }
        tamanho_fila--;
    }
    
    NO_HUFFMAN *raiz = fila[0];
    free(fila);
    
    return raiz;
}

void gerarCodigosHuffman(NO_HUFFMAN *raiz, TABELA_HUFFMAN *tabela) {
    memset(tabela, 0, sizeof(TABELA_HUFFMAN));
    
    if (raiz == NULL) return;
    
    CODIGO_HUFFMAN codigo_inicial;
    codigo_inicial.tamanho = 0;
    
    gerarCodigosRecursivo(raiz, &codigo_inicial, tabela);
}

void liberarArvoreHuffman(NO_HUFFMAN *raiz) {
    if (raiz == NULL) return;
    liberarArvoreHuffman(raiz->esquerda);
    liberarArvoreHuffman(raiz->direita);
    free(raiz);
}

int comprimirArquivoHuffman(const char *arquivo_entrada, 
                            const char *arquivo_saida,
                            double *taxa_compressao) {
    printf("\nComprimindo arquivo com Huffman...\n");
    
    FILE *entrada = abrirArquivo(arquivo_entrada, "rb");
    if (entrada == NULL) return 0;
    
    // Calcula frequências
    int frequencias[256] = {0};
    unsigned char byte;
    long tamanho_original = 0;
    
    while (fread(&byte, 1, 1, entrada) == 1) {
        frequencias[byte]++;
        tamanho_original++;
    }
    
    if (tamanho_original == 0) {
        fclose(entrada);
        return 0;
    }
    
    // Constrói árvore e gera códigos
    NO_HUFFMAN *raiz = construirArvoreHuffman(frequencias);
    TABELA_HUFFMAN tabela;
    gerarCodigosHuffman(raiz, &tabela);
    
    // Abre arquivo de saída
    FILE *saida = abrirArquivo(arquivo_saida, "wb");
    if (saida == NULL) {
        fclose(entrada);
        liberarArvoreHuffman(raiz);
        return 0;
    }
    
    // Escreve cabeçalho: tamanho original + frequências
    fwrite(&tamanho_original, sizeof(long), 1, saida);
    fwrite(frequencias, sizeof(int), 256, saida);
    
    // Comprime dados
    rewind(entrada);
    unsigned char buffer_saida = 0;
    int bits_no_buffer = 0;
    long bytes_escritos = 0;
    
    while (fread(&byte, 1, 1, entrada) == 1) {
        CODIGO_HUFFMAN *codigo = &tabela.codigos[byte];
        
        for (int i = 0; i < codigo->tamanho; i++) {
            buffer_saida = (buffer_saida << 1) | codigo->bits[i];
            bits_no_buffer++;
            
            if (bits_no_buffer == 8) {
                fwrite(&buffer_saida, 1, 1, saida);
                bytes_escritos++;
                buffer_saida = 0;
                bits_no_buffer = 0;
            }
        }
    }
    
    // Escreve bits restantes
    if (bits_no_buffer > 0) {
        buffer_saida <<= (8 - bits_no_buffer);
        fwrite(&buffer_saida, 1, 1, saida);
        bytes_escritos++;
    }
    
    fclose(entrada);
    fclose(saida);
    liberarArvoreHuffman(raiz);
    
    long tamanho_comprimido = sizeof(long) + (256 * sizeof(int)) + bytes_escritos;
    *taxa_compressao = 1.0 - ((double)tamanho_comprimido / tamanho_original);
    
    printf("✓ Compressão concluída!\n");
    printf("  Original: %ld bytes\n", tamanho_original);
    printf("  Comprimido: %ld bytes\n", tamanho_comprimido);
    printf("  Taxa de compressão: %.2f%%\n", *taxa_compressao * 100);
    
    return 1;
}

int descomprimirArquivoHuffman(const char *arquivo_entrada,
                               const char *arquivo_saida) {
    printf("\nDescomprimindo arquivo Huffman...\n");
    
    FILE *entrada = abrirArquivo(arquivo_entrada, "rb");
    if (entrada == NULL) return 0;
    
    // Lê cabeçalho
    long tamanho_original;
    int frequencias[256];
    
    fread(&tamanho_original, sizeof(long), 1, entrada);
    fread(frequencias, sizeof(int), 256, entrada);
    
    // Reconstrói árvore
    NO_HUFFMAN *raiz = construirArvoreHuffman(frequencias);
    if (raiz == NULL) {
        fclose(entrada);
        return 0;
    }
    
    // Abre arquivo de saída
    FILE *saida = abrirArquivo(arquivo_saida, "wb");
    if (saida == NULL) {
        fclose(entrada);
        liberarArvoreHuffman(raiz);
        return 0;
    }
    
    // Descomprime
    NO_HUFFMAN *no_atual = raiz;
    unsigned char byte;
    long bytes_escritos = 0;
    
    while (fread(&byte, 1, 1, entrada) == 1 && bytes_escritos < tamanho_original) {
        for (int bit = 7; bit >= 0 && bytes_escritos < tamanho_original; bit--) {
            int bit_val = (byte >> bit) & 1;
            
            if (bit_val == 0) {
                no_atual = no_atual->esquerda;
            } else {
                no_atual = no_atual->direita;
            }
            
            // Se chegou em folha, escreve byte
            if (no_atual->esquerda == NULL && no_atual->direita == NULL) {
                fwrite(&no_atual->byte, 1, 1, saida);
                bytes_escritos++;
                no_atual = raiz;
            }
        }
    }
    
    fclose(entrada);
    fclose(saida);
    liberarArvoreHuffman(raiz);
    
    printf("✓ Descompressão concluída!\n");
    printf("  Bytes restaurados: %ld\n", bytes_escritos);
    
    return 1;
}

/* ==================== CRIPTOGRAFIA POR TRANSPOSIÇÃO ==================== */

static void calcularOrdemColunas(const char *chave, int *ordem, int tamanho) {
    // Cria array de índices
    char chave_ordenada[TAMANHO_CHAVE];
    strcpy(chave_ordenada, chave);
    
    // Ordena para encontrar a ordem
    for (int i = 0; i < tamanho; i++) {
        ordem[i] = i;
    }
    
    // Bubble sort com rastreamento de índices
    for (int i = 0; i < tamanho - 1; i++) {
        for (int j = 0; j < tamanho - i - 1; j++) {
            if (chave_ordenada[j] > chave_ordenada[j + 1]) {
                // Troca caracteres
                char temp_c = chave_ordenada[j];
                chave_ordenada[j] = chave_ordenada[j + 1];
                chave_ordenada[j + 1] = temp_c;
                
                // Troca ordem
                int temp_o = ordem[j];
                ordem[j] = ordem[j + 1];
                ordem[j + 1] = temp_o;
            }
        }
    }
}

unsigned char *criptografarTransposicao(unsigned char *dados, 
                                        size_t tamanho,
                                        const char *chave) {
    if (chave == NULL) chave = CHAVE_TRANSPOSICAO;
    
    int tam_chave = strlen(chave);
    int ordem[TAMANHO_CHAVE];
    calcularOrdemColunas(chave, ordem, tam_chave);
    
    // Aloca buffer de saída
    unsigned char *saida = (unsigned char *)malloc(tamanho);
    if (saida == NULL) return NULL;
    
    // Calcula linhas necessárias
    int linhas = (tamanho + tam_chave - 1) / tam_chave;
    int pos_saida = 0;
    
    // Lê por colunas na ordem da chave
    for (int col = 0; col < tam_chave; col++) {
        int coluna_real = ordem[col];
        
        for (int linha = 0; linha < linhas; linha++) {
            int pos_entrada = linha * tam_chave + coluna_real;
            
            if (pos_entrada < (int) tamanho) {
                saida[pos_saida++] = dados[pos_entrada];
            }
        }
    }
    
    return saida;
}

unsigned char *descriptografarTransposicao(unsigned char *dados,
                                           size_t tamanho,
                                           const char *chave) {
    if (chave == NULL) chave = CHAVE_TRANSPOSICAO;
    
    int tam_chave = strlen(chave);
    int ordem[TAMANHO_CHAVE];
    calcularOrdemColunas(chave, ordem, tam_chave);
    
    // Aloca buffer de saída
    unsigned char *saida = (unsigned char *)malloc(tamanho);
    if (saida == NULL) return NULL;
    
    // Calcula linhas
    int linhas = (tamanho + tam_chave - 1) / tam_chave;
    int pos_entrada = 0;
    
    // Escreve por colunas na ordem inversa
    for (int col = 0; col < tam_chave; col++) {
        int coluna_real = ordem[col];
        
        for (int linha = 0; linha < linhas; linha++) {
            int pos_saida = linha * tam_chave + coluna_real;
            
            if (pos_saida < (int) tamanho && pos_entrada < (int) tamanho) {
                saida[pos_saida] = dados[pos_entrada++];
            }
        }
    }
    
    return saida;
}

int criptografarArquivo(const char *arquivo_entrada,
                        const char *arquivo_saida) {
    printf("\nCriptografando arquivo com transposição...\n");
    printf("Chave: %s\n", CHAVE_TRANSPOSICAO);
    
    FILE *entrada = abrirArquivo(arquivo_entrada, "rb");
    if (entrada == NULL) return 0;
    
    // Lê arquivo completo
    fseek(entrada, 0, SEEK_END);
    long tamanho = ftell(entrada);
    rewind(entrada);
    
    unsigned char *dados = (unsigned char *)malloc(tamanho);
    if (dados == NULL) {
        fclose(entrada);
        return 0;
    }
    
    fread(dados, 1, tamanho, entrada);
    fclose(entrada);
    
    // Criptografa
    unsigned char *dados_cript = criptografarTransposicao(dados, tamanho, NULL);
    free(dados);
    
    if (dados_cript == NULL) return 0;
    
    // Salva
    FILE *saida = abrirArquivo(arquivo_saida, "wb");
    if (saida == NULL) {
        free(dados_cript);
        return 0;
    }
    
    fwrite(&tamanho, sizeof(long), 1, saida);
    fwrite(dados_cript, 1, tamanho, saida);
    fclose(saida);
    free(dados_cript);
    
    printf("✓ Criptografia concluída!\n");
    printf("  Bytes criptografados: %ld\n", tamanho);
    
    return 1;
}

int descriptografarArquivo(const char *arquivo_entrada,
                           const char *arquivo_saida) {
    printf("\nDescriptografando arquivo...\n");
    
    FILE *entrada = abrirArquivo(arquivo_entrada, "rb");
    if (entrada == NULL) return 0;
    
    // Lê tamanho
    long tamanho;
    fread(&tamanho, sizeof(long), 1, entrada);
    
    // Lê dados criptografados
    unsigned char *dados_cript = (unsigned char *)malloc(tamanho);
    if (dados_cript == NULL) {
        fclose(entrada);
        return 0;
    }
    
    fread(dados_cript, 1, tamanho, entrada);
    fclose(entrada);
    
    // Descriptografa
    unsigned char *dados = descriptografarTransposicao(dados_cript, tamanho, NULL);
    free(dados_cript);
    
    if (dados == NULL) return 0;
    
    // Salva
    FILE *saida = abrirArquivo(arquivo_saida, "wb");
    if (saida == NULL) {
        free(dados);
        return 0;
    }
    
    fwrite(dados, 1, tamanho, saida);
    fclose(saida);
    free(dados);
    
    printf("✓ Descriptografia concluída!\n");
    
    return 1;
}

/* ==================== FUNÇÕES COMBINADAS ==================== */

int comprimirECriptografarArquivo(const char *arquivo_original,
                                  const char *arquivo_seguro,
                                  double *taxa_compressao) {
    char temp_comprimido[512];
    sprintf(temp_comprimido, "%s.tmp.huff", arquivo_original);
    
    printf("\n=== APLICANDO SEGURANÇA AO ARQUIVO ===\n");
    
    // Passo 1: Comprime com Huffman
    if (!comprimirArquivoHuffman(arquivo_original, temp_comprimido, taxa_compressao)) {
        return 0;
    }
    
    // Passo 2: Criptografa com transposição
    if (!criptografarArquivo(temp_comprimido, arquivo_seguro)) {
        remove(temp_comprimido);
        return 0;
    }
    
    // Remove arquivo temporário
    remove(temp_comprimido);
    
    printf("\n✓ Arquivo protegido com sucesso!\n");
    printf("  Arquivo seguro: %s\n", arquivo_seguro);
    
    return 1;
}

int descriptografarEDescomprimirArquivo(const char *arquivo_seguro,
                                        const char *arquivo_original) {
    char temp_descomprimido[512];
    sprintf(temp_descomprimido, "%s.tmp.desc", arquivo_seguro);
    
    printf("\n=== RESTAURANDO ARQUIVO SEGURO ===\n");
    
    // Passo 1: Descriptografa
    if (!descriptografarArquivo(arquivo_seguro, temp_descomprimido)) {
        return 0;
    }
    
    // Passo 2: Descomprime
    if (!descomprimirArquivoHuffman(temp_descomprimido, arquivo_original)) {
        remove(temp_descomprimido);
        return 0;
    }
    
    // Remove arquivo temporário
    remove(temp_descomprimido);
    
    printf("\n✓ Arquivo restaurado com sucesso!\n");
    
    return 1;
}

/* ==================== ESTATÍSTICAS ==================== */

void imprimirEstatisticasCompressao(const char *arquivo_original,
                                    const char *arquivo_comprimido) {
    FILE *f1 = abrirArquivo(arquivo_original, "rb");
    FILE *f2 = abrirArquivo(arquivo_comprimido, "rb");
    
    if (f1 == NULL || f2 == NULL) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return;
    }
    
    fseek(f1, 0, SEEK_END);
    long tam1 = ftell(f1);
    fseek(f2, 0, SEEK_END);
    long tam2 = ftell(f2);
    
    fclose(f1);
    fclose(f2);
    
    double taxa = 1.0 - ((double)tam2 / tam1);
    
    printf("\n=== ESTATÍSTICAS DE COMPRESSÃO ===\n");
    printf("Arquivo original:    %ld bytes\n", tam1);
    printf("Arquivo comprimido:  %ld bytes\n", tam2);
    printf("Espaço economizado:  %ld bytes\n", tam1 - tam2);
    printf("Taxa de compressão:  %.2f%%\n", taxa * 100);
}

int verificarIntegridadeArquivo(const char *arquivo1, const char *arquivo2) {
    FILE *f1 = abrirArquivo(arquivo1, "rb");
    FILE *f2 = abrirArquivo(arquivo2, "rb");
    
    if (f1 == NULL || f2 == NULL) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return 0;
    }
    
    fseek(f1, 0, SEEK_END);
    long tam1 = ftell(f1);
    fseek(f2, 0, SEEK_END);
    long tam2 = ftell(f2);
    
    if (tam1 != tam2) {
        fclose(f1);
        fclose(f2);
        return 0;
    }
    
    rewind(f1);
    rewind(f2);
    
    unsigned char byte1, byte2;
    long pos = 0;
    int integro = 1;
    
    while (fread(&byte1, 1, 1, f1) == 1 && fread(&byte2, 1, 1, f2) == 1) {
        if (byte1 != byte2) {
            printf("Diferença encontrada na posição %ld\n", pos);
            integro = 0;
            break;
        }
        pos++;
    }
    
    fclose(f1);
    fclose(f2);
    
    if (integro) {
        printf("✓ Arquivos são idênticos - Integridade verificada!\n");
    } else {
        printf("✗ Arquivos são diferentes - Possível corrupção!\n");
    }
    
    return integro;
}

/* ============================================================================
 * IMPLEMENTAÇÕES - MÓDULO 1: CARREGAMENTO CSV
 * ============================================================================ */

/*
 * ========================================================================
 * CARREGAMENTO E ORDENAÇÃO DE DADOS DO CSV
 * ========================================================================
*/
int carregarDadosDoCSV(const char *csvPath, int indexGap);

/* ==================== PARSING DO CSV ==================== */

static int parseCSVLine(char *line, PEDIDO *pedido) {
    line[strcspn(line, "\r\n")] = 0;
    
    char *token;
    int field = 0;
    
    token = strtok(line, ",");
    
    while (token != NULL && field < 17) {
        switch (field) {
            case 0: strncpy(pedido->data, token, sizeof(pedido->data) - 1); break;
            case 1: pedido->id_pedido = atoll(token); break;
            case 2: pedido->id_produto = atoll(token); break;
            case 3: pedido->quantidade = atoi(token); break;
            case 4: pedido->id_categoria = atoll(token); break;
            case 5: strncpy(pedido->alias_categoria, token, sizeof(pedido->alias_categoria) - 1); break;
            case 6: pedido->id_marca = atoi(token); break;
            case 7: pedido->preco_usd = atof(token); break;
            case 8: pedido->id_usuario = atoll(token); break;
            case 9: pedido->genero_produto = token[0]; break;
            case 10: strncpy(pedido->cor, token, sizeof(pedido->cor) - 1); break;
            case 11: strncpy(pedido->metal, token, sizeof(pedido->metal) - 1); break;
            case 12: strncpy(pedido->gema, token, sizeof(pedido->gema) - 1); break;
        }
        field++;
        token = strtok(NULL, ",");
    }
    
    return field > 10;
}

/* ==================== CRIAR RUNS ORDENADOS ==================== */

static int createSortedRuns(FILE *csv, int *numOrderRuns, int *numJewelryRuns) {
    printf("\n=== FASE 1: CRIANDO RUNS ORDENADOS ===\n");
    printf("Limite de memoria: %d registros por run\n\n", MEMORY_LIMIT);
    
    char line[CSV_LINE_SIZE];
    PEDIDO *orderBuffer = malloc(MEMORY_LIMIT * sizeof(PEDIDO));
    JOIA *jewelryBuffer = malloc(MEMORY_LIMIT * sizeof(JOIA));
    
    int orderCount = 0;
    int jewelryCount = 0;
    int orderRunNum = 0;
    int jewelryRunNum = 0;
    long totalLines = 0;
    
    fgets(line, sizeof(line), csv); // Pula cabeçalho
    
    while (fgets(line, sizeof(line), csv) != NULL) {
        PEDIDO pedido;
        if (!parseCSVLine(line, &pedido)) continue;
        
        totalLines++;
        orderBuffer[orderCount++] = pedido;
        
        // Verifica se produto já existe
        int joiaExiste = 0;
        for (int i = 0; i < jewelryCount; i++) {
            if (jewelryBuffer[i].id_produto == pedido.id_produto) {
                joiaExiste = 1;
                break;
            }
        }
        
        if (!joiaExiste) {
            JOIA joia;
            joia.id_produto = pedido.id_produto;
            joia.id_categoria = pedido.id_categoria;
            joia.id_marca = pedido.id_marca;
            joia.preco_usd = pedido.preco_usd;
            joia.genero_produto = pedido.genero_produto;
            strncpy(joia.cor, pedido.cor, sizeof(joia.cor) - 1);
            strncpy(joia.metal, pedido.metal, sizeof(joia.metal) - 1);
            strncpy(joia.gema, pedido.gema, sizeof(joia.gema) - 1);
            jewelryBuffer[jewelryCount++] = joia;
        }
        
        // Se buffer de orders cheio
        if (orderCount >= MEMORY_LIMIT) {
            printf("  Criando run de orders #%d (%d registros)\n", orderRunNum, orderCount);
            qsort(orderBuffer, orderCount, sizeof(PEDIDO), comparadorPedidos);
            
            char filename[100];
            sprintf(filename, "../data/temp_order_run_%d.dat", orderRunNum);
            FILE *runFile = fopen(filename, "wb");
            fwrite(orderBuffer, sizeof(PEDIDO), orderCount, runFile);
            fclose(runFile);
            orderRunNum++;
            orderCount = 0;
        }
        
        // Se buffer de jewelry cheio
        if (jewelryCount >= MEMORY_LIMIT) {
            printf("  Criando run de jewelry #%d (%d registros)\n", jewelryRunNum, jewelryCount);
            qsort(jewelryBuffer, jewelryCount, sizeof(JOIA), comparadorJoias);
            
            char filename[100];
            sprintf(filename, "../data/temp_jewelry_run_%d.dat", jewelryRunNum);
            FILE *runFile = fopen(filename, "wb");
            fwrite(jewelryBuffer, sizeof(JOIA), jewelryCount, runFile);
            fclose(runFile);
            jewelryRunNum++;
            jewelryCount = 0;
        }
        
        if (totalLines % 50000 == 0) {
            printf("  Processadas %ld linhas...\n", totalLines);
        }
    }
    
    // Grava runs restantes
    if (orderCount > 0) {
        printf("  Criando run final de orders #%d (%d registros)\n", orderRunNum, orderCount);
        qsort(orderBuffer, orderCount, sizeof(PEDIDO), comparadorPedidos);
        char filename[100];
        sprintf(filename, "../data/temp_order_run_%d.dat", orderRunNum);
        FILE *runFile = fopen(filename, "wb");
        fwrite(orderBuffer, sizeof(PEDIDO), orderCount, runFile);
        fclose(runFile);
        orderRunNum++;
    }
    
    if (jewelryCount > 0) {
        printf("  Criando run final de jewelry #%d (%d registros)\n", jewelryRunNum, jewelryCount);
        qsort(jewelryBuffer, jewelryCount, sizeof(JOIA), comparadorJoias);
        char filename[100];
        sprintf(filename, "../data/temp_jewelry_run_%d.dat", jewelryRunNum);
        FILE *runFile = fopen(filename, "wb");
        fwrite(jewelryBuffer, sizeof(JOIA), jewelryCount, runFile);
        fclose(runFile);
        jewelryRunNum++;
    }
    
    *numOrderRuns = orderRunNum;
    *numJewelryRuns = jewelryRunNum;
    
    free(orderBuffer);
    free(jewelryBuffer);
    
    printf("\nRuns criados: %d orders, %d jewelry\n", orderRunNum, jewelryRunNum);
    printf("Total de linhas: %ld\n\n", totalLines);
    
    return 1;
}

/* ==================== MERGE DOS RUNS ==================== */

static int mergeOrderRuns(int numRuns, FILE *orderHistory, FILE *orderIndex, int indexGap) {
    printf("=== FASE 2: MERGE DOS RUNS DE ORDERS ===\n");
    printf("Mergeando %d runs...\n\n", numRuns);
    
    FILE **runFiles = malloc(numRuns * sizeof(FILE *));
    PEDIDO *currentOrders = malloc(numRuns * sizeof(PEDIDO));
    int *runFinished = calloc(numRuns, sizeof(int));
    
    for (int i = 0; i < numRuns; i++) {
        char filename[100];
        sprintf(filename, "../data/temp_order_run_%d.dat", i);
        runFiles[i] = fopen(filename, "rb");
        
        if (fread(&currentOrders[i], sizeof(PEDIDO), 1, runFiles[i]) == 1) {
            runFinished[i] = 0;
        } else {
            runFinished[i] = 1;
        }
    }
    
    long totalWritten = 0;
    int indexCount = 0;
    
    while (1) {
        long long int minOrderId = LLONG_MAX;
        int minRunIdx = -1;
        
        for (int i = 0; i < numRuns; i++) {
            if (!runFinished[i] && currentOrders[i].id_pedido < minOrderId) {
                minOrderId = currentOrders[i].id_pedido;
                minRunIdx = i;
            }
        }
        
        if (minRunIdx == -1) break;
        
        fwrite(&currentOrders[minRunIdx], sizeof(PEDIDO), 1, orderHistory);
        
        if (totalWritten % indexGap == 0) {
            INDICE idx;
            idx.id = currentOrders[minRunIdx].id_pedido;
            idx.posicao = totalWritten * sizeof(PEDIDO);
            fwrite(&idx, sizeof(INDICE), 1, orderIndex);
            indexCount++;
        }
        
        totalWritten++;
        
        if (fread(&currentOrders[minRunIdx], sizeof(PEDIDO), 1, runFiles[minRunIdx]) != 1) {
            runFinished[minRunIdx] = 1;
        }
        
        if (totalWritten % 50000 == 0) {
            printf("  Escritos %ld registros...\n", totalWritten);
        }
    }
    
    for (int i = 0; i < numRuns; i++) {
        fclose(runFiles[i]);
    }
    
    free(runFiles);
    free(currentOrders);
    free(runFinished);
    
    printf("\nOrders: %ld registros, %d indices\n\n", totalWritten, indexCount);
    return totalWritten;
}

static int mergeJewelryRuns(int numRuns, FILE *jewelryRegister, FILE *jewelryIndex, int indexGap) {
    printf("=== FASE 3: MERGE DOS RUNS DE JEWELRY ===\n");
    printf("Mergeando %d runs (removendo duplicatas)...\n\n", numRuns);
    
    FILE **runFiles = malloc(numRuns * sizeof(FILE *));
    JOIA *currentJewelry = malloc(numRuns * sizeof(JOIA));
    int *runFinished = calloc(numRuns, sizeof(int));
    
    for (int i = 0; i < numRuns; i++) {
        char filename[100];
        sprintf(filename, "../data/temp_jewelry_run_%d.dat", i);
        runFiles[i] = fopen(filename, "rb");
        
        if (fread(&currentJewelry[i], sizeof(JOIA), 1, runFiles[i]) == 1) {
            runFinished[i] = 0;
        } else {
            runFinished[i] = 1;
        }
    }
    
    long totalWritten = 0;
    int indexCount = 0;
    long long int lastProductId = -1;
    
    while (1) {
        long long int minProductId = LLONG_MAX;
        int minRunIdx = -1;
        
        for (int i = 0; i < numRuns; i++) {
            if (!runFinished[i] && currentJewelry[i].id_produto < minProductId) {
                minProductId = currentJewelry[i].id_produto;
                minRunIdx = i;
            }
        }
        
        if (minRunIdx == -1) break;
        
        if (currentJewelry[minRunIdx].id_produto != lastProductId) {
            fwrite(&currentJewelry[minRunIdx], sizeof(JOIA), 1, jewelryRegister);
            
            if (totalWritten % indexGap == 0) {
                INDICE idx;
                idx.id = currentJewelry[minRunIdx].id_produto;
                idx.posicao = totalWritten * sizeof(JOIA);
                fwrite(&idx, sizeof(INDICE), 1, jewelryIndex);
                indexCount++;
            }
            
            lastProductId = currentJewelry[minRunIdx].id_produto;
            totalWritten++;
        }
        
        if (fread(&currentJewelry[minRunIdx], sizeof(JOIA), 1, runFiles[minRunIdx]) != 1) {
            runFinished[minRunIdx] = 1;
        }
    }
    
    for (int i = 0; i < numRuns; i++) {
        fclose(runFiles[i]);
    }
    
    free(runFiles);
    free(currentJewelry);
    free(runFinished);
    
    printf("\nJewelry: %ld registros unicos, %d indices\n\n", totalWritten, indexCount);
    return totalWritten;
}

/* ==================== LIMPAR TEMPORÁRIOS ==================== */

static void cleanupTempFiles(int numOrderRuns, int numJewelryRuns) {
    printf("=== LIMPANDO ARQUIVOS TEMPORARIOS ===\n");
    
    for (int i = 0; i < numOrderRuns; i++) {
        char filename[100];
        sprintf(filename, "../data/temp_order_run_%d.dat", i);
        remove(filename);
    }
    
    for (int i = 0; i < numJewelryRuns; i++) {
        char filename[100];
        sprintf(filename, "../data/temp_jewelry_run_%d.dat", i);
        remove(filename);
    }
    
    printf("Arquivos temporarios removidos\n\n");
}

/* ==================== FUNÇÃO PRINCIPAL ==================== */

int carregarDadosDoCSV(const char *csvPath, int indexGap) {
    printf("\n");
    printf("================================================================\n");
    printf("  CARREGAMENTO E ORDENACAO DE DADOS DO CSV\n");
    printf("  (External Merge Sort)\n");
    printf("================================================================\n\n");
    
    FILE *csv = fopen(csvPath, "r");
    if (!csv) {
        printf("ERRO: Nao foi possivel abrir %s\n", csvPath);
        return 0;
    }
    
    FILE *orderHistory = fopen("../data/orderHistory.dat", "wb+");
    FILE *orderIndex = fopen("../data/orderIndex.dat", "wb+");
    FILE *jewelryRegister = fopen("../data/jewelryRegister.dat", "wb+");
    FILE *jewelryIndex = fopen("../data/jewelryIndex.dat", "wb+");
    
    if (!orderHistory || !orderIndex || !jewelryRegister || !jewelryIndex) {
        printf("ERRO: Nao foi possivel criar arquivos de saida\n");
        if (csv) fclose(csv);
        return 0;
    }
    
    int numOrderRuns, numJewelryRuns;
    if (!createSortedRuns(csv, &numOrderRuns, &numJewelryRuns)) {
        fclose(csv);
        return 0;
    }
    fclose(csv);
    
    mergeOrderRuns(numOrderRuns, orderHistory, orderIndex, indexGap);
    mergeJewelryRuns(numJewelryRuns, jewelryRegister, jewelryIndex, indexGap);
    
    cleanupTempFiles(numOrderRuns, numJewelryRuns);
    
    fclose(orderHistory);
    fclose(orderIndex);
    fclose(jewelryRegister);
    fclose(jewelryIndex);
    
    printf("================================================================\n");
    printf("  DADOS CARREGADOS E ORDENADOS COM SUCESSO!\n");
    printf("================================================================\n\n");
    
    return 1;
}

/* ============================================================================
 * IMPLEMENTAÇÕES - MÓDULOS 6-10: ÍNDICES EM MEMÓRIA
 * ============================================================================ */

/* ==================== CRIAÇÃO E DESTRUIÇÃO ==================== */

ARVORE_BTREE *criarArvoreBTree();


void destruirArvoreBTree(ARVORE_BTREE *arvore);

/* ==================== OPERAÇÕES BÁSICAS ==================== */

int inserirBTree(ARVORE_BTREE *arvore, long long int id_produto, long posicao);

int buscarBTree(ARVORE_BTREE *arvore, long long int id_produto, long *posicao);

int removerBTree(ARVORE_BTREE *arvore, long long int id_produto);

/* ==================== CARREGAMENTO DO ARQUIVO ==================== */

ARVORE_BTREE *carregarIndiceBTreeDeArquivo(const char *nomeArquivo, double *tempo_criacao);

/* ==================== ESTATÍSTICAS ==================== */

void imprimirEstatisticasBTree(ARVORE_BTREE *arvore);


size_t calcularMemoriaUsadaBTree(ARVORE_BTREE *arvore);

/*
 * ========================================================================
 * IMPLEMENTAÇÃO DA ÁRVORE B+ PARA INDEXAÇÃO EM MEMÓRIA
 * ========================================================================
 */

/* ==================== FUNÇÕES AUXILIARES DE NÓ ==================== */

static NO_BTREE *criarNoFolha() {
    NO_BTREE *no = (NO_BTREE *)malloc(sizeof(NO_BTREE));
    if (no == NULL) return NULL;
    
    no->num_chaves = 0;
    no->eh_folha = 1;
    no->proximo = NULL;
    
    for (int i = 0; i <= GRAU_BTREE; i++) {
        no->filhos[i] = NULL;
    }
    
    return no;
}

static NO_BTREE *criarNoInterno() {
    NO_BTREE *no = (NO_BTREE *)malloc(sizeof(NO_BTREE));
    if (no == NULL) return NULL;
    
    no->num_chaves = 0;
    no->eh_folha = 0;
    no->proximo = NULL;
    
    for (int i = 0; i <= GRAU_BTREE; i++) {
        no->filhos[i] = NULL;
    }
    
    return no;
}


static void destruirNo(NO_BTREE *no) {
    if (no == NULL) return;
    
    if (!no->eh_folha) {
        for (int i = 0; i <= no->num_chaves; i++) {
            destruirNo(no->filhos[i]);
        }
    }
    
    free(no);
}

/* ==================== BUSCA ==================== */


static int buscarNoFolha(NO_BTREE *no, long long int chave, long *posicao) {
    for (int i = 0; i < no->num_chaves; i++) {
        if (no->chaves[i] == chave) {
            *posicao = no->posicoes[i];
            return 1;
        }
    }
    return 0;
}

static int buscarRecursivo(NO_BTREE *no, long long int chave, long *posicao) {
    if (no == NULL) return 0;
    
    // Se é folha, busca diretamente
    if (no->eh_folha) {
        return buscarNoFolha(no, chave, posicao);
    }
    
    // Nó interno: encontra o filho correto
    int i = 0;
    while (i < no->num_chaves && chave >= no->chaves[i]) {
        i++;
    }
    
    return buscarRecursivo(no->filhos[i], chave, posicao);
}

/* ==================== INSERÇÃO ==================== */

static void inserirNoFolha(NO_BTREE *no, long long int chave, long posicao) {
    int i = no->num_chaves - 1;
    
    // Move elementos maiores para a direita
    while (i >= 0 && no->chaves[i] > chave) {
        no->chaves[i + 1] = no->chaves[i];
        no->posicoes[i + 1] = no->posicoes[i];
        i--;
    }
    
    // Insere nova chave
    no->chaves[i + 1] = chave;
    no->posicoes[i + 1] = posicao;
    no->num_chaves++;
}

static NO_BTREE *dividirFolha(NO_BTREE *no, long long int *chave_promovida) {
    NO_BTREE *novo = criarNoFolha();
    if (novo == NULL) return NULL;
    
    int meio = (GRAU_BTREE + 1) / 2;
    
    // Move metade das chaves para o novo nó
    for (int i = meio; i < no->num_chaves; i++) {
        novo->chaves[i - meio] = no->chaves[i];
        novo->posicoes[i - meio] = no->posicoes[i];
        novo->num_chaves++;
    }
    
    // Ajusta o nó original
    no->num_chaves = meio;
    
    // Encadeia folhas
    novo->proximo = no->proximo;
    no->proximo = novo;
    
    // Chave promovida é a primeira do novo nó
    *chave_promovida = novo->chaves[0];
    
    return novo;
}

static NO_BTREE *inserirRecursivo(NO_BTREE *no, long long int chave, long posicao, 
                                   long long int *chave_promovida, int *houve_split) {
    *houve_split = 0;
    
    // Nó folha
    if (no->eh_folha) {
        // Se tem espaço, insere diretamente
        if (no->num_chaves < GRAU_BTREE) {
            inserirNoFolha(no, chave, posicao);
            return NULL;
        }
        
        // Nó cheio: precisa dividir
        // Cria array temporário com todas as chaves
        long long int temp_chaves[GRAU_BTREE + 1];
        long temp_posicoes[GRAU_BTREE + 1];
        
        int i = 0, j = 0;
        int inserido = 0;
        
        for (i = 0; i < no->num_chaves; i++) {
            if (!inserido && chave < no->chaves[i]) {
                temp_chaves[j] = chave;
                temp_posicoes[j] = posicao;
                j++;
                inserido = 1;
            }
            temp_chaves[j] = no->chaves[i];
            temp_posicoes[j] = no->posicoes[i];
            j++;
        }
        
        if (!inserido) {
            temp_chaves[j] = chave;
            temp_posicoes[j] = posicao;
        }
        
        // Divide
        int meio = (GRAU_BTREE + 1) / 2;
        NO_BTREE *novo = criarNoFolha();
        
        // Distribui chaves
        no->num_chaves = 0;
        for (i = 0; i < meio; i++) {
            no->chaves[i] = temp_chaves[i];
            no->posicoes[i] = temp_posicoes[i];
            no->num_chaves++;
        }
        
        for (i = meio; i < GRAU_BTREE + 1; i++) {
            novo->chaves[i - meio] = temp_chaves[i];
            novo->posicoes[i - meio] = temp_posicoes[i];
            novo->num_chaves++;
        }
        
        novo->proximo = no->proximo;
        no->proximo = novo;
        
        *chave_promovida = novo->chaves[0];
        *houve_split = 1;
        
        return novo;
    }
    
    // Nó interno: encontra filho apropriado
    int i = 0;
    while (i < no->num_chaves && chave >= no->chaves[i]) {
        i++;
    }
    
    NO_BTREE *novo_filho = inserirRecursivo(no->filhos[i], chave, posicao, 
                                            chave_promovida, houve_split);
    
    if (!(*houve_split)) {
        return NULL;
    }
    
    // Filho foi dividido, precisa inserir chave promovida
    if (no->num_chaves < GRAU_BTREE) {
        // Tem espaço: insere a chave promovida
        int j = no->num_chaves - 1;
        while (j >= i && no->chaves[j] > *chave_promovida) {
            no->chaves[j + 1] = no->chaves[j];
            no->filhos[j + 2] = no->filhos[j + 1];
            j--;
        }
        
        no->chaves[j + 1] = *chave_promovida;
        no->filhos[j + 2] = novo_filho;
        no->num_chaves++;
        
        *houve_split = 0;
        return NULL;
    }
    
    // Nó interno cheio: precisa dividir
    NO_BTREE *novo_interno = criarNoInterno();
    
    // Array temporário
    long long int temp_chaves[GRAU_BTREE + 1];
    NO_BTREE *temp_filhos[GRAU_BTREE + 2];
    
    int j = 0, k = 0;
    int inserido = 0;
    
    for (j = 0; j < no->num_chaves; j++) {
        if (!inserido && j == i) {
            temp_chaves[k] = *chave_promovida;
            temp_filhos[k + 1] = novo_filho;
            k++;
            inserido = 1;
        }
        temp_chaves[k] = no->chaves[j];
        temp_filhos[k] = no->filhos[j];
        k++;
    }
    temp_filhos[k] = no->filhos[j];
    
    if (!inserido) {
        temp_chaves[k] = *chave_promovida;
        temp_filhos[k + 1] = novo_filho;
    }
    
    // Divide nó interno
    int meio = GRAU_BTREE / 2;
    
    no->num_chaves = 0;
    for (j = 0; j < meio; j++) {
        no->chaves[j] = temp_chaves[j];
        no->filhos[j] = temp_filhos[j];
        no->num_chaves++;
    }
    no->filhos[meio] = temp_filhos[meio];
    
    *chave_promovida = temp_chaves[meio];
    
    for (j = meio + 1; j < GRAU_BTREE + 1; j++) {
        novo_interno->chaves[j - meio - 1] = temp_chaves[j];
        novo_interno->filhos[j - meio - 1] = temp_filhos[j];
        novo_interno->num_chaves++;
    }
    novo_interno->filhos[novo_interno->num_chaves] = temp_filhos[GRAU_BTREE + 1];
    
    *houve_split = 1;
    return novo_interno;
}

/* ==================== FUNÇÕES PÚBLICAS ==================== */

ARVORE_BTREE *criarArvoreBTree() {
    ARVORE_BTREE *arvore = (ARVORE_BTREE *)malloc(sizeof(ARVORE_BTREE));
    if (arvore == NULL) return NULL;
    
    arvore->raiz = criarNoFolha();
    arvore->altura = 1;
    arvore->total_nos = 1;
    arvore->total_chaves = 0;
    
    return arvore;
}

void destruirArvoreBTree(ARVORE_BTREE *arvore) {
    if (arvore == NULL) return;
    destruirNo(arvore->raiz);
    free(arvore);
}

int buscarBTree(ARVORE_BTREE *arvore, long long int id_produto, long *posicao) {
    if (arvore == NULL || arvore->raiz == NULL) return 0;
    return buscarRecursivo(arvore->raiz, id_produto, posicao);
}

int inserirBTree(ARVORE_BTREE *arvore, long long int id_produto, long posicao) {
    if (arvore == NULL) return 0;
    
    long long int chave_promovida;
    int houve_split;
    
    NO_BTREE *novo_no = inserirRecursivo(arvore->raiz, id_produto, posicao, 
                                         &chave_promovida, &houve_split);
    
    if (houve_split) {
        // Raiz foi dividida: cria nova raiz
        NO_BTREE *nova_raiz = criarNoInterno();
        nova_raiz->chaves[0] = chave_promovida;
        nova_raiz->filhos[0] = arvore->raiz;
        nova_raiz->filhos[1] = novo_no;
        nova_raiz->num_chaves = 1;
        
        arvore->raiz = nova_raiz;
        arvore->altura++;
        arvore->total_nos++;
    }
    
    arvore->total_chaves++;
    return 1;
}

int removerBTree(ARVORE_BTREE *arvore, long long int id_produto) {
    // Implementação simplificada: apenas marca como removido
    // Para trabalho completo, implementar remoção com redistribuição
    (void)arvore;
    (void)id_produto;
    return 1;
}

ARVORE_BTREE *carregarIndiceBTreeDeArquivo(const char *nomeArquivo, double *tempo_criacao) {
    clock_t inicio = clock();
    
    FILE *arquivo = abrirArquivo(nomeArquivo, "rb");
    if (arquivo == NULL) return NULL;
    
    ARVORE_BTREE *arvore = criarArvoreBTree();
    if (arvore == NULL) {
        fclose(arquivo);
        return NULL;
    }
    
    JOIA joia;
    long posicao = 0;
    int contador = 0;
    
    while (fread(&joia, sizeof(JOIA), 1, arquivo) == 1) {
        inserirBTree(arvore, joia.id_produto, posicao);
        posicao += sizeof(JOIA);
        contador++;
        
        if (contador % 10000 == 0) {
            printf("\rCarregando índice B+: %d produtos...", contador);
            fflush(stdout);
        }
    }
    
    fclose(arquivo);
    
    clock_t fim = clock();
    *tempo_criacao = (double)(fim - inicio) / CLOCKS_PER_SEC;
    
    printf("\rÍndice B+ carregado: %d produtos em %.4f segundos\n", 
           contador, *tempo_criacao);
    
    return arvore;
}

void imprimirEstatisticasBTree(ARVORE_BTREE *arvore) {
    if (arvore == NULL) {
        printf("Arvore não inicializada.\n");
        return;
    }
    
    printf("\n=== Estatísticas da Árvore B+ ===\n");
    printf("Altura: %d\n", arvore->altura);
    printf("Total de nos: %d\n", arvore->total_nos);
    printf("Total de chaves: %d\n", arvore->total_chaves);
    printf("Ordem da arvore: %d\n", GRAU_BTREE);
    
    size_t memoria = calcularMemoriaUsadaBTree(arvore);
    printf("Memoria usada: %.2f MB\n", memoria / (1024.0 * 1024.0));
}

size_t calcularMemoriaUsadaBTree(ARVORE_BTREE *arvore) {
    if (arvore == NULL) return 0;
    
    size_t tamanho_no = sizeof(NO_BTREE);
    size_t tamanho_arvore = sizeof(ARVORE_BTREE);
    
    return tamanho_arvore + (arvore->total_nos * tamanho_no);
}


/*
 * ========================================================================
 * ÍNDICE EM MEMÓRIA - TABELA HASH
 * ========================================================================
*/

/* ==================== CRIAÇÃO E DESTRUIÇÃO ==================== */


TABELA_HASH *criarTabelaHash();

void destruirTabelaHash(TABELA_HASH *tabela);

/* ==================== FUNÇÃO HASH ==================== */

unsigned long calcularHash(long long int id_produto);

/* ==================== OPERAÇÕES BÁSICAS ==================== */


int inserirHash(TABELA_HASH *tabela, long long int id_produto, 
                long long int id_pedido, long posicao_arquivo);

ENTRADA_HASH **buscarHash(TABELA_HASH *tabela, long long int id_produto, int *quantidade);

int removerHash(TABELA_HASH *tabela, long long int id_produto);

/* ==================== CARREGAMENTO DO ARQUIVO ==================== */

TABELA_HASH *carregarIndiceHashDeArquivo(const char *nomeArquivo, double *tempo_criacao);


void imprimirEstatisticasHash(TABELA_HASH *tabela);

size_t calcularMemoriaUsadaHash(TABELA_HASH *tabela);

void analisarColisoes(TABELA_HASH *tabela);


/* ==================== CRIAÇÃO E DESTRUIÇÃO ==================== */

TABELA_HASH *criarTabelaHash() {
    TABELA_HASH *tabela = (TABELA_HASH *)malloc(sizeof(TABELA_HASH));
    if (tabela == NULL) return NULL;
    
    tabela->tamanho = TAMANHO_TABELA_HASH;
    tabela->total_elementos = 0;
    tabela->total_colisoes = 0;
    
    // Aloca array de ponteiros
    tabela->entradas = (ENTRADA_HASH **)calloc(TAMANHO_TABELA_HASH, sizeof(ENTRADA_HASH *));
    if (tabela->entradas == NULL) {
        free(tabela);
        return NULL;
    }
    
    return tabela;
}

void destruirTabelaHash(TABELA_HASH *tabela) {
    if (tabela == NULL) return;
    
    // Libera todas as listas encadeadas
    for (int i = 0; i < tabela->tamanho; i++) {
        ENTRADA_HASH *atual = tabela->entradas[i];
        while (atual != NULL) {
            ENTRADA_HASH *temp = atual;
            atual = atual->proximo;
            free(temp);
        }
    }
    
    free(tabela->entradas);
    free(tabela);
}

/* ==================== FUNÇÃO HASH ==================== */

unsigned long calcularHash(long long int id_produto) {

    unsigned long hash = (unsigned long)(id_produto * 2654435761UL);
    return hash % TAMANHO_TABELA_HASH;
}

/* ==================== OPERAÇÕES BÁSICAS ==================== */

int inserirHash(TABELA_HASH *tabela, long long int id_produto, 
                long long int id_pedido, long posicao_arquivo) {
    if (tabela == NULL) return 0;
    
    // Calcula índice hash
    unsigned long indice = calcularHash(id_produto);
    
    // Cria nova entrada
    ENTRADA_HASH *nova = (ENTRADA_HASH *)malloc(sizeof(ENTRADA_HASH));
    if (nova == NULL) return 0;
    
    nova->id_produto = id_produto;
    nova->id_pedido = id_pedido;
    nova->posicao_arquivo = posicao_arquivo;
    nova->proximo = NULL;
    
    // Se posição vazia, insere diretamente
    if (tabela->entradas[indice] == NULL) {
        tabela->entradas[indice] = nova;
    } else {
        // Colisão: adiciona no início da lista (mais eficiente)
        nova->proximo = tabela->entradas[indice];
        tabela->entradas[indice] = nova;
        tabela->total_colisoes++;
    }
    
    tabela->total_elementos++;
    return 1;
}

ENTRADA_HASH **buscarHash(TABELA_HASH *tabela, long long int id_produto, int *quantidade) {
    *quantidade = 0;
    if (tabela == NULL) return NULL;
    
    // Calcula índice hash
    unsigned long indice = calcularHash(id_produto);
    
    // Conta quantas entradas existem com este id_produto
    ENTRADA_HASH *atual = tabela->entradas[indice];
    int count = 0;
    
    while (atual != NULL) {
        if (atual->id_produto == id_produto) {
            count++;
        }
        atual = atual->proximo;
    }
    
    if (count == 0) return NULL;
    
    // Aloca array de resultados
    ENTRADA_HASH **resultados = (ENTRADA_HASH **)malloc(count * sizeof(ENTRADA_HASH *));
    if (resultados == NULL) return NULL;
    
    // Preenche array de resultados
    atual = tabela->entradas[indice];
    int i = 0;
    
    while (atual != NULL) {
        if (atual->id_produto == id_produto) {
            resultados[i++] = atual;
        }
        atual = atual->proximo;
    }
    
    *quantidade = count;
    return resultados;
}

int removerHash(TABELA_HASH *tabela, long long int id_produto) {
    if (tabela == NULL) return 0;
    
    unsigned long indice = calcularHash(id_produto);
    int removidos = 0;
    
    ENTRADA_HASH *atual = tabela->entradas[indice];
    ENTRADA_HASH *anterior = NULL;
    
    while (atual != NULL) {
        if (atual->id_produto == id_produto) {
            // Remove nó
            ENTRADA_HASH *temp = atual;
            
            if (anterior == NULL) {
                // Primeiro nó da lista
                tabela->entradas[indice] = atual->proximo;
            } else {
                anterior->proximo = atual->proximo;
            }
            
            atual = atual->proximo;
            free(temp);
            
            tabela->total_elementos--;
            removidos++;
        } else {
            anterior = atual;
            atual = atual->proximo;
        }
    }
    
    return removidos;
}

/* ==================== CARREGAMENTO DO ARQUIVO ==================== */

TABELA_HASH *carregarIndiceHashDeArquivo(const char *nomeArquivo, double *tempo_criacao) {
    clock_t inicio = clock();
    
    FILE *arquivo = abrirArquivo(nomeArquivo, "rb");
    if (arquivo == NULL) return NULL;
    
    TABELA_HASH *tabela = criarTabelaHash();
    if (tabela == NULL) {
        fclose(arquivo);
        return NULL;
    }
    
    PEDIDO pedido;
    long posicao = 0;
    int contador = 0;
    
    while (fread(&pedido, sizeof(PEDIDO), 1, arquivo) == 1) {
        // Ignora pedidos removidos
        if (!pedidoRemovido(&pedido)) {
            // Indexa por id_produto (não pela chave de ordenação id_pedido)
            inserirHash(tabela, pedido.id_produto, pedido.id_pedido, posicao);
            contador++;
        }
        
        posicao += sizeof(PEDIDO);
        
        if (contador % 10000 == 0) {
            printf("\rCarregando indice hash: %d pedidos...", contador);
            fflush(stdout);
        }
    }
    
    fclose(arquivo);
    
    clock_t fim = clock();
    *tempo_criacao = (double)(fim - inicio) / CLOCKS_PER_SEC;
    
    printf("\rIndice hash carregado: %d pedidos em %.4f segundos\n", 
           contador, *tempo_criacao);
    
    return tabela;
}

/* ==================== ESTATÍSTICAS ==================== */

void imprimirEstatisticasHash(TABELA_HASH *tabela) {
    if (tabela == NULL) {
        printf("Tabela não inicializada.\n");
        return;
    }
    
    printf("\n=== Estatísticas da Tabela Hash ===\n");
    printf("Tamanho da tabela: %d\n", tabela->tamanho);
    printf("Total de elementos: %d\n", tabela->total_elementos);
    printf("Total de colisoes: %d\n", tabela->total_colisoes);
    
    // Calcula taxa de ocupação
    int posicoes_ocupadas = 0;
    for (int i = 0; i < tabela->tamanho; i++) {
        if (tabela->entradas[i] != NULL) {
            posicoes_ocupadas++;
        }
    }
    
    float taxa_ocupacao = (float)posicoes_ocupadas / tabela->tamanho * 100;
    printf("Posições ocupadas: %d (%.2f%%)\n", posicoes_ocupadas, taxa_ocupacao);
    
    float fator_carga = (float)tabela->total_elementos / tabela->tamanho;
    printf("Fator de carga: %.4f\n", fator_carga);
    
    size_t memoria = calcularMemoriaUsadaHash(tabela);
    printf("Memoria usada: %.2f MB\n", memoria / (1024.0 * 1024.0));
}

size_t calcularMemoriaUsadaHash(TABELA_HASH *tabela) {
    if (tabela == NULL) return 0;
    
    size_t tamanho_tabela = sizeof(TABELA_HASH);
    size_t tamanho_array = tabela->tamanho * sizeof(ENTRADA_HASH *);
    size_t tamanho_entradas = tabela->total_elementos * sizeof(ENTRADA_HASH);
    
    return tamanho_tabela + tamanho_array + tamanho_entradas;
}

void analisarColisoes(TABELA_HASH *tabela) {
    if (tabela == NULL) return;
    
    printf("\n=== Analise de Colisoes ===\n");
    
    // Histograma de tamanhos de cadeia
    int max_cadeia = 0;
    int cadeias_vazias = 0;
    int cadeias_unitarias = 0;
    
    int histograma[20] = {0}; // Contadores para tamanhos 0-19+
    
    for (int i = 0; i < tabela->tamanho; i++) {
        int tamanho = 0;
        ENTRADA_HASH *atual = tabela->entradas[i];
        
        while (atual != NULL) {
            tamanho++;
            atual = atual->proximo;
        }
        
        if (tamanho == 0) {
            cadeias_vazias++;
        } else if (tamanho == 1) {
            cadeias_unitarias++;
        }
        
        if (tamanho > max_cadeia) {
            max_cadeia = tamanho;
        }
        
        if (tamanho < 20) {
            histograma[tamanho]++;
        } else {
            histograma[19]++;
        }
    }
    
    printf("Maior cadeia: %d elementos\n", max_cadeia);
    printf("Cadeias vazias: %d\n", cadeias_vazias);
    printf("Cadeias unitarias: %d (sem colisao)\n", cadeias_unitarias);
    
    printf("\nDistribuicao dos tamanhos de cadeia:\n");
    for (int i = 0; i <= 10 && i <= max_cadeia; i++) {
        if (histograma[i] > 0) {
            printf("  Tamanho %2d: %d posicoes\n", i, histograma[i]);
        }
    }
    
    if (max_cadeia > 10) {
        printf("  Tamanho >10: ");
        int soma = 0;
        for (int i = 11; i < 20; i++) {
            soma += histograma[i];
        }
        printf("%d posicoes\n", soma);
    }
    
    // Estrategia de resolucao
    printf("\nEstrategia de resolucao de colisoes: Encadeamento (Chaining)\n");
}

/* ============================================================================
 * IMPLEMENTAÇÕES - MÓDULOS 2-5: OPERAÇÕES BÁSICAS DE ARQUIVO
 * ============================================================================ */

/* Funções auxiliares para operações de arquivo */

FILE *abrirArquivo(const char *nomeArquivo, const char *modo) {
    FILE *arquivo = fopen(nomeArquivo, modo);
    if (arquivo == NULL) {
        printf("ERRO: Não foi possível abrir o arquivo '%s' no modo '%s'\n", nomeArquivo, modo);
    }
    return arquivo;
}

int pedidoRemovido(PEDIDO *pedido) {
    return (pedido->data[0] == FLAG_REMOVIDO);
}

int comparadorPedidos(const void *a, const void *b) {
    PEDIDO *p1 = (PEDIDO *)a;
    PEDIDO *p2 = (PEDIDO *)b;
    if (p1->id_pedido < p2->id_pedido) return -1;
    if (p1->id_pedido > p2->id_pedido) return 1;
    return 0;
}

int comparadorJoias(const void *a, const void *b) {
    JOIA *j1 = (JOIA *)a;
    JOIA *j2 = (JOIA *)b;
    if (j1->id_produto < j2->id_produto) return -1;
    if (j1->id_produto > j2->id_produto) return 1;
    return 0;
}

/* ============================================================================
 * INTERFACE DO USUÁRIO - MENU E OPÇÕES
 * ============================================================================ */

/* ==================== FUNÇÕES DE MENU ==================== */

void exibirMenu() {
    printf("\n" "========================================\n");
    printf("  MENU PRINCIPAL\n");
    printf("========================================\n");
    printf("\n--- DADOS ---\n");
    printf("1.  Carregar dados do CSV (criar .dat)\n");
    printf("\n--- FUNCIONALIDADES BASICAS ---\n");
    printf("2.  Mostrar primeiros registros\n");
    printf("3.  Buscar produto especifico\n");
    printf("4.  Inserir novo registro\n");
    printf("5.  Remover registro\n");
    printf("\n--- INDICES EM MEMORIA ---\n");
    printf("6.  Carregar indices em memoria\n");
    printf("7.  Buscar produto (Arvore B+)\n");
    printf("8.  Buscar pedidos por produto (Hash)\n");
    printf("9.  Estatisticas dos indices\n");
    printf("10. Analise de colisoes (Hash)\n");
    printf("11. Executar benchmarks completos\n");
    printf("\n--- COMPRESSAO E CRIPTOGRAFIA ---\n");
    printf("12. Comprimir arquivo (Huffman)\n");
    printf("13. Descomprimir arquivo (Huffman)\n");
    printf("14. Criptografar arquivo (Transposicao)\n");
    printf("15. Descriptografar arquivo (Transposicao)\n");
    printf("16. Proteger arquivo (Comprimir + Criptografar)\n");
    printf("17. Restaurar arquivo protegido\n");
    printf("18. Verificar integridade\n");
    printf("\n0.  Sair\n");
    printf("========================================\n");
    printf("Escolha uma opcao: ");
}

/* ==================== OPÇÕES DO MENU ==================== */

void opcaoCarregarCSV() {
    printf("\n" "=== CARREGAR DADOS DO CSV ===\n");
    
    // Verifica se arquivos já existem
    FILE *test = fopen(ARQUIVO_PRODUTOS, "rb");
    if (test) {
        fclose(test);
        printf("\nArquivos .dat ja existem. Deseja recriar? (s/n): ");
        char resp;
        scanf(" %c", &resp);
        if (resp != 's' && resp != 'S') {
            return;
        }
    }
    
    printf("\nCarregando dados de %s...\n", ARQUIVO_CSV);
    printf("Este processo pode demorar alguns minutos.\n\n");
    
    if (carregarDadosDoCSV(ARQUIVO_CSV, 1000)) {
        printf("\nArquivos .dat criados com sucesso!\n");
    } else {
        printf("\nErro ao carregar dados do CSV!\n");
    }
}

void opcaoCarregarIndices() {
    printf("\n" "=== CARREGANDO INDICES EM MEMORIA ===\n\n");
    
    if (indice_produtos_memoria != NULL) {
        printf("Indice de produtos ja esta carregado.\n");
        printf("Deseja recarregar? (s/n): ");
        char resp;
        scanf(" %c", &resp);
        if (resp != 's' && resp != 'S') {
            return;
        }
        destruirArvoreBTree(indice_produtos_memoria);
        indice_produtos_memoria = NULL;
    }
    
    if (indice_pedidos_memoria != NULL) {
        destruirTabelaHash(indice_pedidos_memoria);
        indice_pedidos_memoria = NULL;
    }
    
    double tempo_btree, tempo_hash;
    
    printf("Carregando indice B+ de produtos...\n");
    indice_produtos_memoria = carregarIndiceBTreeDeArquivo(ARQUIVO_PRODUTOS, &tempo_btree);
    
    if (indice_produtos_memoria == NULL) {
        printf("\nERRO: NNao foi possivel carregar indice de produtos.\n");
        printf("Verifique se o arquivo %s existe.\n", ARQUIVO_PRODUTOS);
        return;
    }
    
    printf("\nCarregando indice hash de pedidos...\n");
    indice_pedidos_memoria = carregarIndiceHashDeArquivo(ARQUIVO_PEDIDOS, &tempo_hash);
    
    if (indice_pedidos_memoria == NULL) {
        printf("\nERRO: Nao foi possivel carregar indice de pedidos.\n");
        printf("Verifique se o arquivo %s existe.\n", ARQUIVO_PEDIDOS);
        destruirArvoreBTree(indice_produtos_memoria);
        indice_produtos_memoria = NULL;
        return;
    }
    
    printf("\nIndices carregados com sucesso!\n");
    printf("  Tempo B+:   %.4f segundos\n", tempo_btree);
    printf("  Tempo Hash: %.4f segundos\n", tempo_hash);
}

void opcaoBuscarProduto() {
    if (indice_produtos_memoria == NULL) {
        printf("\nERRO: Indice de produtos nao carregado.\n");
        printf("Use a opcao 1 para carregar os indices primeiro.\n");
        return;
    }
    
    printf("\n" "=== BUSCAR PRODUTO (ARVORE B+) ===\n");
    printf("Digite o ID do produto: ");
    long long int id_produto;
    scanf("%lld", &id_produto);
    
    long posicao;
    int encontrado = buscarBTree(indice_produtos_memoria, id_produto, &posicao);
    
    if (encontrado) {
        printf("\nProduto encontrado!\n");
        printf("  ID: %lld\n", id_produto);
        printf("  Posicao no arquivo: %ld bytes\n", posicao);
        
        // Lê o registro do arquivo
        FILE *arquivo = abrirArquivo(ARQUIVO_PRODUTOS, "rb");
        if (arquivo != NULL) {
            JOIA joia;
            fseek(arquivo, posicao, SEEK_SET);
            if (fread(&joia, sizeof(JOIA), 1, arquivo) == 1) {
                printf("\n  Detalhes:\n");
                printf("    Categoria: %lld\n", joia.id_categoria);
                printf("    Marca: %d\n", joia.id_marca);
                printf("    Preco: $%.2f\n", joia.preco_usd);
                printf("    Genero: %c\n", joia.genero_produto);
                printf("    Cor: %s\n", joia.cor);
                printf("    Metal: %s\n", joia.metal);
                printf("    Gema: %s\n", joia.gema);
            }
            fclose(arquivo);
        }
    } else {
        printf("\n✗ Produto não encontrado.\n");
    }
}

void opcaoBuscarPedidosPorProduto() {
    if (indice_pedidos_memoria == NULL) {
        printf("\nERRO: Indice de pedidos nao carregado.\n");
        printf("Use a opcao 1 para carregar os indices primeiro.\n");
        return;
    }
    
    printf("\n" "=== BUSCAR PEDIDOS POR PRODUTO (HASH) ===\n");
    printf("Digite o ID do produto: ");
    long long int id_produto;
    scanf("%lld", &id_produto);
    
    int quantidade;
    ENTRADA_HASH **resultados = buscarHash(indice_pedidos_memoria, id_produto, &quantidade);
    
    if (resultados == NULL || quantidade == 0) {
        printf("\n✗ Nenhum pedido encontrado para este produto.\n");
        return;
    }
    
    printf("\nEncontrados %d pedidos com este produto:\n\n", quantidade);
    
    // Mostra até 10 primeiros resultados
    int limite = quantidade > 10 ? 10 : quantidade;
    for (int i = 0; i < limite; i++) {
        printf("  %d. Pedido ID: %lld (posicao: %ld bytes)\n",
               i + 1,
               resultados[i]->id_pedido,
               resultados[i]->posicao_arquivo);
    }
    
    if (quantidade > 10) {
        printf("  ... e mais %d pedidos.\n", quantidade - 10);
    }
    
    free(resultados);
}

void opcaoEstatisticasIndices() {
    printf("\n" "=== ESTATISTICAS DOS INDICES ===\n");
    
    if (indice_produtos_memoria != NULL) {
        imprimirEstatisticasBTree(indice_produtos_memoria);
    } else {
        printf("\nIndice de produtos (B+): NAO CARREGADO\n");
    }
    
    if (indice_pedidos_memoria != NULL) {
        imprimirEstatisticasHash(indice_pedidos_memoria);
    } else {
        printf("\nÍndice de pedidos (Hash): NÃO CARREGADO\n");
    }
}

void opcaoAnalisarColisoes() {
    if (indice_pedidos_memoria == NULL) {
        printf("\nERRO: Indice de pedidos nao carregado.\n");
        return;
    }
    
    analisarColisoes(indice_pedidos_memoria);
}

void opcaoBenchmarks() {
    printf("\n" "=== EXECUTAR BENCHMARKS COMPLETOS ===\n");
    printf("\nEsta operacao pode demorar alguns minutos.\n");
    printf("Deseja continuar? (s/n): ");
    char resp;
    scanf(" %c", &resp);
    
    if (resp != 's' && resp != 'S') {
        printf("Operacao cancelada.\n");
        return;
    }
    
    // Libera índices existentes se houver
    if (indice_produtos_memoria != NULL) {
        destruirArvoreBTree(indice_produtos_memoria);
        indice_produtos_memoria = NULL;
    }
    if (indice_pedidos_memoria != NULL) {
        destruirTabelaHash(indice_pedidos_memoria);
        indice_pedidos_memoria = NULL;
    }
    
    gerarRelatorioCompleto(ARQUIVO_PRODUTOS, ARQUIVO_PEDIDOS);
    
    printf("\nPressione ENTER para continuar...");
    getchar();
    getchar();
}

void opcaoMostrarRegistros() {
    printf("\n" "=== PRIMEIROS REGISTROS ===\n\n");
    
    FILE *arquivo = abrirArquivo(ARQUIVO_PRODUTOS, "rb");
    if (arquivo == NULL) {
        printf("Erro ao abrir arquivo de produtos.\n");
        return;
    }
    
    printf("Primeiras 5 joias:\n");
    JOIA joia;
    for (int i = 0; i < 5 && fread(&joia, sizeof(JOIA), 1, arquivo) == 1; i++) {
        printf("  %d. ID: %lld, Preco: $%.2f, Gema: %s\n",
               i + 1, joia.id_produto, joia.preco_usd, joia.gema);
    }
    
    fclose(arquivo);
}

void opcaoBuscarProdutoArquivo() {
    printf("\n" "=== BUSCAR PRODUTO (INDICE DE ARQUIVO) ===\n");
    printf("Digite o ID do produto: ");
    long long int id_produto;
    scanf("%lld", &id_produto);
    
    FILE *arquivo = abrirArquivo(ARQUIVO_PRODUTOS, "rb");
    FILE *indice = abrirArquivo(ARQUIVO_INDICE_PRODUTOS, "rb");
    
    if (!arquivo || !indice) {
        printf("Erro ao abrir arquivos.\n");
        if (arquivo) fclose(arquivo);
        if (indice) fclose(indice);
        return;
    }
    
    // Busca binária no índice
    fseek(indice, 0, SEEK_END);
    long fileSize = ftell(indice);
    int totalEntries = fileSize / sizeof(INDICE);
    
    if (totalEntries == 0) {
        printf("Indice vazio.\n");
        fclose(arquivo);
        fclose(indice);
        return;
    }
    
    int left = 0, right = totalEntries - 1;
    long startPosition = 0;
    INDICE currentIndex;
    
    while (left <= right) {
        int middle = left + (right - left) / 2;
        fseek(indice, middle * sizeof(INDICE), SEEK_SET);
        fread(&currentIndex, sizeof(INDICE), 1, indice);
        
        if (currentIndex.id == id_produto) {
            startPosition = currentIndex.posicao;
            break;
        }
        
        if (currentIndex.id < id_produto) {
            startPosition = currentIndex.posicao;
            left = middle + 1;
        } else {
            right = middle - 1;
        }
    }
    
    // Busca sequencial a partir da posição encontrada
    fseek(arquivo, startPosition, SEEK_SET);
    JOIA joia;
    int encontrado = 0;
    
    for (int i = 0; i < 1000; i++) {
        if (fread(&joia, sizeof(JOIA), 1, arquivo) != 1)
            break;
        
        if (joia.id_produto == id_produto) {
            printf("\nProduto encontrado!\n");
            printf("  ID: %lld\n", joia.id_produto);
            printf("  Categoria: %lld\n", joia.id_categoria);
            printf("  Marca: %d\n", joia.id_marca);
            printf("  Preco: $%.2f\n", joia.preco_usd);
            printf("  Genero: %c\n", joia.genero_produto);
            printf("  Cor: %s\n", joia.cor);
            printf("  Metal: %s\n", joia.metal);
            printf("  Gema: %s\n", joia.gema);
            encontrado = 1;
            break;
        }
        if (joia.id_produto > id_produto)
            break;
    }
    
    if (!encontrado) {
        printf("\nProduto nao encontrado.\n");
    }
    
    fclose(arquivo);
    fclose(indice);
}

void opcaoInserir() {
    printf("\n" "=== INSERIR NOVO PEDIDO ===\n");
    
    FILE *arquivo = abrirArquivo(ARQUIVO_PEDIDOS, "rb+");
    if (!arquivo) {
        printf("Erro ao abrir arquivo de pedidos.\n");
        return;
    }
    
    PEDIDO novoPedido;
    
    printf("ID do pedido: ");
    scanf("%lld", &novoPedido.id_pedido);
    printf("ID do produto: ");
    scanf("%lld", &novoPedido.id_produto);
    printf("ID da categoria: ");
    scanf("%lld", &novoPedido.id_categoria);
    printf("ID da marca: ");
    scanf("%d", &novoPedido.id_marca);
    printf("Quantidade: ");
    scanf("%d", &novoPedido.quantidade);
    printf("Preco USD: ");
    scanf("%f", &novoPedido.preco_usd);
    printf("ID do usuario: ");
    scanf("%lld", &novoPedido.id_usuario);
    
    // Data atual UTC
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    strftime(novoPedido.data, sizeof(novoPedido.data), "%Y-%m-%d %H:%M:%S UTC", t);
    
    // Outros campos
    printf("Genero do produto (M/F/U): ");
    scanf(" %c", &novoPedido.genero_produto);
    printf("Cor: ");
    scanf("%s", novoPedido.cor);
    printf("Metal: ");
    scanf("%s", novoPedido.metal);
    printf("Gema: ");
    scanf("%s", novoPedido.gema);
    printf("Alias da categoria: ");
    scanf("%s", novoPedido.alias_categoria);
    
    // Insere no final do arquivo
    fseek(arquivo, 0, SEEK_END);
    long posicao = ftell(arquivo);
    
    if (fwrite(&novoPedido, sizeof(PEDIDO), 1, arquivo) == 1) {
        printf("\nPedido inserido com sucesso na posicao %ld bytes!\n", posicao);
        printf("IMPORTANTE: Reconstrua o indice para otimizar buscas!\n");
    } else {
        printf("\nErro ao inserir pedido.\n");
    }
    
    fclose(arquivo);
}

void opcaoRemover() {
    printf("\n" "=== REMOVER PEDIDO ===\n");
    printf("Digite o ID do pedido a remover: ");
    long long int id_pedido;
    scanf("%lld", &id_pedido);
    
    FILE *arquivo = abrirArquivo(ARQUIVO_PEDIDOS, "rb+");
    if (!arquivo) {
        printf("Erro ao abrir arquivo de pedidos.\n");
        return;
    }
    
    // Busca o pedido
    PEDIDO pedido;
    long posicao = 0;
    int encontrado = 0;
    
    while (fread(&pedido, sizeof(PEDIDO), 1, arquivo) == 1) {
        if (pedido.id_pedido == id_pedido && pedido.data[0] != FLAG_REMOVIDO) {
            printf("\nPedido encontrado:\n");
            printf("  ID: %lld\n", pedido.id_pedido);
            printf("  Data: %s\n", pedido.data);
            printf("  Produto: %lld\n", pedido.id_produto);
            printf("  Quantidade: %d\n", pedido.quantidade);
            printf("  Preco: $%.2f\n", pedido.preco_usd);
            
            printf("\nConfirma remocao? (s/n): ");
            char confirma;
            scanf(" %c", &confirma);
            
            if (confirma == 's' || confirma == 'S') {
                // Marca como removido
                pedido.data[0] = FLAG_REMOVIDO;
                
                // Volta e escreve
                fseek(arquivo, posicao, SEEK_SET);
                fwrite(&pedido, sizeof(PEDIDO), 1, arquivo);
                fflush(arquivo);
                
                printf("\nPedido removido com sucesso!\n");
                contador_remocoes++;
                
                if (contador_remocoes >= LIMITE_RECONSTRUCAO) {
                    printf("\nAVISO: %d remocoes realizadas.\n", contador_remocoes);
                    printf("Recomenda-se reconstruir o indice!\n");
                }
            } else {
                printf("\nRemocao cancelada.\n");
            }
            encontrado = 1;
            break;
        }
        posicao = ftell(arquivo);
    }
    
    if (!encontrado) {
        printf("\nPedido nao encontrado.\n");
    }
    
    fclose(arquivo);
}

/* ==================== OPÇÕES DE COMPRESSÃO E CRIPTOGRAFIA ==================== */

void opcaoComprimir() {
    printf("\n" "=== COMPRIMIR ARQUIVO COM HUFFMAN ===\n");
    printf("Escolha o arquivo:\n");
    printf("1. Produtos (jewelryRegister.dat)\n");
    printf("2. Pedidos (orderHistory.dat)\n");
    printf("3. Outro arquivo\n");
    printf("Opção: ");
    
    int opcao;
    scanf("%d", &opcao);
    
    char entrada[256], saida[256];
    
    switch(opcao) {
        case 1:
            strcpy(entrada, ARQUIVO_PRODUTOS);
            strcpy(saida, "../data/jewelryRegister.dat.huff");
            break;
        case 2:
            strcpy(entrada, ARQUIVO_PEDIDOS);
            strcpy(saida, "../data/orderHistory.dat.huff");
            break;
        case 3:
            printf("Digite o caminho do arquivo: ");
            scanf("%s", entrada);
            sprintf(saida, "%s.huff", entrada);
            break;
        default:
            printf("Opção inválida.\n");
            return;
    }
    
    double taxa;
    if (comprimirArquivoHuffman(entrada, saida, &taxa)) {
        printf("\nCompressao bem-sucedida!\n");
        printf("Arquivo comprimido: %s\n", saida);
        imprimirEstatisticasCompressao(entrada, saida);
    } else {
        printf("\nErro ao comprimir arquivo.\n");
    }
}

void opcaoDescomprimir() {
    printf("\n" "=== DESCOMPRIMIR ARQUIVO HUFFMAN ===\n");
    printf("Digite o caminho do arquivo .huff: ");
    
    char entrada[256], saida[256];
    scanf("%s", entrada);
    
    // Remove extensão .huff
    strcpy(saida, entrada);
    char *ext = strstr(saida, ".huff");
    if (ext != NULL) {
        *ext = '\0';
    } else {
        strcat(saida, ".decompressed");
    }
    
    if (descomprimirArquivoHuffman(entrada, saida)) {
        printf("\nDescompressao bem-sucedida!\n");
        printf("Arquivo restaurado: %s\n", saida);
    } else {
        printf("\nErro ao descomprimir arquivo.\n");
    }
}

void opcaoCriptografar() {
    printf("\n" "=== CRIPTOGRAFAR COM TRANSPOSIÇÃO ===\n");
    printf("Chave utilizada: %s\n\n", CHAVE_TRANSPOSICAO);
    
    printf("Escolha o arquivo:\n");
    printf("1. Produtos (jewelryRegister.dat)\n");
    printf("2. Pedidos (orderHistory.dat)\n");
    printf("3. Outro arquivo\n");
    printf("Opção: ");
    
    int opcao;
    scanf("%d", &opcao);
    
    char entrada[256], saida[256];
    
    switch(opcao) {
        case 1:
            strcpy(entrada, ARQUIVO_PRODUTOS);
            strcpy(saida, "../data/jewelryRegister.dat.crypt");
            break;
        case 2:
            strcpy(entrada, ARQUIVO_PEDIDOS);
            strcpy(saida, "../data/orderHistory.dat.crypt");
            break;
        case 3:
            printf("Digite o caminho do arquivo: ");
            scanf("%s", entrada);
            sprintf(saida, "%s.crypt", entrada);
            break;
        default:
            printf("Opção inválida.\n");
            return;
    }
    
    if (criptografarArquivo(entrada, saida)) {
        printf("\nCriptografia bem-sucedida!\n");
        printf("  Arquivo criptografado: %s\n", saida);
    } else {
        printf("\nErro ao criptografar arquivo.\n");
    }
}

void opcaoDescriptografar() {
    printf("\n" "=== DESCRIPTOGRAFAR ARQUIVO ===\n");
    printf("Digite o caminho do arquivo .crypt: ");
    
    char entrada[256], saida[256];
    scanf("%s", entrada);
    
    // Remove extensão .crypt
    strcpy(saida, entrada);
    char *ext = strstr(saida, ".crypt");
    if (ext != NULL) {
        *ext = '\0';
    } else {
        strcat(saida, ".decrypted");
    }
    
    if (descriptografarArquivo(entrada, saida)) {
        printf("\nDescriptografia bem-sucedida!\n");
        printf("Arquivo restaurado: %s\n", saida);
    } else {
        printf("\nErro ao descriptografar arquivo.\n");
    }
}

void opcaoProtegerArquivo() {
    printf("\n" "=== PROTEGER ARQUIVO (COMPRIMIR + CRIPTOGRAFAR) ===\n");
    printf("Esta opcao aplica compressao Huffman seguida de criptografia\n");
    printf("por transposicao usando a chave: %s\n\n", CHAVE_TRANSPOSICAO);
    
    printf("Escolha o arquivo:\n");
    printf("1. Produtos (jewelryRegister.dat)\n");
    printf("2. Pedidos (orderHistory.dat)\n");
    printf("3. Outro arquivo\n");
    printf("Opcao: ");
    
    int opcao;
    scanf("%d", &opcao);
    
    char entrada[256], saida[256];
    
    switch(opcao) {
        case 1:
            strcpy(entrada, ARQUIVO_PRODUTOS);
            strcpy(saida, "../data/jewelryRegister.sec");
            break;
        case 2:
            strcpy(entrada, ARQUIVO_PEDIDOS);
            strcpy(saida, "../data/orderHistory.sec");
            break;
        case 3:
            printf("Digite o caminho do arquivo: ");
            scanf("%s", entrada);
            sprintf(saida, "%s.sec", entrada);
            break;
        default:
            printf("Opcao invalida.\n");
            return;
    }
    
    double taxa;
    if (comprimirECriptografarArquivo(entrada, saida, &taxa)) {
        printf("\n;======================================;\n");
        printf("|   ARQUIVO PROTEGIDO COM SUCESSO!     |\n");
        printf(";======================================;\n");
        printf("\nO arquivo foi:\n");
        printf("  1. Comprimido com Huffman (taxa: %.2f%%)\n", taxa * 100);
        printf("  2. Criptografado com transposição\n");
        printf("\nArquivo seguro: %s\n", saida);
        printf("\nPara restaurar, use a opção 16.\n");
    } else {
        printf("\nErro ao proteger arquivo.\n");
    }
}

void opcaoRestaurarArquivo() {
    printf("\n" "=== RESTAURAR ARQUIVO PROTEGIDO ===\n");
    printf("Digite o caminho do arquivo .sec: ");
    
    char entrada[256], saida[256];
    scanf("%s", entrada);
    
    // Remove extensão .sec
    strcpy(saida, entrada);
    char *ext = strstr(saida, ".sec");
    if (ext != NULL) {
        *ext = '\0';
    } else {
        strcat(saida, ".restored");
    }
    
    if (descriptografarEDescomprimirArquivo(entrada, saida)) {
        printf("\n;======================================;\n");
        printf("|   ARQUIVO RESTAURADO COM SUCESSO!    |\n");
        printf(";======================================;\n");
        printf("\nO arquivo foi:\n");
        printf("  1. Descriptografado\n");
        printf("  2. Descomprimido\n");
        printf("\nArquivo restaurado: %s\n", saida);
    } else {
        printf("\nErro ao restaurar arquivo.\n");
    }
}

void opcaoVerificarIntegridade() {
    printf("\n" "=== VERIFICAR INTEGRIDADE ===\n");
    printf("Esta opção compara dois arquivos byte a byte.\n\n");
    
    char arquivo1[256], arquivo2[256];
    
    printf("Digite o caminho do arquivo original: ");
    scanf("%s", arquivo1);
    
    printf("Digite o caminho do arquivo restaurado: ");
    scanf("%s", arquivo2);
    
    printf("\nVerificando...\n");
    verificarIntegridadeArquivo(arquivo1, arquivo2);
}

/* ==================== FUNÇÃO PRINCIPAL ==================== */

int main() {
    int opcao;
    do {
        exibirMenu();
        scanf("%d", &opcao);

        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF);
        
        switch (opcao) {
            case 1:
                opcaoCarregarCSV();
                break;
            case 2:
                opcaoMostrarRegistros();
                break;
            case 3:
                opcaoBuscarProdutoArquivo();
                break;
            case 4:
                opcaoInserir();
                break;
            case 5:
                opcaoRemover();
                break;
            case 6:
                opcaoCarregarIndices();
                break;
            case 7:
                opcaoBuscarProduto();
                break;
            case 8:
                opcaoBuscarPedidosPorProduto();
                break;
            case 9:
                opcaoEstatisticasIndices();
                break;
            case 10:
                opcaoAnalisarColisoes();
                break;
            case 11:
                opcaoBenchmarks();
                break;
            case 12:
                opcaoComprimir();
                break;
            case 13:
                opcaoDescomprimir();
                break;
            case 14:
                opcaoCriptografar();
                break;
            case 15:
                opcaoDescriptografar();
                break;
            case 16:
                opcaoProtegerArquivo();
                break;
            case 17:
                opcaoRestaurarArquivo();
                break;
            case 18:
                opcaoVerificarIntegridade();
                break;
            case 0:
                printf("\nEncerrando sistema...\n");
                break;
            default:
                printf("\nOpcao invalida! Tente novamente.\n");
        }
        
        if (opcao != 0 && opcao != 6) {
            printf("\nPressione ENTER para continuar...");
            getchar();
            //getchar();
        }
        
    } while (opcao != 0);
    
    // Limpeza
    if (indice_produtos_memoria != NULL) {
        destruirArvoreBTree(indice_produtos_memoria);
    }
    if (indice_pedidos_memoria != NULL) {
        destruirTabelaHash(indice_pedidos_memoria);
    }
    
    printf("\n");
    printf(";======================================;\n");
    printf("|  Obrigado por usar o Sistema ISAM de Joias!             |\n");
    printf(";======================================;\n");
    printf("\n");
    
    return 0;
}




