idf_component_register(SRCS "cmd_system.cpp" "Systems.cpp"
                    INCLUDE_DIRS .
                    REQUIRES console spi_flash)