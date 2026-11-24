#include "ultrasonic_driver.h"
#include "pinout.h"

#if CAL_USA_ULTRASSONICO_FRONTAL

static int16_t s_offset_mm = TOF_OFFSET_FRONTAL_MM;

void ultrasonic_set_offset(int16_t offset_mm) {
    s_offset_mm = offset_mm;
}

#ifdef ARDUINO
#include <Arduino.h>

static volatile uint32_t s_rise_us = 0;
static volatile uint32_t s_duration_us = 0;
static volatile bool s_waiting_fall = false;
static volatile bool s_saw_rise = false;
static volatile bool s_sample_ready = false;

static bool s_in_flight = false;
static uint32_t s_ping_start_us = 0;
static uint32_t s_last_ping_ms = 0;
static uint16_t s_last_dist_mm = 8190;
static uint16_t s_last_raw_mm = 8190;
static bool s_last_valid = false;

static void IRAM_ATTR isr_echo_change(void) {
    uint32_t now_us = (uint32_t)micros();
    if (digitalRead(PIN_ULTRA_ECHO) == HIGH) {
        s_rise_us = now_us;
        s_waiting_fall = true;
        s_saw_rise = true;
    } else if (s_waiting_fall) {
        s_duration_us = now_us - s_rise_us;
        s_waiting_fall = false;
        s_sample_ready = true;
    }
}

static void trigger_ping(uint32_t now_ms) {
    noInterrupts();
    s_waiting_fall = false;
    s_saw_rise = false;
    s_sample_ready = false;
    interrupts();

    s_last_ping_ms = now_ms;
    s_ping_start_us = (uint32_t)micros();
    s_in_flight = true;

    digitalWrite(PIN_ULTRA_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_ULTRA_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_ULTRA_TRIG, LOW);
}

static uint16_t apply_offset_clamped(uint16_t raw_mm, int16_t offset_mm) {
    int32_t cal = (int32_t)raw_mm - (int32_t)offset_mm;
    return (cal < 1) ? 1 : (uint16_t)cal;
}

static void process_echo_state(void) {
    bool ready = false;
    bool saw_rise = false;
    uint32_t duration_us = 0;

    noInterrupts();
    if (s_sample_ready) {
        ready = true;
        duration_us = s_duration_us;
        s_sample_ready = false;
    }
    saw_rise = s_saw_rise;
    interrupts();

    if (ready) {
        s_in_flight = false;
        if (duration_us == 0 || duration_us > ULTRA_TIMEOUT_US) {
            s_last_raw_mm = 8190;
            s_last_dist_mm = 8190;
            s_last_valid = true;
        } else {
            uint32_t raw = (duration_us * 343UL + 1000UL) / 2000UL;
            if (raw > 2000) {
                s_last_raw_mm = 8190;
                s_last_dist_mm = 8190;
            } else {
                uint16_t clamped = (raw < ULTRA_MIN_DIST_MM) ? ULTRA_MIN_DIST_MM : (uint16_t)raw;
                s_last_raw_mm = clamped;
                s_last_dist_mm = apply_offset_clamped(clamped, s_offset_mm);
            }
            s_last_valid = true;
        }
    } else if (s_in_flight && ((uint32_t)micros() - s_ping_start_us) > ULTRA_TIMEOUT_US) {
        s_in_flight = false;
        s_last_raw_mm = 8190;
        s_last_dist_mm = 8190;
        s_last_valid = saw_rise;
    }
}

bool ultrasonic_init(void) {
    pinMode(PIN_ULTRA_TRIG, OUTPUT);
    digitalWrite(PIN_ULTRA_TRIG, LOW);
    pinMode(PIN_ULTRA_ECHO, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_ULTRA_ECHO), isr_echo_change, CHANGE);

    trigger_ping((uint32_t)millis());
    uint32_t t0 = (uint32_t)millis();
    while (!s_saw_rise && ((uint32_t)millis() - t0) < 5) {
        delayMicroseconds(100);
    }
    return s_saw_rise;
}

bool ultrasonic_read(uint16_t* out_dist_mm, uint16_t* out_raw_mm) {
    process_echo_state();
    uint32_t now_ms = (uint32_t)millis();
    if (!s_in_flight && (now_ms - s_last_ping_ms) >= 40) {
        trigger_ping(now_ms);
    }
    if (out_dist_mm) *out_dist_mm = s_last_dist_mm;
    if (out_raw_mm) *out_raw_mm = s_last_raw_mm;
    return s_last_valid;
}

#else
bool ultrasonic_init(void) { return true; }
bool ultrasonic_read(uint16_t* out_dist_mm, uint16_t* out_raw_mm) {
    if (out_dist_mm) *out_dist_mm = 150;
    if (out_raw_mm) *out_raw_mm = 150;
    return true;
}
#endif

#else
void ultrasonic_set_offset(int16_t) {}
bool ultrasonic_init(void) { return false; }
bool ultrasonic_read(uint16_t* out_dist_mm, uint16_t* out_raw_mm) {
    if (out_dist_mm) *out_dist_mm = 8190;
    if (out_raw_mm) *out_raw_mm = 8190;
    return false;
}
#endif
