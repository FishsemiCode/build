/****************************************************************************
 * configs/u1/src/ap_ads7843e_touchscreen.c
 *
 *   Copyright (C) 2019 Fishsemi Inc. All rights reserved.
 *   Author: clyde <liuyan@fishsemi.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdio.h>
#include <debug.h>
#include <assert.h>
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/spi/spi.h>
#include <nuttx/ioexpander/ioexpander.h>
#include <nuttx/input/touchscreen.h>
#include <nuttx/input/ads7843e.h>

#ifdef CONFIG_INPUT_ADS7843E

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DUMMY_IRQ_NUM (-1)

/* Configuration ************************************************************/

#ifndef CONFIG_INPUT
#  error "Touchscreen support requires CONFIG_INPUT"
#endif

#ifndef CONFIG_ADS7843E_IOEDEV
#  error "Touchscreen support requires CONFIG_ADS7843E_IOEDEV"
#endif

#ifndef CONFIG_IOEXPANDER_INT_ENABLE
#  error "Touchscreen support requires CONFIG_IOEXPANDER_INT_ENABLE"
#endif

#ifndef CONFIG_ADS7843E_PENIRQ_GPIO
#  error "Touchscreen support requires CONFIG_ADS7843E_PENIRQ_GPIO"
#endif

#ifndef CONFIG_ADS7843E_SPIDEV
#  error "Touchscreen support requires CONFIG_ADS7843E_SPIDEV"
#endif

#ifndef CONFIG_ADS7843E_FREQUENCY
#  define CONFIG_ADS7843E_FREQUENCY 500000
#endif

#ifndef CONFIG_ADS7843E_DEVMINOR
#  define CONFIG_ADS7843E_DEVMINOR 0
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* IRQ/GPIO access callbacks.  These operations all hidden behind
 * callbacks to isolate the XPT2046 driver from differences in GPIO
 * interrupt handling by varying boards and MCUs.  If possible,
 * interrupts should be configured on both rising and falling edges
 * so that contact and loss-of-contact events can be detected.
 *
 * attach  - Attach the XPT2046 interrupt handler to the GPIO interrupt
 * enable  - Enable or disable the GPIO interrupt
 * clear   - Acknowledge/clear any pending GPIO interrupt
 * pendown - Return the state of the pen down GPIO input
 */

static int  ads7843e_tsc_attach(FAR struct ads7843e_config_s *state, xcpt_t isr);
static void ads7843e_tsc_enable(FAR struct ads7843e_config_s *state, bool enable);
static void ads7843e_tsc_clear(FAR struct ads7843e_config_s *state);
static bool ads7843e_tsc_busy(FAR struct ads7843e_config_s *state);
static bool ads7843e_tsc_pendown(FAR struct ads7843e_config_s *state);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* A reference to a structure of this type must be passed to the XPT2046
 * driver.  This structure provides information about the configuration
 * of the XPT2046 and provides some board-specific hooks.
 *
 * Memory for this structure is provided by the caller.  It is not copied
 * by the driver and is presumed to persist while the driver is active.
 */

