cmake_minimum_required(VERSION 3.14)

# Set application data
set(APP "Microvisor Remote Debug Demo")
set(VERSION_NUMBER "1.0.3")
set(BUILD_NUMBER "0")

message("Building ${APP} ${VERSION_NUMBER} build ${BUILD_NUMBER}")

# Make app data available to the compiler
add_compile_definitions(APP_NAME="${APP}")
add_compile_definitions(APP_VERSION="${VERSION_NUMBER}")
add_compile_definitions(BUILD_NUM=${BUILD_NUMBER})

# Compile app source code file(s)
add_executable(${PROJECT_NAME}
    main.c
    logging.c
    stm32u5xx_hal_timebase_tim_template.c
)

# Link built libraries
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC
    ST_Code
    twilio-microvisor-hal-stm32u5)

# Check for (and prepare to use) passed-in path to public key
if(DEFINED RD_PUBLIC_KEYPATH AND (NOT ${RD_PUBLIC_KEYPATH} EQUAL "NONE"))
    set(PUB_KEY_PATH "${RD_PUBLIC_KEYPATH}")
    set(NO_KEY_GEN 1)
else()
    set(NO_KEY_GEN 0)
endif()

# Optional informational and additional format generation, including Bundle output
add_custom_command(OUTPUT EXTRA_FILES
    DEPENDS ${PROJECT_NAME}
    COMMAND cp "${PROJECT_NAME}" "${PROJECT_NAME}.elf"
    COMMAND ${CMAKE_SIZE} --format=berkeley "${PROJECT_NAME}"
    COMMAND ${CMAKE_OBJDUMP} -h -S "${PROJECT_NAME}.elf" > "${PROJECT_NAME}.list"
    COMMAND ${CMAKE_OBJCOPY} -O binary "${PROJECT_NAME}.elf" "${PROJECT_NAME}.bin"
)

# Bundle with remote debugging key
add_custom_command(OUTPUT REMOTE_DEBUG
    DEPENDS ${PROJECT_NAME}
    COMMAND python3 "${CMAKE_SOURCE_DIR}/twilio-microvisor-tools/bundler-py/bundler.py" "${PROJECT_NAME}.elf" --debug-auth-pubkey "${PUB_KEY_PATH}" "${PROJECT_NAME}.zip"
)

# Bundle without remote debugging key
add_custom_command(OUTPUT NO_REMOTE_DEBUG
    DEPENDS ${PROJECT_NAME}
    COMMAND python3 "${CMAKE_SOURCE_DIR}/twilio-microvisor-tools/bundler-py/bundler.py" "${PROJECT_NAME}.elf" "${PROJECT_NAME}.zip"
)

# Generate remote debugging auth keys
add_custom_command(OUTPUT GEN_KEYS
    COMMAND openssl ecparam -name prime256v1 -genkey -noout -out debug_auth_priv_key.pem 2>/dev/null
    COMMAND openssl ec -in debug_auth_priv_key.pem -pubout -out debug_auth_pub_key.pem 2>/dev/null
)

# Prepare for remote debugging
find_program(OPENSSL "openssl")
if(NOT OPENSSL)
    # No OpenSSL so can't do debug builds
    message(WARNING "OpenSSL could not be found... cannot enable remote debugging")
    set(ENABLE_REMOTE_DEBUGGING 0)
endif()

# Based on build setting (see project CMakeLists.txt),
# select which addtional command sets to run
if(DEFINED ENABLE_REMOTE_DEBUGGING AND ${ENABLE_REMOTE_DEBUGGING} EQUAL 1)
    message("Bundling ${APP} for remote debugging")
    # Don't generate keys if a public key has been passed in
    # via the deploy script --public-key option
    if(DEFINED NO_KEY_GEN AND ${NO_KEY_GEN} EQUAL 1)
        add_custom_target(extras ALL DEPENDS EXTRA_FILES REMOTE_DEBUG)
    else()
        add_custom_target(extras ALL DEPENDS GEN_KEYS EXTRA_FILES REMOTE_DEBUG)
    endif()
else()
    message("Bundling ${APP}")
    add_custom_target(extras ALL DEPENDS EXTRA_FILES NO_REMOTE_DEBUG)
endif()
