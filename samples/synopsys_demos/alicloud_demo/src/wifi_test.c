/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <string.h>
#include <device.h>
#include <drivers/sensor.h>
#include <drivers/display.h>
#include <lvgl.h>
#include <shell/shell.h>
#include <net/socket.h>
#include "dev_sign_api.h"
#include "mqtt_api.h"

char DEMO_PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1] = {0};
char DEMO_DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1] = {0};
char DEMO_DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1] = {0};

void *HAL_Malloc(uint32_t size);
void HAL_Free(void *ptr);
void HAL_Printf(const char *fmt, ...);
int HAL_GetProductKey(char product_key[IOTX_PRODUCT_KEY_LEN + 1]);
int HAL_GetDeviceName(char device_name[IOTX_DEVICE_NAME_LEN + 1]);
int HAL_GetDeviceSecret(char device_secret[IOTX_DEVICE_SECRET_LEN]);
uint64_t HAL_UptimeMs(void);
int HAL_Snprintf(char *str, const int len, const char *fmt, ...);

#define EXAMPLE_TRACE(fmt, ...)  \
    do { \
        HAL_Printf("%s|%03d :: ", __func__, __LINE__); \
        HAL_Printf(fmt, ##__VA_ARGS__); \
        HAL_Printf("%s", "\r\n"); \
    } while(0)

void example_message_arrive(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    iotx_mqtt_topic_info_t     *topic_info = (iotx_mqtt_topic_info_pt) msg->msg;

    switch (msg->event_type) {
        case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
            /* print topic name and topic message */
            EXAMPLE_TRACE("Message Arrived:");
            EXAMPLE_TRACE("Topic  : %.*s", topic_info->topic_len, topic_info->ptopic);
            EXAMPLE_TRACE("Payload: %.*s", topic_info->payload_len, topic_info->payload);
            EXAMPLE_TRACE("\n");
            break;
        default:
            break;
    }
}

int example_subscribe(void *handle)
{
    int res = 0;
    const char *fmt = "/%s/%s/user/get";
    char *topic = NULL;
    int topic_len = 0;

    topic_len = strlen(fmt) + strlen(DEMO_PRODUCT_KEY) + strlen(DEMO_DEVICE_NAME) + 1;
    topic = HAL_Malloc(topic_len);
    if (topic == NULL) {
        EXAMPLE_TRACE("memory not enough");
        return -1;
    }
    memset(topic, 0, topic_len);
    HAL_Snprintf(topic, topic_len, fmt, DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME);

    res = IOT_MQTT_Subscribe(handle, topic, IOTX_MQTT_QOS0, example_message_arrive, NULL);
    if (res < 0) {
        EXAMPLE_TRACE("subscribe failed");
        HAL_Free(topic);
        return -1;
    }

    HAL_Free(topic);
    return 0;
}

char acPayload[128] = {0};

u32_t idnumber;
struct sensor_value temp_val;

int example_publish(void *handle, double temp)
{
    int             res = 0;
    const char     *fmt = "/sys/%s/%s/thing/event/property/post";
    char           *topic = NULL;
    int             topic_len = 0;
    char *payload = "{\"id\": %d,\"params\": {\"Temperature\": %.2f},\"method\": \"thing.event.property.post\"}";

    topic_len = strlen(fmt) + strlen(DEMO_PRODUCT_KEY) + strlen(DEMO_DEVICE_NAME) + 1;
    topic = HAL_Malloc(topic_len);
    if (topic == NULL) {
        EXAMPLE_TRACE("memory not enough");
        return -1;
    }
    memset(topic, 0, topic_len);
    HAL_Snprintf(topic, topic_len, fmt, DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME);

    memset(acPayload, 0, 128);
    HAL_Snprintf(acPayload, 128, payload, idnumber++, temp);

    res = IOT_MQTT_Publish_Simple(0, topic, IOTX_MQTT_QOS0, acPayload, strlen(acPayload));
    if (res < 0) {
        EXAMPLE_TRACE("publish failed, res = %d", res);
        HAL_Free(topic);
        return -1;
    }

    HAL_Free(topic);
    return 0;
}

void example_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    EXAMPLE_TRACE("msg->event_type : %d", msg->event_type);
}


static int sensor_set_attribute(struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, int value)
{
	struct sensor_value sensor_val;
	int ret;

	sensor_val.val1 = value / 1000000;
	sensor_val.val2 = value % 1000000;

	ret = sensor_attr_set(dev, chan, attr, &sensor_val);
	if (ret) {
		printf("sensor_attr_set failed ret %d\n", ret);
	}

	return ret;
}

