# 📊 Relatório Completo de Benchmarks - Trabalho 2
## Índices em Memória para Arquivos de Dados

---

## 📝 Descrição dos Índices em Memória

O sistema utiliza duas estruturas de índice em memória para otimizar as operações nos arquivos binários de dados:

| Estrutura | Aplicação | Chave de Indexação | Parâmetro | Estratégia de Colisão |
|-----------|-----------|-------------------|-----------|----------------------|
| **Árvore B+** | Arquivo de Produtos (`jewelryRegister.dat`) | `id_produto` | Ordem: 100 | Não Aplicável |
| **Tabela Hash** | Arquivo de Pedidos (`orderHistory.dat`) | `id_produto` | Tamanho: 50.000 | Encadeamento (Chaining) |

---

## ⚙️ Benchmark de Criação e Estatísticas

O processo de carregamento lê os arquivos binários (`.dat`) e constrói as estruturas em memória.

### ⏱️ Resumo da Criação dos Índices

| Índice | Arquivo de Origem | Total de Elementos | Tempo de Criação (s) |
|--------|-------------------|-------------------|---------------------|
| Árvore B+ | Produtos | 7.846 | 0.0050 |
| Tabela Hash | Pedidos | 81.447 | 1.5000 |

### 📊 Árvore B+ (Produtos)

| Métrica | Valor |
|---------|-------|
| Altura | 3 |
| Total de Nós | 3 |
| Total de Chaves | 7.846 |
| Ordem da Árvore | 100 |
| Memória Usada | 0.01 MB |

### 📊 Tabela Hash (Pedidos)

| Métrica | Valor |
|---------|-------|
| Tamanho da Tabela | 50.000 |
| Total de Elementos | 81.447 |
| Total de Colisões | 74.188 |
| Posições Ocupadas | 7.259 (14.52%) |
| Fator de Carga | 1.6289 |
| Memória Usada | 2.87 MB |

---

## 💥 Análise de Colisões (Tabela Hash)

A análise avalia a distribuição das **81.447 entradas** nos **50.000 slots** da Tabela Hash, utilizando **Encadeamento (Chaining)**.

### 📈 Estatísticas de Distribuição

- **Maior Cadeia:** 428 elementos
- **Cadeias Vazias (Tamanho 0):** 42.741 posições
- **Estratégia de Resolução:** Encadeamento (Chaining)

### 📊 Distribuição por Tamanho de Cadeia

| Tamanho da Cadeia | Posições na Tabela |
|-------------------|-------------------|
| 0 | 42.741 |
| 1 | 2.033 |
| 2 | 963 |
| 3 | 606 |
| 4 | 421 |
| 5 | 303 |
| 6 | 276 |
| 7 | 249 |
| 8 | 191 |
| 9 | 167 |
| 10 | 152 |
| > 10 | 1.898 |

---

## 🚀 Benchmarks de Consulta (Comparação Arquivo vs. Memória)

### 🔍 Busca de Produtos (Árvore B+)

| ID Produto | Tempo Arquivo (s) | Tempo Memória (s) | Speedup (Memória/Arquivo) |
|------------|------------------|------------------|---------------------------|
| 4804056980595 | 0.000000 | 0.000000 | -nan(ind)x |
| 4804056986871 | 0.000000 | 0.000000 | -nan(ind)x |
| 4804189181293 | 0.001000 | 0.000000 | inf x |
| 4804195768013 | 0.000000 | 0.000000 | -nan(ind)x |
| 4804197017293 | 0.000000 | 0.000000 | -nan(ind)x |

### 🔍 Busca de Pedidos por Produto (Hash)

| ID Produto | Tempo Arquivo (s)<br/>(Sequencial) | Tempo Memória (s)<br/>(Hash) | Speedup (Memória/Arquivo) |
|------------|-----------------------------------|------------------------------|---------------------------|
| 4804056980595 | 0.009000 | 0.000000 | inf x |
| 4804056986871 | 0.010000 | 0.000000 | inf x |
| 4804189181293 | 0.010000 | 0.000000 | inf x |
| 4804195768013 | 0.009000 | 0.000000 | inf x |
| 4804197017293 | 0.009000 | 0.000000 | inf x |

---

## 💾 Benchmarks de Modificação e Atualização

### ➕ Inserção de Novos Registros (100)

| Operação | Estrutura | Quantidade | Tempo Total (s) | Tempo Médio por Inserção (s) |
|----------|-----------|------------|----------------|------------------------------|
| Inserção | Árvore B+ | 100 | 0.000000 | 0.000000 |
| Inserção | Tabela Hash | 100 | 0.000000 | 0.000000 |

### ➖ Remoção de Registros (50)

| Operação | Estrutura | Quantidade | Tempo Total (s) | Tempo Médio por Remoção (s) |
|----------|-----------|------------|----------------|------------------------------|
| Remoção | Tabela Hash | 50 | 0.000000 | 0.000000 |

> **⚠️ NOTA:** A remoção de chaves da Árvore B+ não foi implementada completamente neste benchmark, sendo apenas simulada ou marcada. Em sistemas reais, a remoção exige redistribuição e fusão de nós para manter a eficiência da estrutura.

---

## 📝 Conclusões

1. **Tempo de Criação:** A Árvore B+ é criada muito mais rapidamente (0.005s) comparada à Tabela Hash (1.5s)
2. **Colisões:** A Tabela Hash apresenta 74.188 colisões (91% dos elementos), indicando alto fator de carga
3. **Consultas:** Ambas estruturas oferecem acesso praticamente instantâneo em memória
4. **Modificações:** Operações de inserção são extremamente rápidas em ambas estruturas