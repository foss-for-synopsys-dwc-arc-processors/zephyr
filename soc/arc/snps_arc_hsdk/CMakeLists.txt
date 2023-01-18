# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

if(COMPILER STREQUAL gcc)
  zephyr_cc_option(-mcpu=${GCC_M_CPU})
  zephyr_cc_option(-mno-sdata)
  zephyr_cc_option_ifdef(CONFIG_FPU -mfpu=fpud_all)
else()
  # MWDT compiler options
  zephyr_compile_options(-arcv2hs -core4 -Xdual_issue -Xcode_density -Hrgf_banked_regs=32 -HL
    -Xatomic -Xll64 -Xunaligned -Xdiv_rem=radix4 -Xswap -Xbitscan -Xmpy_option=qmpyh -Xshift_assist
    -Xbarrel_shifter -Xtimer0 -Xtimer1 -Xrtc -Hld_cycles=2 -Xlpb_size=128)
  zephyr_compile_options_ifdef(CONFIG_FPU -Xfpu_mac -Xfpud_div)

  zephyr_ld_options(-Hlib=hs48_slc_full)
endif()
