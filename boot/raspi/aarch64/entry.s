# /*
#     Copyright (c) 2016, Thierry Tremblay
#     All rights reserved.

#     Redistribution and use in source and binary forms, with or without
#     modification, are permitted provided that the following conditions are met:

#     * Redistributions of source code must retain the above copyright notice, this
#       list of conditions and the following disclaimer.

#     * Redistributions in binary form must reproduce the above copyright notice,
#       this list of conditions and the following disclaimer in the documentation
#       and/or other materials provided with the distribution.

#     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#     AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#     IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#     FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#     DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#     SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#     OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# */


.section .boot

      org = 0x80000

.global _start

    // The bootloader passes 3 arguments:
    //  x0 = Device Tree Blob (dtb)
    //  x1 = 0 (reserved for future use)
    //  x2 = 0 (reserved for future use)
    //  x3 = 0 (reserved for future use)
    //
    // Preserve these registers! We want to pass them to raspi_main()

_start:

#    // Turn on unaligned memory access
#    mrc p15, #0, r4, c1, c0, #0
#    orr r4, #0x400000
#    mcr p15, #0, r4, c1, c0, #0

    // Initialize the stack (there is nothing we care about between ATAGS at 0x100 and 0x80000)
    mov sp, #0x80000

    // Initialize FPU
#    ldr r3, =(0xF << 20)
#    mcr p15, #0, r3, c1, c0, #2
#    mov r3, #0x40000000
#    vmsr FPEXC, r3

    // Jump to raspi_main
    bl raspi_main

.halt:
    #cpsid if    // TODO: is this the right way to disable interrupts?
    wfi
    b .halt



// Helper to introduce CPU delay
.global cpu_delay
cpu_delay:
    ret
