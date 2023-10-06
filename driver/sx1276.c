#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "sx1276.h"

static uint8_t sx1276_readRegister(radio_t *radio, uint8_t addr)
{
    uint8_t src = addr & 0x7F; // set wnr bit to 0 for read access
    uint8_t buffer;

    gpio_put(radio->spi->cs_pin, 0);
    spi_write_blocking(radio->spi->inst, &src, 1);
    spi_read_blocking(radio->spi->inst, 0x00, &buffer, 1);
    gpio_put(radio->spi->cs_pin, 1);

    return buffer;
}

static void sx1276_writeRegister(radio_t *radio, uint8_t addr, uint8_t data)
{
    uint8_t src[2];
    src[0] = addr | 0x80; // set wnr bit to 1 for write access
    src[1] = data;

    // uint8_t src = addr | 0x80; // set wnr bit to 1 for write access
    // uint8_t buffer;

    gpio_put(radio->spi->cs_pin, 0);
    spi_write_blocking(radio->spi->inst, src, 2);

    // spi_write_blocking(radio->spi->inst, &src, 1);
    // spi_write_read_blocking(radio->spi->inst, &data, &buffer, 1);

    gpio_put(radio->spi->cs_pin, 1);
}

static uint8_t getVersion(radio_t *radio)
{
    return sx1276_readRegister(radio, REG_VERSION);
}

static uint8_t getOperatingMode(radio_t *radio)
{
    return sx1276_readRegister(radio, REG_OPMODE);
}

static uint64_t getFrequency(radio_t *radio)
{
    uint8_t frMsb = sx1276_readRegister(radio, REG_FRFMSB);
    uint8_t frMid = sx1276_readRegister(radio, REG_FRFMID);
    uint8_t frLsb = sx1276_readRegister(radio, REG_FRFLSB);
    uint64_t combined = (frMsb << 16) + (frMid << 8) + frLsb;
    return (combined * 32000000) >> 19;
}

static uint64_t getBandwidth(radio_t *radio)
{
    uint8_t modemConfig1 = sx1276_readRegister(radio, REG_MODEMCONFIG1);
    uint8_t encoded = (modemConfig1 & ~MCFG1_BW_MASK) >> 4;
    switch (encoded)
    {
    case 0b0000:
        return 7.8 * 1000;
    case 0b0001:
        return 10.4 * 1000;
    case 0b0010:
        return 15.6 * 1000;
    case 0b0011:
        return 20.8 * 1000;
    case 0b0100:
        return 31.25 * 1000;
    case 0b0101:
        return 41.7 * 1000;
    case 0b0110:
        return 62.5 * 1000;
    case 0b0111:
        return 125 * 1000;
    case 0b1000:
        return 250 * 1000;
    case 0b1001:
        return 500 * 1000;
    default:
        return 0;
    }
}

static uint8_t getCodingRate(radio_t *radio)
{
    uint8_t modemConfig1 = sx1276_readRegister(radio, REG_MODEMCONFIG1);
    uint8_t encoded = (modemConfig1 & ~MCFG1_CODINGRATE_MASK) >> 1;
    switch (encoded)
    {
    case 0b0001:
        return 5; // means 4/5
    case 0b0010:
        return 6; // means 4/6
    case 0b0011:
        return 7; // means 4/7
    case 0b0100:
        return 8; // means 4/8
    default:
        return 0;
    }
}

static uint8_t getImplicitHeaderModeOn(radio_t *radio)
{
    uint8_t modemConfig1 = sx1276_readRegister(radio, REG_MODEMCONFIG1);
    return modemConfig1 & ~MCFG1_IMPLICITHEADERMODEON_MASK;
}

static uint8_t getSpreadingFactor(radio_t *radio)
{
    uint8_t modemConfig2 = sx1276_readRegister(radio, REG_MODEMCONFIG2);
    return (modemConfig2 & ~MCFG2_SPREADINGFACTOR_MASK) >> 4;
}

static uint8_t getTxContinuousMode(radio_t *radio)
{
    uint8_t modemConfig2 = sx1276_readRegister(radio, REG_MODEMCONFIG2);
    return (modemConfig2 & ~MCFG2_TXCONTINUOUSMODE_MASK) >> 3;
}

static uint8_t getRxPayloadCrcOn(radio_t *radio)
{
    uint8_t modemConfig2 = sx1276_readRegister(radio, REG_MODEMCONFIG2);
    return (modemConfig2 & ~MCFG2_RXPAYLOADCRCON_MASK) >> 2;
}

