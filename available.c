#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h> // Para isalpha e isspace

// --- Definições de Limites ---
#define MAX_EXPRESSOES 100         // Número máximo de expressões únicas no programa
#define MAX_BLOCOS 20              // Número máximo de blocos básicos
#define MAX_INSTRUCOES_POR_BLOCO 50 // Número máximo de instruções por bloco
#define MAX_TAMANHO_VARIAVEL 10    // Tamanho máximo para o nome de uma variável/operando
#define MAX_TAMANHO_OPERADOR 4     // Tamanho máximo para o operador (+, -, *, /)
#define MAX_TAMANHO_LINHA 100      // Tamanho máximo para uma linha de instrução
#define MAX_PRED_SUC_POR_BLOCO 10  // Número máximo de predecessores/sucessores por bloco

// --- Estruturas de Dados ---

// Representação de uma Expressão (ex: "a+b", "x-y")
typedef struct {
    char operando1[MAX_TAMANHO_VARIAVEL];
    char operador[MAX_TAMANHO_OPERADOR];
    char operando2[MAX_TAMANHO_VARIAVEL];
} Expressao;

// Representação de uma Instrução de Três Enderecos
typedef struct {
    char variavel_resultado[MAX_TAMANHO_VARIAVEL];
    char operando1[MAX_TAMANHO_VARIAVEL];
    char operando2[MAX_TAMANHO_VARIAVEL];
    char operador_char[MAX_TAMANHO_OPERADOR];
    bool eh_operacao_binaria; // Verdadeiro se for 'x = y op z'; Falso se for 'x = y'
} CodigoTresEnderecos;

// Estrutura para um Conjunto de Expressões (usado para ENTRA, SAI, GERA, MATA)
// É um bitset: ids[i]=1 significa que 'pool_expressoes[i]' está no conjunto
typedef struct {
    int ids[MAX_EXPRESSOES]; 
} ConjuntoBitsExpressoes;

// Estrutura de Bloco Basico
typedef struct {
    int id; // ID do bloco (N da entrada)
    CodigoTresEnderecos instrucoes[MAX_INSTRUCOES_POR_BLOCO];
    int num_instrucoes;
    int sucessores[MAX_PRED_SUC_POR_BLOCO];
    int num_sucessores;
    int predecessores[MAX_PRED_SUC_POR_BLOCO]; // Calculado após leitura de todos os blocos
    int num_predecessores;

    ConjuntoBitsExpressoes gera;  // Conjunto GEN
    ConjuntoBitsExpressoes mata;  // Conjunto KILL
    ConjuntoBitsExpressoes entra; // Conjunto IN
    ConjuntoBitsExpressoes sai;   // Conjunto OUT
} BlocoBasico;

// --- Variáveis Globais ---
Expressao pool_expressoes[MAX_EXPRESSOES]; // Pool de todas as expressões únicas do programa
int contador_pool_expressoes = 0;          // Contador de expressões no pool

BlocoBasico blocos[MAX_BLOCOS];
int num_blocos_reais = 0; // Número real de blocos lidos e armazenados

// --- Protótipos das Funções ---
// Funções auxiliares de conjunto
bool sao_expressoes_iguais(const Expressao *e1, const Expressao *e2);
int adicionar_expressao_ao_pool(const char *op1, const char *oper, const char *op2);
void inicializar_conjunto_bits(ConjuntoBitsExpressoes *conjunto);
void copiar_conjunto_bits(ConjuntoBitsExpressoes *destino, const ConjuntoBitsExpressoes *origem);
bool sao_conjuntos_iguais(const ConjuntoBitsExpressoes *c1, const ConjuntoBitsExpressoes *c2);
void uniao_conjuntos(ConjuntoBitsExpressoes *resultado, const ConjuntoBitsExpressoes *c1, const ConjuntoBitsExpressoes *c2);
void intersecao_conjuntos(ConjuntoBitsExpressoes *resultado, const ConjuntoBitsExpressoes *c1, const ConjuntoBitsExpressoes *c2);
void diferenca_conjuntos(ConjuntoBitsExpressoes *resultado, const ConjuntoBitsExpressoes *c1, const ConjuntoBitsExpressoes *c2);
void imprimir_conjunto_expressoes(const char *rotulo, const ConjuntoBitsExpressoes *conjunto);

// Funções para parseamento e cálculo do fluxo de dados
bool parsear_codigo_tres_enderecos(const char *linha, CodigoTresEnderecos *cte);
void popular_pool_global_expressoes();
void calcular_gera_mata_para_todos_blocos();
void ler_grafo_fluxo_controle();
void analise_expressoes_disponiveis();


