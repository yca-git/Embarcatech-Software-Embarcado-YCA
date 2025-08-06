#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "library/library.h"

void setup_joystick();                                // Chama a função de configuração do joystick
char *joystick_read_axis(); // Lê os valores dos eixos do joystick (X e Y)

#endif