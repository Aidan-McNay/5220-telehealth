// =======================================================================
// lorawan_config.h
// =======================================================================
// Declarations of our LoRaWAN configurations

#ifndef LORAWAN_LORAWAN_CONFIG_H
#define LORAWAN_LORAWAN_CONFIG_H

#include "hardware/spi.h"
#include "pico/lorawan.h"
#include "pico/stdlib.h"

// -----------------------------------------------------------------------
// Pin configuration for SX1276 radio module
// -----------------------------------------------------------------------

const struct lorawan_sx1276_settings sx1276_settings = {
    .spi =
        {
            .inst = PICO_DEFAULT_SPI_INSTANCE(),
            .mosi = PICO_DEFAULT_SPI_TX_PIN,
            .miso = PICO_DEFAULT_SPI_RX_PIN,
            .sck  = PICO_DEFAULT_SPI_SCK_PIN,
            .nss  = 17,
        },
    .reset = 9,
    .dio0  = 7,
    .dio1  = 10,
};

// -----------------------------------------------------------------------
// OTAA settings
// -----------------------------------------------------------------------

// LoRaWAN region to use, full list of regions can be found at:
//   http://stackforce.github.io/LoRaMac-doc/LoRaMac-doc-v4.5.1/group___l_o_r_a_m_a_c.html#ga3b9d54f0355b51e85df8b33fd1757eec
#define LORAWAN_REGION LORAMAC_REGION_US915

// LoRaWAN Device EUI (64-bit), NULL value will use Default Dev EUI
#define LORAWAN_DEVICE_EUI "70B3D57ED006FE82"

// LoRaWAN Application / Join EUI (64-bit)
#define LORAWAN_APP_EUI "0000000000000001"

// LoRaWAN Application Key (128-bit)
#define LORAWAN_APP_KEY "0745AC235824D1428092DDDCF56A01A8"

// LoRaWAN Channel Mask, NULL value will use the default channel mask
// for the region
#define LORAWAN_CHANNEL_MASK NULL

const struct lorawan_otaa_settings otaa_settings = {
    .device_eui   = LORAWAN_DEVICE_EUI,
    .app_eui      = LORAWAN_APP_EUI,
    .app_key      = LORAWAN_APP_KEY,
    .channel_mask = LORAWAN_CHANNEL_MASK,
};

#endif  // LORAWAN_LORAWAN_CONFIG_H