// Autodetected polling: rotate HF card slots while a reader is authenticating against us.
//
// Motivation: access-control readers keep trying different keys/cards at the door. When the
// Chameleon rotates through the enabled HF slots on its own, the reader eventually meets the
// card it expects and unlocks - without a phone present. Rotation starts only after repeated
// authentication attempts are observed, stops on a successful authentication, and stops again
// when the reader goes quiet. Disabled by default (stock behaviour preserved).
#ifndef AUTO_POLL_H
#define AUTO_POLL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    AUTO_POLL_EVT_AUTH_ATTEMPT = 0, // a reader started an authentication attempt against us
    AUTO_POLL_EVT_AUTH_SUCCESS,     // an authentication passed: the current card is the right one
} auto_poll_event_t;

// Call once from main() after the peripherals are up.
void auto_poll_init(void);

// May be called from the NFC timeslot / radio interrupt context (only touches volatile state).
void auto_poll_event(auto_poll_event_t event);

// Call from the main application loop; performs the actual slot switching.
void auto_poll_process(void);

// Runtime status, for the BLE GET command.
bool auto_poll_is_rotating(void);

#endif // AUTO_POLL_H
