//
// os/unix/x86_64/fpu/fpu_cmp_f_64.h
//
// This file is subject to the terms and conditions defined in
// 'LICENSE', which is part of this source code package.
//

#include <emmintrin.h>
#include <string.h>

static inline uint8_t fpu_cmp_f_64(
  const uint64_t *fs, const uint64_t *ft) {
  double fs_double, ft_double;
  __m128d fs_reg, ft_reg;

  // Prevent aliasing.
  memcpy(&fs_double, fs, sizeof(fs_double));
  memcpy(&ft_double, ft, sizeof(ft_double));

  fs_reg = _mm_set_sd(fs_double);
  ft_reg = _mm_set_sd(ft_double);

  __asm__ __volatile__(
    "comisd %0, %1\n\t"
    :: "x" (fs_reg),
       "x" (ft_reg)
    : "cc"
  );

  return 0;
}

