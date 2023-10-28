#include <stdio.h>
#include "hardware/uart.h"
#include "hardware/spi.h"

// Registers of LoRa mode

#define REG_FIFO 0x00
#define REG_OPMODE 0x01
#define REG_FRFMSB 0x06
#define REG_FRFMID 0x07
#define REG_FRFLSB 0x08
#define REG_PACONFIG 0x09
#define REG_PARAMP 0x0A
#define REG_OCP 0x0B
#define REG_LNA 0x0C
#define REG_FIFOADDRPTR 0x0D
#define REG_FIFOTXBASEADDR 0x0E
#define REG_FIFORXBASEADDR 0x0F
#define REG_FIFORXCURRENTADDR 0x10
#define REG_IRQFLAGSMASK 0x11
#define REG_IRQFLAGS 0x12
#define REG_RXNBBYTES 0x13
#define REG_RXHEADERCNTVALUEMSB 0x14
#define REG_RXHEADERCNTVALUELSB 0x15
#define REG_RXPACKETCNTVALUEMSB 0x16
#define REG_RXPACKETCNTVALUELSB 0x17
#define REG_MODEMSTAT 0x18
#define REG_PKTSNRVALUE 0x19
#define REG_PKTRSSIVALUE 0x1A
#define REG_RSSIVALUE 0x1B
#define REG_HOPCHANNEL 0x1C
#define REG_MODEMCONFIG1 0x1D
#define REG_MODEMCONFIG2 0x1E
#define REG_SYMBTIMEOUTLSB 0x1F
#define REG_PREAMBLEMSB 0x20
#define REG_PREAMBLELSB 0x21
#define REG_PAYLOADLENGTH 0x22
#define REG_MAXPAYLOADLENGTH 0x23
#define REG_HOPPERIOD 0x24
#define REG_FIFORXBYTEADDR 0x25
#define REG_MODEMCONFIG3 0x26
#define REG_FEIMSB 0x28
#define REG_FEIMID 0x29
#define REG_FEILSB 0x2A
#define REG_RSSIWIDEBAND 0x2C
#define REG_IFFREQ1 0x2F
#define REG_IFFREQ2 0x30
#define REG_DETECTOPTIMIZE 0x31
#define REG_INVERTIQ 0x33
#define REG_HIGHBWOPTIMIZE1 0x36
#define REG_DETECTIONTHRESHOLD 0x37
#define REG_SYNCWORD 0x39
#define REG_HIGHBWOPTIMIZE2 0x3A
#define REG_INVERTIQ2 0x3B
#define REG_DIOMAPPING1 0x40
#define REG_DIOMAPPING2 0x41
#define REG_VERSION 0x42
#define REG_TCXO 0x4B
#define REG_PADAC 0x4D
#define REG_FORMERTEMP 0x5B
#define REG_AGCREF 0x61
#define REG_AGCTHRESH1 0x62
#define REG_AGCTHRESH2 0x63
#define REG_AGCTHRESH3 0x64
#define REG_PLL 0x70

// RegOpMode
#define OPMODE_LONGRANGEMODE 0x80      // can be modified only in Sleep mode
#define OPMODE_LOWFREQUENCYMODEON 0x08 // below 525 MHz
#define OPMODE_MASK 0xF8
#define OPMODE_SLEEP_MODE 0x00
#define OPMODE_STDBY_MODE 0x01
#define OPMODE_FSTX_MODE 0x02
#define OPMODE_TX_MODE 0x03
#define OPMODE_FSRX_MODE 0x04
#define OPMODE_RXCONTINUOUS_MODE 0x05
#define OPMODE_RXSINGLE_MODE 0x06
#define OPMODE_CAD_MODE 0x07

// RegModemConfig1
#define MCFG1_BW_MASK 0b00001111
#define MCFG1_CODINGRATE_MASK 0b11110001
#define MCFG1_IMPLICITHEADERMODEON_MASK 0b11111110

// RegModemConfig2
#define MCFG2_SPREADINGFACTOR_MASK 0b00001111
#define MCFG2_TXCONTINUOUSMODE_MASK 0b11110111
#define MCFG2_RXPAYLOADCRCON_MASK 0b11111011
#define MCFG2_SYMBTIMEOUT_MASK 0b11111100

// RegIrqFlags
// #define IRQFLAGS_RXTIMEOUT_MASK 0b01111111
// #define IRQFLAGS_RXDONE_MASK 0b10111111
// #define IRQFLAGS_PAYLOADCRCERROR_MASK 0b11011111
// #define IRQFLAGS_VALIDHEADER_MASK 0b11101111
// #define IRQFLAGS_TXDONE_MASK 0b11110111
// #define IRQFLAGS_CADDONE_MASK 0b11111011
// #define IRQFLAGS_FHSSCHANGECHANNEL_MASK 0b11111101
// #define IRQFLAGS_CADDETECTED_MASK 0b11111110
#define IRQFLAGS_RXTIMEOUT_MASK 0b10000000
#define IRQFLAGS_RXDONE_MASK 0b01000000
#define IRQFLAGS_PAYLOADCRCERROR_MASK 0b00100000
#define IRQFLAGS_VALIDHEADER_MASK 0b00010000
#define IRQFLAGS_TXDONE_MASK 0b00001000
#define IRQFLAGS_CADDONE_MASK 0b00000100
#define IRQFLAGS_FHSSCHANGECHANNEL_MASK 0b00000010
#define IRQFLAGS_CADDETECTED_MASK 0b00000001

// PaConfig
#define PACFG_PASELECT_MASK 0b01111111
#define PACFG_MAXPOWER_MASK 0b10001111
#define PACFG_OUTPUTPOWER_MASK 0b11110000

// Ocp
#define OCP_ON_MASK 0b11101111
#define OCP_TRIM_MASK 0b11110000

// Lna
#define LNA_GAIN_MASK 0b00011111
#define LNA_BOOSTLF_MASK 0b11100111
#define LNA_BOOSTHF_MASK 0b11111100

// Types

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

// Interface

radio_t sx1276_createRadio(uart_t *uart, spi_t *spi, bool isLowRange);
void sx1276_logInfo(radio_t *radio);

void sx1276_configureSender(radio_t *radio, tx_cfg_t *config);
void sx1276_configureSenderWithDefaults(radio_t *radio);
void sx1276_send(radio_t *radio, uint8_t *data, size_t size);

void sx1276_configureReceiver(radio_t *radio, rx_cfg_t *config);
void sx1276_configureReceiverWithDefaults(radio_t *radio);
void sx1276_receive(radio_t *radio);
