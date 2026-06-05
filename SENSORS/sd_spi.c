#include "sd_spi.h"
#include "DEV_Config.h"
#include <string.h>
#include <stdio.h>

extern SPI_HandleTypeDef hspi1;

#define SD_DUMMY        0xFF
#define SD_TIMEOUT      5000

static uint8_t sd_type = 0;
static uint8_t last_cmd0_resp = 0;

uint8_t SD_GetLastCmd0Resp(void)
{
    return last_cmd0_resp;
}

static void cs_low(void)
{
    HAL_GPIO_WritePin(SD_CARD_CS_PORT, SD_CARD_CS_PIN, GPIO_PIN_RESET);
}

static void cs_high(void)
{
    HAL_GPIO_WritePin(SD_CARD_CS_PORT, SD_CARD_CS_PIN, GPIO_PIN_SET);
}

static void sd_set_speed(uint32_t prescaler)
{
    hspi1.Init.BaudRatePrescaler = prescaler;
    HAL_SPI_Init(&hspi1);
}

static uint8_t spi_xfer(uint8_t data)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

static uint8_t spi_rx(void)
{
    return spi_xfer(SD_DUMMY);
}

static void spi_rx_buf(uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
        buf[i] = spi_xfer(SD_DUMMY);
}

static uint8_t send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t response, retry = 0xFF;

    spi_xfer(cmd | 0x40);
    spi_xfer((uint8_t)(arg >> 24));
    spi_xfer((uint8_t)(arg >> 16));
    spi_xfer((uint8_t)(arg >> 8));
    spi_xfer((uint8_t)(arg));
    spi_xfer(crc | 0x01);

    do
    {
        response = spi_rx();
    } while ((response & 0x80) && --retry);

    return response;
}

