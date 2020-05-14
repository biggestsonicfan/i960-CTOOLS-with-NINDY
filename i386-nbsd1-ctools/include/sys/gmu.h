
/* C program interface to i960(R) Hx Guarded Memory Unit */

/* Functions are available to:
   - define up to two independent protected regions
   - define up to six independent detection regions
   - to enable and disable each protection and detection region
*/

#ifndef __GMU_H__
#define __GMU_H__

#if defined(__i960HA) || defined(__i960HD) || defined(__i960HT)

#define GMU_USER_READ		1
#define GMU_USER_WRITE		2
#define GMU_USER_EXECUTE	4
#define GMU_SUPERVISOR_READ	16
#define GMU_SUPERVISOR_WRITE	32
#define GMU_SUPERVISOR_EXECUTE	64

#define __GMU_GCR   0xff008000
#define __GMU_MPAR0 0xff008010
#define __GMU_MDUB0 0xff008080

/* Definitions of memory-mapped GMU regs */

typedef struct __gmu_protect
{
  volatile unsigned int __gmu_protect_addr_reg;
  volatile unsigned int __gmu_protect_mask_reg;
} * __gmu_protect_regs_type;

typedef struct __gmu_detect
{
  volatile unsigned int __gmu_detect_upper_bound_addr_reg;
  volatile unsigned int __gmu_detect_lower_bound_addr_reg;
} * __gmu_detect_regs_type;

#define __gmu_control_reg_ptr	((volatile unsigned int*)__GMU_GCR)

#define __gmu_protect_regs	((__gmu_protect_regs_type)__GMU_MPAR0)

#define __gmu_detect_regs	((__gmu_detect_regs_type)__GMU_MDUB0)

/*
 * Enable protection for memory region N.
 * region : 0 or 1
 * return : 0 if enabled successfully
 *          1 if not enabled due to invalid region number
 */
#pragma inline gmu_enable_protect_region
static int
gmu_enable_protect_region(unsigned int region)
{
  if (region > 1)
    return 1;

  *__gmu_control_reg_ptr |= 1 << region;
  return 0;
}

/* Disable protection for memory region N.
 * region : 0 or 1
 * return : 0 if disabled successfully
 *          1 if not disabled due to invalid region number
 */
#pragma inline gmu_disable_protect_region
static int
gmu_disable_protect_region(unsigned int region)
{
  if (region > 1)
    return 1;

  *__gmu_control_reg_ptr &= ~(1 << region);
  return 0;
}

/* Define memory protect region N
 * region      : 0 or 1
 * start       : addr of first byte to protect
 * end         : addr of last byte to protect
 * permissions : user or supervisor read/write/execute permissions, constructed
 *               by or'ing GMU_USER_xxx and GMU_SUPERVISOR_xxx values
 * return      : 0 if region extent and permissions set up successfully
 *               1 if set up fails due to invalid region number
 *               2 if set up fails due to selection of entire address space
 */
#pragma inline gmu_define_protect_region
static int
gmu_define_protect_region(unsigned int region,
                          void* start,
                          void* end,
                          unsigned int permissions)
{
  unsigned int block_size;
  unsigned int mask;
  unsigned int region_size;
  unsigned int old_GCR_status;

  if (region > 1 ||
      (permissions & ~(GMU_USER_READ | GMU_USER_WRITE | GMU_USER_EXECUTE |
                       GMU_SUPERVISOR_READ | GMU_SUPERVISOR_WRITE |
                       GMU_SUPERVISOR_EXECUTE)) != 0)
    return 1;

  /* Save old GCR and disable region being modified */
  old_GCR_status = *__gmu_control_reg_ptr;
  gmu_disable_protect_region(region);

  __gmu_protect_regs[region].__gmu_protect_addr_reg = 
            (unsigned int)start & 0xffffff00 | permissions;
  region_size = (unsigned int)end - (unsigned int)start + 1;
  if (region_size > 0x80000000)
    return 2;

  block_size = 256;
  mask = 0xffffff00;
  while (region_size > block_size)
  {
    mask <<= 1;
    block_size <<= 1;
  }
  if (mask == 0)
    return 2;

  __gmu_protect_regs[region].__gmu_protect_mask_reg = mask;

  /* Restore GCR */
  *__gmu_control_reg_ptr = old_GCR_status; 

  return 0;
}

/* Enable detection for memory region N.
 * region : 0 or 1
 * return : 0 if enabled successfully
 *          1 if not enabled due to invalid region number
 */
#pragma inline gmu_enable_detect_region
static int
gmu_enable_detect_region(unsigned int region)
{
  if (region > 5)
    return 1;

  *__gmu_control_reg_ptr |= 1 << (16 + region);
  return 0;
}

/* Disable detection for memory region N.
 * region : 0 or 1
 * return : 0 if disabled successfully
 *          1 if not disabled due to invalid region number
 */
#pragma inline gmu_disable_detect_region
static int
gmu_disable_detect_region(unsigned int region)
{
  if (region > 5)
    return 1;

  *__gmu_control_reg_ptr &= ~(1 << (16 + region));
  return 0;
}

/* Define memory detect region N
 * region      : 0, 1, 2, 3, 4 or 5
 * start       : addr of first byte subject to detection
 * end         : addr of last byte subject to detection
 * permissions : user or supervisor read/write/execute permissions, constructed
 *               by OR'ing together USER_xxx and SUPERVISOR_xxx values
 * return      : 0 if memory range and permissions set up successfully
 *               1 if set up fails due to invalid region number
 */
#pragma inline gmu_define_detect_region
static int
gmu_define_detect_region(unsigned int region,
                         void* start,
                         void* end,
                         unsigned int permissions)
{
unsigned int old_GCR_status;

  if (region > 5 ||
      (permissions & ~(GMU_USER_READ | GMU_USER_WRITE |
                       GMU_USER_EXECUTE | GMU_SUPERVISOR_READ |
                       GMU_SUPERVISOR_WRITE | GMU_SUPERVISOR_EXECUTE)) != 0)
    return 1;

  /* Save old GCR and disable region being modified */
  old_GCR_status = *__gmu_control_reg_ptr;
  gmu_disable_detect_region(region);

  /* Set detect registers */
  __gmu_detect_regs[region].__gmu_detect_upper_bound_addr_reg = 
                  (((unsigned int)end + 256) & 0xffffff00) | permissions;
  __gmu_detect_regs[region].__gmu_detect_lower_bound_addr_reg = 
                  (unsigned int)start & 0xffffff00;

  /* Restore GCR */
  *__gmu_control_reg_ptr = old_GCR_status; 

  return 0;
}

#endif
#endif
