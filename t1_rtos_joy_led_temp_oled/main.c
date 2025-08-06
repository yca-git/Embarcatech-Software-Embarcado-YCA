#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "inc/ssd1306.h"
#include "inc/display_oled.h"

const uint led_pin = 12;

// Função auxiliar: leitura da temperatura interna do RP2040
float read_temperature() {
    const float conversion_factor = 3.3f / (1 << 12);
    adc_select_input(4); // canal do sensor interno
    uint16_t raw = adc_read();
    float voltage = raw * conversion_factor;
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

// Tarefa: piscar LED
void vTaskLed(void *pvParameters) {
    for (;;) {
        gpio_put(led_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_put(led_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(950));
    }
}

// Tarefa: leitura da temperatura interna (apenas atualiza variável global)
void vTaskTemp(void *pvParameters) {
    for (;;) {
        global_tempC = read_temperature();
        printf("\n\033[36mTemperatura:\033[0m \033[94m%.2f C\033[0m\n\n", global_tempC);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// Tarefa: leitura do joystick (apenas atualiza variável global)
void vTaskJoystick(void *pvParameters) {
    const uint adc_max = (1 << 12) - 1;
    const uint limiar_cima = adc_max * 3 / 4;
    const uint limiar_baixo = adc_max / 4;

    for (;;) {
        adc_select_input(0); // eixo Y
        uint y = adc_read();
        adc_select_input(1); // eixo X
        uint x = adc_read();

        if (y > limiar_cima) {
            snprintf(global_direcao, sizeof(global_direcao), "CIMA");
            printf("\033[38;5;208mDirecao:\033[0m \033[33mCIMA\033[0m\n");
        }
        else if (y < limiar_baixo) {
            snprintf(global_direcao, sizeof(global_direcao), "BAIXO");
            printf("\033[38;5;208mDirecao:\033[0m \033[33mBAIXO\033[0m\n");
        }
        else if (x < limiar_baixo) {
            snprintf(global_direcao, sizeof(global_direcao), "ESQUERDA");
            printf("\033[38;5;208mDirecao:\033[0m \033[33mESQUERDA\033[0m\n");
        }
        else if (x > limiar_cima) {
            snprintf(global_direcao, sizeof(global_direcao), "DIREITA");
            printf("\033[38;5;208mDirecao:\033[0m \033[33mDIREITA\033[0m\n");
        }
        else {
            snprintf(global_direcao, sizeof(global_direcao), "NEUTRO");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Tarefa: atualizar display OLED
void vTaskDisplay(void *pvParameters) {
    for (;;) {
        atualizar_display(); // redesenha display com valores atuais
        vTaskDelay(pdMS_TO_TICKS(250)); // atualiza 2x por segundo (evita flicker)
    }
}

int main() {
    stdio_init_all();

    // Inicializa LED
    gpio_init(led_pin);
    gpio_set_dir(led_pin, true);

    // Inicializa ADC
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_gpio_init(26); // ADC0 - eixo Y
    adc_gpio_init(27); // ADC1 - eixo X

    // Inicializa display OLED
    inicializa_display();

    // Cria tarefas com mesmo nivel de prioridade (1)
    xTaskCreate(vTaskLed, "LED Task", 128, NULL, 1, NULL);
    xTaskCreate(vTaskTemp, "Temp Task", 256, NULL, 1, NULL);
    xTaskCreate(vTaskJoystick, "Joystick Task", 256, NULL, 1, NULL);
    xTaskCreate(vTaskDisplay, "Display Task", 256, NULL, 1, NULL);

    // Inicia o escalonador
    vTaskStartScheduler();

    while (true);
    return 0;
}
