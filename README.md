# Trabalho 2: Análises de Fluxo de Dados - Compiladores

## Descrição

Este projeto consiste na implementação de algoritmos de **Análise de Fluxo de Dados**, desenvolvidos para a disciplina de **Compiladores** da **Universidade Federal do Ceará (UFC) - Campus Quixadá**.

O objetivo é realizar análises estáticas sobre programas representados em blocos básicos, permitindo a obtenção de informações utilizadas em otimizações de compiladores.

---

## Estrutura do Projeto

| Arquivo                     | Descrição                                                                       |
| --------------------------- | ------------------------------------------------------------------------------- |
| `analise_longevidade.c`     | Implementação da **Análise de Longevidade (Liveness Analysis)**.                |
| `reaching_definitions.c`    | Implementação da análise de **Definições Alcançantes (Reaching Definitions)**.  |
| `available.c`               | Implementação da análise de **Expressões Disponíveis (Available Expressions)**. |
| `input1.txt` a `input4.txt` | Arquivos de entrada utilizados para testar os algoritmos.                       |

---

## Compilação e Execução

Cada algoritmo pode ser compilado e executado de forma independente.

### 1. Análise de Longevidade (Liveness Analysis)

**Compilação**

```bash
gcc -o analise_longevidade analise_longevidade.c
```

**Execução**

```bash
./analise_longevidade < input2.txt
./analise_longevidade < input3.txt
```

---

### 2. Reaching Definitions

**Compilação**

```bash
gcc -o reaching_definitions reaching_definitions.c
```

**Execução**

```bash
./reaching_definitions < input4.txt
```

---

### 3. Available Expressions

**Compilação**

```bash
gcc -o available available.c
```

**Execução**

```bash
./available < input1.txt
```

---

## Tecnologias Utilizadas

* Linguagem C
* GCC

---

## Integrantes

* Davylla Maria Lima Oliveira — **553304**
* Fabio Rodrigues Borges Filho — **552274**
* Rodrigo dos Santos Albuquerque — **554514**
* Samuel Denis Mota de Sousa Filho — **553830**

---

## Disciplina

**Compiladores**
Universidade Federal do Ceará (UFC) - Campus Quixadá