static struct ads7843e_config_s g_ads7843e_tscinfo =
{
  .frequency = CONFIG_ADS7843E_FREQUENCY,
  .attach    = ads7843e_tsc_attach,
  .enable    = ads7843e_tsc_enable,
  .clear     = ads7843e_tsc_clear,
  .busy      = ads7843e_tsc_busy,
  .pendown   = ads7843e_tsc_pendown,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* IRQ/GPIO access callbacks.  These operations all hidden behind
 * callbacks to isolate the XPT2046 driver from differences in GPIO
 * interrupt handling by varying boards and MCUs.  If possible,
 * interrupts should be configured on both rising and falling edges
 * so that contact and loss-of-contact events can be detected.
 *
 * attach  - Attach the XPT2046 interrupt handler to the GPIO interrupt
 * enable  - Enable or disable the GPIO interrupt
 * clear   - Acknowledge/clear any pending GPIO interrupt
 * pendown - Return the state of the pen down GPIO input
 */

static int ioe_irq_handler(FAR struct ioexpander_dev_s *dev,
                           ioe_pinset_t pinset, FAR void *arg)
{
  xcpt_t handler = (xcpt_t)arg;

  return handler(DUMMY_IRQ_NUM, NULL, NULL);
}

static int ads7843e_tsc_attach(FAR struct ads7843e_config_s *state, xcpt_t handler)
{
  /* Attach then enable the touchscreen interrupt handler */

  ioe_pinset_t pinset = 0;
  pinset = 0x8000000000;

  (void)IOEP_ATTACH(g_ioe[CONFIG_ADS7843E_IOEDEV], pinset, ioe_irq_handler, handler);
  return OK;
}

static void ads7843e_tsc_enable(FAR struct ads7843e_config_s *state, bool enable)
{
  iinfo("enable:%d\n", enable);
  if (enable)
    {
      /* Enable PENIRQ interrupts.  NOTE: The pin interrupt is enabled from worker thread
       * logic after completion of processing of the touchscreen interrupt.
       */

      (void)IOEXP_SETOPTION(g_ioe[CONFIG_ADS7843E_IOEDEV], CONFIG_ADS7843E_PENIRQ_GPIO,
                             IOEXPANDER_OPTION_INTCFG,
                             (FAR void *)IOEXPANDER_VAL_LOW);

    }
  else
    {
      /* Disable PENIRQ interrupts.  NOTE: The PENIRQ interrupt will be disabled from
       * interrupt handling logic.
       */

      (void)IOEXP_SETOPTION(g_ioe[CONFIG_ADS7843E_IOEDEV], CONFIG_ADS7843E_PENIRQ_GPIO,
                             IOEXPANDER_OPTION_INTCFG,
                             (FAR void *)IOEXPANDER_VAL_DISABLE);

    }
}

static void ads7843e_tsc_clear(FAR struct ads7843e_config_s *state)
{
  /* Does nothing */
}

static bool ads7843e_tsc_busy(FAR struct ads7843e_config_s *state)
{
  /* Hmmm... The ADS7843E BUSY pin is not brought out on the Shenzhou board.
   * We will most certainly have to revisit this.  There is this cryptic
   * statement in the XPT2046 spec:  "No DCLK delay required with dedicated
   * serial port."
   *
   * The busy state is used by the ADS7843E driver to control the delay
   * between sending the command, then reading the returned data.
   */

  return false;
}

static bool ads7843e_tsc_pendown(FAR struct ads7843e_config_s *state)
{
  /* XPT2046 uses an an internal pullup resistor.  The PENIRQ output goes low
   * due to the current path through the touch screen to ground, which
   * initiates an interrupt to the processor via TP_INT.
   */

  bool pendown;
  IOEXP_READPIN(g_ioe[CONFIG_ADS7843E_IOEDEV], CONFIG_ADS7843E_PENIRQ_GPIO, &pendown);
  iinfo("pendown:%d\n", pendown);
  return !pendown;
}

static void gpio_setoption(void)
{
  (void)IOEXP_SETDIRECTION(g_ioe[CONFIG_ADS7843E_IOEDEV], CONFIG_ADS7843E_PENIRQ_GPIO,
                            IOEXPANDER_DIRECTION_IN);
  (void)IOEXP_SETOPTION(g_ioe[CONFIG_ADS7843E_IOEDEV], CONFIG_ADS7843E_PENIRQ_GPIO,
                            IOEXPANDER_OPTION_INVERT,
                            (FAR void *)IOEXPANDER_VAL_NORMAL);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ap_tsc_setup
 *
 * Description:
 *   This function is called by board-bringup logic to configure the
 *   touchscreen device.  This function will register the driver as
 *   /dev/inputN where N is the minor device number.
 *
 * Input Parameters:
 *   dev - The input device minor number
 *
 * Returned Value:
 *   Zero is returned on success.  Otherwise, a negated errno value is
 *   returned to indicate the nature of the failure.
 *
 ****************************************************************************/

int ap_ads7843e_tsc_setup(void)
{
  int ret;

  /* Configure and enable the XPT2046 PENIRQ pin as an interrupting input. */

  gpio_setoption();

  /* Initialize and register the SPI touchscreen device */

  ret = ads7843e_register(g_spi[CONFIG_ADS7843E_SPIDEV],
                           &g_ads7843e_tscinfo, CONFIG_ADS7843E_DEVMINOR);
  if (ret < 0)
    {
      ierr("ERROR: Failed to register touchscreen device minor=%d\n",
           CONFIG_ADS7843E_DEVMINOR);
      /* up_spiuninitialize(dev); */
      return -ENODEV;
    }

  return OK;
}

#endif /* CONFIG_INPUT_ADS7843E */
