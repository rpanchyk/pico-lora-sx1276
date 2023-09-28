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

radio_t sx1276_createRadio(uart_t *uart, spi_t *spi, bool isLowRange);

uint8_t sx1276_readRegister(radio_t *radio, uint8_t addr);
void sx1276_writeRegister(radio_t *radio, uint8_t addr, uint8_t data);

void sx1276_getInfo(radio_t *radio, char *buffer);
void sx1276_logInfo(radio_t *radio);

bool sx1276_isSleepMode(radio_t *radio);
bool sx1276_isStandByMode(radio_t *radio);
bool sx1276_isTxMode(radio_t *radio);
bool sx1276_isRxMode(radio_t *radio);
void sx1276_setSleepMode(radio_t *radio);
void sx1276_setStandByMode(radio_t *radio);
void sx1276_setTxMode(radio_t *radio);
void sx1276_setRxMode(radio_t *radio);

void sx1276_send(radio_t *radio, uint8_t *data, size_t size);
void sx1276_receive(radio_t *radio, uint16_t timeout);
