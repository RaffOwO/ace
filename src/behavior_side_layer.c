/*
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_side_layer

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_side_layer_config {
    const uint8_t *left_layers;
    size_t left_layers_len;
    const uint8_t *right_layers;
    size_t right_layers_len;
};

static bool initialized;

static bool layer_in_set(uint8_t layer, const uint8_t *layers, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (layers[i] == layer) {
            return true;
        }
    }

    return false;
}

static int switch_side_layer(uint8_t target, const uint8_t *layers, size_t len) {
    for (size_t i = 0; i < len; i++) {
        zmk_keymap_layer_deactivate(layers[i]);
    }

    return zmk_keymap_layer_activate(target);
}

static int on_side_layer_pressed(struct zmk_behavior_binding *binding,
                                 struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_side_layer_config *config = dev->config;
    uint8_t target = binding->param1;

    if (layer_in_set(target, config->left_layers, config->left_layers_len)) {
        return switch_side_layer(target, config->left_layers, config->left_layers_len);
    }

    if (layer_in_set(target, config->right_layers, config->right_layers_len)) {
        return switch_side_layer(target, config->right_layers, config->right_layers_len);
    }

    return -EINVAL;
}

static int on_side_layer_released(struct zmk_behavior_binding *binding,
                                  struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_side_layer_driver_api = {
    .binding_pressed = on_side_layer_pressed,
    .binding_released = on_side_layer_released,
};

static int behavior_side_layer_init(const struct device *dev) {
    const struct behavior_side_layer_config *config = dev->config;

    if (initialized) {
        return 0;
    }

    if (config->left_layers_len == 0 || config->right_layers_len == 0) {
        return -EINVAL;
    }

    zmk_keymap_layer_activate(config->left_layers[0]);
    zmk_keymap_layer_activate(config->right_layers[0]);
    initialized = true;

    return 0;
}

#define SIDE_LAYER_INST(n)                                                                         \
    static const uint8_t side_layer_left_layers_##n[] = DT_INST_PROP(n, left_layers);              \
    static const uint8_t side_layer_right_layers_##n[] = DT_INST_PROP(n, right_layers);            \
    static const struct behavior_side_layer_config behavior_side_layer_config_##n = {              \
        .left_layers = side_layer_left_layers_##n,                                                 \
        .left_layers_len = ARRAY_SIZE(side_layer_left_layers_##n),                                 \
        .right_layers = side_layer_right_layers_##n,                                               \
        .right_layers_len = ARRAY_SIZE(side_layer_right_layers_##n),                               \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_side_layer_init, NULL, NULL,                               \
                            &behavior_side_layer_config_##n, POST_KERNEL,                          \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_side_layer_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SIDE_LAYER_INST)

#endif
