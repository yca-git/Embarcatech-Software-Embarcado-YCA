#include "joystick.h"

// Definição dos pinos usados para o joystick
const int VRX = 26;          // Pino de leitura do eixo X do joystick (conectado ao ADC)
const int VRY = 27;          // Pino de leitura do eixo Y do joystick (conectado ao ADC)
const int ADC_CHANNEL_0 = 0; // Canal ADC para o eixo X do joystick
const int ADC_CHANNEL_1 = 1; // Canal ADC para o eixo Y do joystick
const int SW = 22;           // Pino de leitura do botão do joystick

void setup_joystick()
{
  // Inicializa o ADC e os pinos de entrada analógica
  adc_gpio_init(VRX); // Configura o pino VRX (eixo X) para entrada ADC
  adc_gpio_init(VRY); // Configura o pino VRY (eixo Y) para entrada ADC

  // Inicializa o pino do botão do joystick
  gpio_init(SW);             // Inicializa o pino do botão
  gpio_set_dir(SW, GPIO_IN); // Configura o pino do botão como entrada
  gpio_pull_up(SW);          // Ativa o pull-up no pino do botão para evitar flutuações
}

// Função para ler os valores dos eixos do joystick (X e Y)
char *joystick_read_axis()
{
  static uint vrx_value = 0;
  static uint vry_value = 0;

  // Leitura do valor do eixo X do joystick
  adc_select_input(ADC_CHANNEL_1); // Seleciona o canal ADC para o eixo X
  sleep_us(2);                     // Pequeno delay para estabilidade
  vrx_value = adc_read();         // Lê o valor do eixo X (0-4095)

  // Leitura do valor do eixo Y do joystick
  adc_select_input(ADC_CHANNEL_0); // Seleciona o canal ADC para o eixo Y
  sleep_us(2);                     // Pequeno delay para estabilidade
  vry_value = adc_read();         // Lê o valor do eixo Y (0-4095)

  // Verifica o estado do joystick
  if (vrx_value < 1000) {
    return "ESQUERDA";
  } else if (vrx_value > 3000) {
    return "DIREITA"; 
  } else if (vry_value < 1000) {
    return "BAIXO"; 
  } else if (vry_value > 3000) {
    return "CIMA"; 
  }
  else return "CENTRO"; // Retorna "CENTRO" se não estiver em nenhuma direção específica
}