// --- Implementação das Funções Auxiliares de Expressões e Conjuntos ---

bool sao_expressoes_iguais(const Expressao *e1, const Expressao *e2) {
    return (strcmp(e1->operando1, e2->operando1) == 0 &&
            strcmp(e1->operador, e2->operador) == 0 &&
            strcmp(e1->operando2, e2->operando2) == 0);
}

int adicionar_expressao_ao_pool(const char *op1, const char *oper, const char *op2) {
    Expressao nova_expressao;
    strcpy(nova_expressao.operando1, op1);
    strcpy(nova_expressao.operador, oper);
    strcpy(nova_expressao.operando2, op2);

    for (int i = 0; i < contador_pool_expressoes; i++) {
        if (sao_expressoes_iguais(&pool_expressoes[i], &nova_expressao)) {
            return i; // Expressão já existe no pool
        }
    }
    if (contador_pool_expressoes >= MAX_EXPRESSOES) {
        fprintf(stderr, "Erro: Limite de expressões únicas atingido no pool global (%d).\n", MAX_EXPRESSOES);
        exit(EXIT_FAILURE);
    }
    pool_expressoes[contador_pool_expressoes] = nova_expressao;
    return contador_pool_expressoes++;
}

void inicializar_conjunto_bits(ConjuntoBitsExpressoes *conjunto) {
    for (int i = 0; i < MAX_EXPRESSOES; i++) {
        conjunto->ids[i] = 0;
    }
}

void copiar_conjunto_bits(ConjuntoBitsExpressoes *destino, const ConjuntoBitsExpressoes *origem) {
    for (int i = 0; i < MAX_EXPRESSOES; i++) {
        destino->ids[i] = origem->ids[i];
    }
}

bool sao_conjuntos_iguais(const ConjuntoBitsExpressoes *c1, const ConjuntoBitsExpressoes *c2) {
    for (int i = 0; i < contador_pool_expressoes; i++) {
        if (c1->ids[i] != c2->ids[i]) {
            return false;
        }
    }
    return true;
}

void uniao_conjuntos(ConjuntoBitsExpressoes *resultado, const ConjuntoBitsExpressoes *c1, const ConjuntoBitsExpressoes *c2) {
    inicializar_conjunto_bits(resultado); // Garante que o resultado esteja limpo antes da união
    for (int i = 0; i < contador_pool_expressoes; i++) {
        resultado->ids[i] = c1->ids[i] | c2->ids[i];
    }
}

void intersecao_conjuntos(ConjuntoBitsExpressoes *resultado, const ConjuntoBitsExpressoes *c1, const ConjuntoBitsExpressoes *c2) {
    inicializar_conjunto_bits(resultado); // Garante que o resultado esteja limpo antes da interseção
    for (int i = 0; i < contador_pool_expressoes; i++) {
        resultado->ids[i] = c1->ids[i] & c2->ids[i];
    }
}

void diferenca_conjuntos(ConjuntoBitsExpressoes *resultado, const ConjuntoBitsExpressoes *c1, const ConjuntoBitsExpressoes *c2) {
    inicializar_conjunto_bits(resultado); // Garante que o resultado esteja limpo antes da diferença
    for (int i = 0; i < contador_pool_expressoes; i++) {
        resultado->ids[i] = c1->ids[i] & (~c2->ids[i]); // (elemento em c1) E (elemento NÃO em c2)
    }
}

void imprimir_conjunto_expressoes(const char *rotulo, const ConjuntoBitsExpressoes *conjunto) {
    printf("%s: {", rotulo);
    bool primeiro = true; 
    for (int i = 0; i < contador_pool_expressoes; i++) {
        if (conjunto->ids[i]) {
            if (!primeiro) printf(", ");
            printf("%s%s%s", pool_expressoes[i].operando1, pool_expressoes[i].operador, pool_expressoes[i].operando2);
            primeiro = false;
        }
    }
    printf("}\n");
}

// --- Funções para Parseamento de Instruções ---

