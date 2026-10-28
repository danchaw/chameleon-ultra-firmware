#include "auto_poll.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_timer.h"
#include "rfid_main.h"
#include "settings.h"
#include "tag_emulation.h"

#define NRF_LOG_MODULE_NAME auto_poll
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

// Rotation starts once this many authentication attempts land within the quiet window.
#define AUTO_POLL_START_ATTEMPTS 2
// A reader that stopped hammering us: rotate down after this much silence.
#define AUTO_POLL_QUIET_STOP_MS 1500
// The sweep limit (in full laps over the enabled HF slots) is a runtime setting —
// settings_get_auto_poll_max_laps(), 0 = unlimited. The door almost certainly does
// not know any of our cards after that; re-arm once the reader leaves and returns.

// Written from the NFC timeslot interrupt, read from the main loop. Single-word volatile
// accesses are atomic on Cortex-M; worst case one extra/lost attempt tick, which is fine.
static volatile uint8_t m_attempt_count;
static volatile uint32_t m_last_attempt_tick;
static volatile bool m_auth_success_evt;
static volatile bool m_latched_stop;

static bool m_rotating;
static uint32_t m_last_rotate_tick;
static uint8_t m_switch_count;

void auto_poll_init(void) {
    m_attempt_count = 0;
    m_last_attempt_tick = app_timer_cnt_get();
    m_auth_success_evt = false;
    m_latched_stop = false;
    m_rotating = false;
    m_last_rotate_tick = 0;
    m_switch_count = 0;
}

void auto_poll_event(auto_poll_event_t event) {
    if (!settings_get_auto_poll_enable()) {
        return;
    }
    if (event == AUTO_POLL_EVT_AUTH_SUCCESS) {
        m_auth_success_evt = true;
        return;
    }
    // AUTH_ATTEMPT: refresh the burst window.
    uint32_t now = app_timer_cnt_get();
    if (app_timer_cnt_diff_compute(now, m_last_attempt_tick) > APP_TIMER_TICKS(AUTO_POLL_QUIET_STOP_MS)) {
        m_attempt_count = 0;
        // A reader that left and came back gets a fresh three-lap budget.
        m_latched_stop = false;
    }
    m_last_attempt_tick = now;
    if (m_attempt_count < 0xFF) {
        m_attempt_count++;
    }
}

// Next HF-enabled slot after slot_now, or slot_now when no other HF slot is enabled.
static uint8_t next_enabled_hf_slot(uint8_t slot_now) {
    for (uint8_t i = 1; i < TAG_MAX_SLOT_NUM; i++) {
        uint8_t slot = (uint8_t)((slot_now + i) % TAG_MAX_SLOT_NUM);
        if (is_slot_enabled(slot, TAG_SENSE_HF)) {
            return slot;
        }
    }
    return slot_now;
}

static uint8_t enabled_hf_slot_count(void) {
    uint8_t count = 0;
    for (uint8_t slot = 0; slot < TAG_MAX_SLOT_NUM; slot++) {
        if (is_slot_enabled(slot, TAG_SENSE_HF)) {
            count++;
        }
    }
    return count;
}

void auto_poll_process(void) {
    if (!settings_get_auto_poll_enable()) {
        m_rotating = false;
        return;
    }
    // Never touch the antenna while the device is a reader: the phone drives that flow.
    if (get_device_mode() != DEVICE_MODE_TAG) {
        return;
    }

    if (m_auth_success_evt) {
        m_auth_success_evt = false;
        m_attempt_count = 0;
        m_latched_stop = false;
        if (m_rotating) {
            m_rotating = false;
            NRF_LOG_INFO("auto_poll: authentication succeeded on slot %d, rotation stopped", tag_emulation_get_slot());
        }
        return;
    }

    uint32_t now = app_timer_cnt_get();

    if (!m_rotating) {
        if (!m_latched_stop && m_attempt_count >= AUTO_POLL_START_ATTEMPTS) {
            m_attempt_count = 0;
            m_switch_count = 0;
            m_rotating = true;
            m_last_rotate_tick = now; // first switch after one full interval
            NRF_LOG_INFO("auto_poll: reader hammering detected, rotating every %d ms", settings_get_auto_poll_interval_ms());
        }
        return;
    }

    // Rotating: the reader gave up?
    if (app_timer_cnt_diff_compute(now, m_last_attempt_tick) > APP_TIMER_TICKS(AUTO_POLL_QUIET_STOP_MS)) {
        m_rotating = false;
        NRF_LOG_INFO("auto_poll: reader quiet, rotation stopped");
        return;
    }

    if (app_timer_cnt_diff_compute(now, m_last_rotate_tick) >= APP_TIMER_TICKS(settings_get_auto_poll_interval_ms())) {
        uint8_t current = tag_emulation_get_slot();
        uint8_t next = next_enabled_hf_slot(current);
        // Lap limit: m_switch_count counts slot switches; one lap = one switch per enabled HF slot.
        uint8_t laps = settings_get_auto_poll_max_laps();
        uint8_t hf_count = enabled_hf_slot_count();
        if (laps > 0 && hf_count > 0 && m_switch_count >= (uint8_t)(laps * (hf_count > 1 ? hf_count : 1)) && next != current) {
            m_rotating = false;
            m_latched_stop = true; // stays off until the reader leaves and a fresh visit begins
            NRF_LOG_INFO("auto_poll: no match after %d laps, rotation stopped and latched", laps);
            return;
        }
        if (next != current) {
            tag_emulation_change_slot(next, true);
            m_switch_count++;
            NRF_LOG_INFO("auto_poll: rotated slot %d -> %d", current, next);
        }
        m_last_rotate_tick = app_timer_cnt_get();
    }
}

bool auto_poll_is_rotating(void) {
    return m_rotating;
}
