aux_source_directory(. CLIENT_SRC)
aux_source_directory(../src CLIENT_SRC)
add_library(CLIENT_LIB ${CLIENT_SRC})