int mqtt_sample(const struct shell *shell, size_t argc, char **argv)
{
    void                   *pclient = NULL;
    int                     res = 0;
    int                     loop_cnt = 0;
    iotx_mqtt_param_t       mqtt_params;
    double temp;

    HAL_GetProductKey(DEMO_PRODUCT_KEY);
    HAL_GetDeviceName(DEMO_DEVICE_NAME);
    HAL_GetDeviceSecret(DEMO_DEVICE_SECRET);

    EXAMPLE_TRACE("mqtt example");

    memset(&mqtt_params, 0x0, sizeof(mqtt_params));

    mqtt_params.handle_event.h_fp = example_event_handle;

    pclient = IOT_MQTT_Construct(&mqtt_params);
    if (NULL == pclient) {
        EXAMPLE_TRACE("MQTT construct failed");
        return -1;
    }

    res = example_subscribe(pclient);
    if (res < 0) {
        IOT_MQTT_Destroy(&pclient);
        return -1;
    }

    while (1) {
        if (0 == loop_cnt % 20) {
            temp = sensor_value_to_double(&temp_val);
            printf("temperature %.6f C\n", temp);
            example_publish(pclient, temp);
        }

        IOT_MQTT_Yield(pclient, 200);

        loop_cnt += 1;
    }

    return 0;
}

SHELL_CMD_REGISTER(ali_mqtt_sample, NULL, "ali_mqtt_sample", mqtt_sample);


void main(void)
{
	u32_t count = 0U;
	char count_str[25] = {0};
	struct device *display_dev;
	lv_obj_t *hello_world_label;
	lv_obj_t *count_label;
	lv_obj_t *led1;
    lv_obj_t *led2;

	int ret;


	display_dev = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);

	if (display_dev == NULL) {
		printk("device not found.  Aborting test.");
		return;
	}

	struct device *dev = device_get_binding(DT_INST_0_ADI_ADT7420_LABEL);

	if (dev == NULL) {
		printk("Failed to get device binding\n");
		return;
	}

		/* Set upddate rate to 240 mHz */
	sensor_set_attribute(dev, SENSOR_CHAN_AMBIENT_TEMP,
			     SENSOR_ATTR_SAMPLING_FREQUENCY, 240 * 1000);

    led1 = lv_led_create(lv_scr_act(), NULL);
	lv_obj_align(led1, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

	hello_world_label = lv_label_create(lv_scr_act(), NULL);
	lv_label_set_text(hello_world_label, "Hello$$world!");
	lv_obj_align(hello_world_label, led1, LV_ALIGN_OUT_RIGHT_MID, 0, 0);



	count_label = lv_label_create(lv_scr_act(), NULL);
	lv_obj_align(count_label, hello_world_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    led2 = lv_led_create(lv_scr_act(), NULL);
	lv_obj_align(led2, NULL, LV_ALIGN_IN_TOP_RIGHT, 0, 0);

	lv_obj_t * chart = lv_chart_create(lv_scr_act(), NULL);                         /*Create the chart*/
    lv_obj_set_size(chart, lv_obj_get_width(lv_scr_act()), lv_obj_get_height(lv_scr_act())/2);   /*Set the size*/
    lv_obj_align(chart, led1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);                   /*Align below the slider*/
    lv_chart_set_series_width(chart, 3);                                            /*Set the line width*/
	lv_chart_set_range(chart, 0, 100);
	//lv_chart_set_div_line_count(chart, 10, 25);
	lv_chart_set_point_count(chart, 50);
    /*Add a RED data series and set some points*/
    lv_chart_series_t * dl1 = lv_chart_add_series(chart, LV_COLOR_RED);
    lv_chart_series_t * dl2 = lv_chart_add_series(chart, LV_COLOR_BLUE);
	display_blanking_off(display_dev);

	while (1) {
		if ((count % 10) == 0U) {
			ret = sensor_sample_fetch(dev);
			if (ret) {
				printk("sensor_sample_fetch failed ret %d\n", ret);
			}
			ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
						&temp_val);
			if (ret) {
				printk("sensor_channel_get failed ret %d\n", ret);
			}
			sprintf(count_str, "temperature %.2f C", sensor_value_to_double(&temp_val));
			lv_label_set_text(count_label, count_str);
			sprintf(count_str, "hello#world ");
			lv_label_set_text(hello_world_label, count_str);
			lv_chart_set_next(chart, dl1, temp_val.val1);
			lv_chart_set_next(chart, dl2, temp_val.val2/10000);
			lv_led_toggle(led1);
			lv_led_toggle(led2);
		}
		lv_task_handler();
		k_sleep(5);

	}
}

