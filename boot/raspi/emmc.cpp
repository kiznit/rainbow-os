/*
    Copyright (c) 2016, Thierry Tremblay
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "emmc.hpp"

#define EMMC_ARG2           0x00
#define EMMC_BLKSIZECNT     0x04
#define EMMC_ARG1           0x08
#define EMMC_CMDTM          0x0C
#define EMMC_RESP0          0x10
#define EMMC_RESP1          0x14
#define EMMC_RESP2          0x18
#define EMMC_RESP3          0x1C
#define EMMC_DATA           0x20
#define EMMC_STATUS         0x24
#define EMMC_CONTROL0       0x28
#define EMMC_CONTROL1       0x2C
#define EMMC_INTERRUPT      0x30
#define EMMC_IRPT_MASK      0x34
#define EMMC_IRPT_EN        0x38
#define EMMC_CONTROL2       0x3C
#define EMMC_FORCE_IRPT     0x50
#define EMMC_BOOT_TIMEOUT   0x70
#define EMMC_DBG_SEL        0x74
#define EMMC_EXRDFIFO_CFG   0x80
#define EMMC_EXRDFIFO_EN    0x84
#define EMMC_TUNE_STEP      0x88
#define EMMC_TUNE_STEPS_STD 0x8C
#define EMMC_TUNE_STEPS_DDR 0x90
#define EMMC_SPI_INT_SPT    0xF0
#define EMMC_SLOTISR_VER    0xFC



ExtendedMassMediaController::ExtendedMassMediaController(const MachineDescription& machine)
{
    m_registers = machine.peripheral_base + 0x00300000;
}



int ExtendedMassMediaController::PowerOn()
{
    return -1;
}



int ExtendedMassMediaController::PowerOff()
{
    return -1;
}
