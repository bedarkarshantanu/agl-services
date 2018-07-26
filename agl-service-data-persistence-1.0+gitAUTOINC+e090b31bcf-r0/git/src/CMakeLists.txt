PROJECT_TARGET_ADD(persistence-binding)

find_package(GDBM)
if(DB_FOUND)
  add_definitions(-DUSE_GDBM)
else(DB_FOUND)
  find_package(BerkeleyDB REQUIRED)
endif(DB_FOUND)
include_directories(${DB_INCLUDE_DIR})

add_library(persistence-binding MODULE persistence-binding.c)
target_link_libraries(persistence-binding ${DB_LIBRARY})

set_target_properties(persistence-binding PROPERTIES
        PREFIX "afb-"
        LABELS "BINDING"
        LINK_FLAGS ${BINDINGS_LINK_FLAG}
        OUTPUT_NAME ${TARGET_NAME})

add_custom_command(TARGET ${TARGET_NAME}
    PRE_BUILD
    COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/../package/htdocs
    COMMAND cp -rv ${CMAKE_CURRENT_SOURCE_DIR}/../htdocs ${CMAKE_CURRENT_BINARY_DIR}/../package/)

install(TARGETS persistence-binding
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)