bool parsear_codigo_tres_enderecos(const char *linha, CodigoTresEnderecos *cte) {
    char linha_temp[MAX_TAMANHO_LINHA];
    strcpy(linha_temp, linha);

    // Inicializa
    cte->variavel_resultado[0] = '\0';
    cte->operando1[0] = '\0';
    cte->operando2[0] = '\0';
    cte->operador_char[0] = '\0';
    cte->eh_operacao_binaria = false;

    char *ptr_igual = strchr(linha_temp, '=');
    if (!ptr_igual) return false; // Não é uma instrução de atribuição (ex: "goto L1", comentário)

    *ptr_igual = '\0'; // Separa LHS (lado esquerdo) de RHS (lado direito)
    // Remove espaços em branco do início/fim do LHS
    char *lhs_start = linha_temp;
    while(isspace((unsigned char)*lhs_start)) lhs_start++;
    char *lhs_end = lhs_start + strlen(lhs_start) - 1;
    while(lhs_end > lhs_start && isspace((unsigned char)*lhs_end)) lhs_end--;
    *(lhs_end + 1) = '\0';
    strcpy(cte->variavel_resultado, lhs_start); // LHS

    char *ptr_rhs = ptr_igual + 1; // RHS
    // Remove espaços em branco do início do RHS
    while(isspace((unsigned char)*ptr_rhs)) ptr_rhs++;

    char *op_ptr = NULL;
    char operadores_validos[] = "+-*/";
    for(int i = 0; i < strlen(operadores_validos); i++) {
        op_ptr = strchr(ptr_rhs, operadores_validos[i]);
        if (op_ptr) {
            *op_ptr = '\0'; // Separa operando1 de operando2
            // Remove espaços do operando1
            char *op1_start = ptr_rhs;
            while(isspace((unsigned char)*op1_start)) op1_start++;
            char *op1_end = op1_start + strlen(op1_start) - 1;
            while(op1_end > op1_start && isspace((unsigned char)*op1_end)) op1_end--;
            *(op1_end + 1) = '\0';
            strcpy(cte->operando1, op1_start);

            // Remove espaços do operando2
            char *op2_start = op_ptr + 1;
            while(isspace((unsigned char)*op2_start)) op2_start++;
            strcpy(cte->operando2, op2_start);
            
            strcpy(cte->operador_char, (char[]){operadores_validos[i], '\0'});
            cte->eh_operacao_binaria = true;
            return true;
        }
    }

    // Se não for operação binária, é atribuição simples (x = y)
    // Remove espaços do operando1 (que é o 'y')
    char *op1_start = ptr_rhs;
    while(isspace((unsigned char)*op1_start)) op1_start++;
    strcpy(cte->operando1, op1_start);
    cte->eh_operacao_binaria = false;
    return true;
}

// --- Funções de Cálculo GERA e MATA ---

void popular_pool_global_expressoes() {
    contador_pool_expressoes = 0; // Garante que o pool seja preenchido do zero
    for (int i = 0; i < num_blocos_reais; i++) {
        for (int j = 0; j < blocos[i].num_instrucoes; j++) {
            CodigoTresEnderecos *instr = &blocos[i].instrucoes[j];
            if (instr->eh_operacao_binaria) {
                adicionar_expressao_ao_pool(instr->operando1, instr->operador_char, instr->operando2);
            }
        }
    }
}

