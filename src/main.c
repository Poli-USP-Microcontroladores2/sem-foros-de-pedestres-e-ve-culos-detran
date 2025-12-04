#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>

#define modo 0 // 1 para ativar o modo noturno, 0 para desativar
#define priority 3

// Pinos de Sincronia e Pedestre
#define SYNC_PIN 12
#define SYNC_PIN_2 4
#define PEDESTRE_PIN 16 // PTA16

/* Tempos */
#define TEMPO_VERDE  3000
#define TEMPO_AMARELO 1000
#define TEMPO_VERMELHO 4000

/* DeviceTree Alias */
#define LED0_NODE DT_ALIAS(led0)     // Verde
#define LED0_NODE_ver DT_ALIAS(led2) // Vermelho
#define GPIO_NODE DT_NODELABEL(gpioa)

#define SYNC_PIN_NODE DT_NODELABEL(gpioa)

static const struct device *sync_port = DEVICE_DT_GET(SYNC_PIN_NODE);

static const struct gpio_dt_spec ledVerde = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec ledVermelho = GPIO_DT_SPEC_GET(LED0_NODE_ver, gpios);
static const struct device *gpio_dev = DEVICE_DT_GET(GPIO_NODE);

// Callback para o botão de pedestre
static struct gpio_callback pedestre_cb_data;

// Flag para avisar as threads que o botão foi apertado
volatile bool pedestre_acionado = false;

K_SEM_DEFINE(verde, 1, 1);    // Começa com o Verde
K_SEM_DEFINE(amarelo, 0, 1);
K_SEM_DEFINE(vermelho, 0, 1);

// --- INTERRUPÇÃO DO PEDESTRE (PTA16) ---
void pedestre_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    // Apenas sinaliza a flag. As threads vão ler isso e abortar seus loops.
    pedestre_acionado = true;
}

// --- THREAD AMARELO (NOVA) ---
void thread_amarelo(void *p1, void *p2, void *p3)
{
    while (1)
    {
        k_sem_take(&amarelo, K_FOREVER);
        
        printk("Estado: AMARELO\n");

        // Amarelo = Verde + Vermelho Ligados
        gpio_pin_set_dt(&ledVermelho, 1);
        gpio_pin_set_dt(&ledVerde, 1);
        
        // O amarelo cumpre seu tempo fixo (geralmente não é interrompido)
        k_msleep(TEMPO_AMARELO); 
        
        gpio_pin_set_dt(&ledVermelho, 0);
        gpio_pin_set_dt(&ledVerde, 0);

        // Limpa a flag de pedestre, pois já atendemos o pedido (passamos pelo amarelo)
        pedestre_acionado = false;

        // Depois do amarelo, sempre vai para o vermelho
        k_sem_give(&vermelho);
    }
}

// --- THREAD VERDE ---
void farol_aberto(void *p1, void *p2, void *p3)
{
    while (1)
    {
        k_sem_take(&verde, K_FOREVER);
        printk("Estado: VERDE\n");

        gpio_pin_set_dt(&ledVermelho, 0);
        gpio_pin_set_dt(&ledVerde, 1);

        // Loop fracionado para permitir interrupção rápida
        // 40 x 100ms = 4000ms (4 segundos)
        for (int i = 0; i < (TEMPO_VERDE / 100); i++)
        {
            if (pedestre_acionado) {
                printk("Verde Interrompido pelo Pedestre!\n");
                break; // Sai do loop 'for' imediatamente
            }
            k_msleep(100);
        }

        gpio_pin_set_dt(&ledVerde, 0);
        
        // Do verde, vai para o Amarelo
        k_sem_give(&amarelo);
    }
}

// --- THREAD VERMELHO ---
void farol_fechado(void *p1, void *p2, void *p3)
{
    while (1)
    {
        k_sem_take(&vermelho, K_FOREVER);
        printk("Estado: VERMELHO\n");

        gpio_pin_set_dt(&ledVerde, 0);
        gpio_pin_set_dt(&ledVermelho, 1);

        // Loop fracionado para permitir interrupção
        for (int i = 0; i < (TEMPO_VERMELHO / 100); i++)
        {
            if (pedestre_acionado) {
                printk("Vermelho Interrompido pelo Pedestre!\n");
                break; 
            }
            k_msleep(100);
        }

        gpio_pin_set_dt(&ledVermelho, 0);

        // Se foi interrompido pelo pedestre, vai para Amarelo (conforme seu pedido "troque para thread amarelo")
        // Se foi ciclo normal, vai para Verde.
        if (pedestre_acionado) {
            k_sem_give(&amarelo);
        } else {
            k_sem_give(&verde);
        }
    }
}

void modus_nocturnus(void *p1, void *p2, void *p3){
    if(modo == 1){
        // Trava os semáforos normais para não interferirem
        k_sem_take(&verde, K_NO_WAIT);
        k_sem_take(&vermelho, K_NO_WAIT);
        k_sem_take(&amarelo, K_NO_WAIT);
        
        while (1){
            gpio_pin_set_dt(&ledVermelho, 1); // Pisca vermelho (ou amarelo se ligar o verde junto)
            k_msleep(1000);
            gpio_pin_set_dt(&ledVermelho, 0);
            k_msleep(1000);
        }
    }
}

// Definição das Threads
K_THREAD_DEFINE(mn, 512, modus_nocturnus, NULL, NULL, NULL, -1, 0, 0);
K_THREAD_DEFINE(t_verde, 512, farol_aberto, NULL, NULL, NULL, priority, 0, 0);
K_THREAD_DEFINE(t_amarelo, 512, thread_amarelo, NULL, NULL, NULL, priority, 0, 0); 
K_THREAD_DEFINE(t_vermelho, 512, farol_fechado, NULL, NULL, NULL, priority, 0, 0);

void main(void)
{
    printk("Iniciando sistema...\n");

    if (!device_is_ready(ledVerde.port) || !device_is_ready(ledVermelho.port))
    {
        return;
    }

    gpio_pin_configure_dt(&ledVerde, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledVermelho, GPIO_OUTPUT_INACTIVE);

    // Configuração dos pinos de sincronia originais
    gpio_pin_configure(gpio_dev, SYNC_PIN, GPIO_OUTPUT_HIGH);
    gpio_pin_configure(gpio_dev, SYNC_PIN_2, GPIO_INPUT);
    gpio_pin_set(gpio_dev, SYNC_PIN, 0);

    // --- CONFIGURAÇÃO DO BOTÃO PEDESTRE (PTA16) ---
    // Configura PTA16 como Entrada com Pull-Up
    gpio_pin_configure(gpio_dev, PEDESTRE_PIN, GPIO_INPUT | GPIO_PULL_UP);
    
    // Configura interrupção na borda de descida (quando encostar no GND)
    gpio_pin_interrupt_configure(gpio_dev, PEDESTRE_PIN, GPIO_INT_EDGE_FALLING);
    
    // Inicializa callback
    gpio_init_callback(&pedestre_cb_data, pedestre_isr, BIT(PEDESTRE_PIN));
    gpio_add_callback(gpio_dev, &pedestre_cb_data);
    // ----------------------------------------------

    printk("Sistema Pronto.\n");
    gpio_pin_set(gpio_dev, SYNC_PIN, 1);

    while (gpio_pin_get(sync_port, SYNC_PIN))
    {
    }
    
    while (1)
    {
        k_sleep(K_FOREVER);
    }
}