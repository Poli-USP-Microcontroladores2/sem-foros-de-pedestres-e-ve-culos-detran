#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>

#define priority 3

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 4000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED0_NODE_ver DT_ALIAS(led1)

static const struct gpio_dt_spec ledVerde = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec ledVermelho = GPIO_DT_SPEC_GET(LED0_NODE_ver, gpios);

K_SEM_DEFINE(verde, 0, 1);
K_SEM_DEFINE(vermelho, 1, 1);

void farol_aberto(void *p1, void *p2, void *p3)
{
	printk("oi");
	while(1){
		k_sem_take(&verde, K_FOREVER);
		gpio_pin_set_dt(&ledVerde, 1);
		k_msleep(SLEEP_TIME_MS);
		gpio_pin_set_dt(&ledVerde, 0);
		k_sem_give(&vermelho);
	}
}

void farol_fechado(void *p1, void *p2, void *p3)
	{
		printk("oi");
		while(1){
		k_sem_take(&vermelho, K_FOREVER);
		gpio_pin_set_dt(&ledVermelho, 1);
		k_msleep(SLEEP_TIME_MS);
		gpio_pin_set_dt(&ledVermelho, 0);
		k_sem_give(&verde);
		}

}

K_THREAD_DEFINE(ab, 512, farol_aberto, NULL, NULL, NULL, priority, 0, 0);
K_THREAD_DEFINE(fe, 512, farol_fechado, NULL, NULL, NULL, priority, 0, 0);

void main(void)
{
    printk("Iniciando a thread main...\n"); // Mensagem para sabermos que a main comecou

    /* --- Verificacao do LED Verde (led0) --- */
    if (!device_is_ready(ledVerde.port)) {
        // Este printk e especifico
        printk("ERRO FATAL: O dispositivo ledVerde (alias 'led0') nao foi encontrado ou nao esta pronto.\n");
        return; // Sai da main
    }
    printk("Dispositivo ledVerde (led0) OK.\n");


    /* --- Verificacao do LED Vermelho (led1) --- */
    if (!device_is_ready(ledVermelho.port)) {
        // Este printk e especifico
        printk("ERRO FATAL: O dispositivo ledVermelho (alias 'led1') nao foi encontrado ou nao esta pronto.\n");
        return; // Sai da main
    }
    printk("Dispositivo ledVermelho (led1) OK.\n");


    /* --- Se chegamos aqui, tudo deu certo --- */
    printk("LEDs prontos. Configurando pinos...\n");
    gpio_pin_configure_dt(&ledVerde, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledVermelho, GPIO_OUTPUT_INACTIVE);

    printk("Configuracao concluida. Main ira dormir.\n");

    while (1)
    {
        k_sleep(K_FOREVER);
    }
}