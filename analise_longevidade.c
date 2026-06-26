#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_BLOCOS 50
#define MAX_VARS 26 //variáveis a até z
#define MAX_INT_BLOCO 20
#define MAX_SUCESSORES 10
#define MAX_LINHA 256

typedef struct{
    int id;
    char instrucoes[MAX_INT_BLOCO][MAX_LINHA];
    int qtd_instrucoes;
    
    int sucessores[MAX_SUCESSORES];
    int qtd_sucessores;
    int antes[MAX_BLOCOS];
    int qtd_antes; //predecessores

    bool use[MAX_VARS];
    bool def[MAX_VARS];
    bool in[MAX_VARS];
    bool out[MAX_VARS];
} Bloco;


Bloco* blocos[MAX_BLOCOS + 1]={NULL}; 
int maior_id_bloco=0;

int var_indice(char c){
    if(c >= 'a' && c<='z'){
        return c-'a';
    }
    return -1;
}

char indice_var(int i){
    return 'a' + i;
}

void uniao_bool(bool* A, const bool* B){
    for(int i = 0; i < MAX_VARS; i++){
        if(B[i]){
            A[i]=true;
        }
    }
}

void diferenca_bool(bool* A, const bool* B){
    for(int i = 0; i < MAX_VARS; i++){
        if(B[i]){
            A[i]= false;
        }
    }
}

void parse_input(){
    char linha[MAX_LINHA];
    int id, m;

    while(scanf("%d %d\n", &id, &m) == 2){
        if(id > maior_id_bloco) maior_id_bloco = id;
        
        
        blocos[id] = (Bloco*) calloc(1, sizeof(Bloco));
        blocos[id]->id = id;
        blocos[id]->qtd_instrucoes = m;

        for(int i = 0; i < m; i++){
            fgets(blocos[id]->instrucoes[i], MAX_LINHA, stdin);
            blocos[id]->instrucoes[i][strcspn(blocos[id]->instrucoes[i], "\n")] = 0;
        }

        fgets(linha, MAX_LINHA, stdin);
        char* token = strtok(linha, " \n");
        while(token){
            int s = atoi(token);
            if(s != 0){
                blocos[id]->sucessores[blocos[id]->qtd_sucessores++] = s;
            }
            token=strtok(NULL, " \n");
        }
        int ch = getchar();
        if(ch != '\n' && ch != EOF){
            ungetc(ch, stdin);
        }
    }

    for(int i = 1; i <= maior_id_bloco; ++i){
        if(!blocos[i]) continue;
        for(int j=0; j < blocos[i]->qtd_sucessores; ++j){
            int suc_id = blocos[i]->sucessores[j];
            if(blocos[suc_id]) {
                blocos[suc_id]->antes[blocos[suc_id]->qtd_antes++] = i;
            }
        }
    }
}

void calcula_use_def(){
    for(int i = 1; i <= maior_id_bloco; i++){
        Bloco* b = blocos[i];
        if(!b) continue;

        bool definidos_neste_bloco[MAX_VARS] = {false};

        for(int j = 0; j < b->qtd_instrucoes; j++){
            char* instrucao = b->instrucoes[j];
            char* ptr_igual = strchr(instrucao,'=');
            
            if(!ptr_igual) continue; 

            char* rhs = ptr_igual + 1;
            for(int k = 0; rhs[k] != '\0'; k++){
                int idx_uso = var_indice(rhs[k]);
                if(idx_uso != -1 && !definidos_neste_bloco[idx_uso]){
                    b->use[idx_uso] = true;
                }
            }
            char var_def_char = *(ptr_igual - 2); 
            int idx_def = var_indice(var_def_char);
            if(idx_def != -1){
                b->def[idx_def] = true;
                definidos_neste_bloco[idx_def] = true;
            }
        }
    }
}

void run_longevidade(){
    bool mudou;
    do{
        mudou = false;
        for(int i = maior_id_bloco; i >= 1; i--){
            Bloco* b = blocos[i];
            if (!b) continue;
            
            bool old_in[MAX_VARS];
            memcpy(old_in, b->in, sizeof(old_in));
            memset(b->out, 0, sizeof(b->out));
           
            for(int s = 0; s < b->qtd_sucessores; s++){
                int id_sucessor = b->sucessores[s];
                if(blocos[id_sucessor]){
                    uniao_bool(b->out, blocos[id_sucessor]->in);
                }
            }
            
           
            memcpy(b->in, b->out, sizeof(b->in)); 
            diferenca_bool(b->in, b->def);        
            uniao_bool(b->in, b->use);       

           
            if(memcmp(old_in, b->in, sizeof(old_in)) != 0){
                mudou = true;
            }
        }
    } while(mudou);
}

void print_results(){
    for(int i = 1; i <= maior_id_bloco; i++){
        Bloco* b = blocos[i];
        if(!b) continue;

        printf("Bloco %d\n", b->id);
        
        printf("  IN : {");
        bool primeiro = true;
        for(int j=0; j<MAX_VARS; j++){
            if(b->in[j]){
                printf("%s%c", primeiro ? "" : ", ", indice_var(j));
                primeiro = false;
            }
        }
        printf("}\n");
        printf("  OUT: {");
        primeiro = true;
        for(int j=0; j<MAX_VARS; j++){
            if(b->out[j]){
                printf("%s%c", primeiro ? "" : ", ", indice_var(j));
                primeiro = false;
            }
        }
        printf("}\n");
    }
}

int main(){
    parse_input();
    calcula_use_def();
    run_longevidade();
    print_results();

    for(int i = 1; i <= maior_id_bloco; i++){
        if(blocos[i]){
            free(blocos[i]);
        }
    }
    return 0;
}

