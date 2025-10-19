file(GLOB_RECURSE SOURCES 
    src/core/*.c
    src/draw/*.c
    src/font/*.c
    src/hal/*.c
    src/misc/*.c
    src/widgets/*.c
    src/extra/*.c
)

file(GLOB_RECURSE DEMO_SOURCES
    demos/widgets/*.c
    demos/benchmark/*.c
    demos/stress/*.c
)

idf_component_register(
    SRCS ${SOURCES} ${DEMO_SOURCES}
    INCLUDE_DIRS "." "src" "demos"
    REQUIRES
)
