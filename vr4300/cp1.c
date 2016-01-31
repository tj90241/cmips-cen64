//
// vr4300/cp1.c: VR4300 floating point unit coprocessor.
//
// CEN64: Cycle-Accurate Nintendo 64 Emulator.
// Copyright (C) 2015, Tyler J. Stachecki.
//
// This file is subject to the terms and conditions defined in
// 'LICENSE', which is part of this source code package.
//

#include "common.h"
#include "fpu/fpu.h"
#include "vr4300/cp1.h"
#include "vr4300/cpu.h"
#include "vr4300/decoder.h"
#include "vr4300/fault.h"

//
// Raises a MCI interlock for a set number of cycles.
//
static inline int vr4300_do_mci(struct vr4300 *vr4300, unsigned cycles) {
  vr4300->pipeline.cycles_to_stall = cycles - 1;
  vr4300->regs[PIPELINE_CYCLE_TYPE] = 3;
  return 1;
}

//
// ABS.fmt
//
int VR4300_CP1_ABS(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32, fd32;
  uint64_t result;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_abs_32(&fs32, &fd32);
      result = fd32;
      break;

    case VR4300_FMT_D:
      fpu_abs_64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 3);
}

//
// ADD.fmt
//
int VR4300_CP1_ADD(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32, ft32, fd32;
  uint64_t result;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;
      ft32 = ft;

      fpu_add_32(&fs32, &ft32, &fd32);
      result = fd32;
      break;

    case VR4300_FMT_D:
      fpu_add_64(&fs, &ft, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 3);
}

//
// BC1F
// BC1FL
// BC1T
// BC1TL
//
int VR4300_BC1(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_icrf_latch *icrf_latch = &vr4300->pipeline.icrf_latch;
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  unsigned opcode = (iw >> 16) & 0x3;

  uint64_t offset = (uint64_t) ((int16_t) iw) << 2;
  uint64_t taken_pc = rfex_latch->common.pc + (offset + 4);
  uint32_t cond = vr4300->regs[VR4300_CP1_FCR31];

  // XXX: The VR4300 manual says that the results of a FPU
  // FCR writes aren't ready on the next cycle, but it seems
  // that this might actually not be a limitiation of the real
  // hardware?
  if (vr4300->pipeline.dcwb_latch.dest == VR4300_CP1_FCR31)
    cond = vr4300->pipeline.dcwb_latch.result;

  switch (opcode) {
    case 0x0: // BC1F
      if (!(cond >> 23 & 0x1))
        icrf_latch->pc = taken_pc;
      break;

    case 0x1: // BC1T
      if (cond >> 23 & 0x1)
        icrf_latch->pc = taken_pc;
      break;

    case 0x2: // BC1FL
      if (!(cond >> 23 & 0x1))
        icrf_latch->pc = taken_pc;
      else
        rfex_latch->iw_mask = 0;
      break;

    case 0x3: // BC1TL
      if (cond >> 23 & 0x1)
        icrf_latch->pc = taken_pc;
      else
        rfex_latch->iw_mask = 0;
      break;
  }

  return 0;
}

//
// C.eq.fmt
// C.seq.fmt
//
int VR4300_CP1_C_EQ_C_SEQ(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = VR4300_CP1_FCR31;
  uint64_t result = vr4300->regs[dest];

  uint32_t fs32, ft32;
  uint8_t flag;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;
      ft32 = ft;

      result &= ~(1 << 23);
      flag = fpu_cmp_eq_32(&fs32, &ft32);
      break;

    case VR4300_FMT_D:
      result &= ~(1 << 23);
      flag = fpu_cmp_eq_64(&fs, &ft);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result | (flag << 23);
  exdc_latch->dest = dest;
  return 0;
}

//
// C.f.fmt
// C.sf.fmt
//
int VR4300_CP1_C_F_C_SF(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = VR4300_CP1_FCR31;
  uint64_t result = vr4300->regs[dest];

  uint32_t fs32, ft32;
  uint8_t flag;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;
      ft32 = ft;

      result &= ~(1 << 23);
      flag = fpu_cmp_f_32(&fs32, &ft32);
      break;

    case VR4300_FMT_D:
      result &= ~(1 << 23);
      flag = fpu_cmp_f_64(&fs, &ft);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result | (flag << 23);
  exdc_latch->dest = dest;
  return 0;
}