static uint16_t getSymbTimeout(radio_t *radio)
{
    uint8_t symbTimeoutMsb = sx1276_readRegister(radio, REG_MODEMCONFIG2) & ~MCFG2_SYMBTIMEOUT_MASK;
    uint8_t symbTimeoutLsb = sx1276_readRegister(radio, REG_SYMBTIMEOUTLSB);
    return (symbTimeoutMsb << 8) + symbTimeoutLsb;
}

static bool sx1276_isSleepMode(radio_t *radio)
{
    uint8_t opMode = getOperatingMode(radio);
    return (opMode & ~OPMODE_MASK) == OPMODE_SLEEP_MODE;
}

static void sx1276_setSleepMode(radio_t *radio)
{
    uart_puts(radio->uart->inst, "Going to SLEEP mode ");
    if (!sx1276_isSleepMode(radio))
    {
        sx1276_writeRegister(radio, REG_OPMODE, radio->opModeDefaults | OPMODE_SLEEP_MODE);
        while (!sx1276_isSleepMode(radio))
        {
            uart_puts(radio->uart->inst, ".");
        }
        uart_puts(radio->uart->inst, " Done\r\n");
    }
    else
    {
        uart_puts(radio->uart->inst, "... Already\r\n");
    }
}

static bool sx1276_isStandByMode(radio_t *radio)
{
    uint8_t opMode = getOperatingMode(radio);
    return (opMode & ~OPMODE_MASK) == OPMODE_STDBY_MODE;
}

static void sx1276_setStandByMode(radio_t *radio)
{
    uart_puts(radio->uart->inst, "Going to STANDBY mode ");
    if (!sx1276_isStandByMode(radio))
    {
        sx1276_writeRegister(radio, REG_OPMODE, radio->opModeDefaults | OPMODE_STDBY_MODE);
        while (!sx1276_isStandByMode(radio))
        {
            uart_puts(radio->uart->inst, ".");
        }
        uart_puts(radio->uart->inst, " Done\r\n");
    }
    else
    {
        uart_puts(radio->uart->inst, "... Already\r\n");
    }
}

static bool sx1276_isTxMode(radio_t *radio)
{
    uint8_t opMode = getOperatingMode(radio);
    return (opMode & ~OPMODE_MASK) == OPMODE_TX_MODE;
}

static void sx1276_setTxMode(radio_t *radio)
{
    uart_puts(radio->uart->inst, "Going to TX mode ");
    if (!sx1276_isTxMode(radio))
    {
        sx1276_writeRegister(radio, REG_OPMODE, radio->opModeDefaults | OPMODE_TX_MODE);
        while (!sx1276_isTxMode(radio))
        {
            uart_puts(radio->uart->inst, ".");
        }
        uart_puts(radio->uart->inst, " Done\r\n");
    }
    else
    {
        uart_puts(radio->uart->inst, "... Already\r\n");
    }
}

static bool sx1276_isRxMode(radio_t *radio)
{
    uint8_t opMode = getOperatingMode(radio);
    return (opMode & ~OPMODE_MASK) == OPMODE_RXSINGLE_MODE;
}

static void sx1276_setRxMode(radio_t *radio)
{
    // uart_puts(radio->uart->inst, "Going to RX mode ");
    if (!sx1276_isRxMode(radio))
    {
        sx1276_writeRegister(radio, REG_OPMODE, radio->opModeDefaults | OPMODE_RXSINGLE_MODE);
        while (!sx1276_isRxMode(radio))
        {
            // uart_puts(radio->uart->inst, ".");
        }
        // uart_puts(radio->uart->inst, " Done\r\n");
    }
    else
    {
        // uart_puts(radio->uart->inst, "... Already\r\n");
    }
}

