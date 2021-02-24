/****************************************************************************
 * configs/u1/src/init.d/rcS.sp
 *
 *   Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *   Author: Pinecone <Pinecone@pinecone.net>
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

#ifdef CONFIG_FS_LITTLEFS

  /* Mount RAMDISK */

  #ifdef CONFIG_SONG_RAMDISK
    mount -t littlefs -o autoformat /dev/ram1 /disksp
    if [ $? -eq 0 ]; then
      mkdir /disksp/retent >/dev/null
    fi
  #endif

  /* Mount externl nand-flash to /nand */

#ifdef CONFIG_MTD_GD5F
  mount -t littlefs -o autoformat /dev/data-nand /nand > /dev/null
#endif

  /* Mount external flash to /persist */

  mount -t littlefs -o autoformat /dev/data /persist >/dev/null
  /* if [ $? -eq 0 ]; then
    mkdir /persist/data >/dev/null
    mkdir /persist/services >/dev/null
    mkdir /persist/log >/dev/null
    ln -s /persist/data data
    ln -s /persist/services services
    ln -s /persist/log log
  fi */

  /* Mount internal flash to /onchip */

  /* mount -t littlefs -o autoformat /dev/persist /onchip */

#endif

#ifdef CONFIG_RPMSG_USRSOCK
  usrsock &
#endif

#ifdef CONFIG_SERVICES_SOFTSIM
softsim &
#endif
