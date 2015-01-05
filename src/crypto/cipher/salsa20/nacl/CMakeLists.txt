fastd_cipher_impl(salsa20 nacl
  salsa20_nacl.c
)
fastd_cipher_impl_include_directories(salsa20 nacl ${NACL_INCLUDE_DIRS})
if(ANDROID_ABI)
    # static linking hack
    fastd_cipher_impl_link_libraries(salsa20 nacl ${NACL_LDFLAGS_OTHER})
else(ANDROID_ABI)
    fastd_cipher_impl_link_libraries(salsa20 nacl ${NACL_LIBRARIES})
endif(ANDROID_ABI)
fastd_cipher_impl_require(salsa20 nacl NACL)
