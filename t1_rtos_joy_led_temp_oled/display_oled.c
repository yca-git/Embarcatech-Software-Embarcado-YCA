#include "inc/display_oled.h"
#include <stdio.h>
#include "hardware/i2c.h"
#include "inc/ssd1306.h"

// Configuração de hardware constante
#define I2C_PORT i2c1
#define PINO_SCL 14
#define PINO_SDA 15

ssd1306_t disp;

// Variáveis globais para exibir status (atualizadas pelas tasks)
float global_tempC = 0.0f;
char global_direcao[16] = "NEUTRO";

// Inicializa o hardware do display OLED
void inicializa_display() {
    i2c_init(I2C_PORT, 400*1000);  // Inicializa I2C a 400kHz
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SCL);
    gpio_pull_up(PINO_SDA);

    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT);
}

// Exibe texto no OLED
void print_texto(char *msg, uint32_t x, uint32_t y, uint32_t scale) {
    ssd1306_draw_string(&disp, x, y, scale, msg);
}

// Limpa a tela do OLED
void limpar_tela() {
    ssd1306_clear(&disp);
}

// Função que redesenha toda tela com as infos atuais
void atualizar_display() {
    limpar_tela();
    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "Temperatura: %.2f C", global_tempC);
    print_texto(temp_str, 0, 12, 1);

    char dir_str[32];
    snprintf(dir_str, sizeof(dir_str), "Direcao: %s", global_direcao);
    print_texto(dir_str, 0, 24, 1);

    ssd1306_show(&disp);
}


