#include "lwip/apps/mqtt.h"     
#include "../include/mqtt.h"   
#include "lwipopts.h"            
#include "pico/stdio.h"
#include <stdio.h> 
#include <string.h>
#include "pico/stdlib.h"
#include "lwip/dns.h"
#include "lwip/tcp.h" 
/* Variável global estática para armazenar a instância do cliente MQTT
 * 'static' limita o escopo deste arquivo */
static mqtt_client_t *client;
bool conected = false;

// Estrutura para lidar com resolução DNS
typedef struct {
    ip_addr_t resolved_addr;
    bool dns_resolved;
    bool dns_failed;
    const char *client_id;
    const char *user;
    const char *pass;
    char *text_buffer;
} dns_resolve_context_t;

static dns_resolve_context_t dns_context;

// Estrutura para teste de conectividade TCP
typedef struct {
    bool tcp_connected;
    bool tcp_failed;
    bool tcp_timeout;
} tcp_test_context_t;

static tcp_test_context_t tcp_test_ctx;

// Callback para teste de conectividade TCP
static err_t tcp_test_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
    tcp_test_context_t *ctx = (tcp_test_context_t *)arg;
    if (err == ERR_OK) {
        printf("✅ Teste TCP: Conectividade OK na porta 1883\n");
        ctx->tcp_connected = true;
    } else {
        printf("❌ Teste TCP: Erro na conexão: %d\n", err);
        ctx->tcp_failed = true;
    }
    
    // Fecha a conexão de teste imediatamente
    if (tpcb) {
        tcp_close(tpcb);
    }
    return ERR_OK;
}

// Função para testar conectividade TCP antes do MQTT
static bool test_tcp_connectivity(ip_addr_t *addr, char *text_buffer) {
    printf("🔍 Testando conectividade TCP para %s:1883...\n", ipaddr_ntoa(addr));
    sniprintf(text_buffer, 100, "Testando TCP...\n");
    
    tcp_test_ctx.tcp_connected = false;
    tcp_test_ctx.tcp_failed = false;
    tcp_test_ctx.tcp_timeout = false;
    
    struct tcp_pcb *test_pcb = tcp_new();
    if (!test_pcb) {
        printf("❌ Erro ao criar PCB TCP para teste\n");
        sniprintf(text_buffer, 100, "Erro PCB TCP\n");
        return false;
    }
    
    tcp_arg(test_pcb, &tcp_test_ctx);
    tcp_connect(test_pcb, addr, 1883, tcp_test_connected_cb);
    
    // Aguarda resultado do teste (timeout de 10 segundos)
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while (!tcp_test_ctx.tcp_connected && !tcp_test_ctx.tcp_failed) {
        sleep_ms(100);
        if (to_ms_since_boot(get_absolute_time()) - start_time > 10000) {
            printf("❌ Teste TCP: Timeout na conexão\n");
            tcp_test_ctx.tcp_timeout = true;
            break;
        }
    }
    
    if (test_pcb) {
        tcp_close(test_pcb);
    }
    
    if (tcp_test_ctx.tcp_connected) {
        sniprintf(text_buffer, 100, "TCP OK\n");
        return true;
    } else if (tcp_test_ctx.tcp_timeout) {
        sniprintf(text_buffer, 100, "TCP Timeout\n");
        return false;
    } else {
        sniprintf(text_buffer, 100, "TCP Falhou\n");
        return false;
    }
}

// Callback para resolução DNS
static void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    dns_resolve_context_t *ctx = (dns_resolve_context_t *)callback_arg;
    
    if (ipaddr != NULL) {
        ctx->resolved_addr = *ipaddr;
        ctx->dns_resolved = true;
        printf("DNS resolvido: %s -> %s\n", name, ipaddr_ntoa(ipaddr));
    } else {
        ctx->dns_failed = true;
        printf("Falha na resolução DNS para: %s\n", name);
    }
}