//
// C.ole.fmt
// C.le.fmt
//
int VR4300_CP1_C_OLE_C_LE(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = VR4300_CP1_FCR31;
  uint64_t result = vr4300->regs[dest];

  uint32_t fs32, ft32;
  uint8_t flag;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;
      ft32 = ft;

      result &= ~(1 << 23);
      flag = fpu_cmp_ole_32(&fs32, &ft32);
      break;

    case VR4300_FMT_D:
      result &= ~(1 << 23);
      flag = fpu_cmp_ole_64(&fs, &ft);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result | (flag << 23);
  exdc_latch->dest = dest;
  return 0;
}

//
// C.olt.fmt
// C.lt.fmt
//
int VR4300_CP1_C_OLT_C_LT(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = VR4300_CP1_FCR31;
  uint64_t result = vr4300->regs[dest];

  uint32_t fs32, ft32;
  uint8_t flag;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;
      ft32 = ft;

      result &= ~(1 << 23);
      flag = fpu_cmp_olt_32(&fs32, &ft32);
      break;

    case VR4300_FMT_D:
      result &= ~(1 << 23);
      flag = fpu_cmp_olt_64(&fs, &ft);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result | (flag << 23);
  exdc_latch->dest = dest;
  return 0;
}

//
// C.ueq.fmt
// C.ngl.fmt
//
int VR4300_CP1_C_UEQ_C_NGL(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = VR4300_CP1_FCR31;
  uint64_t result = vr4300->regs[dest];

  uint32_t fs32, ft32;
  uint8_t flag;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;
      ft32 = ft;

      result &= ~(1 << 23);
      flag = fpu_cmp_ueq_32(&fs32, &ft32);
      break;

    case VR4300_FMT_D:
      result &= ~(1 << 23);
      flag = fpu_cmp_ueq_64(&fs, &ft);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result | (flag << 23);
  exdc_latch->dest = dest;
  return 0;
}

//
// C.ule.fmt
// C.ngt.fmt
//
int VR4300_CP1_C_ULE_C_NGT(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = VR4300_CP1_FCR31;
  uint64_t result = vr4300->regs[dest];

  uint32_t fs32, ft32;
  uint8_t flag;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;
      ft32 = ft;

      result &= ~(1 << 23);
      flag = fpu_cmp_ule_32(&fs32, &ft32);
      break;

    case VR4300_FMT_D:
      result &= ~(1 << 23);
      flag = fpu_cmp_ule_64(&fs, &ft);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result | (flag << 23);
  exdc_latch->dest = dest;
  return 0;
}

//
// C.ult.fmt
// C.nge.fmt
//
int VR4300_CP1_C_ULT_C_NGE(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = VR4300_CP1_FCR31;
  uint64_t result = vr4300->regs[dest];

  uint32_t fs32, ft32;
  uint8_t flag;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;
      ft32 = ft;

      result &= ~(1 << 23);
      flag = fpu_cmp_ult_32(&fs32, &ft32);
      break;

    case VR4300_FMT_D:
      result &= ~(1 << 23);
      flag = fpu_cmp_ult_64(&fs, &ft);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result | (flag << 23);
  exdc_latch->dest = dest;
  return 0;
}

//
// C.un.fmt
// C.ngle.fmt
//
int VR4300_CP1_C_UN_C_NGLE(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = VR4300_CP1_FCR31;
  uint64_t result = vr4300->regs[dest];

  uint32_t fs32, ft32;
  uint8_t flag;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;
      ft32 = ft;

      result &= ~(1 << 23);
      flag = fpu_cmp_un_32(&fs32, &ft32);
      break;

    case VR4300_FMT_D:
      result &= ~(1 << 23);
      flag = fpu_cmp_un_64(&fs, &ft);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result | (flag << 23);
  exdc_latch->dest = dest;
  return 0;
}

//
// CEIL.l.fmt
//
int VR4300_CP1_CEIL_L(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32;
  uint64_t result;

#ifndef CEN64_ARCH_HAS_CEIL
  fpu_state_t saved_state = fpu_get_state();
  fpu_set_state((saved_state & ~FPU_ROUND_MASK) | FPU_ROUND_POSINF);

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_cvt_i64_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_cvt_i64_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  fpu_set_state((saved_state & FPU_ROUND_MASK) |
    (fpu_get_state() & ~FPU_ROUND_MASK));
#else
  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_ceil_i64_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_ceil_i64_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }
#endif

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 5);
}

//
// CEIL.w.fmt
//
int VR4300_CP1_CEIL_W(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32;
  uint32_t result;

#ifndef CEN64_ARCH_HAS_CEIL
  fpu_state_t saved_state = fpu_get_state();
  fpu_set_state((saved_state & ~FPU_ROUND_MASK) | FPU_ROUND_POSINF);

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_cvt_i32_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_cvt_i32_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  fpu_set_state((saved_state & FPU_ROUND_MASK) |
    (fpu_get_state() & ~FPU_ROUND_MASK));
#else
  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_ceil_i32_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_ceil_i32_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }
#endif

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 5);
}

//
// CFC1
//
int VR4300_CFC1(struct vr4300 *vr4300,
  uint32_t iw, uint64_t rs, uint64_t rt) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  unsigned dest = GET_RT(iw);
  unsigned src = GET_RD(iw);
  uint64_t result;

  switch (src) {
    case 0: src = VR4300_CP1_FCR0; break;
    case 31: src = VR4300_CP1_FCR31; break;

    default:
      src = 0;

      assert(0 && "CFC1: Read reserved FCR.");
      break;
  }

  result = vr4300->regs[src];

  // XXX: The VR4300 manual says that the results of a FPU
  // FCR writes aren't ready on the next cycle, but it seems
  // that this might actually not be a limitiation of the real
  // hardware?
  if (vr4300->pipeline.dcwb_latch.dest == VR4300_CP1_FCR31)
    result = vr4300->pipeline.dcwb_latch.result;

  // Undefined while the next instruction
  // executes, so we can cheat and use the RF.
  exdc_latch->result = (int32_t) result;
  exdc_latch->dest = dest;
  return 0;
}

//
// CTC1
//
// XXX: Raise exception on cause/enable.
// XXX: In such cases, ensure write occurs.
//
int VR4300_CTC1(struct vr4300 *vr4300,
  uint32_t iw, uint64_t rs, uint64_t rt) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  unsigned dest = GET_RD(iw);

  if (dest == 31)
    dest = VR4300_CP1_FCR31;

  else {
    assert(0 && "CTC1: Write to fixed/reserved FCR.");

    dest = 0;
    rt = 0;
  }

  // Undefined while the next instruction
  // executes, so we can cheat and use WB.
  exdc_latch->result = rt;
  exdc_latch->dest = dest;
  return 0;
}

//
// CVT.d.fmt
//
int VR4300_CP1_CVT_D(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32;
  uint64_t result;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_cvt_f64_f32(&fs32, &result);
      break;

    case VR4300_FMT_W:
      fs32 = fs;

      fpu_cvt_f64_i32(&fs32, &result);
      break;

    case VR4300_FMT_L:
      fpu_cvt_f64_i64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return fmt != VR4300_FMT_S
    ? vr4300_do_mci(vr4300, 5)
    : 0;
}

//
// CVT.l.fmt
//
int VR4300_CP1_CVT_L(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32;
  uint64_t result;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_cvt_i64_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_cvt_i64_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 5);
}

//
// CVT.s.fmt
//
int VR4300_CP1_CVT_S(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32;
  uint32_t result;

  switch (fmt) {
    case VR4300_FMT_D:
      fpu_cvt_f32_f64(&fs, &result);
      break;

    case VR4300_FMT_W:
      fs32 = fs;

      fpu_cvt_f32_i32(&fs32, &result);
      break;

    case VR4300_FMT_L:
      fpu_cvt_f32_i64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300,
    fmt == VR4300_FMT_D ? 2 : 5);
}

//
// CVT.w.fmt
//
int VR4300_CP1_CVT_W(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32;
  uint32_t result;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_cvt_i32_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_cvt_i32_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 5);
}

//
// DIV.fmt
//
int VR4300_CP1_DIV(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32, ft32, fd32;
  uint64_t result;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;
      ft32 = ft;

      fpu_div_32(&fs32, &ft32, &fd32);
      result = fd32;
      break;

    case VR4300_FMT_D:
      fpu_div_64(&fs, &ft, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300,
    fmt == VR4300_FMT_D ? 58 : 29);
}

//
// DMFC1
//
int VR4300_DMFC1(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t unused(rt)) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  unsigned dest = GET_RT(iw);

  exdc_latch->result = fs;
  exdc_latch->dest = dest;
  return 0;
}

//
// DMTC1
//
int VR4300_DMTC1(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t rt) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  unsigned dest = GET_FS(iw);

  exdc_latch->result = rt;
  exdc_latch->dest = dest;
  return 0;
}

//
// FLOOR.l.fmt
//
int VR4300_CP1_FLOOR_L(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32;
  uint64_t result;

#ifndef CEN64_ARCH_HAS_FLOOR
  fpu_state_t saved_state = fpu_get_state();
  fpu_set_state((saved_state & ~FPU_ROUND_MASK) | FPU_ROUND_NEGINF);

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_cvt_i64_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_cvt_i64_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  fpu_set_state((saved_state & FPU_ROUND_MASK) |
    (fpu_get_state() & ~FPU_ROUND_MASK));
#else
  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_floor_i64_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_floor_i64_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }
