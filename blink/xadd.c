/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2022 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/
#include <limits.h>
#include <stdatomic.h>

#include "blink/alu.h"
#include "blink/assert.h"
#include "blink/endian.h"
#include "blink/log.h"
#include "blink/machine.h"
#include "blink/modrm.h"
#include "blink/mop.h"
#include "blink/swap.h"

void OpXaddEbGb(P) {
  u8 x, y, z, *p, *q;
  p = GetModrmRegisterBytePointerWrite1(A);
  q = ByteRexrReg(m, rde);
  x = Load8(p);
  y = Get8(q);
#if !defined(__riscv) && !defined(__MICROBLAZE__)
  do {
    z = Add8(x, y, &m->flags);
    Put8(q, x);
    if (!Lock(rde)) {
      *p = z;
      break;
    }
  } while (!atomic_compare_exchange_weak_explicit(
      (atomic_uchar *)p, &x, *q, memory_order_release, memory_order_relaxed));
#else
  if (!Lock(rde)) {
    z = Add8(x, y, &m->flags);
    Put8(q, x);
    Store8(p, z);
  } else {
    LOGF("can't %s on this platform", "lock xaddb");
    OpUdImpl(m);
  }
#endif
}

void OpXaddEvqpGvqp(P) {
  u8 *p, *q;
  q = RegRexrReg(m, rde);
  p = GetModrmRegisterWordPointerWriteOszRexw(A);
  if (Rexw(rde)) {
    u64 x, y, z;
    if (Lock(rde)) {
#if LONG_BIT == 64
      if (!((intptr_t)p & 7)) {
        x = atomic_load_explicit((_Atomic(u64) *)p, memory_order_acquire);
        y = atomic_load_explicit((_Atomic(u64) *)q, memory_order_relaxed);
        y = Little64(y);
        do {
          atomic_store_explicit((_Atomic(u64) *)q, x, memory_order_relaxed);
          z = Little64(kAlu[ALU_ADD][ALU_INT64](Little64(x), y, &m->flags));
        } while (!atomic_compare_exchange_weak_explicit((_Atomic(u64) *)p, &x,
                                                        z, memory_order_release,
                                                        memory_order_relaxed));
      } else {
        LOGF("can't %s misaligned address", "lock xaddq");
        OpUdImpl(m);
      }
#else
      LOGF("can't %s on this platform", "lock xaddq");
      OpUdImpl(m);
#endif
    } else {
      x = Load64(p);
      y = Get64(q);
      z = kAlu[ALU_ADD][ALU_INT64](x, y, &m->flags);
      Put64(q, x);
      Store64(p, z);
    }
  } else if (!Osz(rde)) {
    u32 x, y, z;
    if (Lock(rde)) {
      if (!((intptr_t)p & 3)) {
        x = atomic_load_explicit((_Atomic(u32) *)p, memory_order_acquire);
        y = atomic_load_explicit((_Atomic(u32) *)q, memory_order_relaxed);
        y = Little32(y);
        do {
          atomic_store_explicit((_Atomic(u32) *)q, x, memory_order_relaxed);
          z = Little32(kAlu[ALU_ADD][ALU_INT32](Little32(x), y, &m->flags));
        } while (!atomic_compare_exchange_weak_explicit((_Atomic(u32) *)p, &x,
                                                        z, memory_order_release,
                                                        memory_order_relaxed));
      } else {
        LOGF("can't %s misaligned address", "lock xaddq");
        OpUdImpl(m);
      }
    } else {
      x = Load32(p);
      y = Get32(q);
      z = kAlu[ALU_ADD][ALU_INT32](x, y, &m->flags);
      Put32(q, x);
      Store32(p, z);
    }
    Put32(q + 4, 0);
    if (IsModrmRegister(rde)) {
      Put32(p + 4, 0);
    }
  } else {
    u16 x, y, z;
    unassert(!Lock(rde));
    x = Load16(p);
    y = Get16(q);
    z = kAlu[ALU_ADD][ALU_INT16](x, y, &m->flags);
    Put16(q, x);
    Store16(p, z);
  }
}
