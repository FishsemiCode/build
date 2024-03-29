/****************************************************************************
 * configs/u1/src/ap.c
 *
 *   Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *   Author: Xiang Xiao <xiaoxiang@pinecone.net>
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

#include <nuttx/arch.h>
#include <arch/board/board.h>
#include <nuttx/ioexpander/ioexpander.h>
#include <nuttx/ioexpander/gpio.h>
#include <nuttx/pinctrl/pinctrl.h>
#include <nuttx/leds/fishled.h>
#include <nuttx/lcd/vk1056b_lcd.h>
#include <nuttx/lcd/ili9486_lcd.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/i2c/tca6424a.h>
#include <nuttx/sensors/cs1237.h>

#if defined(CONFIG_U1_AP) || defined(CONFIG_U1_APLITE) || \
      defined(CONFIG_U1_APFACTORY) || defined(CONFIG_U1_APU1X) || \
      defined(CONFIG_U1_APFISHNB)

/****************************************************************************
 * Public Functions Prototypes
 ****************************************************************************/

#ifdef CONFIG_INPUT_ADS7843E
extern int ap_ads7843e_tsc_setup(void);
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_LCD_VK1056B
static void up_lcd_vk1056b_init(void)
{
  static const struct lcd_vk1056b_config_s config =
  {
    .power_gpio = 37,
    .cs_gpio = 22,
    .wr_gpio = 21,
    .data_gpio = 19,
  };

  lcd_vk1056b_register(&config, g_ioe[0]);
}
#endif

#ifdef CONFIG_LCD_ILI9486
static void up_lcd_ili9486_init(void)
{
  static const struct lcd_ili9486_config_s config =
  {
    .power_gpio = 37,
    .rst_gpio = 36,
    .spi_cs_num = 0,
    .spi_rs_gpio = 38,
    /* freq max 27M */
    .spi_freq = 26000000,
    .spi_nbits = 16,
  };

  lcd_ili9486_register(&config, g_spi[0], g_ioe[0]);
}
#endif

#ifdef CONFIG_SENSORS_CS1237
static void cs1237_init(void)
{
  static struct cs1237_config_s config =
  {
    .refo = CS1237_REFO_ON,
    .pga = CS1237_PGA_1,
    .freq = CS1237_FREQ_10HZ,
    .ch = CS1237_CH_A,
    .clk_io = 14,
    .data_io = 13,
  };

  cs1237_register("/dev/cs12370", g_pinctrl[0], g_ioe[0], &config);
}
#endif

#ifdef CONFIG_FISHLED
static void fishled_init(void)
{
  static const struct led_config_s config[] =
  {
    {
      .gpio_port = 31,
      .level = 1,
    },
    {
      .gpio_port = 32,
      .level = 0,
    },
  };

  fishled_initialize(g_ioe[0], &config[0], sizeof(config) / sizeof((config)[0]));
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void board_earlyinitialize(void)
{
}

void board_lateinitialize(void)
{
#ifdef CONFIG_MTD_GD25
  FAR struct mtd_dev_s *mtd;
#endif

#ifdef CONFIG_FISHLED
  fishled_init();
#endif

#ifdef CONFIG_SNSHUB_DRIVER_ICM42605
  gpio_lower_half(g_ioe[0], 10, GPIO_INTERRUPT_RISING_PIN, 10);
#endif

#ifdef CONFIG_LCD_VK1056B
  up_lcd_vk1056b_init();
#endif

#ifdef CONFIG_LCD_ILI9486
  up_lcd_ili9486_init();
#endif

#ifdef CONFIG_INPUT_ADS7843E
  ap_ads7843e_tsc_setup();
#endif

#ifdef CONFIG_MTD_GD25
  mtd = gd25_initialize(g_spi[0], 0);
  if(mtd)
    {
      register_mtddriver("/dev/ctdata", mtd, 0, mtd);
    }
#endif

#ifdef CONFIG_I2C_TCA6424A
  tca6424a_register(g_i2c[0], 0);
  tca6424a_register(g_i2c[1], 1);
#endif

#ifdef CONFIG_SENSORS_CS1237
  cs1237_init();
#endif
}

void board_finalinitialize(void)
{
}

#endif /* defined(CONFIG_U1_AP) || defined(CONFIG_U1_APLITE)
          defined(CONFIG_U1_APFACTORY) || defined(CONFIG_U1_APU1X)  */