#endif

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 5);
}

//
// FLOOR.w.fmt
//
int VR4300_CP1_FLOOR_W(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32;
  uint32_t result;

#ifndef CEN64_ARCH_HAS_FLOOR
  fpu_state_t saved_state = fpu_get_state();
  fpu_set_state((saved_state & ~FPU_ROUND_MASK) | FPU_ROUND_NEGINF);

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_cvt_i32_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_cvt_i32_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  fpu_set_state((saved_state & FPU_ROUND_MASK) |
    (fpu_get_state() & ~FPU_ROUND_MASK));
#else
  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_floor_i32_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_floor_i32_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }
#endif

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 5);
}

//
// LDC1
//
// TODO/FIXME: Check for unaligned addresses.
//
int VR4300_LDC1(struct vr4300 *vr4300,
  uint32_t iw, uint64_t rs, uint64_t rt) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  unsigned dest = GET_FT(iw);

  exdc_latch->request.vaddr = rs + (int16_t) iw;
  exdc_latch->request.data = ~0ULL;
  exdc_latch->request.wdqm = 0ULL;
  exdc_latch->request.postshift = 0;
  exdc_latch->request.access_type = VR4300_ACCESS_DWORD;
  exdc_latch->request.type = VR4300_BUS_REQUEST_READ;
  exdc_latch->request.size = 8;

  exdc_latch->dest = dest;
  exdc_latch->result = 0;
  return 0;
}

//
// LWC1
//
// TODO/FIXME: Check for unaligned addresses.
//
int VR4300_LWC1(struct vr4300 *vr4300,
  uint32_t iw, uint64_t rs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  uint32_t status = vr4300->regs[VR4300_CP0_REGISTER_STATUS];
  uint64_t address = (rs + (int16_t) iw);
  unsigned dest = GET_FT(iw);

  uint64_t result = 0;
  unsigned postshift = 0;

  if (!(status & 0x04000000)) {
    result = dest & 0x1
      ? ft & 0x00000000FFFFFFFFULL
      : ft & 0xFFFFFFFF00000000ULL;

    postshift = (dest & 0x1) << 5;
    dest &= ~0x1;
  }

  exdc_latch->request.vaddr = address;
  exdc_latch->request.data = ~0U;
  exdc_latch->request.wdqm = 0ULL;
  exdc_latch->request.postshift = postshift;
  exdc_latch->request.access_type = VR4300_ACCESS_WORD;
  exdc_latch->request.type = VR4300_BUS_REQUEST_READ;
  exdc_latch->request.size = 4;

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return 0;
}

//
// MUL.fmt
//
int VR4300_CP1_MUL(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32, ft32, fd32;
  uint64_t result;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;
      ft32 = ft;

      fpu_mul_32(&fs32, &ft32, &fd32);
      result = fd32;
      break;

    case VR4300_FMT_D:
      fpu_mul_64(&fs, &ft, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300,
    fmt == VR4300_FMT_D ? 8 : 5);
}

//
// MFC1
//
int VR4300_MFC1(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t unused(rt)) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  uint32_t status = vr4300->regs[VR4300_CP0_REGISTER_STATUS];
  unsigned dest = GET_RT(iw);
  uint64_t result;

  if (status & 0x04000000)
    result = (int32_t) fs;

  else {
    result = (GET_FS(iw) & 0x1)
      ? (int32_t) (fs >> 32)
      : (int32_t) (fs);
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return 0;
}

//
// MOV.fmt
//
int VR4300_CP1_MOV(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  unsigned dest = GET_FD(iw);

  exdc_latch->result = fs;
  exdc_latch->dest = dest;
  return 0;
}

//
// MTC1
//
int VR4300_MTC1(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t rt) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  uint32_t status = vr4300->regs[VR4300_CP0_REGISTER_STATUS];
  uint64_t result = (int32_t) rt;
  unsigned dest = GET_FS(iw);

  if (!(status & 0x04000000)) {
    result = (dest & 0x1)
      ? ((uint32_t) fs) | (rt << 32)
      : (fs & ~0xFFFFFFFFULL) | ((uint32_t) rt);

    dest &= ~0x1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return 0;
}

//
// NEG.fmt
//
int VR4300_CP1_NEG(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32, fd32;
  uint64_t result;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_neg_32(&fs32, &fd32);
      result = fd32;
      break;

    case VR4300_FMT_D:
      fpu_neg_64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return 0;
}

