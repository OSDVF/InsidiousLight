idf_component_register(SRCS "main.cpp" "WiFi Functions.cpp" "WebServer.cpp" "DnsServer.cpp" "WifiCommands.cpp" "Settings.cpp"
                    EMBED_TXTFILES "certs/cacert.pem"
                                   "certs/prvtkey.pem"
                                   "embed/upload_script.html"
                    INCLUDE_DIRS ".")