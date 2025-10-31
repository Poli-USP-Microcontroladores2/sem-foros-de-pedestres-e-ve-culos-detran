#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>

// --- Configuração de LEDs via DeviceTree ---
#define LED_VERDE_NODE DT_ALIAS(led0)  // LED verde
#define LED_VERMELHO_NODE DT_ALIAS(led2)  // LED vermelho

static const struct gpio_dt_spec ledVerde = GPIO_DT_SPEC_GET(LED_VERDE_NODE, gpios);
static const struct gpio_dt_spec ledVermelho = GPIO_DT_SPEC_GET(LED_VERMELHO_NODE, gpios);

#define PRIORITY 5

#define TEMPO_VERDE_MS 3000   // Thread A dorme
#define TEMPO_AMARELO_MS 1000   // Thread B dorme
#define TEMPO_VERMELHO_MS 4000   // Thread C dorme

K_SEM_DEFINE(led_amarelo, 0, 1);
K_SEM_DEFINE(led_verde, 1, 1);
K_SEM_DEFINE(led_vermelho, 0, 1);

void thread_Verde(void *p1, void *p2, void *p3)
{
    while (1) {
        k_sem_take(&led_verde, K_FOREVER);
        
        gpio_pin_set_dt(&ledVerde, 1);

        k_msleep(TEMPO_VERDE_MS);

        gpio_pin_set_dt(&ledVerde, 0);

        k_sem_give(&led_amarelo);
    }
}

void thread_Amarelo(void *p1, void *p2, void *p3)
{
    while (1) {
        k_sem_take(&led_amarelo, K_FOREVER);
        
        gpio_pin_set_dt(&ledVermelho, 1);
        gpio_pin_set_dt(&ledVerde, 1); 

        k_msleep(TEMPO_AMARELO_MS);

        gpio_pin_set_dt(&ledVermelho, 0);
        gpio_pin_set_dt(&ledVerde, 0); 

        k_sem_give(&led_vermelho);
    }
}

void thread_Vermelho(void *p1, void *p2, void *p3)
{
    while (1) {
        k_sem_take(&led_vermelho, K_FOREVER);

        gpio_pin_set_dt(&ledVermelho, 1);

        k_msleep(TEMPO_VERMELHO_MS);

        gpio_pin_set_dt(&ledVermelho, 0);

        k_sem_give(&led_verde);
    }
}

K_THREAD_DEFINE(a_tid, 512, thread_Verde, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(b_tid, 512, thread_Amarelo, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(c_tid, 512, thread_Vermelho, NULL, NULL, NULL, PRIORITY, 0, 0);

void main(void)
{
    if (!device_is_ready(ledVerde.port) || !device_is_ready(ledVermelho.port)) {
        return;
    }

    gpio_pin_configure_dt(&ledVerde, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledVermelho, GPIO_OUTPUT_INACTIVE);

    while (1) {
        k_sleep(K_FOREVER);
    }
}