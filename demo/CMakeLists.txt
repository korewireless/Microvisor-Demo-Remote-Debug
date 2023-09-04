cmake_minimum_required(VERSION 3.14)

# Set application data
set(APP "Microvisor Remote Debug Demo")
set(VERSION_NUMBER "3.1.0")
set(BUILD_NUMBER "1")

message("Building ${APP} ${VERSION_NUMBER} build ${BUILD_NUMBER}")

# FROM 3.0.2
# Pass in version data
configure_file(app_version.in app_version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Compile app source code file(s)
add_executable(${PROJECT_NAME}
    http.c
    logging.c
    main.c
    network.c
    uart_logging.c
    stm32u5xx_hal_timebase_tim_template.c
)

# Link built libraries
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC
    ST_Code
    twilio-microvisor-hal-stm32u5)

# Optional informational and additional format generation
# NOTE From 2.0.3, this generates an alternative .bin file
#      than was previously the case
add_custom_command(OUTPUT EXTRA_FILES DEPENDS ${PROJECT_NAME}
    COMMAND mv "${PROJECT_NAME}" "${PROJECT_NAME}.elf"
    COMMAND ${CMAKE_SIZE} --format=berkeley "${PROJECT_NAME}.elf"
    COMMAND ${CMAKE_OBJDUMP} -h -S "${PROJECT_NAME}.elf" > "${PROJECT_NAME}.list"
    COMMAND ${CMAKE_OBJCOPY} --output-target ihex "${PROJECT_NAME}.elf" "${PROJECT_NAME}.hex"
    COMMAND ${CMAKE_OBJCOPY} --input-target ihex --output-target binary --gap-fill 0xFF "${PROJECT_NAME}.hex" "${PROJECT_NAME}.bin"
)

# Prepare the additional files
add_custom_target(extras ALL DEPENDS EXTRA_FILES)
