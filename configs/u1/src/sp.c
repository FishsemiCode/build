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
#include <nuttx/environ.h>
#include <nuttx/ioexpander/ioexpander.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/power/consumer.h>

#include <arch/board/board.h>
#include <string.h>

#if defined(CONFIG_U1_SP) || defined(CONFIG_U1_RECOVERY)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define putreg32(v,a)               (*(volatile uint32_t *)(a) = (v))
#define MUX_PIN18                   (0xb0050038)

#define TOP_PMICFSM_BASE            (0xb2010000)
#define TOP_PMICFSM_PLLTIME         (TOP_PMICFSM_BASE + 0xe8)
#define TOP_PMICFSM_WAKEUP_REASON   (TOP_PMICFSM_BASE + 0x18)
#define TOP_PMICFSM_RTC             (1 << 3)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_MTD_GD25
static int evb_ldo4_init(void)
{
  FAR struct regulator *reg;

  reg = regulator_get(NULL, "ldo4");
  if (!reg)
    {
      syslog(LOG_ERR, "failed to get ldo4\n");
      return -ENODEV;
    }

  /* provide different voltage for gd25 between module board and evb:
   * evb    ---> 3.3V */
  regulator_set_voltage(reg, 3300000, 3300000);

  if (regulator_enable(reg))
    {
      regulator_put(reg);
      syslog(LOG_ERR, "failed to enable ldo4\n");
      return -EINVAL;
    }

  /* wait for the power to be stable */
  usleep(5000);

  return 0;
}
#endif

static int antsw_ldo4_init(char * u1bx_ver)
{
  FAR struct regulator *reg;

  reg = regulator_get(NULL, "ldo4");
  if (!reg)
    {
      syslog(LOG_ERR, "failed to get ldo4\n");
      return -ENODEV;
    }

  /*enable ldo4 and set 3.0v to supply ANT-SW on U1BOX version A*/
  /*with no other versions currently*/
  regulator_set_voltage(reg, 3000000, 3000000);

  if (regulator_enable(reg))
    {
      regulator_put(reg);
      syslog(LOG_ERR, "failed to enable ldo4\n");
      return -EINVAL;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void board_earlyinitialize(void)
{
}

void board_lateinitialize(void)
{
  FAR struct mtd_dev_s *mtd;
  char *id;

  id = getenv_global("board-id");
  if(id && !strncmp(id, "U1BX", 4))
    {
      /*ldo4 -> antenna switch*/
      antsw_ldo4_init(id+4);
      /*nand flash on board*/
#ifdef CONFIG_MTD_GD5F
      mtd = gd5f_initialize(g_spi[1], 1);
      if(mtd)
        {
          register_mtddriver("/dev/data-nand", mtd, 0, mtd);
        }
#endif
    }
  else if(id && !strncmp(id, "U1SK", 4))
    {
      /*ldo4 -> antenna switch*/
      antsw_ldo4_init(id+4);
    }
  else
    {
#ifdef CONFIG_MTD_GD25
      /*ldo4 -> spi flash*/
      evb_ldo4_init();
#endif
    }

#ifdef CONFIG_MTD_GD25
  mtd = gd25_initialize(g_spi[1], 0);
  if(mtd)
    {
      register_mtddriver("/dev/data", mtd, 0, mtd);
      setenv_global("external-flash", "Y", 1);
    }
  else
    {
      setenv_global("external-flash", "N", 1);
    }
#endif

  if (id && !strncmp(id, "DTU1", 4))
    {
      syslog(0, "It is DTU1 board!\n");
      IOEXP_SETDIRECTION(g_ioe[0], 38, IOEXPANDER_DIRECTION_OUT);
      IOEXP_SETDIRECTION(g_ioe[0], 39, IOEXPANDER_DIRECTION_OUT);
      IOEXP_WRITEPIN(g_ioe[0], 38, true);
      IOEXP_WRITEPIN(g_ioe[0], 39, true);
    }
}

void board_finalinitialize(void)
{
}

#endif
