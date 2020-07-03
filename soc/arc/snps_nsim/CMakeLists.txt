# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL gcc)
zephyr_compile_options(-mcpu=${GCC_M_CPU} -mno-sdata)
zephyr_compile_options_ifdef(CONFIG_CPU_ARCEM -mmpy-option=wlh1)
zephyr_compile_options_ifdef(CONFIG_CPU_ARCHS -mmpy-option=plus_qmacw)
if(CONFIG_CPU_ARCHS)
zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpud_all)
else()
zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpuda_all)
endif()
else()
# metaware compiler options
if(CONFIG_SOC_NSIM_EM)
zephyr_compile_options(-arcv2em -core3 -Xdiv_rem=radix2 -Xmpy_option=mpyd
		       -Xbitscan -Xswap -Xbarrel_shifter -Xshift_assist -Xdsp2
		       -Xdsp_complex -Xdsp_divsqrt=radix2 -Xdsp_itu -Xdsp_accshift=full
		       -Xfpus_div -Xfpu_mac -Xfpuda -Xfpus_mpy_slow -Xfpus_div_slow -Xbitstream
		       -Xtimer0 -Xtimer1)
elseif(CONFIG_SOC_NSIM_SEM)
zephyr_compile_options(-arcv2em -core3 -Xcode_density -Xdiv_rem=radix2 -Xswap
		       -Xbitscan -Xmpy_option=mpyd -Xshift_assist -Xbarrel_shifter
		       -Xdsp2 -Xdsp_complex -Xdsp_divsqrt=radix2 -Xdsp_accshift=limited
		       -Xtimer0 -Xtimer1 -Xsec_timer0 -Xstack_check -Xsec_modes -Xdmac)
elseif(CONFIG_SOC_NSIM_HS)
zephyr_compile_options(-arcv2hs -core3 -Xatomic -Xunaligned -Xcode_density -Xswap
		       -Xbitscan -Xmpy_option=mpy -Xshift_assist -Xbarrel_shifter
		       -Xtimer0 -Xtimer1)
elseif(CONFIG_SOC_NSIM_HS_SMP)
zephyr_compile_options(-arcv2hs -core2 -Xatomic -Xll64 -Xdiv_rem=radix4 -Xunaligned
		       -Xcode_density -Xswap -Xbitscan -Xmpy_option=qmpyh -Xshift_assist
		       -Xbarrel_shifter -Xfpud_div -Xfpu_mac -Xrtc -Xtimer0 -Xtimer1)
endif()
endif()
zephyr_sources(
  soc.c
  soc_config.c
  )
