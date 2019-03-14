/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Date           Author       Notes
 * 2018-03-13     ZeroFree     first implementation
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>

#include "drv_gpio.h"
#include "nrf.h"

#define MCU_GPIO_USE_PORT_EVENT 1

#define NRF_GPIO_INDEX(pin) (pin)
#define NRF_GPIO_PORT(pin)  (NRF_P0)
#define NRF_GPIO_MASK(pin)  (1 << NRF_GPIO_INDEX(pin))
#define NRF_GPIOTE_PIN_MASK GPIOTE_CONFIG_PSEL_Msk

/* GPIO interrupts */
#define NRF_GPIO_MAX_IRQ    8

#if MCU_GPIO_USE_PORT_EVENT
#define NRF_GPIO_SENSE_TRIG_NONE    0x00 /* just 0 */
#define NRF_GPIO_SENSE_TRIG_BOTH    0x01 /* something else than both below */
#define NRF_GPIO_SENSE_TRIG_HIGH    0x02 /* GPIO_PIN_CNF_SENSE_High */
#define NRF_GPIO_SENSE_TRIG_LOW     0x03 /* GPIO_PIN_CNF_SENSE_Low */
#endif

/* Storage for GPIO callbacks */
struct nrf_gpio_irq
{
    void (*func)(void *args);
    void              *args;
#if MCU_GPIO_USE_PORT_EVENT
    rt_int16_t        pin;
    uint8_t           sense_trig;
#endif
};

static struct nrf_gpio_irq nrf_gpio_irqs[NRF_GPIO_MAX_IRQ];