// Callback de status de conexão MQTT
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    bool *connected = (bool*)arg;
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("Conectado ao broker MQTT\n");
        *connected = true;
    } else {
        *connected = false;
        // Diagnóstico detalhado do erro
        switch(status) {
            case MQTT_CONNECT_REFUSED_PROTOCOL_VERSION:
                printf("❌ MQTT erro: Versão do protocolo não suportada\n");
                break;
            case MQTT_CONNECT_REFUSED_IDENTIFIER:
                printf("❌ MQTT erro: ID do cliente rejeitado\n");
                break;
            case MQTT_CONNECT_REFUSED_SERVER:
                printf("❌ MQTT erro: Servidor indisponível\n");
                break;
            case MQTT_CONNECT_REFUSED_USERNAME_PASS:
                printf("❌ MQTT erro: Usuário/senha inválidos\n");
                break;
            case MQTT_CONNECT_REFUSED_NOT_AUTHORIZED_:
                printf("❌ MQTT erro: Não autorizado\n");
                break;
            case MQTT_CONNECT_DISCONNECTED:
                printf("❌ MQTT erro: Desconectado (timeout/rede)\n");
                break;
            case MQTT_CONNECT_TIMEOUT:
                printf("❌ MQTT erro: Timeout na conexão\n");
                break;
            default:
                printf("❌ MQTT erro desconhecido: %d\n", status);
                break;
        }
    }
}

void meu_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) { // 
    printf("Payload recebido: %.*s\n", len, data);
}

// Callback chamado quando a inscrição for confirmada
void meu_subscribe_cb(void *arg, err_t result) {
    if (result == ERR_OK) {
        printf("Inscrição realizada com sucesso!\n");
    } else {
        printf("Falha ao se inscrever no tópico.\n");
    }
}


void inscrever_topico(mqtt_client_t *client, const char *topico) {
    mqtt_set_inpub_callback(client, NULL, meu_data_cb, NULL);
    mqtt_sub_unsub(client, topico, 0, meu_subscribe_cb, NULL, 1);
}

/* Função para configurar e iniciar a conexão MQTT
 * Parâmetros:
 *   - client_id: identificador único para este cliente
 *   - broker_ip: endereço IP ou hostname do broker (ex: "192.168.1.1" ou "mqtt.broker.com")
 *   - user: nome de usuário para autenticação (pode ser NULL)
 *   - pass: senha para autenticação (pode ser NULL) */
