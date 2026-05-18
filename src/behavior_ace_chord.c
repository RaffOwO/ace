/*
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_ace_chord

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ACE_HID_USAGE_KEY 0x07
#define ACE_HID_USAGE_CONSUMER 0x0C
#define ACE_HID(page, id) (((page) << 16) | (id))
#define ACE_KEY(id) ACE_HID(ACE_HID_USAGE_KEY, (id))
#define ACE_CONSUMER(id) ACE_HID(ACE_HID_USAGE_CONSUMER, (id))
#define ACE_MOD_LSFT 0x02
#define ACE_LS(keycode) ((ACE_MOD_LSFT << 24) | (keycode))

#define KC_A ACE_KEY(0x04)
#define KC_B ACE_KEY(0x05)
#define KC_C ACE_KEY(0x06)
#define KC_D ACE_KEY(0x07)
#define KC_E ACE_KEY(0x08)
#define KC_F ACE_KEY(0x09)
#define KC_G ACE_KEY(0x0A)
#define KC_H ACE_KEY(0x0B)
#define KC_I ACE_KEY(0x0C)
#define KC_J ACE_KEY(0x0D)
#define KC_K ACE_KEY(0x0E)
#define KC_L ACE_KEY(0x0F)
#define KC_M ACE_KEY(0x10)
#define KC_N ACE_KEY(0x11)
#define KC_O ACE_KEY(0x12)
#define KC_P ACE_KEY(0x13)
#define KC_Q ACE_KEY(0x14)
#define KC_R ACE_KEY(0x15)
#define KC_S ACE_KEY(0x16)
#define KC_T ACE_KEY(0x17)
#define KC_U ACE_KEY(0x18)
#define KC_V ACE_KEY(0x19)
#define KC_W ACE_KEY(0x1A)
#define KC_X ACE_KEY(0x1B)
#define KC_Y ACE_KEY(0x1C)
#define KC_Z ACE_KEY(0x1D)

#define KC_N1 ACE_KEY(0x1E)
#define KC_N2 ACE_KEY(0x1F)
#define KC_N3 ACE_KEY(0x20)
#define KC_N4 ACE_KEY(0x21)
#define KC_N5 ACE_KEY(0x22)
#define KC_N6 ACE_KEY(0x23)
#define KC_N7 ACE_KEY(0x24)
#define KC_N8 ACE_KEY(0x25)
#define KC_N9 ACE_KEY(0x26)
#define KC_N0 ACE_KEY(0x27)

#define KC_MINUS ACE_KEY(0x2D)
#define KC_EQUAL ACE_KEY(0x2E)
#define KC_LBKT ACE_KEY(0x2F)
#define KC_RBKT ACE_KEY(0x30)
#define KC_BSLH ACE_KEY(0x31)
#define KC_SEMI ACE_KEY(0x33)
#define KC_SQT ACE_KEY(0x34)
#define KC_GRAVE ACE_KEY(0x35)
#define KC_COMMA ACE_KEY(0x36)
#define KC_DOT ACE_KEY(0x37)
#define KC_FSLH ACE_KEY(0x38)

#define KC_F1 ACE_KEY(0x3A)
#define KC_F2 ACE_KEY(0x3B)
#define KC_F3 ACE_KEY(0x3C)
#define KC_F4 ACE_KEY(0x3D)
#define KC_F5 ACE_KEY(0x3E)
#define KC_F6 ACE_KEY(0x3F)
#define KC_F7 ACE_KEY(0x40)
#define KC_F8 ACE_KEY(0x41)
#define KC_F9 ACE_KEY(0x42)
#define KC_F10 ACE_KEY(0x43)
#define KC_F11 ACE_KEY(0x44)
#define KC_F12 ACE_KEY(0x45)

#define KC_EXCL (ACE_LS(ACE_KEY(0x1E)))
#define KC_AT (ACE_LS(ACE_KEY(0x1F)))
#define KC_HASH (ACE_LS(ACE_KEY(0x20)))
#define KC_DLLR (ACE_LS(ACE_KEY(0x21)))
#define KC_PRCNT (ACE_LS(ACE_KEY(0x22)))
#define KC_CARET (ACE_LS(ACE_KEY(0x23)))
#define KC_AMPS (ACE_LS(ACE_KEY(0x24)))
#define KC_STAR (ACE_LS(ACE_KEY(0x25)))
#define KC_LPAR (ACE_LS(ACE_KEY(0x26)))
#define KC_RPAR (ACE_LS(ACE_KEY(0x27)))
#define KC_UNDER (ACE_LS(KC_MINUS))
#define KC_PLUS (ACE_LS(KC_EQUAL))
#define KC_LBRC (ACE_LS(KC_LBKT))
#define KC_RBRC (ACE_LS(KC_RBKT))
#define KC_PIPE (ACE_LS(KC_BSLH))
#define KC_COLON (ACE_LS(KC_SEMI))
#define KC_DQT (ACE_LS(KC_SQT))
#define KC_LT (ACE_LS(KC_COMMA))
#define KC_GT (ACE_LS(KC_DOT))
#define KC_QMARK (ACE_LS(KC_FSLH))

#define KC_C_NEXT ACE_CONSUMER(0x00B5)
#define KC_C_PREV ACE_CONSUMER(0x00B6)
#define KC_C_VOL_UP ACE_CONSUMER(0x00E9)
#define KC_C_VOL_DN ACE_CONSUMER(0x00EA)
#define KC_C_MUTE ACE_CONSUMER(0x00E2)

#define ACE_CHORD_SIDE_LEFT 0
#define ACE_CHORD_SIDE_RIGHT 1
#define ACE_CHORD_SIDES 2

#define ACE_LAYER_BASE 0
#define ACE_LAYER_NUM 1
#define ACE_LAYER_SYM 2
#define ACE_LAYER_FUNC 3

#define ACE_CHORD_NONE 0

struct behavior_ace_chord_config {
    int timeout_ms;
};

struct ace_chord_entry {
    uint16_t mask;
    uint32_t keycode;
};

struct ace_chord_state {
    uint16_t mask;
    uint32_t keycode;
    const struct behavior_ace_chord_config *config;
    bool sent;
    bool timer_cancelled;
    struct k_work_delayable timeout_work;
};

static struct ace_chord_state chord_states[ACE_CHORD_SIDES];

static const struct ace_chord_entry base_entries[] = {
    {BIT(1), KC_S},     {BIT(2), KC_N},     {BIT(3), KC_I},     {BIT(4), KC_A},
    {BIT(5), KC_O},     {BIT(6), KC_T},     {BIT(7), KC_E},
    {BIT(3) | BIT(7), KC_D},
    {BIT(2) | BIT(6), KC_L},
    {BIT(1) | BIT(5), KC_B},
    {BIT(3) | BIT(2), KC_R},
    {BIT(2) | BIT(1), KC_C},
    {BIT(7) | BIT(6), KC_H},
    {BIT(6) | BIT(5), KC_U},
    {BIT(3) | BIT(1), KC_W},
    {BIT(7) | BIT(5), KC_M},
    {BIT(1) | BIT(7), KC_Z},
    {BIT(3) | BIT(5), KC_X},
    {BIT(7) | BIT(2), KC_P},
    {BIT(5) | BIT(2), KC_G},
    {BIT(6) | BIT(3), KC_F},
    {BIT(4) | BIT(5), KC_Y},
    {BIT(6) | BIT(1), KC_V},
    {BIT(4) | BIT(3), KC_Q},
    {BIT(4) | BIT(7), KC_J},
    {BIT(4) | BIT(1), KC_K},
};

static const struct ace_chord_entry num_entries[] = {
    {BIT(1), KC_N9},    {BIT(2), KC_N8},    {BIT(3), KC_N7},    {BIT(4), KC_N0},
    {BIT(5), KC_N3},    {BIT(6), KC_N2},    {BIT(7), KC_N1},
    {BIT(3) | BIT(7), KC_N4},
    {BIT(2) | BIT(6), KC_N5},
    {BIT(1) | BIT(5), KC_N6},
    {BIT(3) | BIT(2), KC_PLUS},
    {BIT(2) | BIT(1), KC_MINUS},
    {BIT(7) | BIT(6), KC_STAR},
    {BIT(6) | BIT(5), KC_FSLH},
    {BIT(3) | BIT(1), KC_EQUAL},
    {BIT(7) | BIT(2), KC_LPAR},
    {BIT(6) | BIT(3), KC_RPAR},
    {BIT(1) | BIT(7), KC_PRCNT},
    {BIT(3) | BIT(5), KC_CARET},
    {BIT(5) | BIT(2), KC_LT},
    {BIT(6) | BIT(1), KC_GT},
    {BIT(4) | BIT(5), KC_DOT},
    {BIT(4) | BIT(3), KC_COMMA},
    {BIT(7) | BIT(5), KC_COLON},
    {BIT(4) | BIT(7), KC_SEMI},
    {BIT(4) | BIT(1), KC_UNDER},
};

static const struct ace_chord_entry sym_entries[] = {
    {BIT(1), KC_RPAR},  {BIT(2), KC_COLON}, {BIT(3), KC_LPAR},  {BIT(4), KC_SQT},
    {BIT(5), KC_RBKT},  {BIT(6), KC_EQUAL}, {BIT(7), KC_LBKT},
    {BIT(2) | BIT(6), KC_SEMI},
    {BIT(4) | BIT(5), KC_DQT},
    {BIT(4) | BIT(1), KC_GRAVE},
    {BIT(7) | BIT(6), KC_LBRC},
    {BIT(6) | BIT(5), KC_RBRC},
    {BIT(3) | BIT(2), KC_LT},
    {BIT(2) | BIT(1), KC_GT},
    {BIT(7) | BIT(2), KC_FSLH},
    {BIT(6) | BIT(3), KC_BSLH},
    {BIT(3) | BIT(7), KC_PIPE},
    {BIT(1) | BIT(5), KC_AMPS},
    {BIT(3) | BIT(1), KC_MINUS},
    {BIT(7) | BIT(5), KC_UNDER},
    {BIT(1) | BIT(7), KC_EXCL},
    {BIT(3) | BIT(5), KC_QMARK},
    {BIT(5) | BIT(2), KC_HASH},
    {BIT(1) | BIT(6), KC_AT},
    {BIT(3) | BIT(4), KC_DLLR},
};

static const struct ace_chord_entry func_entries[] = {
    {BIT(1), KC_F9},    {BIT(2), KC_F8},    {BIT(3), KC_F7},    {BIT(4), KC_F10},
    {BIT(5), KC_F3},    {BIT(6), KC_F2},    {BIT(7), KC_F1},
    {BIT(3) | BIT(7), KC_F4},
    {BIT(2) | BIT(6), KC_F5},
    {BIT(1) | BIT(5), KC_F6},
    {BIT(4) | BIT(7), KC_F11},
    {BIT(4) | BIT(5), KC_F12},
    {BIT(3) | BIT(2), KC_C_PREV},
    {BIT(2) | BIT(1), KC_C_NEXT},
    {BIT(7) | BIT(6), KC_C_VOL_DN},
    {BIT(6) | BIT(5), KC_C_VOL_UP},
    {BIT(1) | BIT(7), KC_C_MUTE},
};

static int send_keycode(uint32_t keycode, bool pressed, int64_t timestamp) {
    return raise_zmk_keycode_state_changed_from_encoded(keycode, pressed, timestamp);
}

static int tap_keycode(uint32_t keycode, int64_t timestamp) {
    int err = send_keycode(keycode, true, timestamp);
    if (err < 0) {
        return err;
    }

    return send_keycode(keycode, false, timestamp);
}

static uint32_t resolve_keycode(uint8_t layer, uint16_t mask) {
    const struct ace_chord_entry *entries = NULL;
    size_t len = 0;

    switch (layer) {
    case ACE_LAYER_BASE:
        entries = base_entries;
        len = ARRAY_SIZE(base_entries);
        break;
    case ACE_LAYER_NUM:
        entries = num_entries;
        len = ARRAY_SIZE(num_entries);
        break;
    case ACE_LAYER_SYM:
        entries = sym_entries;
        len = ARRAY_SIZE(sym_entries);
        break;
    case ACE_LAYER_FUNC:
        entries = func_entries;
        len = ARRAY_SIZE(func_entries);
        break;
    default:
        return ACE_CHORD_NONE;
    }

    for (size_t i = 0; i < len; i++) {
        if (entries[i].mask == mask) {
            return entries[i].keycode;
        }
    }

    return ACE_CHORD_NONE;
}

static void clear_state(struct ace_chord_state *state) {
    state->mask = 0;
    state->keycode = ACE_CHORD_NONE;
    state->config = NULL;
    state->sent = false;
    state->timer_cancelled = false;
}

static void cancel_timer(struct ace_chord_state *state) {
    int cancel_result = k_work_cancel_delayable(&state->timeout_work);
    state->timer_cancelled = cancel_result > 0;
}

static int side_from_position(uint32_t position) {
    return position < 13 ? ACE_CHORD_SIDE_LEFT : ACE_CHORD_SIDE_RIGHT;
}

static void ace_chord_timeout_handler(struct k_work *item) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(item);
    struct ace_chord_state *state = CONTAINER_OF(d_work, struct ace_chord_state, timeout_work);

    if (state->mask == 0 || state->config == NULL || state->sent) {
        return;
    }

    if (state->timer_cancelled) {
        state->timer_cancelled = false;
        return;
    }

    if (state->keycode == ACE_CHORD_NONE) {
        clear_state(state);
        return;
    }

    state->sent = true;
    send_keycode(state->keycode, true, k_uptime_get());
}

static int on_ace_chord_pressed(struct zmk_behavior_binding *binding,
                                struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_ace_chord_config *config = dev->config;
    const uint8_t layer = binding->param1;
    const uint8_t key = binding->param2;
    const uint16_t key_mask = BIT(key);
    struct ace_chord_state *state = &chord_states[side_from_position(event.position)];

    if (key > 12) {
        return -EINVAL;
    }

    if (state->sent && state->keycode != ACE_CHORD_NONE) {
        send_keycode(state->keycode, false, event.timestamp);
        clear_state(state);
    }

    if (state->mask == 0) {
        state->config = config;
        state->keycode = ACE_CHORD_NONE;
        state->sent = false;
        state->timer_cancelled = false;
    }

    state->mask |= key_mask;
    state->keycode = resolve_keycode(layer, state->mask);
    k_work_reschedule(&state->timeout_work, K_MSEC(config->timeout_ms));

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_ace_chord_released(struct zmk_behavior_binding *binding,
                                 struct zmk_behavior_binding_event event) {
    const uint8_t key = binding->param2;
    const uint16_t key_mask = BIT(key);
    struct ace_chord_state *state = &chord_states[side_from_position(event.position)];

    if (key > 12 || state->mask == 0) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    if (state->sent) {
        state->mask &= ~key_mask;
        if (state->mask == 0 && state->keycode != ACE_CHORD_NONE) {
            send_keycode(state->keycode, false, event.timestamp);
            clear_state(state);
        }
        return ZMK_BEHAVIOR_OPAQUE;
    }

    uint32_t keycode = state->keycode;
    cancel_timer(state);
    clear_state(state);

    if (keycode != ACE_CHORD_NONE) {
        tap_keycode(keycode, event.timestamp);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_ace_chord_driver_api = {
    .binding_pressed = on_ace_chord_pressed,
    .binding_released = on_ace_chord_released,
};

static int behavior_ace_chord_init(const struct device *dev) {
    for (int i = 0; i < ACE_CHORD_SIDES; i++) {
        clear_state(&chord_states[i]);
        k_work_init_delayable(&chord_states[i].timeout_work, ace_chord_timeout_handler);
    }

    return 0;
}

#define ACE_CHORD_INST(n)                                                                         \
    static const struct behavior_ace_chord_config behavior_ace_chord_config_##n = {               \
        .timeout_ms = DT_INST_PROP(n, timeout_ms),                                                \
    };                                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_ace_chord_init, NULL, NULL,                               \
                            &behavior_ace_chord_config_##n, POST_KERNEL,                          \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_ace_chord_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ACE_CHORD_INST)

#endif
