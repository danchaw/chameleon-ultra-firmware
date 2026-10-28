#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

#include "utils.h"

#define SETTINGS_CURRENT_VERSION 8
#define SETTINGS_SLEEP_TIMEOUT_DEFAULT_S 8   // default wake timeout in seconds (matches SLEEP_DELAY_MS_BUTTON_WAKEUP)
#define SETTINGS_SLEEP_TIMEOUT_MIN_S      5
#define SETTINGS_SLEEP_TIMEOUT_MAX_S      60
#define SETTINGS_LONG_PRESS_THRESHOLD_DEFAULT_MS 1000
#define SETTINGS_LONG_PRESS_THRESHOLD_MIN_MS     200
#define SETTINGS_AUTO_POLL_INTERVAL_DEFAULT_MS 100
#define SETTINGS_AUTO_POLL_INTERVAL_MIN_MS      50
#define SETTINGS_AUTO_POLL_INTERVAL_MAX_MS      500
// Full sweeps over the enabled HF slots before the poller gives up; 0 = unlimited.
#define SETTINGS_AUTO_POLL_LAPS_DEFAULT 3
#define SETTINGS_AUTO_POLL_LAPS_MAX     10
#define BLE_PAIRING_KEY_LEN 6
#define DEFAULT_BLE_PAIRING_KEY "123456"  // length must == 6

typedef enum {
    SettingsAnimationModeFull = 0U,
    SettingsAnimationModeMinimal = 1U,
    SettingsAnimationModeSymmetric = 3U,
    SettingsAnimationModeNone = 2U,
    SettingsAnimationModeMAX = 4U,
} settings_animation_mode_t;

typedef enum {
    // Set this button to have no function
    //    (But always can wakeup device, why didn't to disable this function? i dont known, you can ask chatgpt.)
    SettingsButtonDisable = 0U,
    // Card slot number sequence will increase after pressing
    SettingsButtonCycleSlot = 1U,
    // Card slot number sequence decreases after pressing
    SettingsButtonCycleSlotDec = 2U,
    // Read the UID card number immediately after pressing, continue searching, and simulate immediately after reading the card
    SettingsButtonCloneIcUid = 3U,
    SettingsButtonShowBattery = 4U,
    // Toggle NFC field generator on/off (Ultra only, must be in reader mode)
    SettingsButtonNfcFieldGenerator = 5U,
} settings_button_function_t;

typedef struct ALIGN_U32 {
    uint16_t version;

    // 1 byte
    uint8_t animation_config : 2;
    uint8_t ble_pairing_enable : 1;
    uint8_t auto_poll_enable : 1; // rotate HF slots while a reader is authenticating (added on version8)
    uint8_t auto_poll_laps : 4;   // sweep limit, 0 = unlimited (added on version8; fills the old reserved0 nibble)

    // 1 byte
    uint8_t button_a_press : 4;
    uint8_t button_b_press : 4;

    // 1 byte
    uint8_t button_a_long_press : 4;
    uint8_t button_b_long_press : 4;

    // 6 byte
    uint8_t ble_connect_key[6];

    // 1 byte (add on version6)
    uint8_t sleep_timeout; // wake timeout in seconds after button wakeup

    // 2 bytes (add on version7)
    uint16_t long_press_threshold; // long button press threshold in ms

    // 2 bytes (add on version8)
    uint16_t auto_poll_interval_ms; // auto slot polling interval in ms

    /*
     * Warning !!!!!!!!!!!!!!!!!!!!!! <-------------
     * If you need to add settings,
     * please be sure to consult the documentation of the bit field
     * and fully use the space of this structure before considering reallocating memory space.
     */
} settings_data_t;

void settings_init_config(void);
void settings_migrate(void);
void settings_load_config(void);
uint8_t settings_save_config(void);
uint8_t settings_get_animation_config(void);
void settings_set_animation_config(uint8_t value);
uint8_t settings_get_button_press_config(char which);
uint8_t settings_get_long_button_press_config(char which);
void settings_set_button_press_config(char which, uint8_t value);
void settings_set_long_button_press_config(char which, uint8_t value);
bool is_settings_button_type_valid(char type);
uint8_t *settings_get_ble_connect_key(void);
void settings_set_ble_connect_key(uint8_t *key);
void settings_set_ble_pairing_enable(bool enable);
bool settings_get_ble_pairing_enable(void);
bool settings_get_ble_pairing_enable_first_load(void);
uint32_t settings_get_sleep_timeout(void);
void settings_set_sleep_timeout(uint8_t seconds);
void settings_init_sleep_timeout_config(void);
uint16_t settings_get_long_press_threshold(void);
void settings_set_long_press_threshold(uint16_t duration);
void settings_init_long_press_threshold_config(void);
bool settings_get_auto_poll_enable(void);
uint16_t settings_get_auto_poll_interval_ms(void);
uint8_t settings_get_auto_poll_max_laps(void);
void settings_set_auto_poll_config(bool enable, uint16_t interval_ms, uint8_t max_laps);
void settings_init_auto_poll_config(void);
#endif
