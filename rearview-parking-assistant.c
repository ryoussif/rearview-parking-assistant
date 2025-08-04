 
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/i2c.h"
#include "utils/uartstdio.h"
#include "driverlib/tm4c123gh6pm.h"
#include "driverlib/timer.h"

#define PART_TM4C1233H6PM 1
#define TRIGGER_PIN GPIO_PIN_4  // PA4
#define ECHO_PIN    GPIO_PIN_6  // PB6

#define GREEN_LED GPIO_PIN_3    // PF3
#define RED_LED   GPIO_PIN_1    // PF1
#define LED_PORT  GPIO_PORTF_BASE

#define SPEED_THRESHOLD 100     // cm/s

void InitConsole(void);
void GPIO_init(void);
void Timer0A_Capture_Init(void);
uint32_t Measure_distance(void);
void Delay_MicroSecond(int us);
void Delay(uint32_t ms);

int main(void)
{
        uint32_t last_distance_cm = 0;
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);  // 40 MHz system clock

    InitConsole();
    GPIO_init();
    Timer0A_Capture_Init();

    UARTprintf("Ultrasonic Sensor Test Start\n");

    while (1)
    {
        uint32_t ticks = Measure_distance();
        uint32_t distance_cm = (ticks * 343) / (2 * 4000);

        int32_t delta = distance_cm - last_distance_cm;
        uint32_t speed_cmps = (delta > 0 ? delta : -delta) * 5;

        last_distance_cm = distance_cm;

        UARTprintf("Distance: %u cm | Speed: %u cm/s\n", distance_cm, speed_cmps);

        if (distance_cm < 15 || speed_cmps > SPEED_THRESHOLD)
        {
            GPIOPinWrite(LED_PORT, RED_LED | GREEN_LED, RED_LED);
        }
        else if (distance_cm <= 45)
        {
            GPIOPinWrite(LED_PORT, RED_LED | GREEN_LED, RED_LED | GREEN_LED);
        }
        else
        {
            GPIOPinWrite(LED_PORT, RED_LED | GREEN_LED, GREEN_LED);
        }

        Delay(200);
    }
}

void InitConsole(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTStdioConfig(0, 9600, 16000000);
}

void GPIO_init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));

    GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, TRIGGER_PIN);
    GPIOPinWrite(GPIO_PORTA_BASE, TRIGGER_PIN, 0);

    GPIOPinConfigure(GPIO_PB6_T0CCP0);
    GPIOPinTypeTimer(GPIO_PORTB_BASE, ECHO_PIN);
    GPIOPadConfigSet(GPIO_PORTB_BASE, ECHO_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOPinTypeGPIOOutput(LED_PORT, RED_LED | GREEN_LED);
    GPIOPinWrite(LED_PORT, RED_LED | GREEN_LED, 0);
}

void Timer0A_Capture_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));

    TimerDisable(TIMER0_BASE, TIMER_A);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME_UP);
    TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 0xFFFF);
    TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT);
    TimerEnable(TIMER0_BASE, TIMER_A);
}

uint32_t Measure_distance(void)
{
    uint32_t risingEdge = 0, fallingEdge = 0, pulseWidth = 0;
    uint32_t timeout;

    // Send 10µs trigger pulse
    GPIOPinWrite(GPIO_PORTA_BASE, TRIGGER_PIN, 0);
    Delay_MicroSecond(2);
    GPIOPinWrite(GPIO_PORTA_BASE, TRIGGER_PIN, TRIGGER_PIN);
    Delay_MicroSecond(10);
    GPIOPinWrite(GPIO_PORTA_BASE, TRIGGER_PIN, 0);

    // Wait for rising edge with timeout
    TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
    TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);

    timeout = 2000000;  // ~50 ms timeout at 40 MHz
    while (!(TimerIntStatus(TIMER0_BASE, true) & TIMER_CAPA_EVENT) && timeout--)
        ;
    if (timeout == 0) return 0;  // Timeout occurred

    risingEdge = TimerValueGet(TIMER0_BASE, TIMER_A);
    TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);

    // Wait for falling edge with timeout
    TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_NEG_EDGE);

    timeout = 2000000;  // reset timeout for second wait
    while (!(TimerIntStatus(TIMER0_BASE, true) & TIMER_CAPA_EVENT) && timeout--)
        ;
    if (timeout == 0) return 0;  // Timeout occurred

    fallingEdge = TimerValueGet(TIMER0_BASE, TIMER_A);
    TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);

    if (fallingEdge > risingEdge)
        pulseWidth = fallingEdge - risingEdge;
    else
        pulseWidth = (0xFFFF - risingEdge + fallingEdge);

    return pulseWidth;
}


void Delay_MicroSecond(int us)
{
    SysCtlDelay((SysCtlClockGet() / 3 / 1000000) * us);
}

void Delay(uint32_t ms)
{
    SysCtlDelay((SysCtlClockGet() / 3 / 1000) * ms);
}