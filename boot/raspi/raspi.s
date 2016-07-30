.section .boot

      org = 0x8000

.global _start

_start:
      mov    sp, #0x8000
      bl      LedInit
Boucle:
      bl      LedOn
      mov    r0, #500
      bl      Delay
      bl      LedOff
      mov    r0, #500
      bl      Delay
      b      Boucle


      .balign 16
Request:
      .word      0x1c
      .word      0
      .word      0x00040010
      .word      4
      .word      0
      .word      0
      .word      0

pLed:   .word      0
pBal:   .word      0x3f00b880
pHtr:   .word      0x3f003004
LedInit:
      push   {r0, r1, lr}
      ldr    r1, pBal
LedInit1:
      ldr    r0, [r1, #0x18]
      ands   r0, #0x80000000
      bne    LedInit1
      adr    r0, Request + 8
      str    r0, [r1, #0x20]
LedInit2:
      ldr    r0, [r1, #0x18]
      ands   r0, #0x40000000
      bne    LedInit2
      ldr    r0, Request + 0x14
      bic    r0, #0xc0000000
      str    r0, pLed
      pop    {r0, r1, pc}


LedOn:
      push   {r0, r1, lr}
      ldr    r1, pLed
      ldr    r0, [r1]
      add    r0, #0x00010000
      str    r0, [r1]
      pop    {r0, r1, pc}


LedOff:
      push   {r0, r1, lr}
      ldr    r1, pLed
      ldr    r0, [r1]
      add    r0, #0x00000001
      str    r0, [r1]
      pop    {r0, r1, pc}

Delay:
//      r0 = mS
      push   {r0, r1, r2, lr}
      mov    r1, #1000
      mul    r0, r1
      ldr    r1, pHtr
      ldr    r2, [r1]
Delay1:
      ldr    r3, [r1]
      sub    r3, r2
      cmp    r3, r0
      blo    Delay1
      pop    {r0, r1, r2, pc}
