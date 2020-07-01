include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)


aux_source_directory(src/ SRC)
target_sources(app PRIVATE ${SRC})



