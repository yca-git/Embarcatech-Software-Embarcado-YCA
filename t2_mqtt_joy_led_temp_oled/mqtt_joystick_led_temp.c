// By YCA - bitdoglab - embarcatech 2025 - tarefa 2 software embarcado

#include "library/library.h"


int main() {
    
    adc_init(); 
    led_init(); 
    stdio_init_all(); 
    inicializa_oled();
    setup_joystick(); 
    init_wifi(NULL);
    clear_display();
    print_apresentacao();
    
    // Configurações para broker externo laica
    char mqtt_buffer[100];
    const char *broker_ip = "mqtt.iot.natal.br"; // Broker externo Laica
    const char *client_id = "yuri";
    const char *topic_status_temp = "ha/desafio09/yuri.aquino/temp"; 
    const char *topic_status_joy = "ha/desafio09/yuri.aquino/joy";
    const char *user_mqtt = ""; // Coloque aqui o usuário do MQTT
    const char *pass_mqtt = ""; // Coloque aqui a senha do MQTT
    const char *wifi_ssid = ""; //Coloque aqui o SSID do wifi
    const char *wifi_pass = ""; //Coloque aqui a senha do wifi

    // Estados de controle
    bool wifi_connected = false;
    bool mqtt_connected = false;
    char *last_state_joy = "CENTRO"; //ponteiro para string

    printf("=== INICIANDO SISTEMA SMART PARKING ===\n");

    while (1) {
        // PASSO 1: CONECTAR AO WIFI
        if (!wifi_connected) {
            printf("1️⃣ Conectando ao WiFi '%s'...\n", wifi_ssid);
            print_oled("Conectando", "WiFi...", wifi_ssid);
            
            connect_to_wifi(wifi_ssid, wifi_pass, NULL);
            sleep_ms(5000); // Aumentar de 3000 para 5000
            
            if (is_connected()) {
                printf("✅ WiFi conectado com sucesso!\n");
                wifi_connected = true;
                print_oled("WiFi", "Conectado!", "");
                sleep_ms(2000);
            } else {
                printf("❌ Falha na conexão WiFi. Tentando novamente...\n");
                print_oled("WiFi", "Erro!", "Tentando novamente");
                sleep_ms(10000); // Aumentar de 5000 para 10000
                continue;
            }
        }
        
        // PASSO 2: CONECTAR AO BROKER MQTT
        while (wifi_connected && !mqtt_connected) {
            printf("2️⃣ Conectando ao broker MQTT %s...\n", broker_ip);
            print_oled("Conectando", "MQTT...", broker_ip);
            
            mqtt_setup(client_id, broker_ip, user_mqtt, pass_mqtt, mqtt_buffer);
            printf("Status MQTT: %s\n", mqtt_buffer);
            sleep_ms(1000); 
            
            if (mqtt_is_connected()) {
                printf("✅ Confirmado: MQTT conectado com sucesso!\n");
                mqtt_connected = true;
                print_oled("MQTT", "Conectado!", "Pronto para uso");
                sleep_ms(2000);
            } else {
                printf("❌ Falha na conexão MQTT. Tentando novamente...\n");
                print_oled("MQTT", "Erro!", "Tentando novamente");
                sleep_ms(15000); // 
                continue; // Tentar novamente a conexão MQTT
            }
        }
        
        // PASSO 3: LEITURA E ENVIO DO STATUS DA TEMPERATURA E JOYSTICK
        if (wifi_connected && mqtt_connected) {
            static uint32_t last_time = 0;
            uint32_t current_time = to_ms_since_boot(get_absolute_time()); // Tempo atual em milissegundos

            if (current_time - last_time > 30000) { // Publica temperatura a cada 30 segundos
                last_time = current_time;
                float current_temp = read_temperature(); // Retorna a temperatura
                printf("📊 Publicando temperatura de %.2f°C\n", current_temp);
                char temp_str[16];
                snprintf(temp_str, sizeof(temp_str), "%d", (int)current_temp);
                print_oled("Publicando temperatura... ", temp_str, "");

                // Enviar temperatura para o broker MQTT
                mqtt_comm_publish(topic_status_temp, (const uint8_t*)temp_str, strlen(temp_str));
                blink_led(); //sinal visual de envio da temperatura
            }
            
            char *current_state_joy = joystick_read_axis(); // Lê o estado do joystick

            if (current_state_joy != last_state_joy && current_state_joy != "CENTRO") { //Publica estado do joystick se mudou
                printf("3️⃣ joystick mudou: %s\n", current_state_joy);
                
                const char *status_joy = current_state_joy;

                printf("Enviando para broker: tópico='%s', mensagem='%s'\n", topic_status_joy, status_joy);

                // Enviar o estado do joystick para o broker MQTT
                mqtt_comm_publish(topic_status_joy, (const char*)status_joy, strlen(status_joy));
                blink_led(); //sinal visual de envio da temperatura

                printf(" Dados enviados para o broker! \n");
                print_oled(status_joy, "Enviado MQTT", "");
                
                // Aguardar um pouco após o envio
                sleep_ms(1000);
                
                last_state_joy = current_state_joy;
            }
            
            // Verificações de conexão com timeouts
            static uint32_t last_check = 0;
            uint32_t now = to_ms_since_boot(get_absolute_time()); // Tempo atual em milissegundos
            
            if (now - last_check > 60000) { // Verificar a cada 60 segundos
                if (!is_connected()) {
                    printf("⚠️ WiFi desconectado!\n");
                    wifi_connected = false;
                    mqtt_connected = false;
                } else if (!mqtt_is_connected()) {
                    printf("⚠️ MQTT desconectado!\n");
                    mqtt_connected = false;
                }
                last_check = now;
            }
        }
    }

    return 0;
}
// O codigo aceita endereço de ip e dns 