radio_t sx1276_createRadio(uart_t *uart, spi_t *spi, bool isLowRange)
{
    // UART
    uart_init(uart->inst, uart->baud_rate);
    gpio_set_function(uart->tx_pin, GPIO_FUNC_UART);
    gpio_set_function(uart->rx_pin, GPIO_FUNC_UART);

    // SPI
    spi_init(spi->inst, 1000 * 1000);
    spi_set_format(spi->inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(spi->sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi->mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi->miso_pin, GPIO_FUNC_SPI);
    gpio_init(spi->cs_pin);
    gpio_set_dir(spi->cs_pin, GPIO_OUT);
    gpio_put(spi->cs_pin, 1);

    // RADIO
    uint8_t opModeDefaults = OPMODE_LONGRANGEMODE | (isLowRange ? OPMODE_LOWFREQUENCYMODEON : 0x00);
    radio_t radio = {uart, spi, opModeDefaults};
    sx1276_setSleepMode(&radio); // to enable Lora when change mode next time

    // TODO
    sx1276_writeRegister(&radio, REG_MODEMCONFIG2, 0b01110000);
    sx1276_writeRegister(&radio, REG_SYMBTIMEOUTLSB, 0b11111111);
    // sx1276_writeRegister(&radio, REG_IRQFLAGSMASK, 0x00);

    return radio;
}

void sx1276_logInfo(radio_t *radio)
{
    uint8_t version = getVersion(radio);
    uint8_t opMode = getOperatingMode(radio);
    uint64_t frequency = getFrequency(radio);
    uint64_t bandwidth = getBandwidth(radio);
    uint8_t codingRate = getCodingRate(radio);
    uint8_t implicitHeaderModeOn = getImplicitHeaderModeOn(radio);
    uint8_t spreadingFactor = getSpreadingFactor(radio);
    uint8_t txContinuousMode = getTxContinuousMode(radio);
    uint8_t rxPayloadCrcOn = getRxPayloadCrcOn(radio);
    uint16_t symbTimeout = getSymbTimeout(radio);

    char buffer[512];
    sprintf(buffer,
            "\r\nVersion: %u \
            \r\nOperatingMode: %u \
            \r\nFrequency: %llu \
            \r\nBandwidth: %llu \
            \r\nCodingRate: 4/%u \
            \r\nImplicitHeaderModeOn: %u \
            \r\nSpreadingFactor: %u \
            \r\nTxContinuousMode: %u \
            \r\nRxPayloadCrcOn: %u \
            \r\nSymbTimeout: %lu \
            \r\n\r\n",
            version,
            opMode,
            frequency,
            bandwidth,
            codingRate,
            implicitHeaderModeOn,
            spreadingFactor,
            txContinuousMode,
            rxPayloadCrcOn,
            symbTimeout);
    uart_puts(radio->uart->inst, buffer);
}

void sx1276_configureSender(radio_t *radio, tx_cfg_t *config)
{
}

void sx1276_configureSenderWithDefaults(radio_t *radio)
{
}

void sx1276_send(radio_t *radio, uint8_t *data, size_t size)
{
    while (sx1276_isTxMode(radio))
    {
        uart_puts(radio->uart->inst, "Wait for ongoing TX is finished...\r\n");
        sleep_ms(1);
    }
    sx1276_setStandByMode(radio);

    uint8_t addrPtr = sx1276_readRegister(radio, REG_FIFOTXBASEADDR);
    sx1276_writeRegister(radio, REG_FIFOADDRPTR, addrPtr);

    uart_puts(radio->uart->inst, "Preparing to send '");
    for (size_t i = 0; i < size; i++)
    {
        uart_putc(radio->uart->inst, data[i]);
        sx1276_writeRegister(radio, REG_FIFO, data[i]);
    }
    uart_puts(radio->uart->inst, "' ... Done\r\n");
    sx1276_writeRegister(radio, REG_PAYLOADLENGTH, size);

    sx1276_setTxMode(radio);

    // TX mode switched to STANDBY mode automatically after data is sent
    uart_puts(radio->uart->inst, "Sending ");
    while (!sx1276_isStandByMode(radio))
    {
        uart_puts(radio->uart->inst, ".");
        sleep_ms(1);
    }
    uart_puts(radio->uart->inst, " Done\r\n");
}

void sx1276_configureReceiver(radio_t *radio, rx_cfg_t *config)
{
}

void sx1276_configureReceiverWithDefaults(radio_t *radio)
{
}

void sx1276_receive(radio_t *radio)
{
    // while (sx1276_isRxMode(radio))
    // {
    //     uart_puts(radio->uart->inst, "Wait for ongoing RX is finished...\r\n");
    //     sleep_ms(1);
    // }
    // sx1276_setStandByMode(radio);

    if (!sx1276_isRxMode(radio))
    {
        // clear IRQs
        sx1276_writeRegister(radio, REG_IRQFLAGSMASK, 0b00001111);
        uint8_t irqFlagsMask = sx1276_readRegister(radio, REG_IRQFLAGSMASK);
        uart_putc(radio->uart->inst, irqFlagsMask);

        sx1276_writeRegister(radio, REG_IRQFLAGS, 0xFF);

        uint8_t addrPtr = sx1276_readRegister(radio, REG_FIFORXBASEADDR);
        sx1276_writeRegister(radio, REG_FIFOADDRPTR, addrPtr);

        sx1276_setRxMode(radio);
    }

    // RX mode switched to STANDBY mode automatically after data is received or timed out
    // uart_puts(radio->uart->inst, "Receiving ...");
    while (!sx1276_isStandByMode(radio))
    {
        // uart_puts(radio->uart->inst, ".");
        // sleep_ms(1);
    }
    // uart_puts(radio->uart->inst, " Done\r\n");

    // Ensure that ValidHeader, PayloadCrcError, RxDone and RxTimeout interrupts
    // are not asserted to ensure that packet reception has terminated successfully
    uint8_t irqFlagsMask = sx1276_readRegister(radio, REG_IRQFLAGSMASK);
    uint8_t irqFlags = sx1276_readRegister(radio, REG_IRQFLAGS);

    // while (!irqFlags)
    // {
    //     irqFlags = sx1276_readRegister(radio, REG_IRQFLAGS);
    // }

    uint8_t rxTimeout = (irqFlags & IRQFLAGS_RXTIMEOUT_MASK) >> 7;
    uint8_t rxDone = (irqFlags & IRQFLAGS_RXDONE_MASK) >> 6;
    uint8_t payloadCrcError = (irqFlags & IRQFLAGS_PAYLOADCRCERROR_MASK) >> 5;
    uint8_t validHeader = (irqFlags & IRQFLAGS_VALIDHEADER_MASK) >> 4;

    uint8_t rxHeaderCntValueMsb = sx1276_readRegister(radio, REG_RXHEADERCNTVALUEMSB);
    uint8_t rxHeaderCntValueLsb = sx1276_readRegister(radio, REG_RXHEADERCNTVALUELSB);
    uint16_t rxHeaderCntValue = (rxHeaderCntValueMsb << 8) + rxHeaderCntValueLsb;

    uint8_t rxPacketCntValueMsb = sx1276_readRegister(radio, REG_RXPACKETCNTVALUEMSB);
    uint8_t rxPacketCntValueLsb = sx1276_readRegister(radio, REG_RXPACKETCNTVALUELSB);
    uint16_t rxPacketCntValue = (rxPacketCntValueMsb << 8) + rxPacketCntValueLsb;

    uint8_t rxNbBytes = sx1276_readRegister(radio, REG_RXNBBYTES);

    if (rxTimeout)
    {
        uart_puts(radio->uart->inst, "Timeout error\r\n");
    }
    else if (rxDone)
    {
        if (!validHeader)
        {
            uart_puts(radio->uart->inst, "Header error\r\n");
        }
        else if (payloadCrcError)
        {
            uart_puts(radio->uart->inst, "Payload error\r\n");
        }
        else
        {
            // uart_puts(radio->uart->inst, "Reception successful\r\n");

            char buffer[512];
            sprintf(buffer,
                    "IrqFlagsMask: %u \
                    \r\nIrqFlags: %u \
                    \r\nRxTimeout: %u \
                    \r\nRxDone: %u \
                    \r\nPayloadCrcError: %u \
                    \r\nValidHeader: %u \
                    \r\nRxHeaderCntValue: %u \
                    \r\nRxPacketCntValue: %u \
                    \r\nRxNbBytes: %u \
                    \r\n",
                    irqFlagsMask,
                    irqFlags,
                    rxTimeout,
                    rxDone,
                    payloadCrcError,
                    validHeader,
                    rxHeaderCntValue,
                    rxPacketCntValue,
                    rxNbBytes);
            // uart_puts(radio->uart->inst, buffer);

            uint8_t addrPtr = sx1276_readRegister(radio, REG_FIFORXCURRENTADDR);
            // uint8_t addrPtr = sx1276_readRegister(radio, REG_FIFORXBYTEADDR) - rxNbBytes;
            sx1276_writeRegister(radio, REG_FIFOADDRPTR, addrPtr);

            // uart_puts(radio->uart->inst, "Reading data ...");
            uint8_t data[rxNbBytes];
            for (uint8_t i = 0; i < rxNbBytes; i++)
            {
                data[i] = sx1276_readRegister(radio, REG_FIFO);
            }
            // uart_puts(radio->uart->inst, " Done\r\n");

            uart_puts(radio->uart->inst, "Received: ");
            uart_puts(radio->uart->inst, data);
            uart_puts(radio->uart->inst, "\r\n\r\n");
        }

        // sx1276_writeRegister(radio, REG_FIFOADDRPTR, sx1276_readRegister(radio, REG_FIFORXBASEADDR));
    }
}