void calcular_gera_mata_para_todos_blocos() {
    for (int i = 0; i < num_blocos_reais; i++) {
        inicializar_conjunto_bits(&blocos[i].gera);
        inicializar_conjunto_bits(&blocos[i].mata);

        // --- Calcular GERA(B): Expressões geradas no bloco e não mortas dentro dele ---
        // Variaveis que foram redefinidas 'abaixo' da instrução atual, dentro do mesmo bloco
        // Um array de booleanos, indexado pelo valor ASCII do caractere da variável (ex: 'a' -> 97)
        bool variaveis_redefinidas_no_bloco[256]; 
        memset(variaveis_redefinidas_no_bloco, 0, sizeof(variaveis_redefinidas_no_bloco));

        // Percorrer as instruções de trás para frente
        for (int j = blocos[i].num_instrucoes - 1; j >= 0; j--) {
            CodigoTresEnderecos *instr = &blocos[i].instrucoes[j];
            
            if (instr->eh_operacao_binaria) {
                bool op1_redefinido = false;
                if (strlen(instr->operando1) == 1 && isalpha(instr->operando1[0])) {
                    op1_redefinido = variaveis_redefinidas_no_bloco[(unsigned char)instr->operando1[0]];
                }
                bool op2_redefinido = false;
                if (strlen(instr->operando2) == 1 && isalpha(instr->operando2[0])) {
                    op2_redefinido = variaveis_redefinidas_no_bloco[(unsigned char)instr->operando2[0]];
                }
                
                if (!op1_redefinido && !op2_redefinido) {
                    int id_expressao = adicionar_expressao_ao_pool(instr->operando1, instr->operador_char, instr->operando2);
                    blocos[i].gera.ids[id_expressao] = 1;
                }
            }
            
            // Marcar a variável do resultado como redefinida para afetar instruções anteriores
            if (strlen(instr->variavel_resultado) == 1 && isalpha(instr->variavel_resultado[0])) {
                variaveis_redefinidas_no_bloco[(unsigned char)instr->variavel_resultado[0]] = true;
            }
        }

        // --- Calcular MATA(B): Expressões mortas pelas redefinições no bloco ---
        // Iteramos sobre todas as instruções do bloco, e para cada variável redefinida
        // marcamos as expressões do pool global que contêm essa variável.
        for (int j = 0; j < blocos[i].num_instrucoes; j++) {
            CodigoTresEnderecos *instr = &blocos[i].instrucoes[j];
            char variavel_redefinida_char = '\0';
            if (strlen(instr->variavel_resultado) == 1 && isalpha(instr->variavel_resultado[0])) {
                variavel_redefinida_char = instr->variavel_resultado[0];
            }

            if (variavel_redefinida_char != '\0') {
                for (int e = 0; e < contador_pool_expressoes; e++) {
                    // Expressão é morta se a variável redefinida é um de seus operandos
                    if ((strlen(pool_expressoes[e].operando1) == 1 && pool_expressoes[e].operando1[0] == variavel_redefinida_char) ||
                        (strlen(pool_expressoes[e].operando2) == 1 && pool_expressoes[e].operando2[0] == variavel_redefinida_char)) {
                        blocos[i].mata.ids[e] = 1;
                    }
                }
            }
        }
    }
}

// --- Leitura da Entrada no Formato da Questão ---
void ler_grafo_fluxo_controle() {
    int N_bloco_id, M_num_instrucoes;
    char buffer_linha[MAX_TAMANHO_LINHA];

    // Loop para ler os blocos até o EOF (End Of File)
    while (scanf("%d %d", &N_bloco_id, &M_num_instrucoes) == 2) {
        int indice_bloco_array = N_bloco_id; // Assumimos que ID do bloco é o índice do array

        if (indice_bloco_array >= MAX_BLOCOS) {
            fprintf(stderr, "Erro: ID de bloco %d excede MAX_BLOCOS (%d). Ajuste MAX_BLOCOS.\n", N_bloco_id, MAX_BLOCOS);
            exit(EXIT_FAILURE);
        }

        blocos[indice_bloco_array].id = N_bloco_id;
        blocos[indice_bloco_array].num_instrucoes = 0;
        blocos[indice_bloco_array].num_sucessores = 0;
        blocos[indice_bloco_array].num_predecessores = 0; 

        getchar(); // Consome o '\n' restante da linha do scanf

        for (int i = 0; i < M_num_instrucoes; i++) {
            if (fgets(buffer_linha, sizeof(buffer_linha), stdin) == NULL) {
                fprintf(stderr, "Erro: EOF inesperado ao ler instrução para o bloco %d.\n", N_bloco_id);
                exit(EXIT_FAILURE);
            }
            buffer_linha[strcspn(buffer_linha, "\n")] = '\0'; // Remove o '\n'

            if (blocos[indice_bloco_array].num_instrucoes >= MAX_INSTRUCOES_POR_BLOCO) {
                fprintf(stderr, "Erro: Limite de instruções (%d) para o bloco %d atingido.\n", MAX_INSTRUCOES_POR_BLOCO, N_bloco_id);
                exit(EXIT_FAILURE);
            }
            // Tenta parsear a linha como instrução; pode falhar para comentários/linhas vazias
            if (!parsear_codigo_tres_enderecos(buffer_linha, &blocos[indice_bloco_array].instrucoes[blocos[indice_bloco_array].num_instrucoes])) {
                fprintf(stderr, "Aviso: Impossível parsear instrução '%s' no bloco %d. (Pode ser um comentário ou linha vazia)\n", buffer_linha, N_bloco_id);
            }
            blocos[indice_bloco_array].num_instrucoes++;
        }

        if (fgets(buffer_linha, sizeof(buffer_linha), stdin) == NULL) {
            fprintf(stderr, "Erro: EOF inesperado ao ler sucessores para o bloco %d.\n", N_bloco_id);
            exit(EXIT_FAILURE);
        }
        buffer_linha[strcspn(buffer_linha, "\n")] = '\0';

        char *token = strtok(buffer_linha, " ");
        while (token != NULL) {
            int id_sucessor = atoi(token);
            if (id_sucessor == 0 && blocos[indice_bloco_array].num_sucessores == 0) { // '0' indica fim da lista de sucessores
                break;
            }
            if (blocos[indice_bloco_array].num_sucessores >= MAX_PRED_SUC_POR_BLOCO) {
                fprintf(stderr, "Erro: Limite de sucessores (%d) para o bloco %d atingido.\n", MAX_PRED_SUC_POR_BLOCO, N_bloco_id);
                exit(EXIT_FAILURE);
            }
            blocos[indice_bloco_array].sucessores[blocos[indice_bloco_array].num_sucessores++] = id_sucessor;
            token = strtok(NULL, " ");
        }
        
        // num_blocos_reais é o maior ID de bloco encontrado + 1, para iterar corretamente
        num_blocos_reais = (indice_bloco_array >= num_blocos_reais) ? (indice_bloco_array + 1) : num_blocos_reais;
    }

    // --- Segunda Passagem: Preencher os predecessores de todos os blocos ---
    for (int i = 0; i < num_blocos_reais; i++) {
        for (int j = 0; j < blocos[i].num_sucessores; j++) {
            int id_sucessor = blocos[i].sucessores[j];
            
            // Encontrar o índice do bloco sucessor no array 'blocos' pelo seu ID
            int indice_sucessor_array = -1;
            for(int k=0; k < num_blocos_reais; k++) {
                if(blocos[k].id == id_sucessor) {
                    indice_sucessor_array = k;
                    break;
                }
            }

            if (indice_sucessor_array != -1) {
                if (blocos[indice_sucessor_array].num_predecessores >= MAX_PRED_SUC_POR_BLOCO) {
                     fprintf(stderr, "Erro: Limite de predecessores (%d) para o bloco %d atingido.\n", MAX_PRED_SUC_POR_BLOCO, id_sucessor);
                     exit(EXIT_FAILURE);
                }
                blocos[indice_sucessor_array].predecessores[blocos[indice_sucessor_array].num_predecessores++] = blocos[i].id;
            } else {
                fprintf(stderr, "Aviso: Sucessor %d do bloco %d não encontrado nos blocos lidos. Pode ser um bloco de saída válido.\n", id_sucessor, blocos[i].id);
            }
        }
    }
}

