/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel_structs.h>
#include <sys/printk.h>
#include <string.h>
#include <user_app_framework/user_app_framework.h>
#include <user_app_framework/user_app_config.h>
#include "pse_sys_service.h"
#include "wol_service.h"
#include <drivers/gpio.h>
#include "pm_service.h"
#include "driver/sedi_driver_ipc.h"

#define LOG_LEVEL CONFIG_WOL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(wol);

#define ENABLE_WAKE (1)
#define DISABLE_WAKE (0)

#define GPIO_CONFIG_PIN (GPIO_INPUT)

#define GBE0_PCI_FUNC_INDEX (PSE_DEV_GBE0)
#define GBE1_PCI_FUNC_INDEX (PSE_DEV_GBE1)

#define GPIO_WAKE_EVENT_INT_0 36
#define GPIO_WAKE_EVENT_INT_1 37

#define WOL_STACK_SIZE 512
#define WOL_PRIORITY 5

#define BIT32(x) ((uint32_t)1 << (x))

struct fdata_t {
	sys_snode_t snode;      /* pointer to the next item in FIFO */
	uint32_t pci_fun;       /* PCI function number */
};

K_THREAD_STACK_DEFINE(wol_stack_area, WOL_STACK_SIZE);

static struct fdata_t data[2];
static struct k_fifo wol_fifo;
static struct k_thread wol_thread;

bool pme_assert_in_progress[2] = { false, false };

struct gbe_dev_ctx_t {
	const struct device *dev;
	sedi_wake_event_instance_t pin;
	gpio_pin_t gpio_pin;
	struct gpio_callback gpio_cb;
};

#if defined(CONFIG_GBE0_WOL_SERVICE)
const static struct device *gbe0_gpio_dev;
static struct gbe_dev_ctx_t gbe0_dev_ctx;
#endif

#if defined(CONFIG_GBE1_WOL_SERVICE)
const static struct device *gbe1_gpio_dev;
static struct gbe_dev_ctx_t gbe1_dev_ctx;
#endif

static void wol_assert_pme(void *p1, void *p2, void *p3)
{
	struct fdata_t *rx_data;
	int idx = 0;

	while (1) {
		rx_data = k_fifo_get(&wol_fifo, K_FOREVER);

		if (rx_data != NULL) {
			sedi_pm_trigger_pme(rx_data->pci_fun);

			if (rx_data->pci_fun == GBE1_PCI_FUNC_INDEX) {
				idx = 1;
			} else {
				idx = 0;
			}

			pme_assert_in_progress[idx] = false;
		}
	}
}

static void submit_wol_fifo_data(uint32_t pci)
{
	int idx = 0;

	if (pci == GBE1_PCI_FUNC_INDEX) {
		idx = 1;
	}

	data[idx].pci_fun = pci;
	/* Put items into fifo */
	if (pme_assert_in_progress[idx] == false) {
		pme_assert_in_progress[idx] = true;
		k_fifo_put(&wol_fifo, &data[idx]);
	}
}

static void wol_init(void)
{
	k_fifo_init(&wol_fifo);

	/* create a child thread */
	k_thread_create(&wol_thread, wol_stack_area, WOL_STACK_SIZE,
			wol_assert_pme, &wol_fifo, NULL, NULL, WOL_PRIORITY, 0,
			K_NO_WAIT);
}

void gbe_d3_notification(sedi_pm_d3_event_t d3_event, void *ctx)
{
	LOG_DBG("%s %d\n", __func__, (int) d3_event);

	struct gbe_dev_ctx_t *dev_ctx = (struct gbe_dev_ctx_t *)ctx;
	uint32_t flags = GPIO_INPUT | GPIO_INT_DISABLE;

	if (!dev_ctx || !dev_ctx->dev) {
		LOG_ERR("invalid device context\n");
		return;
	}

	if (d3_event == PM_EVENT_HOST_D3_ENTRY) {
		/* enable  GPIO interrupt. */
		gpio_pin_interrupt_configure(dev_ctx->dev,
					     dev_ctx->gpio_pin,
					     GPIO_INT_EDGE_FALLING);
		/* enable GPIO  pin as wake source. */
		sedi_pm_configure_wake_source(PM_WAKE_EVENT_GPIO, dev_ctx->pin,
					      ENABLE_WAKE);
	}
	#if defined CONFIG_GPIO_INT_DIS_ON_D3_EXIT
	else {
		/* disable  GPIO interrupt. */
		gpio_pin_interrupt_configure(dev_ctx->dev,
					     dev_ctx->gpio_pin, flags);
		/* disable GPIO pin as wake source. */
		sedi_pm_configure_wake_source(PM_WAKE_EVENT_GPIO, dev_ctx->pin,
					      DISABLE_WAKE);
	}
	#endif
}

