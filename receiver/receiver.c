#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "sx1276.h"

#define UART0_ID uart0
#define UART0_BAUD_RATE 115200
#define UART0_TX_PIN 0
#define UART0_RX_PIN 1

#define SPI0_ID spi0
#define SPI0_CS_PIN 17
#define SPI0_SCK_PIN 18
#define SPI0_MOSI_PIN 19
#define SPI0_MISO_PIN 16

#define DIO0_PIN 20
#define DIO1_PIN 21
#define RESET_PIN 22

int main()
{
    // stdio_init_all();
    sleep_ms(2000); // debug only to wait for serial (UART) initialized

    uart_t uart = {UART0_ID, UART0_BAUD_RATE, UART0_TX_PIN, UART0_RX_PIN};
    spi_t spi = {SPI0_ID, SPI0_CS_PIN, SPI0_SCK_PIN, SPI0_MOSI_PIN, SPI0_MISO_PIN};
    radio_t radio = sx1276_createRadio(&uart, &spi, true);

    sx1276_logInfo(&radio);

    while (true)
    {
        sx1276_receive(&radio, 0);
    }
}
