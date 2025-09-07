#include "../include/wifi.h"       
#include "pico/cyw43_arch.h"        
#include <stdio.h>                   
#include "pico/stdio.h"
#include "library/library.h"
#include "lwip/ip_addr.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/raw.h"
#include "lwip/timeouts.h"

int status;
static bool ping_success = false;
static absolute_time_t last_ping_time;

// Callback para receber resposta do ping
static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr) {
    struct icmp_echo_hdr *iecho;
    
    if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) &&
        pbuf_remove_header(p, PBUF_IP_HLEN) == 0) {
        
        iecho = (struct icmp_echo_hdr *)p->payload;
        
        if ((iecho->type == ICMP_ER) && (iecho->id == 0xABCD)) {
            ping_success = true;
        }
    }
    
    pbuf_free(p);
    return 1; // Consome o pacote
}

// Função para fazer ping no gateway
bool ping_gateway() {
    struct raw_pcb *ping_pcb;
    struct pbuf *p;
    struct icmp_echo_hdr *iecho;
    ip_addr_t gateway_addr;
    
    // Reset do resultado
    ping_success = false;
    
    // Pega o gateway da interface
    gateway_addr = cyw43_state.netif[CYW43_ITF_STA].gw;
    
    // Verifica se temos um gateway válido
    if (ip_addr_isany(&gateway_addr)) {
        return false;
    }
    
    // Cria socket RAW para ICMP
    ping_pcb = raw_new(IP_PROTO_ICMP);
    if (ping_pcb == NULL) {
        return false;
    }
    
    raw_recv(ping_pcb, ping_recv, NULL);
    
    // Cria pacote ICMP
    p = pbuf_alloc(PBUF_IP, sizeof(struct icmp_echo_hdr), PBUF_RAM);
    if (p == NULL) {
        raw_remove(ping_pcb);
        return false;
    }
    
    iecho = (struct icmp_echo_hdr *)p->payload;
    iecho->type = ICMP_ECHO;
    iecho->code = 0;
    iecho->chksum = 0;
    iecho->id = 0xABCD;
    iecho->seqno = htons(1);
    iecho->chksum = inet_chksum(iecho, sizeof(struct icmp_echo_hdr));
    
    // Envia o ping
    raw_sendto(ping_pcb, p, &gateway_addr);
    pbuf_free(p);
    
    // Aguarda resposta por até 1 segundo
    absolute_time_t timeout = make_timeout_time_ms(1000);
    while (!ping_success && !time_reached(timeout)) {
        cyw43_arch_poll();
        sleep_ms(10);
    }
    
    raw_remove(ping_pcb);
    return ping_success;
}

void restart_wifi(){
    cyw43_arch_deinit(); // Desativa o Wi-Fi    
    cyw43_arch_init(); // Reativa o Wi-Fi
}

void init_wifi(char *text_buffer){
     if (cyw43_arch_init()) {
       // sniprintf(text_buffer, 100, "Erro ao iniciar Wi-Fi\n");
        return;
    }
}

bool connect_to_wifi(const char *ssid, const char *password, char *text_buffer) {
   // sniprintf(text_buffer, 100, "Conectando...\n");
    printf("Tentando conectar ao WiFi: %s\n", ssid);
    print_oled("", "Conectando ao Wi-Fi", "");
    
    // Habilita o modo estação (STA) para se conectar a um ponto de acesso.
    cyw43_arch_enable_sta_mode();

    // Tenta conectar à rede Wi-Fi com um tempo limite de 10 segundos
    status = 1;
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 10000)) { 
        print_oled("", "Erro ao conectar", "");
        printf("Falha na conexão WiFi\n");
        //sniprintf(text_buffer, 100, "Erro ao conectar\n");
        return false;
    } else {  
        print_oled("", "Conectado ao Wi-Fi", "");
        printf("WiFi conectado com sucesso\n");
        
        // Aguarda um pouco para obter IP
        sleep_ms(2000);
        
        // Verifica e exibe o IP obtido
        uint32_t ip = cyw43_state.netif[CYW43_ITF_STA].ip_addr.addr;
        printf("IP obtido: %d.%d.%d.%d\n", 
               ip & 0xFF, (ip >> 8) & 0xFF, 
               (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
        
        //sniprintf(text_buffer, 100, "Conectado ao Wi-Fi\n");
        return true;      
    }
}

bool is_connected(){
    // Verifica primeiro o status básico do link
    int link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    if (link_status != CYW43_LINK_UP) {
        return false;
    }
    
    // Verifica se temos um IP válido
    uint32_t ip = cyw43_state.netif[CYW43_ITF_STA].ip_addr.addr;
    if (ip == 0) {
        return false;
    }
    
    // Teste de conectividade real: faz ping no gateway a cada 300 segundos
    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(last_ping_time, now) > 300000000) { // 300 segundos
        last_ping_time = now;
        
        printf("Testando conectividade com ping...\n");
        bool ping_ok = ping_gateway();
        printf("Ping resultado: %s\n", ping_ok ? "OK" : "FALHA");
        
        if (!ping_ok) {
            printf("Ping falhou - WiFi considerado desconectado\n");
            return false;
        }
    }
    
    return true;
}

void wifi_status_debug(){
    int link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    uint32_t ip = cyw43_state.netif[CYW43_ITF_STA].ip_addr.addr;
    
    printf("=== WiFi Status Debug ===\n");
    printf("Link Status: %d (CYW43_LINK_UP = %d)\n", link_status, CYW43_LINK_UP);
    printf("IP Address: %d.%d.%d.%d\n", 
           ip & 0xFF, (ip >> 8) & 0xFF, 
           (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
    printf("Is Connected: %s\n", is_connected() ? "true" : "false");
    printf("========================\n");
}
