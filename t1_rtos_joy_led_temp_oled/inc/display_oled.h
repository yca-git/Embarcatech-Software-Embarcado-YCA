#ifndef DISPLAY_OLED_H
#define DISPLAY_OLED_H

#include "ssd1306.h"

// Variáveis globais para serem atualizadas pelas tasks
extern float global_tempC;
extern char global_direcao[16];

// Inicializa o display OLED (deve ser chamada no início)
void inicializa_display(void);

// Limpa toda a tela do OLED
void limpar_tela(void);

// Exibe um texto na tela (posição x, y e tamanho scale)
void print_texto(char *msg, uint32_t x, uint32_t y, uint32_t scale);

// Atualiza o display OLED mostrando temperatura e direção atuais
void atualizar_display(void);

// Função auxiliar de delay em milissegundos (busy wait)
void timer_milliseconds(int milliseconds);

#endif // DISPLAY_OLED_H
