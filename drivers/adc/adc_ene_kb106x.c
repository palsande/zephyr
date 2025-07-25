/*
 * Copyright (c) 2025 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb106x_adc

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <errno.h>
#include <reg/adc.h>
#include <zephyr/logging/log.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

LOG_MODULE_REGISTER(adc_ene_kb106x, CONFIG_ADC_LOG_LEVEL);

struct adc_kb106x_config {
	/* ADC Register base address */
	struct adc_regs *adc;
	/* Pin control */
	const struct pinctrl_dev_config *pcfg;
};

struct adc_kb106x_data {
	struct adc_context ctx;
	const struct device *adc_dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint16_t *buf_end;
};

/* ADC local functions */
static bool adc_kb106x_validate_buffer_size(const struct adc_sequence *sequence)
{
	int chan_count = 0;
	size_t buff_need;
	uint32_t chan_mask;

	for (chan_mask = 0x80; chan_mask != 0; chan_mask >>= 1) {
		if (chan_mask & sequence->channels) {
			chan_count++;
		}
	}

	buff_need = chan_count * sizeof(uint16_t);

	if (sequence->options) {
		buff_need *= 1 + sequence->options->extra_samplings;
	}

	if (buff_need > sequence->buffer_size) {
		return false;
	}

	return true;
}

/* ADC Sample Flow (by using adc_context.h api function)
 *  1. Start ADC sampling (set up flag ctx->sync)
 *     adc_context_start_read() -> adc_context_start_sampling()
 *  2. Wait ADC sample finish (by monitor flag ctx->sync)
 *     adc_context_wait_for_completion
 *  3. Finish ADC sample (isr clear flag ctx->sync)
 *     adc_context_on_sampling_done -> adc_context_complete
 */
static int adc_kb106x_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_kb106x_config *config = dev->config;
	struct adc_kb106x_data *data = dev->data;
	int error = 0;

	if (!sequence->channels || (sequence->channels & ~BIT_MASK(ADC_MAX_CHAN))) {
		LOG_ERR("Invalid ADC channels.");
		return -EINVAL;
	}
	/* Fixed 10 bit resolution of ene ADC */
	if (sequence->resolution != ADC_RESOLUTION) {
		LOG_ERR("Unfixed 10 bit ADC resolution.");
		return -ENOTSUP;
	}
	/* Check sequence->buffer_size is enough */
	if (!adc_kb106x_validate_buffer_size(sequence)) {
		LOG_ERR("ADC buffer size too small.");
		return -ENOMEM;
	}
	/* Only support single sampling */
	if (sequence->options != NULL) {
		LOG_ERR("ADC only support single sampling.");
		return -ENOTSUP;
	}

	/* assign record buffer pointer */
	data->buffer = sequence->buffer;
	data->buf_end = data->buffer + sequence->buffer_size / sizeof(uint16_t);
	/* store device for adc_context_start_read() */
	data->adc_dev = dev;
	/* Inform adc start sampling */
	adc_context_start_read(&data->ctx, sequence);
	/* Since kb106x adc has no irq. So need polling the adc conversion
	 * flag to be valid, then record adc value.
	 */
	uint32_t channels = (config->adc->ADCCFG & ADC_CHANNEL_BIT_MASK) >> ADC_CHANNEL_BIT_POS;

	while (channels) {
		int count;
		int ch_num;

		count = 0;
		ch_num = find_lsb_set(channels) - 1;
		/* wait valid flag */
		while (config->adc->ADCDAT[ch_num] & ADC_INVALID_VALUE) {
			k_busy_wait(ADC_WAIT_TIME);
			count++;
			if (count >= ADC_WAIT_CNT) {
				LOG_ERR("ADC busy timeout...");
				error = -EBUSY;
				break;
			}
		}
		/* check buffer size is enough then record adc value */
		if (data->buffer < data->buf_end) {
			*data->buffer = (uint16_t)(config->adc->ADCDAT[ch_num]);
			data->buffer++;
		} else {
			error = -EINVAL;
			break;
		}

		/* clear completed channel */
		channels &= ~BIT(ch_num);
	}
	/* Besause polling the adc conversion flag. don't need wait_for_completion*/

	/* Inform adc sampling is done */
	adc_context_on_sampling_done(&data->ctx, dev);
	return error;
}

/* ADC api functions */
static int adc_kb106x_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	if (channel_cfg->channel_id >= ADC_MAX_CHAN) {
		LOG_ERR("Invalid channel %d.", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported channel acquisition time.");
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported.");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Unsupported channel gain %d.", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Unsupported channel reference.");
		return -ENOTSUP;
	}
	LOG_DBG("ADC channel %d configured.", channel_cfg->channel_id);
	return 0;
}

static int adc_kb106x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_kb106x_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = adc_kb106x_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#if defined(CONFIG_ADC_ASYNC)
static int adc_kb106x_read_async(const struct device *dev, const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	struct adc_kb106x_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = adc_kb106x_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif /* CONFIG_ADC_ASYNC */

/* ADC api function (using by adc_context.H function) */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_kb106x_data *data = CONTAINER_OF(ctx, struct adc_kb106x_data, ctx);
	const struct device *dev = data->adc_dev;
	const struct adc_kb106x_config *config = dev->config;

	data->repeat_buffer = data->buffer;
	config->adc->ADCCFG = (config->adc->ADCCFG & ~ADC_CHANNEL_BIT_MASK) |
			      (ctx->sequence.channels << ADC_CHANNEL_BIT_POS);
	config->adc->ADCCFG |= ADC_FUNCTION_ENABLE;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_kb106x_data *data = CONTAINER_OF(ctx, struct adc_kb106x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static DEVICE_API(adc, adc_kb106x_api) = {
	.channel_setup = adc_kb106x_channel_setup,
	.read = adc_kb106x_read,
	.ref_internal = ADC_VREF_ANALOG,
#if defined(CONFIG_ADC_ASYNC)
	.read_async = adc_kb106x_read_async,
#endif
};

static int adc_kb106x_init(const struct device *dev)
{
	const struct adc_kb106x_config *config = dev->config;
	struct adc_kb106x_data *data = dev->data;
	int ret;

	adc_context_unlock_unconditionally(&data->ctx);
	/* Configure pin-mux for ADC device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("ADC pinctrl setup failed (%d).", ret);
		return ret;
	}

	return 0;
}

#define ADC_KB106X_DEVICE(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct adc_kb106x_data adc_kb106x_data_##inst = {                                   \
		ADC_CONTEXT_INIT_TIMER(adc_kb106x_data_##inst, ctx),                               \
		ADC_CONTEXT_INIT_LOCK(adc_kb106x_data_##inst, ctx),                                \
		ADC_CONTEXT_INIT_SYNC(adc_kb106x_data_##inst, ctx),                                \
	};                                                                                         \
	static const struct adc_kb106x_config adc_kb106x_config_##inst = {                         \
		.adc = (struct adc_regs *)DT_INST_REG_ADDR(inst),                                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &adc_kb106x_init, NULL, &adc_kb106x_data_##inst,               \
			      &adc_kb106x_config_##inst, PRE_KERNEL_1,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &adc_kb106x_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_KB106X_DEVICE)
