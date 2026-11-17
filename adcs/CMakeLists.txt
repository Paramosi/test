project(CFE_ADCS_APP C)

set(APP_SRC_FILES
  fsw/src/adcs_app.c
  fsw/src/adcs_utils.c
  fsw/src/adcs_dispatch.c
  fsw/src/adcs_cmds.c
)

# Create the app module
add_cfe_app(adcs ${APP_SRC_FILES})

target_include_directories(adcs PUBLIC 
  fsw/inc
  ../../submodules/libgscsp/lib/libfp_client/include
  ../../submodules/libgscsp/lib/libftp_client/include
  ../../submodules/libgscsp/lib/libgosh_client/include
  ../../submodules/libgscsp/lib/libgscsp/include
  ../../submodules/libgscsp/lib/libgscsp/lib/libcsp/include
  ../../submodules/libgscsp/lib/libgscsp/lib/libcsp/src # #include <csp_conn.h>
  ../../submodules/libgscsp/lib/libparam_client/include
  ../../submodules/libgscsp/lib/libparam_client/include/deprecated
  ../../submodules/libgscsp/lib/librgosh_client/include
  ../../submodules/libgscsp/lib/libutil/include
  ../../submodules/libgscsp/build/lib/libgscsp/lib/libcsp/include
  ../stx/fsw/inc
)

target_compile_options(adcs PRIVATE -Wno-pedantic -Wno-error=pedantic)

if(CMAKE_SYSTEM_PROCESSOR	STREQUAL "arm")
    # OBC build
    target_link_libraries(adcs ${PROJECT_SOURCE_DIR}/../../submodules/libgscsp/build/obc/lib/libcsp-client.a)
else()
    # CPU1 build
    target_link_libraries(adcs ${PROJECT_SOURCE_DIR}/../../submodules/libgscsp/build/cpu1/lib/libcsp-client.a)
endif()