// --- Algoritmo de Expressões Disponíveis ---
void analise_expressoes_disponiveis() {
    // 1. Inicialização
    // O bloco de entrada (geralmente ID 0) tem ENTRA vazio. 
    // Os outros ENTRAs são o superconjunto de todas as expressões do programa.
    // O SAI de todos os blocos é inicialmente o seu GERA.

    ConjuntoBitsExpressoes superconjunto_expressoes;
    inicializar_conjunto_bits(&superconjunto_expressoes);

    // Preenche o superconjunto com todas as IDs de expressões do pool global
    for(int i = 0; i < contador_pool_expressoes; i++) {
        superconjunto_expressoes.ids[i] = 1; 
    }

    // Encontra o índice do bloco inicial (por ID)
    int id_bloco_inicial = 0; // Por convenção, o bloco com ID 0 é o inicial
    int indice_bloco_inicial_array = -1;
    for (int i = 0; i < num_blocos_reais; i++) {
        if (blocos[i].id == id_bloco_inicial) {
            indice_bloco_inicial_array = i;
            break;
        }
    }
    
    // Inicializa ENTRA e SAI para todos os blocos
    for (int i = 0; i < num_blocos_reais; i++) {
        if (blocos[i].id == id_bloco_inicial) { // Bloco inicial
            inicializar_conjunto_bits(&blocos[i].entra); // ENTRA(entrada) = vazio
        } else {
            copiar_conjunto_bits(&blocos[i].entra, &superconjunto_expressoes); // ENTRA(outros) = superconjunto
        }
        copiar_conjunto_bits(&blocos[i].sai, &blocos[i].gera); // SAI(B) = GERA(B) inicialmente
    }

    // 2. Iteração de Ponto Fixo (Loop principal do algoritmo)
    bool houve_mudanca;
    do {
        houve_mudanca = false; // Assume que não haverá mudanças nesta iteração
        for (int i = 0; i < num_blocos_reais; i++) {
            ConjuntoBitsExpressoes entra_antigo;
            copiar_conjunto_bits(&entra_antigo, &blocos[i].entra); // Salva o ENTRA antigo

            ConjuntoBitsExpressoes sai_antigo;
            copiar_conjunto_bits(&sai_antigo, &blocos[i].sai); // Salva o SAI antigo

            ConjuntoBitsExpressoes nova_entrada_candidata;
            inicializar_conjunto_bits(&nova_entrada_candidata);

            if (blocos[i].id == id_bloco_inicial) {
                // O ENTRA do bloco inicial nunca muda e permanece vazio.
                // (nova_entrada_candidata já é vazio devido à inicialização)
            } else if (blocos[i].num_predecessores > 0) {
                // Calcula ENTRA(B) = Interseção dos SAIs de todos os predecessores de B
                int indice_primeiro_pred_array = -1;
                // Encontra o índice do primeiro predecessor no array 'blocos'
                for(int k=0; k < num_blocos_reais; k++) {
                    if(blocos[k].id == blocos[i].predecessores[0]) {
                        indice_primeiro_pred_array = k;
                        break;
                    }
                }
                
                if(indice_primeiro_pred_array != -1) {
                    // Inicia a nova entrada candidata com o SAI do primeiro predecessor
                    copiar_conjunto_bits(&nova_entrada_candidata, &blocos[indice_primeiro_pred_array].sai);
                    
                    // Intersecta com os SAIs dos demais predecessores
                    for (int j = 1; j < blocos[i].num_predecessores; j++) {
                        int indice_pred_atual_array = -1;
                        for(int k=0; k < num_blocos_reais; k++) {
                            if(blocos[k].id == blocos[i].predecessores[j]) {
                                indice_pred_atual_array = k;
                                break;
                            }
                        }
                        if(indice_pred_atual_array != -1) {
                            ConjuntoBitsExpressoes intersecao_temp;
                            intersecao_conjuntos(&intersecao_temp, &nova_entrada_candidata, &blocos[indice_pred_atual_array].sai);
                            copiar_conjunto_bits(&nova_entrada_candidata, &intersecao_temp);
                        }
                    }
                } else {
                    // Se o primeiro predecessor não foi encontrado (problema no grafo, bloco inacessível, etc.),
                    // o ENTRA para um bloco não-inicial sem predecessores válidos permanece o superconjunto.
                     copiar_conjunto_bits(&nova_entrada_candidata, &superconjunto_expressoes);
                }

            } else { // Se não é o bloco inicial (ID != id_bloco_inicial) e não tem predecessores (inacessível)
                copiar_conjunto_bits(&nova_entrada_candidata, &superconjunto_expressoes); // Permanece o superconjunto inicial
            }
            copiar_conjunto_bits(&blocos[i].entra, &nova_entrada_candidata);

            // Calcula novo SAI: SAI(B) = GERA(B) U (ENTRA(B) - MATA(B))
            ConjuntoBitsExpressoes entra_menos_mata;
            diferenca_conjuntos(&entra_menos_mata, &blocos[i].entra, &blocos[i].mata);
            uniao_conjuntos(&blocos[i].sai, &blocos[i].gera, &entra_menos_mata);

            // Verifica se houve mudança em ENTRA ou SAI para continuar a iteração
            if (!sao_conjuntos_iguais(&entra_antigo, &blocos[i].entra) || !sao_conjuntos_iguais(&sai_antigo, &blocos[i].sai)) {
                houve_mudanca = true;
            }
        }
    } while (houve_mudanca);
}

// --- Função Principal ---
int main() {
    // 1. Ler o grafo de fluxo de controle do input (stdin)
    ler_grafo_fluxo_controle();

    // 2. Preencher o pool de todas as expressões únicas do programa e calcular GERA/MATA para cada bloco
    popular_pool_global_expressoes();
    calcular_gera_mata_para_todos_blocos(); // Agora sem os prints de DEBUG

    // 3. Executar a análise de Expressões Disponíveis (cálculo de ENTRA e SAI)
    analise_expressoes_disponiveis();

    // 4. Imprimir os resultados para cada bloco
    printf("\n--- Resultados da Análise de Expressões Disponíveis ---\n");
    for (int i = 0; i < num_blocos_reais; i++) {
        printf("\nBloco %d:\n", blocos[i].id); 
        imprimir_conjunto_expressoes("  GERA", &blocos[i].gera);
        imprimir_conjunto_expressoes("  MATA", &blocos[i].mata);
        imprimir_conjunto_expressoes("  ENTRA", &blocos[i].entra);
        imprimir_conjunto_expressoes("  SAI", &blocos[i].sai);
    }

    return 0;
}
