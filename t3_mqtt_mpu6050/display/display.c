#include "display.h" 

ssd1306_t disp; 

// Função para inicializar o display
void inicializa_oled(){ 
    stdio_init_all(); 
    i2c_init(I2C_PORT, 400*1000); // Inicializa a interface I2C no porto especificado com uma velocidade de 400 kHz
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C); // Configura o pino SCL para a função I2C
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C); // Configura o pino SDA para a função I2C
    gpio_pull_up(PINO_SCL);    // Ativa o resistor de pull-up interno no pino SCL
    gpio_pull_up(PINO_SDA);    // Ativa o resistor de pull-up interno no pino SDA
    disp.external_vcc = false; // Configura a estrutura disp para indicar que o display OLED está usando a alimentação interna (VCC)
    ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT); // Inicializa o display OLED com as dimensões 128x64 pixels, endereço I2C 0x3C e a porta I2C especificado
}

// Função para imprimir texto no display
void print_oled(const char *msg1, const char *msg2, const char *msg3){
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 10, 10, 1, msg1);
    ssd1306_draw_string(&disp, 10, 30, 1, msg2);
    ssd1306_draw_string(&disp, 10, 50, 1, msg3);
    ssd1306_show(&disp);
}

// Função para limpar o display
void clear_display(){
    ssd1306_clear(&disp);
    ssd1306_show(&disp);
}

// Função para exibir animação de apresentação
void print_apresentacao(){ 
    
    const char *words[]= {"TAREFA", "TRES", "YCA"};
    
    ssd1306_draw_empty_square(&disp, 0, 0, 120, 63);
    ssd1306_show(&disp);
    sleep_ms(2000);

    for(int i=0, y=6; i<sizeof(words)/sizeof(char *); ++i, y+=20) {
        ssd1306_draw_string(&disp, 10, y, 2, words[i]);
        ssd1306_show(&disp);
        sleep_ms(800);
    }

}

