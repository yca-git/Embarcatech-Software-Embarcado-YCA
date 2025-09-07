// By YCA - bitdoglab - embarcatech 2025 - tarefa 2 software embarcado

#include "library/library.h"
#include <math.h>




int main() {
    
    adc_init(); 
    led_init(); 
    stdio_init_all(); 
    inicializa_oled();
    setup_joystick(); 
    init_wifi(NULL);
    clear_display();
    print_apresentacao();
    mpu6050_setup_i2c();       // Configura barramento I2C
    mpu6050_reset();           // Reinicia o sensor
    
    // Configurações para broker externo laica
    char mqtt_buffer[100];
    const char *broker_ip = "mqtt.iot.natal.br"; // Broker externo Laica
    const char *client_id = "yuri";
    const char *topic_status_temp = "ha/desafio09/yuri.aquino/temp"; 
    const char *topic_status_joy = "ha/desafio09/yuri.aquino/joy";
    const char *topic_status_mpu = "ha/desafio09/yuri.aquino/mpu";
    const char *user_mqtt = "desafio09"; // Coloque aqui o usuário do MQTT
    const char *pass_mqtt = "desafio09.laica"; // Coloque aqui a senha do MQTT
    const char *wifi_ssid = "YCA"; //Coloque aqui o SSID do wifi
    const char *wifi_pass = "00000000"; //Coloque aqui a senha do wifi

    // Estados de controle
    bool wifi_connected = false;
    bool mqtt_connected = false;
    char *last_state_joy = "CENTRO"; //ponteiro para string
    
    int16_t accel[3], gyro[3], temp;


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
        
        // PASSO 3: LEITURA E ENVIO DOS DADOS 
        if (wifi_connected && mqtt_connected) {
            static uint32_t last_time_mpu = 0;
            static float last_accel_g[3] = {0.0, 0.0, 0.0};
            static float last_gyro_dps[3] = {0.0, 0.0, 0.0};
            static bool first_reading = true;
            uint32_t current_time = to_ms_since_boot(get_absolute_time()); // Tempo atual em milissegundos

            // Lê os dados do sensor sempre
            mpu6050_read_raw(accel, gyro, &temp); // Armazena valores lidos nas variaveis

            // Conversão para unidades físicas
            float accel_g[3] = {accel[0] / 16384.0, accel[1] / 16384.0, accel[2] / 16384.0}; // Assumindo escala de ±2g
            float gyro_dps[3] = {gyro[0] / 131.0, gyro[1] / 131.0, gyro[2] / 131.0}; // Assumindo escala de ±250°/s

            printf("Accel (g) X: %.2f, Y: %.2f, Z: %.2f\n", accel_g[0], accel_g[1], accel_g[2]);
            printf("Gyro (°/s) X: %.2f, Y: %.2f, Z: %.2f\n", gyro_dps[0], gyro_dps[1], gyro_dps[2]);

            // Verifica se houve alteração significativa nos valores (tolerância de 0.01)
            bool values_changed = false;
            const float tolerance = 1;
            
            for (int i = 0; i < 3; i++) {
                if (fabs(accel_g[i] - last_accel_g[i]) > tolerance || 
                    fabs(gyro_dps[i] - last_gyro_dps[i]) > tolerance) {
                    values_changed = true;
                    break;
                }
            }

            // Condições para publicar:
            // 1. Primeira leitura
            // 2. A cada 10 segundos se houve alteração
            // 3. A cada 60 segundos se não houve alteração
            bool should_publish = first_reading ||
                                  (values_changed && (current_time - last_time_mpu > 10000)) ||
                                  (!values_changed && (current_time - last_time_mpu > 60000));

            if (should_publish) {
                // Criar JSON para envio via MQTT
                char json_payload[400];
                snprintf(json_payload, sizeof(json_payload),
                    "{\n"
                    "  \"team\": \"desafio09\",\n"
                    "  \"device\": \"bitdoglab01_yuri\",\n"
                    "  \"sensor\": \"MPU-6050\",\n"
                    "  \"data\": {\n"
                    "    \"accel\": {\n"
                    "      \"x\": %.2f,\n"
                    "      \"y\": %.2f,\n"
                    "      \"z\": %.2f\n"
                    "    },\n"
                    "    \"gyro\": {\n"
                    "      \"x\": %.2f,\n"
                    "      \"y\": %.2f,\n"
                    "      \"z\": %.2f\n"
                    "    }\n"
                    "  }\n"
                    "}",
                    accel_g[0], accel_g[1], accel_g[2],
                    gyro_dps[0], gyro_dps[1], gyro_dps[2]);

                // Publicar dados do MPU6050 via MQTT
                if (values_changed || first_reading) {
                    printf("📤 Enviando dados MPU6050 (alteração detectada)...\n");
                    print_oled("Enviando", "MPU6050", "mudanca");
                } else {
                    printf("📤 Enviando dados MPU6050 (envio periódico)...\n");
                    print_oled("Enviando", "MPU6050", "periodico");
                }
                
                mqtt_comm_publish(topic_status_mpu, json_payload, strlen(json_payload));
                
                // Atualizar valores anteriores
                for (int i = 0; i < 3; i++) {
                    last_accel_g[i] = accel_g[i];
                    last_gyro_dps[i] = gyro_dps[i];
                }
                
                last_time_mpu = current_time;
                first_reading = false;
                sleep_ms(1000); // espera 1 segundo
            }
            
            sleep_ms(100); // Pequena pausa para não sobrecarregar o processador
            
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
