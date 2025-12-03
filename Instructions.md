# Algoritmos e Estruturas de Dados II
## Trabalho II: índices para os arquivos de dados
### Objetivo: 
Criação de índices para a **organização sequencial-indexado**, desenvolvida no
Trabalho I

### Organização: 
em duplas (ou individual) – mesma organização do Trabalho I.
### Contexto do trabalho:
O contexto dos dados é: **compras em uma loja online de joias de médio porte, com dados de dezembro de 2018 a dezembro de 2021 (3 anos).**
No primeiro trabalho, foram construídos **dois arquivos de dados** (produtos – joias, e compras), com registros de tamanho fixo. Também foram criados arquivos de índices: **arquivo de índice parcial** para o índice com base na chave de ordenação do arquivo de dados, e **arquivo de índice exaustivo** para o índice com base em uma coluna que não era chave de ordenação. Também foram implementadas operações de inserção e remoção de dados, e de reorganização de arquivos de dados.

  **Atividades a realizar: implementar índices em memória para os arquivos de dados:**

1) Implemente uma estrutura de índice em memória utilizando **árvore B ou árvore B+** para o arquivo de produtos. Esse índice **deve ser sobre a chave de ordenação do arquivo**, e a ordem da árvore (valor que define a quantidade de elementos por nodo) deve ser estabelecida a partir do
tamanho do bloco/página. Lembre-se que cada “elemento” deve conter a chave de acesso e o endereço do registro no arquivo no caso de uma árvore B. Para uma árvore B+, os endereços de acesso aos registros estão no nível das folhas.
2) Implemente uma estrutura de índice em memória utilizando **tabela hash** para uma coluna do arquivo de compras **que não seja a coluna da chave de ordenação**, a mesma coluna que foi utilizada para a construção do arquivo de índice exaustivo do primeiro trabalho. Defina e implemente uma estratégia para a resolução de colisões.
3) Para cada índice criado:
    1. Anote o tempo que durou a criação de cada um dos índices em memória.
    2. Faça consultas (4 ou 5) utilizando primeiro os índices de arquivo, e depois as mesmas consultas utilizando os índices de memória (depois de criados). Anote os tempos em uma tabela comparativa.
    3. Faça testes de inclusão ou remoção de novos registros com recriação ou modificação dos índices, e anote os resultados (tempo que demorou para recriar/modificar).
A criação dos índices em memória e algumas consultas serão também apresentados para a professora, mas esses resultados podem ser anotados anteriormente em seus experimentos e serão entregues junto com o código fonte do programa de criação dos índices.
### Postar no AVA:
+ Descrição dos índices de memória: qual árvore, quantidade de elementos por nodo (ou grau da árvore), qual estratégia de resolução de colisões.
+ Tabelas com resultados dos tempos de criação/recriação/atualização dos índices, tempos comparativos de consultas com os índices de arquivo
+ Código de implementação dos índice