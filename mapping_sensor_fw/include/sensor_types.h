#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SENSOR_CHANNELS 20u
#define SENSOR_SAMPLES 4u
#define SENSOR_ALL ((1UL << SENSOR_CHANNELS) - 1UL)

typedef enum {
    FW_OK, FW_BUSY, FW_INVALID_ARGUMENT, FW_INVALID_STATE,
    FW_TIMEOUT, FW_HARDWARE_ERROR
} FwStatus;

/* Numeric values preserve the existing STATUS UART interface. */
typedef enum {
    SCAN_IDLE, SCAN_SELECT_WAIT, SCAN_OFF_ACQUIRE,
    SCAN_LIGHT_WAIT, SCAN_ON_ACQUIRE, SCAN_FAULT
} ScanState;

typedef struct {
    uint16_t dac[SENSOR_CHANNELS];
    int16_t offset[SENSOR_CHANNELS];
    uint16_t gain_q12[SENSOR_CHANNELS];
    int16_t enter[SENSOR_CHANNELS];
    int16_t leave[SENSOR_CHANNELS];
    uint16_t select_us, light_us, sample_us;
    uint8_t confirm, polarity;
} SensorConfig;

typedef struct {
    uint32_t sequence, config_version, valid, detected, saturated;
    uint16_t off[SENSOR_CHANNELS], on[SENSOR_CHANNELS];
    int32_t signal[SENSOR_CHANNELS];
} SensorFrame;

/* A frame owns its configuration until the consumer releases it. */
typedef struct {
    SensorFrame values;
    SensorConfig config;
} RawFrame;

typedef struct {
    ScanState state;
    uint8_t channel;
    bool continuous;
    uint32_t fault, frames, config_version, rx_dropped, tx_dropped;
} AppStatus;
#endif