//
// ROUND.l.fmt
//
int VR4300_CP1_ROUND_L(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32;
  uint64_t result;

#ifndef CEN64_ARCH_HAS_ROUND
  fpu_state_t saved_state = fpu_get_state();
  fpu_set_state((saved_state & ~FPU_ROUND_MASK) | FPU_ROUND_NEAREST);

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_cvt_i64_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_cvt_i64_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  fpu_set_state((saved_state & FPU_ROUND_MASK) |
    (fpu_get_state() & ~FPU_ROUND_MASK));
#else
  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_round_i64_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_round_i64_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }
#endif

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 5);
}

//
// ROUND.w.fmt
//
int VR4300_CP1_ROUND_W(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32;
  uint32_t result;

#ifndef CEN64_ARCH_HAS_ROUND
  fpu_state_t saved_state = fpu_get_state();
  fpu_set_state((saved_state & ~FPU_ROUND_MASK) | FPU_ROUND_NEAREST);

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_cvt_i32_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_cvt_i32_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;

  }

  fpu_set_state((saved_state & FPU_ROUND_MASK) |
    (fpu_get_state() & ~FPU_ROUND_MASK));
#else
  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_round_i32_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_round_i32_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }
#endif

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 5);
}

//
// SDC1
//
// TODO/FIXME: Check for unaligned addresses.
//
int VR4300_SDC1(struct vr4300 *vr4300,
  uint32_t iw, uint64_t rs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  exdc_latch->request.vaddr = rs + (int16_t) iw;
  exdc_latch->request.data = ft;
  exdc_latch->request.wdqm = ~0ULL;
  exdc_latch->request.access_type = VR4300_ACCESS_DWORD;
  exdc_latch->request.type = VR4300_BUS_REQUEST_WRITE;
  exdc_latch->request.size = 8;

  return 0;
}

//
// SQRT.fmt
//
int VR4300_CP1_SQRT(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32, fd32;
  uint64_t result;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_sqrt_32(&fs32, &fd32);
      result = fd32;
      break;

    case VR4300_FMT_D:
      fpu_sqrt_64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300,
    fmt == VR4300_FMT_D ? 58 : 29);
}

//
// SUB.fmt
//
int VR4300_CP1_SUB(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32, ft32, fd32;
  uint64_t result;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;
      ft32 = ft;

      fpu_sub_32(&fs32, &ft32, &fd32);
      result = fd32;
      break;

    case VR4300_FMT_D:
      fpu_sub_64(&fs, &ft, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 3);
}

//
// SWC1
//
// TODO/FIXME: Check for unaligned addresses.
//
int VR4300_SWC1(struct vr4300 *vr4300,
  uint32_t iw, uint64_t rs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  uint32_t status = vr4300->regs[VR4300_CP0_REGISTER_STATUS];
  unsigned ft_reg = GET_FT(iw);

  if (!(status & 0x04000000))
    ft >>= ((ft_reg & 0x1) << 5);

  exdc_latch->request.vaddr = rs + (int16_t) iw;
  exdc_latch->request.data = ft;
  exdc_latch->request.wdqm = ~0U;
  exdc_latch->request.access_type = VR4300_ACCESS_WORD;
  exdc_latch->request.type = VR4300_BUS_REQUEST_WRITE;
  exdc_latch->request.size = 4;

  return 0;
}

//
// TRUNC.l.fmt
//
int VR4300_CP1_TRUNC_L(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32;
  uint64_t result;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_trunc_i64_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_trunc_i64_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 5);
}

//
// TRUNC.w.fmt
//
int VR4300_CP1_TRUNC_W(struct vr4300 *vr4300,
  uint32_t iw, uint64_t fs, uint64_t ft) {
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;
  enum vr4300_fmt fmt = GET_FMT(iw);
  unsigned dest = GET_FD(iw);

  uint32_t fs32;
  uint32_t result;

  switch (fmt) {
    case VR4300_FMT_S:
      fs32 = fs;

      fpu_trunc_i32_f32(&fs32, &result);
      break;

    case VR4300_FMT_D:
      fpu_trunc_i32_f64(&fs, &result);
      break;

    default:
      VR4300_INV(vr4300);
      return 1;
  }

  exdc_latch->result = result;
  exdc_latch->dest = dest;
  return vr4300_do_mci(vr4300, 5);
}

// Initializes the coprocessor.
void vr4300_cp1_init(struct vr4300 *vr4300) {
  fpu_set_state(FPU_ROUND_NEAREST | FPU_MASK_EXCPS);
}

