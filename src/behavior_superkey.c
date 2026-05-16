/*
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_superkey

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define SUPERKEY_MAX_ACTIVE 8
#define SUPERKEY_POSITION_FREE UINT32_MAX
#define SUPERKEY_KEYCODE_FREE 0

struct behavior_superkey_config {
    int tapping_term_ms;
    int double_tap_ms;
    int release_after_ms;
};

struct active_superkey {
    uint32_t position;
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
    uint8_t source;
#endif
    uint32_t keycode;
    int64_t timestamp;
    const struct behavior_superkey_config *config;
    bool hold_sent;
    bool consumed;
    bool timer_cancelled;
    struct k_work_delayable hold_timer;
};

struct oneshot_superkey {
    uint32_t position;
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
    uint8_t source;
#endif
    uint32_t keycode;
    int64_t tapped_at;
    int64_t release_at;
    bool timer_cancelled;
    struct k_work_delayable release_timer;
};

struct locked_superkey {
    uint32_t keycode;
};

static struct active_superkey active_superkeys[SUPERKEY_MAX_ACTIVE];
static struct oneshot_superkey oneshot_superkeys[SUPERKEY_MAX_ACTIVE];
static struct locked_superkey locked_superkeys[SUPERKEY_MAX_ACTIVE];

static int send_keycode(uint32_t keycode, bool pressed, int64_t timestamp) {
    return raise_zmk_keycode_state_changed_from_encoded(keycode, pressed, timestamp);
}

static bool event_is_modifier(const struct zmk_keycode_state_changed *ev) {
    return is_mod(ev->usage_page, ev->keycode);
}

static void clear_active(struct active_superkey *active) {
    active->position = SUPERKEY_POSITION_FREE;
    active->keycode = SUPERKEY_KEYCODE_FREE;
    active->config = NULL;
    active->hold_sent = false;
    active->consumed = false;
    active->timer_cancelled = false;
}

static void clear_oneshot(struct oneshot_superkey *oneshot) {
    oneshot->position = SUPERKEY_POSITION_FREE;
    oneshot->keycode = SUPERKEY_KEYCODE_FREE;
    oneshot->tapped_at = 0;
    oneshot->release_at = 0;
    oneshot->timer_cancelled = false;
}

static struct active_superkey *find_active(uint32_t position) {
    for (int i = 0; i < SUPERKEY_MAX_ACTIVE; i++) {
        if (active_superkeys[i].position == position) {
            return &active_superkeys[i];
        }
    }

    return NULL;
}

static struct active_superkey *store_active(struct zmk_behavior_binding_event event,
                                            uint32_t keycode,
                                            const struct behavior_superkey_config *config) {
    for (int i = 0; i < SUPERKEY_MAX_ACTIVE; i++) {
        if (active_superkeys[i].position != SUPERKEY_POSITION_FREE) {
            continue;
        }

        active_superkeys[i].position = event.position;
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        active_superkeys[i].source = event.source;
#endif
        active_superkeys[i].keycode = keycode;
        active_superkeys[i].timestamp = event.timestamp;
        active_superkeys[i].config = config;
        active_superkeys[i].hold_sent = false;
        active_superkeys[i].consumed = false;
        active_superkeys[i].timer_cancelled = false;
        return &active_superkeys[i];
    }

    return NULL;
}

static struct oneshot_superkey *find_oneshot(uint32_t keycode) {
    for (int i = 0; i < SUPERKEY_MAX_ACTIVE; i++) {
        if (oneshot_superkeys[i].keycode == keycode) {
            return &oneshot_superkeys[i];
        }
    }

    return NULL;
}

static void cancel_oneshot(struct oneshot_superkey *oneshot) {
    int cancel_result = k_work_cancel_delayable(&oneshot->release_timer);
    if (cancel_result == -EINPROGRESS) {
        oneshot->timer_cancelled = true;
    }
    clear_oneshot(oneshot);
}

static struct oneshot_superkey *store_oneshot(struct zmk_behavior_binding_event event,
                                              uint32_t keycode,
                                              const struct behavior_superkey_config *config) {
    struct oneshot_superkey *existing = find_oneshot(keycode);
    if (existing != NULL) {
        cancel_oneshot(existing);
    }

    for (int i = 0; i < SUPERKEY_MAX_ACTIVE; i++) {
        if (oneshot_superkeys[i].position != SUPERKEY_POSITION_FREE) {
            continue;
        }

        oneshot_superkeys[i].position = event.position;
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        oneshot_superkeys[i].source = event.source;
#endif
        oneshot_superkeys[i].keycode = keycode;
        oneshot_superkeys[i].tapped_at = event.timestamp;
        oneshot_superkeys[i].release_at = event.timestamp + config->release_after_ms;
        oneshot_superkeys[i].timer_cancelled = false;

        int32_t ms_left = oneshot_superkeys[i].release_at - k_uptime_get();
        if (ms_left > 0) {
            k_work_schedule(&oneshot_superkeys[i].release_timer, K_MSEC(ms_left));
        }

        return &oneshot_superkeys[i];
    }

    return NULL;
}

static struct locked_superkey *find_locked(uint32_t keycode) {
    for (int i = 0; i < SUPERKEY_MAX_ACTIVE; i++) {
        if (locked_superkeys[i].keycode == keycode) {
            return &locked_superkeys[i];
        }
    }

    return NULL;
}

static int lock_keycode(uint32_t keycode, int64_t timestamp) {
    if (find_locked(keycode) != NULL) {
        return 0;
    }

    for (int i = 0; i < SUPERKEY_MAX_ACTIVE; i++) {
        if (locked_superkeys[i].keycode != SUPERKEY_KEYCODE_FREE) {
            continue;
        }

        locked_superkeys[i].keycode = keycode;
        return send_keycode(keycode, true, timestamp);
    }

    LOG_ERR("No free SuperKey lock slots");
    return -ENOMEM;
}

static int unlock_keycode(struct locked_superkey *locked, int64_t timestamp) {
    uint32_t keycode = locked->keycode;
    locked->keycode = SUPERKEY_KEYCODE_FREE;
    return send_keycode(keycode, false, timestamp);
}

static int send_hold(struct active_superkey *active, int64_t timestamp) {
    if (active->hold_sent || active->consumed) {
        return 0;
    }

    active->hold_sent = true;
    return send_keycode(active->keycode, true, timestamp);
}

static void superkey_hold_timer_handler(struct k_work *item) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(item);
    struct active_superkey *active = CONTAINER_OF(d_work, struct active_superkey, hold_timer);

    if (active->position == SUPERKEY_POSITION_FREE) {
        return;
    }

    if (active->timer_cancelled) {
        active->timer_cancelled = false;
        return;
    }

    send_hold(active, active->timestamp + active->config->tapping_term_ms);
}

static void superkey_oneshot_timer_handler(struct k_work *item) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(item);
    struct oneshot_superkey *oneshot = CONTAINER_OF(d_work, struct oneshot_superkey, release_timer);

    if (oneshot->position == SUPERKEY_POSITION_FREE) {
        return;
    }

    if (oneshot->timer_cancelled) {
        oneshot->timer_cancelled = false;
        return;
    }

    clear_oneshot(oneshot);
}

static int on_superkey_pressed(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_superkey_config *config = dev->config;
    const uint32_t keycode = binding->param1;

    struct active_superkey *active = store_active(event, keycode, config);
    if (active == NULL) {
        LOG_ERR("No free SuperKey active slots");
        return ZMK_BEHAVIOR_OPAQUE;
    }

    struct locked_superkey *locked = find_locked(keycode);
    if (locked != NULL) {
        unlock_keycode(locked, event.timestamp);
        active->consumed = true;
        return ZMK_BEHAVIOR_OPAQUE;
    }

    struct oneshot_superkey *oneshot = find_oneshot(keycode);
    if (oneshot != NULL && event.timestamp <= oneshot->tapped_at + config->double_tap_ms) {
        cancel_oneshot(oneshot);
        lock_keycode(keycode, event.timestamp);
        active->consumed = true;
        return ZMK_BEHAVIOR_OPAQUE;
    }

    k_work_schedule(&active->hold_timer, K_MSEC(config->tapping_term_ms));
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_superkey_released(struct zmk_behavior_binding *binding,
                                struct zmk_behavior_binding_event event) {
    struct active_superkey *active = find_active(event.position);
    if (active == NULL) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    int cancel_result = k_work_cancel_delayable(&active->hold_timer);
    if (event.timestamp > active->timestamp + active->config->tapping_term_ms) {
        send_hold(active, active->timestamp + active->config->tapping_term_ms);
    }

    if (active->consumed) {
        clear_active(active);
        return ZMK_BEHAVIOR_OPAQUE;
    }

    if (active->hold_sent) {
        send_keycode(active->keycode, false, event.timestamp);
    } else if (store_oneshot(event, active->keycode, active->config) == NULL) {
        LOG_ERR("No free SuperKey one-shot slots");
    }

    if (cancel_result == -EINPROGRESS) {
        active->timer_cancelled = true;
    }
    clear_active(active);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int superkey_position_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (int i = 0; i < SUPERKEY_MAX_ACTIVE; i++) {
        struct active_superkey *active = &active_superkeys[i];
        if (active->position == SUPERKEY_POSITION_FREE || active->position == ev->position) {
            continue;
        }

        send_hold(active, ev->timestamp);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static int superkey_keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || !ev->state || event_is_modifier(ev)) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    bool has_oneshot = false;
    for (int i = 0; i < SUPERKEY_MAX_ACTIVE; i++) {
        if (oneshot_superkeys[i].position != SUPERKEY_POSITION_FREE) {
            has_oneshot = true;
            break;
        }
    }

    if (!has_oneshot) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    struct zmk_keycode_state_changed_event dupe_ev = copy_raised_zmk_keycode_state_changed(ev);
    uint32_t applied_keycodes[SUPERKEY_MAX_ACTIVE] = {};
    size_t applied_count = 0;

    for (int i = 0; i < SUPERKEY_MAX_ACTIVE; i++) {
        struct oneshot_superkey *oneshot = &oneshot_superkeys[i];
        if (oneshot->position == SUPERKEY_POSITION_FREE) {
            continue;
        }

        if (ev->timestamp > oneshot->release_at) {
            clear_oneshot(oneshot);
            continue;
        }

        k_work_cancel_delayable(&oneshot->release_timer);
        applied_keycodes[applied_count++] = oneshot->keycode;
        send_keycode(oneshot->keycode, true, ev->timestamp);
    }

    if (applied_count == 0) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    ZMK_EVENT_RAISE_AFTER(dupe_ev, behavior_superkey);

    for (size_t i = 0; i < applied_count; i++) {
        send_keycode(applied_keycodes[i], false, ev->timestamp);
    }

    for (int i = 0; i < SUPERKEY_MAX_ACTIVE; i++) {
        clear_oneshot(&oneshot_superkeys[i]);
    }

    return ZMK_EV_EVENT_CAPTURED;
}

ZMK_LISTENER(behavior_superkey_position, superkey_position_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_superkey_position, zmk_position_state_changed);

ZMK_LISTENER(behavior_superkey, superkey_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_superkey, zmk_keycode_state_changed);

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata param_values[] = {{
    .display_name = "Key",
    .type = BEHAVIOR_PARAMETER_VALUE_TYPE_HID_USAGE,
}};

static const struct behavior_parameter_metadata_set param_metadata_set[] = {{
    .param1_values = param_values,
    .param1_values_len = ARRAY_SIZE(param_values),
}};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(param_metadata_set),
    .sets = param_metadata_set,
};

#endif

static const struct behavior_driver_api behavior_superkey_driver_api = {
    .binding_pressed = on_superkey_pressed,
    .binding_released = on_superkey_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif
};

static int behavior_superkey_init(const struct device *dev) {
    static bool init_first_run = true;

    if (!init_first_run) {
        return 0;
    }

    for (int i = 0; i < SUPERKEY_MAX_ACTIVE; i++) {
        k_work_init_delayable(&active_superkeys[i].hold_timer, superkey_hold_timer_handler);
        clear_active(&active_superkeys[i]);

        k_work_init_delayable(&oneshot_superkeys[i].release_timer, superkey_oneshot_timer_handler);
        clear_oneshot(&oneshot_superkeys[i]);

        locked_superkeys[i].keycode = SUPERKEY_KEYCODE_FREE;
    }

    init_first_run = false;
    return 0;
}

#define SUPERKEY_INST(n)                                                                           \
    static const struct behavior_superkey_config behavior_superkey_config_##n = {                  \
        .tapping_term_ms = DT_INST_PROP(n, tapping_term_ms),                                       \
        .double_tap_ms = DT_INST_PROP(n, double_tap_ms),                                           \
        .release_after_ms = DT_INST_PROP(n, release_after_ms),                                     \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_superkey_init, NULL, NULL,                                 \
                            &behavior_superkey_config_##n, POST_KERNEL,                            \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_superkey_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SUPERKEY_INST)

#endif
