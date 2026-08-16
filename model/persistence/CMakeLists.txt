# Modulo di persistenza JSON (Qt Core): dipende dalla libreria `events`
# ma NON da Qt Widgets: il modello logico resta riutilizzabile senza GUI.
file(GLOB_RECURSE PERSISTENCE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/persistence/*.cpp")
add_library(events_persistence STATIC ${PERSISTENCE_SOURCES})
target_include_directories(events_persistence PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(events_persistence PUBLIC events Qt6::Core)
