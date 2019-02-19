zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_cc_option(-mcpu=${GCC_M_CPU})
zephyr_cc_option(-mno-sdata -mdiv-rem -mswap -mnorm)
zephyr_cc_option_ifdef(CONFIG_CODE_DENSITY -mcode-density)
