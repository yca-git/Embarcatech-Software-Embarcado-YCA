#ifndef DISPLAY_H
#define DISPLAY_H

#include "library/library.h"

#define I2C_PORT i2c1
#define PINO_SCL 14
#define PINO_SDA 15

// Declaração das funções para inicializar o display OLED, imprimir texto e limpar o display
void inicializa_oled();
void print_oled(const char *msg1, const char *msg2, const char *msg3);
void clear_display();
void print_apresentacao();

#endif