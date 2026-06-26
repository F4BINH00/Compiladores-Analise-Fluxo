#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_BLOCOS 100
#define MAX_LINHAS 1000 
#define MAX_VARS 100   
#define MAX_DEFS MAX_LINHAS //Cada instrução de atribuição é uma definição única

typedef struct {
    int id;
    char var_def[20]; //Variável definida (lado esquerdo)
    char op1[20];
    char op[5];
    char op2[20];
    bool op_binaria; //Indicar se é uma operação binária (a = b + c) ou simples (a = b)
} Instrucao;

typedef struct {
    int id;
    int inicio_instr, fim_instr;
    int sucessores[10];
    int n_sucessores;

    int gen[MAX_DEFS];  
    int kill[MAX_DEFS]; 
    int in[MAX_DEFS];   
    int out[MAX_DEFS];  
} BlocoBasico;

int n_blocos = 0;
int n_instr = 0;
Instrucao instrucoes[MAX_LINHAS];
BlocoBasico blocos[MAX_BLOCOS];

int def_count = 0;          
Instrucao def_list[MAX_DEFS]; 

int instr_to_def_id[MAX_LINHAS];


// Funções utilitárias de conjuntos (bit vetorial)
void init_conjunto(int *conj, int tam, bool val) {
    for (int i = 0; i < tam; i++)
        conj[i] = val ? 1 : 0;
}

void uniao(int *dest, int *a, int *b, int tam) {
    for (int i = 0; i < tam; i++)
        dest[i] = a[i] || b[i];
}

bool comparar_conjunto(int *a, int *b, int tam) {
    for (int i = 0; i < tam; i++)
        if (a[i] != b[i]) return false;
    return true;
}

void copiar_conjunto(int *dest, int *src, int tam) {
    for (int i = 0; i < tam; i++)
        dest[i] = src[i];
}

void subtrair(int *dest, int *a, int *b, int tam) {
    for (int i = 0; i < tam; i++)
        dest[i] = a[i] && !b[i];
}

void imprimir_conjunto(const char *rotulo, int *conj, int tam) {
    printf("%s: {", rotulo);
    bool primeiro = true;
    for (int i = 0; i < tam; i++) {
        if (conj[i]) {
            if (!primeiro) printf(", ");
            printf("d%d", i); 
            primeiro = false;
        }
    }
    printf("}\n");
}
// Coleta todas as definições globais e as mapeia
void coletar_todas_defs() {
    def_count = 0;
    for (int i = 0; i < n_instr; i++) {
        def_list[def_count] = instrucoes[i]; 
        def_list[def_count].id = def_count; 
        instr_to_def_id[i] = def_count;     
        def_count++;
    }
}

// Processa instruções e define GEN/KILL
void calcular_gen_kill() {
    for (int b = 0; b < n_blocos; b++) {
        BlocoBasico *bloco = &blocos[b];
        init_conjunto(bloco->gen, def_count, false);
        init_conjunto(bloco->kill, def_count, false);

        for (int i = bloco->inicio_instr; i <= bloco->fim_instr; i++) {
            int current_def_id = instr_to_def_id[i];
            char *def_var_name = instrucoes[i].var_def;

            bloco->gen[current_def_id] = 1;

            for (int j = i + 1; j <= bloco->fim_instr; j++) {
                if (strcmp(instrucoes[j].var_def, def_var_name) == 0) {
                    bloco->gen[current_def_id] = 0;
                    break;
                }
            }
        }

        for (int k = 0; k < def_count; k++) { 
            char *global_def_var_name = def_list[k].var_def;

            bool var_is_defined_in_block = false;
            for (int i = bloco->inicio_instr; i <= bloco->fim_instr; i++) {
                if (strcmp(instrucoes[i].var_def, global_def_var_name) == 0) {
                    var_is_defined_in_block = true;
                    break;
                }
            }

            if (var_is_defined_in_block && bloco->gen[k] == 0) {
                bloco->kill[k] = 1;
            }
        }
    }
}


// Algoritmo de fix-point para Reaching Definitions (Definições Alcançantes)
void reaching_definitions() {
    bool mudou = true;

    // Inicialização: Todos os IN e OUT são vazios, exceto o IN do bloco inicial.
    for (int b = 0; b < n_blocos; b++) {
        init_conjunto(blocos[b].in, def_count, false);
        init_conjunto(blocos[b].out, def_count, false);
    }

    while (mudou) {
        mudou = false;
        for (int b = 0; b < n_blocos; b++) {
            BlocoBasico *bloco = &blocos[b];
            int temp_in[MAX_DEFS];

            // Calcula IN[b] = união dos OUT[p] para todos predecessores p
            init_conjunto(temp_in, def_count, false); // Começa vazio para a união
            if (bloco->id == 0) {
                // IN do bloco de entrada permanece vazio
                init_conjunto(temp_in, def_count, false);
            } else {
                bool tem_pred_real = false; // Flag para verificar se o bloco tem predecessores
                for (int p_idx = 0; p_idx < n_blocos; p_idx++) {
                    for (int s_idx = 0; s_idx < blocos[p_idx].n_sucessores; s_idx++) {
                        if (blocos[p_idx].sucessores[s_idx] == bloco->id) {
                            uniao(temp_in, temp_in, blocos[p_idx].out, def_count);
                            tem_pred_real = true;
                        }
                    }
                }
                if (!tem_pred_real) {
                    init_conjunto(temp_in, def_count, false);
                }
            }


            if (!comparar_conjunto(temp_in, bloco->in, def_count)) {
                copiar_conjunto(bloco->in, temp_in, def_count);
                mudou = true;
            }

            // OUT[b] = GEN[b] U (IN[b] - KILL[b])
            int diff[MAX_DEFS];
            subtrair(diff, bloco->in, bloco->kill, def_count);
            int out_tmp[MAX_DEFS];
            uniao(out_tmp, bloco->gen, diff, def_count);

            if (!comparar_conjunto(out_tmp, bloco->out, def_count)) {
                copiar_conjunto(bloco->out, out_tmp, def_count);
                mudou = true;
            }
        }
    }
}

