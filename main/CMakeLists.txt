idf_component_register(SRCS "main.cpp" "WiFi Functions.cpp" "WebServer.cpp"
                    EMBED_TXTFILES "certs/cacert.pem"
                                   "certs/prvtkey.pem"
                    INCLUDE_DIRS ".")
component_compile_options(-std=c++14)