static void nrf_pin_mode(struct rt_device *device, rt_base_t pin, rt_base_t mode)
{
    uint32_t conf;
    NRF_GPIO_Type *port;
    uint8_t dir = 0;
    int index = NRF_GPIO_INDEX(pin);

    switch (mode)
    {
    case PIN_MODE_OUTPUT:
        conf = GPIO_PIN_CNF_DIR_Output |
               (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
        dir = 1;
        break;
    case PIN_MODE_INPUT:
        conf = 0;
        break;
    case PIN_MODE_INPUT_PULLUP:
        conf = GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos;
        break;
    case PIN_MODE_INPUT_PULLDOWN:
        conf = GPIO_PIN_CNF_PULL_Pulldown << GPIO_PIN_CNF_PULL_Pos;
        break;
    default:
        conf = 0;
        break;
    }

    port = NRF_GPIO_PORT(pin);
    port->PIN_CNF[index] = conf;

    if (dir == 1)
    {
        /* output */
        port->DIRSET = NRF_GPIO_MASK(pin);
        port->OUTCLR = NRF_GPIO_MASK(pin);
    }
    else
    {
        /* input */
        port->DIRCLR = NRF_GPIO_MASK(pin);
    }
}

static void nrf_pin_write(struct rt_device *device, rt_base_t pin, rt_base_t value)
{
    NRF_GPIO_Type *port;

    port = NRF_GPIO_PORT(pin);
    if (value)
    {
        port->OUTSET = NRF_GPIO_MASK(pin);
    }
    else
    {
        port->OUTCLR = NRF_GPIO_MASK(pin);
    }
}

static int nrf_pin_read(struct rt_device *device, rt_base_t pin)
{
    NRF_GPIO_Type *port;
    port = NRF_GPIO_PORT(pin);

    return (port->DIR & NRF_GPIO_MASK(pin)) ?
           (port->OUT >> NRF_GPIO_INDEX(pin)) & 1UL :
           (port->IN >> NRF_GPIO_INDEX(pin)) & 1UL;
}

/*
 * GPIO irq handler
 *
 * Handles the gpio interrupt attached to a gpio pin.
 *
 * @param index
 */
static void nrf_gpio_irq_handler(void)
{
#if MCU_GPIO_USE_PORT_EVENT
    NRF_GPIO_Type *gpio_port;
    int pin_index;
    int pin_state;
    uint8_t sense_trig;
    uint32_t gpio_state;
#endif
    int i;

    rt_interrupt_enter();

#if MCU_GPIO_USE_PORT_EVENT
    NRF_GPIOTE->EVENTS_PORT = 0;
    gpio_state = NRF_P0->IN;

    for (i = 0; i < NRF_GPIO_MAX_IRQ; i++)
    {
        if ((!nrf_gpio_irqs[i].func) ||
                (nrf_gpio_irqs[i].sense_trig == NRF_GPIO_SENSE_TRIG_NONE))
        {
            continue;
        }

        gpio_port = NRF_GPIO_PORT(nrf_gpio_irqs[i].pin);
        pin_index = NRF_GPIO_INDEX(nrf_gpio_irqs[i].pin);

        /* Store current SENSE setting */
        sense_trig = (gpio_port->PIN_CNF[pin_index] & GPIO_PIN_CNF_SENSE_Msk) >> GPIO_PIN_CNF_SENSE_Pos;

        if (!sense_trig)
        {
            continue;
        }

        /*
         * SENSE values are 0x02 for high and 0x03 for low, so bit #0 is the
         * opposite of state which triggers interrupt (thus its value should be
         * different than pin state).
         */
        pin_state = (gpio_state >> nrf_gpio_irqs[i].pin) & 0x01;
        if (pin_state == (sense_trig & 0x01))
        {
            continue;
        }

        /*
         * Toggle sense to clear interrupt or allow detection of opposite edge
         * when trigger on both edges is requested.
         */
        gpio_port->PIN_CNF[pin_index] &= ~GPIO_PIN_CNF_SENSE_Msk;
        if (sense_trig == NRF_GPIO_SENSE_TRIG_HIGH)
        {
            gpio_port->PIN_CNF[pin_index] |= GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos;
        }
        else
        {
            gpio_port->PIN_CNF[pin_index] |= GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos;
        }

        /*
         * Call handler in case SENSE configuration matches requested one or
         * trigger on both edges is requested.
         */
        if ((nrf_gpio_irqs[i].sense_trig == NRF_GPIO_SENSE_TRIG_BOTH) ||
                (nrf_gpio_irqs[i].sense_trig == sense_trig))
        {
            nrf_gpio_irqs[i].func(nrf_gpio_irqs[i].args);
        }
    }
#else
    for (i = 0; i < NRF_GPIO_MAX_IRQ; i++)
    {
        if (NRF_GPIOTE->EVENTS_IN[i] && (NRF_GPIOTE->INTENSET & (1 << i)))
        {
            NRF_GPIOTE->EVENTS_IN[i] = 0;
            if (nrf_gpio_irqs[i].func)
            {
                nrf_gpio_irqs[i].func(nrf_gpio_irqs[i].args);
            }
        }
    }
#endif

    rt_interrupt_leave();
}

void GPIOTE_IRQHandler(void)
{
    nrf_gpio_irq_handler();
}

static void nrf_gpio_irq_setup(void)
{
    static uint8_t irq_setup = 0;

    if (!irq_setup)
    {
        NVIC_EnableIRQ(GPIOTE_IRQn);
        irq_setup = 1;

#if MCU_GPIO_USE_PORT_EVENT
        NRF_GPIOTE->INTENCLR = GPIOTE_INTENCLR_PORT_Msk;
        NRF_GPIOTE->EVENTS_PORT = 0;
#endif
    }
}

/*
 * Find out whether we have an GPIOTE pin event to use.
 */
static int hal_gpio_find_empty_slot(void)
{
    int i;

    for (i = 0; i < NRF_GPIO_MAX_IRQ; i++)
    {
        if (nrf_gpio_irqs[i].func == NULL)
        {
            return i;
        }
    }
    return -1;
}

/*
 * Find the GPIOTE event which handles this pin.
 */
static int nrf_gpio_find_pin(int pin)
{
    int i;

#if MCU_GPIO_USE_PORT_EVENT
    for (i = 0; i < NRF_GPIO_MAX_IRQ; i++)
    {
        if (nrf_gpio_irqs[i].func && nrf_gpio_irqs[i].pin == pin)
        {
            return i;
        }
    }
#else
    pin = pin << GPIOTE_CONFIG_PSEL_Pos;

    for (i = 0; i < NRF_GPIO_MAX_IRQ; i++)
    {
        if (nrf_gpio_irqs[i].func &&
                (NRF_GPIOTE->CONFIG[i] & NRF_GPIOTE_PIN_MASK) == pin)
        {
            return i;
        }
    }
#endif

    return -1;
}

static rt_err_t nrf_pin_attach_irq(struct rt_device *device, rt_int32_t pin,
                                   rt_uint32_t mode, void (*hdr)(void *args), void *args)
{
    uint32_t conf;
    int i;

    nrf_gpio_irq_setup();
    i = hal_gpio_find_empty_slot();
    if (i < 0)
    {
        return -RT_EFULL;
    }

#if MCU_GPIO_USE_PORT_EVENT
    (void)conf;
    nrf_gpio_irqs[i].pin = pin;

    switch (mode)
    {
    case PIN_IRQ_MODE_RISING:
        nrf_gpio_irqs[i].sense_trig = NRF_GPIO_SENSE_TRIG_HIGH;
        break;
    case PIN_IRQ_MODE_FALLING:
        nrf_gpio_irqs[i].sense_trig = NRF_GPIO_SENSE_TRIG_LOW;
        break;
    case PIN_IRQ_MODE_RISING_FALLING:
        nrf_gpio_irqs[i].sense_trig = NRF_GPIO_SENSE_TRIG_BOTH;
        break;
    default:
        nrf_gpio_irqs[i].sense_trig = NRF_GPIO_SENSE_TRIG_NONE;
        return -1;
    }
#else
    switch (mode)
    {
    case PIN_IRQ_MODE_RISING:
        conf = GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos;
        break;
    case PIN_IRQ_MODE_FALLING:
        conf = GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos;
        break;
    case PIN_IRQ_MODE_RISING_FALLING:
        conf = GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos;
        break;
    default:
        return -RT_ERROR;
    }

    conf |= pin << GPIOTE_CONFIG_PSEL_Pos;
    conf |= GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos;
    NRF_GPIOTE->CONFIG[i] = conf;
#endif

    nrf_gpio_irqs[i].func = hdr;
    nrf_gpio_irqs[i].args = args;

    return RT_EOK;
}

static rt_err_t nrf_pin_detach_irq(struct rt_device *device, rt_int32_t pin)
{
    rt_err_t ret = RT_EOK;

    int i;

    i = nrf_gpio_find_pin(pin);
    if (i < 0)
    {
        return -RT_ERROR;
    }
    /* disable pin irq */
    rt_pin_irq_enable(pin, 0);

#if MCU_GPIO_USE_PORT_EVENT
    nrf_gpio_irqs[i].sense_trig = NRF_GPIO_SENSE_TRIG_NONE;
#else
    NRF_GPIOTE->CONFIG[i] = 0;
    NRF_GPIOTE->EVENTS_IN[i] = 0;
#endif

    nrf_gpio_irqs[i].args = NULL;
    nrf_gpio_irqs[i].func = NULL;

    return ret;
}

static rt_err_t nrf_pin_irq_enable(struct rt_device *device, rt_base_t pin, rt_uint32_t enabled)
{
#if MCU_GPIO_USE_PORT_EVENT
    NRF_GPIO_Type *nrf_gpio;
    int pin_index, sense_enabled = 0;
#endif
    int i;

    i = nrf_gpio_find_pin(pin);
    if (i < 0)
    {
        return -RT_ERROR;
    }

#if MCU_GPIO_USE_PORT_EVENT
    nrf_gpio = NRF_GPIO_PORT(pin);
    pin_index = NRF_GPIO_INDEX(pin);
    nrf_gpio->PIN_CNF[pin_index] &= ~GPIO_PIN_CNF_SENSE_Msk;

    if (enabled)
    {
        /*
         * Always set initial SENSE to opposite of current pin state to avoid
         * triggering immediately
         */
        if (nrf_gpio->IN & (1 << pin_index))
        {
            nrf_gpio->PIN_CNF[pin_index] |= GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos;
        }
        else
        {
            nrf_gpio->PIN_CNF[pin_index] |= GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos;
        }

        NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
    }
    else
    {
        for (i = 0; i < NRF_GPIO_MAX_IRQ; i++)
        {
            if (nrf_gpio_irqs[i].sense_trig != NRF_GPIO_SENSE_TRIG_NONE)
            {
                sense_enabled = 1;
                break;
            }
        }
        if (!sense_enabled)
        {
            NRF_GPIOTE->INTENCLR = GPIOTE_INTENCLR_PORT_Msk;
        }
    }
#else
    if (enabled)
    {
        NRF_GPIOTE->EVENTS_IN[i] = 0;
        NRF_GPIOTE->INTENSET = 1 << i;
    }
    else
    {
        NRF_GPIOTE->INTENCLR = 1 << i;
    }

#endif

    return RT_EOK;
}

const static struct rt_pin_ops nrf_pin_ops =
{
    nrf_pin_mode,
    nrf_pin_write,
    nrf_pin_read,

    nrf_pin_attach_irq,
    nrf_pin_detach_irq,
    nrf_pin_irq_enable
};

int rt_hw_pin_init(void)
{
    rt_err_t ret = RT_EOK;

    ret = rt_device_pin_register("pin", &nrf_pin_ops, RT_NULL);

    return ret;
}

INIT_BOARD_EXPORT(rt_hw_pin_init);
