set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS _GNU_SOURCE __APPLE_USE_RFC_3542)
set(FASTD_CFLAGS "${PTHREAD_CFLAGS} -std=c99 ${UECC_CFLAGS_OTHER} ${NACL_CFLAGS_OTHER} ${OPENSSL_CRYPTO_CFLAGS_OTHER} ${JSONC_CFLAGS_OTHER} ${CFLAGS_LTO} -Wall")

if(ANDROID_ABI)
    set(FASTD_CFLAGS "-fPIE ${FASTD_CFLAGS}")
endif(ANDROID_ABI)

include_directories(${FASTD_SOURCE_DIR} ${FASTD_BINARY_DIR}/src)
link_directories(${UECC_LIBRARY_DIRS} ${NACL_LIBRARY_DIRS} ${OPENSSL_CRYPTO_LIBRARY_DIRS} ${JSONC_LIBRARY_DIRS})


include(generate_version)

include(fastd_module)
add_subdirectory(protocols)
add_subdirectory(methods)
add_subdirectory(crypto)

include(check_reqs)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fastd_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/fastd_config.h)

BISON_TARGET(fastd_config_parse config.y ${CMAKE_CURRENT_BINARY_DIR}/config.yy.c)

add_executable(fastd
  async.c
  capabilities.c
  config.c
  handshake.c
  hkdf_sha256.c
  fastd.c
  lex.c
  log.c
  options.c
  peer.c
  peer_hashtable.c
  poll.c
  random.c
  receive.c
  resolve.c
  send.c
  sha256.c
  shell.c
  socket.c
  status.c
  tuntap.c
  vector.c
  verify.c
  ${BISON_fastd_config_parse_OUTPUTS}
)
set_property(TARGET fastd PROPERTY COMPILE_FLAGS "${FASTD_CFLAGS}")
if(ANDROID_ABI)
    # Android 5.0+ requires PIE binaries.
    # Also removes UECC/NACL .so directives as we're linking with static libs
    set_property(TARGET fastd PROPERTY LINK_FLAGS "-fPIE -pie ${PTHREAD_LDFLAGS} ${OPENSSL_CRYPTO_LDFLAGS_OTHER} ${JSONC_LDFLAGS_OTHER} ${LDFLAGS_LTO}")
else(ANDROID_ABI)
    set_property(TARGET fastd PROPERTY LINK_FLAGS "${PTHREAD_LDFLAGS} ${UECC_LDFLAGS_OTHER} ${NACL_LDFLAGS_OTHER} ${OPENSSL_CRYPTO_LDFLAGS_OTHER} ${JSONC_LDFLAGS_OTHER} ${LDFLAGS_LTO}")
endif(ANDROID_ABI)

set_property(TARGET fastd APPEND PROPERTY INCLUDE_DIRECTORIES ${CAP_INCLUDE_DIR} ${NACL_INCLUDE_DIRS} ${JSONC_INCLUDE_DIRS})
if(ANDROID_ABI)
    # Link with static uecc lib (using $UECC_LDFLAGS_OTHER)
    # NaCL/libsodium static lib is statically linked in crypto sub targets
    target_link_libraries(fastd protocols methods ciphers macs ${RT_LIBRARY} ${CAP_LIBRARY} ${UECC_LDFLAGS_OTHER} ${OPENSSL_CRYPTO_LIBRARIES} ${JSONC_LIBRARIES})
else(ANDROID_ABI)
    target_link_libraries(fastd protocols methods ciphers macs ${RT_LIBRARY} ${CAP_LIBRARY} ${UECC_LIBRARIES} ${NACL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES} ${JSONC_LIBRARIES})
endif(ANDROID_ABI)

add_dependencies(fastd version)

install(TARGETS fastd RUNTIME DESTINATION bin)
