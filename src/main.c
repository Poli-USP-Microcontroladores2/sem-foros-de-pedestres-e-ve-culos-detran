#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>

#define modo 0 //modo0==normal; modo1==noturno
#define priority 3
#define SYNC_PIN 12

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 4000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED0_NODE_ver DT_ALIAS(led2)
#define SYNC_PIN_NODE DT_NODELABEL(gpioa)

#define BUTTON_NODE DT_NODELABEL(user_button_0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;

static const struct gpio_dt_spec ledVerde = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec ledVermelho = GPIO_DT_SPEC_GET(LED0_NODE_ver, gpios);
static const struct device *sync_port = DEVICE_DT_GET(SYNC_PIN_NODE);

K_SEM_DEFINE(verde, 0, 1);
K_SEM_DEFINE(vermelho, 1, 1);
K_SEM_DEFINE(button_sem, 0, 1);
K_SEM_DEFINE(preempt_red, 0, 1);

void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    k_sem_give(&button_sem);
    k_sem_give(&preempt_red);
}

void thread_botao(void *p1, void *p2, void *p3)
{
    while (1)
    {
        k_sem_take(&button_sem, K_FOREVER);
        printk("e tchain e tome");

    }
}

void farol_aberto(void *p1, void *p2, void *p3)
{
    printk("oi");
    if(modo == 0){
        while (1)
        {
            printk("onoff:%d\n", gpio_pin_get(sync_port,SYNC_PIN));
            k_sem_take(&verde, K_FOREVER);
            k_sem_reset(&preempt_red);
            printk("AAAAAAAAAAAAAAAAAAAAAAAAAAA: %d", k_sem_count_get(&preempt_red));
            gpio_pin_set_dt(&ledVermelho, 0);
            gpio_pin_set_dt(&ledVerde, 1);
            k_msleep(SLEEP_TIME_MS);
            gpio_pin_set_dt(&ledVerde, 0);
            k_sem_give(&vermelho);
        }
        }else if(modo == 1){
            while(1){
            k_sem_take(&verde, K_FOREVER);
            gpio_pin_set_dt(&ledVermelho, 0);
            k_msleep(1000);
            k_sem_give(&vermelho);
        }   
        }
    }


void farol_fechado(void *p1, void *p2, void *p3)
{
    if(modo == 0){
    while (1)
    {
        k_sem_take(&vermelho, K_FOREVER);
        bool preempted = false;
        gpio_pin_set_dt(&ledVerde, 0);
        gpio_pin_set_dt(&ledVermelho, 1);
        k_sem_reset(&preempt_red);
        for (int i = 0; i < 40; i++)
        {
            // Tenta pegar o semáforo de preempção SEM esperar
            if (k_sem_take(&preempt_red, K_NO_WAIT) == 0)
            {
                // CONSEGUIMOS! O botão foi apertado.
                printk("Farol: VERMELHO INTERROMPIDO!\n");
                preempted = true;
                K_SECONDS(1);
                break;
            }

            // Se não foi interrompido, dorme o pequeno intervalo
            k_msleep(100);
        }

        if (!preempted)
        {
            k_sem_take(&preempt_red, K_NO_WAIT);
        }
        gpio_pin_set_dt(&ledVermelho, 0);
        k_sem_give(&verde);
    }
    }else if(modo == 1){
    while(1){
    k_sem_take(&vermelho, K_FOREVER);
    gpio_pin_set_dt(&ledVermelho, 1);
    k_msleep(1000);
    k_sem_give(&verde);
    }
    }
}

K_THREAD_DEFINE(ab, 512, farol_aberto, NULL, NULL, NULL, priority, 0, 0);
K_THREAD_DEFINE(fe, 512, farol_fechado, NULL, NULL, NULL, priority, 0, 0);
K_THREAD_DEFINE(re, 512, thread_botao, NULL, NULL, NULL, 2, 0, 0);

void main(void)
{
    printk("Iniciando a thread main...\n");

    if (!device_is_ready(ledVerde.port))
    {
        printk("ERRO ledVerde\n");
        return;
    }
    printk("ledVerde OK.\n");

    if (!device_is_ready(ledVermelho.port))
    {
        printk("ERRO ledVermelho\n");
        return;
    }
    printk("ledVermelho OK.\n");

    gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_RISING);
    gpio_init_callback(&button_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    printk("Configurando pinos\n");
    gpio_pin_configure_dt(&ledVerde, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledVermelho, GPIO_OUTPUT_INACTIVE);
    ///////////////////////////////////////////////////////////
    gpio_pin_configure(sync_port, SYNC_PIN, GPIO_OUTPUT_HIGH);

    gpio_pin_set(sync_port, SYNC_PIN, 0);
    printk("onoff:%d\n", gpio_pin_get(sync_port,SYNC_PIN));

    printk("concluida Main\n");
    gpio_pin_set(sync_port, SYNC_PIN, 1);
    while (1)
    {
        k_sleep(K_FOREVER);
    }
}