void mqtt_setup(const char *client_id, const char *broker_ip, const char *user, const char *pass, char *text_buffer) {
    if (client != NULL && mqtt_client_is_connected(client)) {
        return;
    }
    ip_addr_t broker_addr;  // Estrutura para armazenar o IP do broker
    sniprintf(text_buffer, 100,"Conectando ao broker\n");

    // Primeiro, tenta converter diretamente como IP
    if (ip4addr_aton(broker_ip, &broker_addr)) {
        printf("Usando endereço IP direto: %s\n", broker_ip);
        sniprintf(text_buffer, 100, "IP direto: %s\n", broker_ip);
    } else {
        // Se não for um IP válido, tenta resolver via DNS
        printf("Resolvendo hostname via DNS: %s\n", broker_ip);
        sniprintf(text_buffer, 100, "Resolvendo DNS...\n");
        
        // Inicializa o contexto DNS
        dns_context.dns_resolved = false;
        dns_context.dns_failed = false;
        dns_context.client_id = client_id;
        dns_context.user = user;
        dns_context.pass = pass;
        dns_context.text_buffer = text_buffer;
        
        // Tenta resolver o hostname
        err_t dns_err = dns_gethostbyname(broker_ip, &broker_addr, dns_found_cb, &dns_context);
        
        if (dns_err == ERR_OK) {
            // DNS foi resolvido imediatamente (cache)
            printf("DNS resolvido do cache\n");
            sniprintf(text_buffer, 100, "DNS resolvido: %s\n", ipaddr_ntoa(&broker_addr));
        } else if (dns_err == ERR_INPROGRESS) {
            // DNS está sendo resolvido assincronamente
            printf("Aguardando resolução DNS...\n");
            
            // Aguarda a resolução DNS (timeout de 10 segundos)
            uint32_t start_time = to_ms_since_boot(get_absolute_time());
            while (!dns_context.dns_resolved && !dns_context.dns_failed) {
                sleep_ms(100);
                if (to_ms_since_boot(get_absolute_time()) - start_time > 10000) {
                    sniprintf(text_buffer, 100, "Timeout DNS\n");
                    printf("Timeout na resolução DNS\n");
                    return;
                }
            }
            
            if (dns_context.dns_failed) {
                sniprintf(text_buffer, 100, "Erro DNS\n");
                printf("Falha na resolução DNS\n");
                return;
            }
            
            broker_addr = dns_context.resolved_addr;
            sniprintf(text_buffer, 100, "DNS resolvido: %s\n", ipaddr_ntoa(&broker_addr));
        } else {
            sniprintf(text_buffer, 100, "Erro DNS: %d\n", dns_err);
            printf("Erro na consulta DNS: %d\n", dns_err);
            return;
        }
    }

    // Teste de conectividade TCP antes de tentar MQTT
    if (!test_tcp_connectivity(&broker_addr, text_buffer)) {
        printf("❌ Falha no teste de conectividade TCP - abortando conexão MQTT\n");
        return;
    }

    if (client != NULL) { // Se já existe um cliente, desconecta e libera
        mqtt_disconnect(client);
        mqtt_client_free(client);
        client = NULL;
    }

    // Cria uma nova instância do cliente MQTT
    client = mqtt_client_new();
    if (client == NULL) {
        sniprintf(text_buffer, 100,"Falha no cliente\n");
        return;
    }


    // Configura as informações de conexão do cliente
    struct mqtt_connect_client_info_t ci = {
        .client_id = client_id,  // ID do cliente
        .client_user = (user && strlen(user) > 0) ? user : NULL,     // Usuário (NULL se vazio)
        .client_pass = (pass && strlen(pass) > 0) ? pass : NULL,     // Senha (NULL se vazio)
        .keep_alive = 60,        // Keep alive de 60 segundos
        .will_topic = NULL,      // Sem mensagem de última vontade
        .will_msg = NULL,
        .will_qos = 0,
        .will_retain = 0
    };

    // Atualiza o buffer para indicar tentativa de conexão
    sniprintf(text_buffer, 100, "Conectando MQTT %s...\n", ipaddr_ntoa(&broker_addr));
    printf("🔌 Tentando conectar ao broker MQTT em %s:1883\n", ipaddr_ntoa(&broker_addr));
    printf("   Client ID: %s\n", client_id);
    printf("   User: %s\n", ci.client_user ? ci.client_user : "(sem autenticação)");

    // Inicia a conexão com o broker
    // Parâmetros:
    //   - client: instância do cliente
    //   - &broker_addr: endereço do broker
    //   - 1883: porta padrão MQTT
    //   - mqtt_connection_cb: callback de status
    //   - &conected: argumento para o callback
    //   - &ci: informações de conexão
    mqtt_client_connect(client, &broker_addr, 1883, mqtt_connection_cb, &conected, &ci);
}

/* Callback de confirmação de publicação
 * Chamado quando o broker confirma recebimento da mensagem (para QoS > 0)
 * Parâmetros:
 *   - arg: argumento opcional
 *   - result: código de resultado da operação */
static void mqtt_pub_request_cb(void *arg, err_t result) {
    if (result == ERR_OK) {
        printf("✅ Publicação MQTT enviada com sucesso!\n");
    } else {
        printf("Erro ao publicar via MQTT: %d\n", result);
    }
}

/* Função para publicar dados em um tópico MQTT
 * Parâmetros:
 *   - topic: nome do tópico (ex: "sensor/temperatura")
 *   - data: payload da mensagem (bytes)
 *   - len: tamanho do payload */
void mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len) {
    // Envia a mensagem MQTT
    err_t status = mqtt_publish(
        client,              // Instância do cliente
        topic,               // Tópico de publicação
        data,                // Dados a serem enviados
        len,                 // Tamanho dos dados
        0,                   // QoS 0 (nenhuma confirmação)
        1,                   // Não reter mensagem
        mqtt_pub_request_cb, // Callback de confirmação
        NULL                 // Argumento para o callback
    );

    if (status != ERR_OK) {
        printf("mqtt_publish falhou ao ser enviada: %d\n", status);
    }
}

bool mqtt_is_connected(){ // resolve erro de "PANIC client != NULL"
    if (mqtt_client_is_connected(client)) return true;
    else return false;
}

