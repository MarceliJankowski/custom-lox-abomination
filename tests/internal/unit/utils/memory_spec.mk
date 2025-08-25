${unit_test_mk_target_prefix}/utils/memory_spec: link_flags += -Wl,--wrap=realloc,--wrap=free
${unit_test_mk_target_prefix}/utils/memory_spec: ${unit_test_mk_prerequisite_prefix}/utils/memory.o
