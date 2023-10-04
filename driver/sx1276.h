#include <stdio.h>
#include "hardware/uart.h"
#include "hardware/spi.h"

typedef struct
{
    uart_inst_t *inst;
    uint baud_rate;
    int tx_pin;
    int rx_pin;
} uart_t;

typedef struct
{
    spi_inst_t *inst;
    uint cs_pin;
    uint sck_pin;
    uint mosi_pin;
    uint miso_pin;
} spi_t;

typedef struct
{
    uart_t *uart;
    spi_t *spi;
    uint8_t opModeDefaults;
} radio_t;

typedef struct
{

} tx_cfg_t;

typedef struct
{
    uint16_t timeout;
} rx_cfg_t;

radio_t sx1276_createRadio(uart_t *uart, spi_t *spi, bool isLowRange);
void sx1276_logInfo(radio_t *radio);

void sx1276_configureSender(radio_t *radio, tx_cfg_t *config);
void sx1276_configureSenderWithDefaults(radio_t *radio);
void sx1276_send(radio_t *radio, uint8_t *data, size_t size);

void sx1276_configureReceiver(radio_t *radio, rx_cfg_t *config);
void sx1276_configureReceiverWithDefaults(radio_t *radio);
void sx1276_receive(radio_t *radio);