// Leitura da entrada
void ler_entrada() {
    char linha[256];

    // Lê N e M
    if (fgets(linha, sizeof(linha), stdin) == NULL) {
        fprintf(stderr, "Erro leitura N e M.\n");
        exit(EXIT_FAILURE);
    }
    // Ajuste para ler "dados: N M" ou apenas "N M"
    if (sscanf(linha, "dados: %d %d", &n_blocos, &n_instr) != 2) {
        // Se não for "dados: N M", tenta ler apenas "N M"
        if (sscanf(linha, "%d %d", &n_blocos, &n_instr) != 2) {
            fprintf(stderr, "Formato inválido N e M.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Ignora a linha em branco após "dados: N M" se existir
    char next_char = fgetc(stdin);
    if (next_char != '\n' && next_char != EOF) {
        ungetc(next_char, stdin); // Volta o caractere para o stream se não for '\n'
    }


    // Lê instruções
    for (int i = 0; i < n_instr; i++) {
        if (fgets(linha, sizeof(linha), stdin) == NULL) {
            fprintf(stderr, "Erro leitura instrução %d.\n", i);
            exit(EXIT_FAILURE);
        }
        linha[strcspn(linha, "\n")] = 0; // Remove o '\n'

        char lhs[20], rhs1[20], op[5], rhs2[20];
        
        // Tenta ler com operador binário
        if (sscanf(linha, "%19s = %19s %4s %19s", lhs, rhs1, op, rhs2) == 4) {
            strcpy(instrucoes[i].var_def, lhs);
            strcpy(instrucoes[i].op1, rhs1);
            strcpy(instrucoes[i].op, op);
            strcpy(instrucoes[i].op2, rhs2);
            instrucoes[i].op_binaria = true;
        } 
        // Tenta ler com atribuição simples
        else if (sscanf(linha, "%19s = %19s", lhs, rhs1) == 2) {
            strcpy(instrucoes[i].var_def, lhs);
            strcpy(instrucoes[i].op1, rhs1);
            strcpy(instrucoes[i].op, ""); // Operador vazio para atribuição simples
            strcpy(instrucoes[i].op2, ""); // Operando 2 vazio
            instrucoes[i].op_binaria = false;
        } else {
            fprintf(stderr, "Instrução inválida: '%s'\n", linha); // Imprime a linha problemática
            exit(EXIT_FAILURE);
        }
        instrucoes[i].id = i;
    }

    // Divide instruções por bloco (distribuição simples igual)
    int instr_por_bloco = n_instr / n_blocos;
    for (int i = 0; i < n_blocos; i++) {
        blocos[i].id = i;
        blocos[i].inicio_instr = i * instr_por_bloco;
        blocos[i].fim_instr = (i == n_blocos - 1) ? n_instr - 1 : (i + 1) * instr_por_bloco - 1;
        blocos[i].n_sucessores = 0;
    }

    // Ignora a linha em branco antes dos sucessores (se existir)
    next_char = fgetc(stdin);
    if (next_char != '\n' && next_char != EOF) {
        ungetc(next_char, stdin);
    }
    
    // Lê sucessores
    for (int i = 0; i < n_blocos; i++) {
        char succ_line[100]; // Buffer para ler a linha de sucessores
        if (fgets(succ_line, sizeof(succ_line), stdin) == NULL) {
            // Este erro é comum se o último bloco não tem sucessores e a entrada termina.
            // Apenas printa um aviso se não for o último bloco.
            if (i < n_blocos - 1) { 
                fprintf(stderr, "Erro leitura sucessores bloco %d (fgets retornou NULL).\n", i);
                exit(EXIT_FAILURE); // Saia apenas se for um erro crítico.
            }
        }
        succ_line[strcspn(succ_line, "\n")] = 0; // Remove '\n'

        char *token = strtok(succ_line, " ");
        while (token != NULL) {
            int x = atoi(token);
            if (x == 0) break; // '0' indica o fim da lista de sucessores
            if (blocos[i].n_sucessores < 10) { // Limite de 10 sucessores
                blocos[i].sucessores[blocos[i].n_sucessores++] = x;
            } else {
                fprintf(stderr, "Aviso: Mais de 10 sucessores para o bloco %d, ignorando o resto.\n", i);
                break; // Parar de ler sucessores para evitar buffer overflow
            }
            token = strtok(NULL, " ");
        }
    }
}

int main() {
    ler_entrada();
    coletar_todas_defs(); // Agora coleta definições, não expressões
    calcular_gen_kill(); // Lógica de GEN/KILL para Reaching Definitions
    reaching_definitions(); // O algoritmo de fix-point

    for (int i = 0; i < n_blocos; i++) {
        printf("Bloco %d:\n", i);
        imprimir_conjunto("  IN", blocos[i].in, def_count); // Usa def_count
        imprimir_conjunto("  OUT", blocos[i].out, def_count); // Usa def_count
    }

    return 0;
}
