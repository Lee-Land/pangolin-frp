aux_source_directory(. SERVER_SRC)
aux_source_directory(../src SERVER_SRC)
add_library(SERVER_LIB ${SERVER_SRC})