int SD_SPI_Init(void)
{
    uint8_t r;
    uint32_t timer;
    int ret = 0;
    uint8_t i;
    int retries;

    /* TXB0104 needs higher speed — use 4 MHz */
    sd_set_speed(SPI_BAUDRATEPRESCALER_16);

    cs_high();
    for (i = 0; i < 10; i++)
        spi_xfer(0xFF);

    /* CMD0 — retry up to 100 times like ST driver */
    retries = 0;
    do
    {
        cs_low();
        r = send_cmd(0, 0, 0x95);
        cs_high();
        spi_xfer(0xFF);
        last_cmd0_resp = r;
        if (r == 0x01)
            break;
        retries++;
    } while (retries < 100);

    if (r != 0x01)
    {
        ret = -1;
        goto done;
    }

    /* CMD8 — check SD version */
    cs_low();
    r = send_cmd(8, 0x000001AA, 0x87);
    uint8_t r7[4];
    for (i = 0; i < 4; i++)
        r7[i] = spi_rx();
    cs_high();
    spi_xfer(0xFF);

    if (r == 0x01 && r7[2] == 0x01 && r7[3] == 0xAA)
    {
        /* SD v2+ */
        printf("  Card is SD v2+, initializing...\r\n");
        timer = HAL_GetTick();
        int acmd_count = 0;
        do
        {
            cs_low();
            uint8_t r55 = send_cmd(55, 0, 0xFF);
            cs_high();
            for (int i = 0; i < 4; i++)
                spi_xfer(0xFF);

            cs_low();
            r = send_cmd(41, 0x40000000, 0xFF);

            /* Wait for card to release busy (MISO goes high) */
            uint32_t busy_start = HAL_GetTick();
            uint8_t busy;
            do
            {
                busy = spi_rx();
            } while (busy == 0x00 && (HAL_GetTick() - busy_start) < 1000);
            cs_high();
            for (int i = 0; i < 4; i++)
                spi_xfer(0xFF);

            acmd_count++;
            if (acmd_count <= 5 || (acmd_count % 100) == 0)
                printf("  ACMD41 #%d: CMD55=0x%02X, ACMD41=0x%02X\r\n", acmd_count, r55, r);

            if ((HAL_GetTick() - timer) > SD_TIMEOUT)
            {
                printf("  ACMD41 timeout after %d attempts\r\n", acmd_count);
                ret = -3;
                goto done;
            }
        } while (r == 0x01);

        if (r != 0x00)
        {
            ret = -3;
            goto done;
        }

        /* CMD58 — read OCR */
        cs_low();
        r = send_cmd(58, 0, 0xFF);
        uint8_t ocr[4];
        for (i = 0; i < 4; i++)
            ocr[i] = spi_rx();
        cs_high();
        spi_xfer(0xFF);
        sd_type = (ocr[0] & 0x40) ? 2 : 1;
    }
    else if ((r & 0x04) == 0x04)
    {
        /* CMD8 illegal — SD v1 or MMC */
        printf("  CMD8 illegal, trying SD v1...\r\n");
        timer = HAL_GetTick();
        int acmd_count = 0;
        do
        {
            cs_low();
            send_cmd(55, 0, 0xFF);
            cs_high();
            for (int i = 0; i < 4; i++)
                spi_xfer(0xFF);

            cs_low();
            r = send_cmd(41, 0, 0xFF);

            /* Wait for card to release busy */
            uint32_t busy_start = HAL_GetTick();
            uint8_t busy;
            do
            {
                busy = spi_rx();
            } while (busy == 0x00 && (HAL_GetTick() - busy_start) < 1000);
            cs_high();
            for (int i = 0; i < 4; i++)
                spi_xfer(0xFF);

            acmd_count++;
            if ((HAL_GetTick() - timer) > SD_TIMEOUT)
            {
                ret = -4;
                goto done;
            }
        } while (r == 0x01);
        sd_type = 0;
    }
    else
    {
        ret = -2;
        goto done;
    }

    /* CMD16 — set block size */
    cs_low();
    r = send_cmd(16, SD_BLOCK_SIZE, 0xFF);
    cs_high();
    spi_xfer(0xFF);
    if (r != 0x00)
    {
        ret = -5;
        goto done;
    }

done:
    cs_high();
    return ret;
}

int SD_SPI_ReadBlock(uint32_t block, uint8_t *buf)
{
    if (sd_type == 2)
        block <<= 9;

    cs_low();
    uint8_t r = send_cmd(17, block, 0xFF);
    cs_high();
    spi_xfer(0xFF);
    if (r != 0x00)
        return -1;

    uint32_t timer = HAL_GetTick();
    uint8_t token;
    do
    {
        token = spi_rx();
        if (token == 0xFE)
            break;
    } while ((HAL_GetTick() - timer) < 200);

    if (token != 0xFE)
    {
        cs_high();
        spi_xfer(0xFF);
        return -2;
    }

    spi_rx_buf(buf, SD_BLOCK_SIZE);
    spi_rx(); /* CRC */
    spi_rx();

    cs_high();
    spi_xfer(0xFF);

    return 0;
}

int SD_SPI_WriteBlock(uint32_t block, const uint8_t *buf)
{
    if (sd_type == 2)
        block <<= 9;

    cs_low();
    uint8_t r = send_cmd(24, block, 0xFF);
    cs_high();
    spi_xfer(0xFF);
    if (r != 0x00)
        return -1;

    cs_low();
    spi_xfer(0xFE); /* Data token */
    for (uint16_t i = 0; i < SD_BLOCK_SIZE; i++)
        spi_xfer(buf[i]);
    spi_xfer(0xFF); /* CRC */
    spi_xfer(0xFF);

    if ((spi_rx() & 0x1F) != 0x05)
    {
        cs_high();
        spi_xfer(0xFF);
        return -2;
    }

    while (spi_rx() == 0); /* Wait for busy end */
    cs_high();
    spi_xfer(0xFF);

    return 0;
}
