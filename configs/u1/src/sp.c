/****************************************************************************
 * configs/u1/src/sp.c
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
#include <nuttx/ioexpander/ioexpander.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/power/consumer.h>

#include <arch/board/board.h>

#ifdef CONFIG_U1_SP

#define putreg32(v,a)               (*(volatile uint32_t *)(a) = (v))
#define MUX_PIN18                   (0xb0050038)

#ifdef CONFIG_MTD_GD25
static void board_flash_init(void)
{
  FAR struct mtd_dev_s *mtd;
  FAR struct regulator *reg;
  FAR bool module;

  reg = regulator_get(NULL, "ldo4");
  if (!reg)
    {
      syslog(LOG_ERR, "failed to get ldo4\n");
      return;
    }

  /* XXX: temporarily set the board_id gpio pinmux, later
   * we will use official pinmux api instead */
  putreg32(0x12, MUX_PIN18);
  IOEXP_SETDIRECTION(g_ioe[0], 18, IOEXPANDER_DIRECTION_IN);
  IOEXP_READPIN(g_ioe[0], 18, &module);

  /* provide different voltage for gd25 between module board and evb:
   * module ---> 1.8V
   * evb    ---> 3.3V */
  if (module)
    regulator_set_voltage(reg, 1800000, 1800000);
  else
    regulator_set_voltage(reg, 3300000, 3300000);

  if (regulator_enable(reg))
    {
      regulator_put(reg);
      syslog(LOG_ERR, "failed to enable ldo4\n");
      return;
    }

  /* wait for the power to be stable */
  usleep(5000);

  mtd = gd25_initialize(g_spi[1]);
  register_mtddriver("/dev/data", mtd, 0, mtd);
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
  board_flash_init();
#endif
}

void board_finalinitialize(void)
{
}

#endif
