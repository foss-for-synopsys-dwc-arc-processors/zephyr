# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL gcc)
  # GNU compiler options
  zephyr_compile_options(-mcpu=${GCC_M_CPU})
  if(NOT CONFIG_64BIT)
    # ISA_ARCV2 & 32BIT
    zephyr_compile_options(-mno-sdata)
    zephyr_compile_options_ifdef(CONFIG_CPU_ARCEM -mmpy-option=wlh1)
    zephyr_compile_options_ifdef(CONFIG_CPU_ARCHS -mmpy-option=plus_qmacw)
    if(CONFIG_CPU_ARCHS)
      zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpud_all)
    else()
      zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpuda_all)
    endif()
  endif()
else()
  # MWDT compiler options
  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_EM -arcv2em -core3 -Xdiv_rem=radix2
			       -Xmpy_option=mpyd -Xbitscan -Xswap -Xbarrel_shifter
			       -Xshift_assist -Xdsp2 -Xdsp_complex
			       -Xdsp_divsqrt=radix2 -Xdsp_itu -Xdsp_accshift=full
			       -Xfpus_div -Xfpu_mac -Xfpuda -Xfpus_mpy_slow
			       -Xfpus_div_slow -Xbitstream -Xtimer0 -Xtimer1)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_SEM -arcv2em -core3 -Xcode_density
			       -Xdiv_rem=radix2 -Xswap -Xbitscan -Xmpy_option=mpyd
			       -Xshift_assist -Xbarrel_shifter -Xdsp2
			       -Xdsp_complex -Xdsp_divsqrt=radix2
			       -Xdsp_accshift=limited -Xtimer0 -Xtimer1
			       -Xsec_timer0 -Xstack_check -Xsec_modes -Xdmac)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_HS
             -arcv2hs
             -core4
             -uarch_rev=1:4
             -Xcode_density
             -HL
             -Xatomic
             -Xll64
             -Xunaligned
             -Xdiv_rem=radix4
             -Xswap
             -Xbitscan
             -Xmpy_option=qmpyh
             -Xshift_assist
             -Xbarrel_shifter
             -Xtimer0
             -Xtimer1
             -Xrtc
             # -Xstack_check
             -dcache=32768,64,2,a
             -Hld_cycles=1
             -DDCCM_SYSTEM_BASE_CORE0=0x80000000
             -Hccm
             -DICCM0_SYSTEM_BASE_CORE0=0x0000000
             -Xstu=4
             -Xvdsp4
             -Xvec_unit_rev_minor=1
             -Xvec_width=512
             -Xvec_mem_size=256k
             -Xvec_max_fetch_size=16
             -Xvec_num_slots=3
             -Xvec_super_with_scalar
             -Xvec_regs=40
             -Xvec_num_rd_ports=6
             -Xvec_num_acc=8
             -Xvec_num_mpy=2
             -Xvec_mpy32
             -Xvec_num_alu=3
             -Xvec_guard_bit_option=2
             -Xvec_stack_check
             -DVEC_MEM_SYS_BASE_CORE0=0xb4000000)

  zephyr_ld_option_ifdef(CONFIG_SOC_NSIM_HS -Hlib=vpx5_integer_full)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_HS_SMP -arcv2hs -core2 -Xatomic
			       -Xll64 -Xdiv_rem=radix4 -Xunaligned -Xcode_density
			       -Xswap -Xbitscan -Xmpy_option=qmpyh -Xshift_assist
			       -Xbarrel_shifter -Xfpud_div -Xfpu_mac -Xrtc
			       -Xtimer0 -Xtimer1)

  zephyr_ld_option_ifdef(CONFIG_SOC_NSIM_HS_SMP -Hlib=hs38_full)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_HS_MPUV6 -arcv2hs -core2 -Xatomic
			       -Xunaligned -Xcode_density -Xswap -Xbitscan
			       -Xmpy_option=qmpyh -Xshift_assist -Xbarrel_shifter
			       -Xtimer0 -Xtimer1)

  zephyr_ld_option_ifdef(CONFIG_SOC_NSIM_HS_MPUV6 -Hlib=hs38_full)
endif()

zephyr_sources(
  soc.c
  soc_config.c
  )