#if defined(CONFIG_GBE0_WOL_SERVICE)
static void rgmii_int0_callback(const struct device *dev,
				struct gpio_callback *gpio_cb, uint32_t pins)
{
	submit_wol_fifo_data(GBE0_PCI_FUNC_INDEX);
}
#endif

#if defined(CONFIG_GBE1_WOL_SERVICE)
static void rgmii_int1_callback(const struct device *dev,
				struct gpio_callback *gpio_cb, uint32_t pins)
{
	submit_wol_fifo_data(GBE1_PCI_FUNC_INDEX);
}
#endif

static void config_gpio(struct gbe_dev_ctx_t *ctx, uint32_t pci_fun)
{
	sedi_dev_ownership_t ownership = sedi_get_dev_ownership(pci_fun);
	uint32_t flags = GPIO_INPUT | GPIO_INT_DISABLE;

	if (DEV_PSE_OWNED == ownership || DEV_NO_OWNER == ownership) {
		LOG_WRN("error! PCI Func %x not owned by Host\n",
			(int) pci_fun);
		goto exit;
	}

	/* we will enable GPIO interrupt only on GBE D3 entry notification. */
	gpio_pin_interrupt_configure(ctx->dev, ctx->gpio_pin, flags);

	/* configure GPIO pin for interrupt notification. */
	if (gpio_pin_configure(ctx->dev, ctx->gpio_pin, GPIO_CONFIG_PIN)) {
		LOG_ERR("failed to configure GPIO pin %d\n",
			(int) ctx->gpio_pin);
		goto exit;
	}

#if defined(CONFIG_GBE0_WOL_SERVICE)
	if (pci_fun == GBE0_PCI_FUNC_INDEX) {
		gpio_init_callback(&ctx->gpio_cb, rgmii_int0_callback,
				   BIT32(ctx->gpio_pin));
	}
#endif

#if defined(CONFIG_GBE1_WOL_SERVICE)
	if (pci_fun == GBE1_PCI_FUNC_INDEX) {
		gpio_init_callback(&ctx->gpio_cb, rgmii_int1_callback,
				   BIT32(ctx->gpio_pin));
	}
#endif
	if (gpio_add_callback(ctx->dev, &ctx->gpio_cb)) {
		LOG_ERR("failed to configure GPIO pin %d\n",
			(int) ctx->gpio_pin);
		goto exit;
	}

	/* register D3 notification for PCI func 18(GBE0) */
	if (sedi_pm_register_d3_notification(pci_fun, gbe_d3_notification,
					     ctx)) {
		LOG_ERR("D3 registeration for GBE1 function failed!!\n");
		goto exit;
	}

	sedi_pm_set_control(SEDI_PM_IOCTL_WOL_SUPPORT, 1);
	return;

exit:
	LOG_WRN("WOL config for PCI func %d failed\n", (int) pci_fun);
}

void wol_config(void)
{
	if (sedi_get_config(SEDI_CONFIG_WOL_EN, NULL) != SEDI_CONFIG_SET) {
		LOG_DBG("%s(): No WOL service\n", __func__);
		return;
	}

#if defined(CONFIG_GBE0_WOL_SERVICE)
	LOG_DBG("configure GBE0 WOL\n");

	gbe0_gpio_dev = device_get_binding(CONFIG_GBE0_GPIO_DEV);

	if (!gbe0_gpio_dev) {
		LOG_ERR("failed to bind device %s\n", CONFIG_GBE0_GPIO_DEV);
		return;
	}

	gbe0_dev_ctx.dev = gbe0_gpio_dev;
	/* GPIO wake event instance number */
	gbe0_dev_ctx.pin.gpio_pin = GPIO_WAKE_EVENT_INT_0;
	/* GPIO pin number */
	gbe0_dev_ctx.gpio_pin = CONFIG_RGMII_INT_0;

	config_gpio(&gbe0_dev_ctx, GBE0_PCI_FUNC_INDEX);
#endif

#if defined(CONFIG_GBE1_WOL_SERVICE)
	LOG_DBG("configure GBE1 WOL\n");

	gbe1_gpio_dev = device_get_binding(CONFIG_GBE1_GPIO_DEV);

	if (!gbe1_gpio_dev) {
		LOG_ERR("failed to bind device %s\n", CONFIG_GBE1_GPIO_DEV);
		return;
	}

	gbe1_dev_ctx.dev = gbe1_gpio_dev;
	/* GPIO wake event instance number */
	gbe1_dev_ctx.pin.gpio_pin = GPIO_WAKE_EVENT_INT_1;
	/* GPIO pin number */
	gbe1_dev_ctx.gpio_pin = CONFIG_RGMII_INT_1;

	config_gpio(&gbe1_dev_ctx, GBE1_PCI_FUNC_INDEX);
#endif
	wol_init();
}

void wol_service_init(void)
{
	LOG_DBG("WOL service init\n");
}

