#   Copyright (c) 2024, Thierry Tremblay
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice, this
#     list of conditions and the following disclaimer.
#
#   * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set(CMAKE_COVERAGE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/coverage)

if (CMAKE_C_COMPILER_ID MATCHES "GNU")

    # GCC
    find_program(GENHTML genhtml)
    find_program(LCOV lcov)

    if(NOT LCOV)
        message(FATAL_ERROR "lcov not found")
    endif()

    if(NOT GENHTML)
        message(FATAL_ERROR "genhtml not found")
    endif()

    # Add code converage functionality
    function(add_coverage)
        # Arguments parsing
        set(single_value_keywords DEPENDS SRCDIR BINDIR)
        set(multi_value_keywords EXCLUDE)
        cmake_parse_arguments(
            ARG "${options}" "${single_value_keywords}" "${multi_value_keywords}" ${ARGN}
        )

        # Setup "coverage" target
        add_custom_target(coverage COMMAND DEPENDS ${ARG_DEPENDS})

        set(EXCLUDES)
        foreach(ITEM ${ARG_EXCLUDE})
            set(EXCLUDES ${EXCLUDES} --exclude '${ITEM}')
        endforeach()

        add_custom_command(
            TARGET coverage
            POST_BUILD
            COMMENT "Generating coverage information"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}
            COMMAND ${LCOV}
                --capture
                --no-external
                ${EXCLUDES}
                -b ${ARG_SRCDIR}
                -d ${ARG_BINDIR}
                -o ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}/coverage.info
            COMMAND ${GENHTML} ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}/coverage.info
                -o ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}
        )
    endfunction()

    # Configure a target for code coverage
    function(target_add_converage TARGET)
        target_compile_options(${TARGET} PUBLIC --coverage)
        target_link_options(${TARGET} PUBLIC --coverage)
    endfunction()

elseif (CMAKE_C_COMPILER_ID MATCHES "(Apple)?[Cc]lang")

    # Clang
    find_program(LLVM_COV llvm-cov)
    find_program(LLVM_PROFDATA llvm-profdata)

    if(NOT LLVM_COV)
        message(FATAL_ERROR "llvm-cov not found")
    endif()

    if(NOT LLVM_PROFDATA)
        message(FATAL_ERROR "llvm-profdata not found")
    endif()

    # Add code converage functionality
    function(add_coverage)
        # Arguments parsing
        set(single_value_keywords DEPENDS SRCDIR BINDIR OUTDIR)
        set(multi_value_keywords EXCLUDE)
        cmake_parse_arguments(
            ARG "${options}" "${single_value_keywords}" "${multi_value_keywords}" ${ARGN}
        )

        # Setup "coverage" target
        add_custom_target(coverage COMMAND DEPENDS ${ARG_DEPENDS})

        set(EXCLUDES)
        foreach(ITEM ${ARG_EXCLUDE})
            set(EXCLUDES ${EXCLUDES} --ignore-filename-regex='${ITEM}')
        endforeach()

        add_custom_command(
            TARGET coverage
            POST_BUILD
            COMMENT "Generating coverage information"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}
            COMMAND ${LLVM_PROFDATA} merge
                --sparse `cat ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}/profraw.list`
                -o ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}/coverage.profdata
            COMMAND ${LLVM_COV} show
                `cat ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}/binaries.list`
                -instr-profile=${CMAKE_COVERAGE_OUTPUT_DIRECTORY}/coverage.profdata
                -show-line-counts-or-regions
                -output-dir=${CMAKE_COVERAGE_OUTPUT_DIRECTORY}
                -format="html"
                ${EXCLUDES}
        )
    endfunction()

    add_custom_target(
        coverage-clean
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}/binaries.list
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}/profraw.list
    )

    # Configure a target for code coverage
    function(target_add_converage TARGET)
        target_compile_options(${TARGET} PUBLIC -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(${TARGET} PUBLIC -fprofile-instr-generate -fcoverage-mapping)
        add_custom_target(
            coverage-${TARGET}
            DEPENDS coverage-clean
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}
            COMMAND ${CMAKE_COMMAND} -E echo "-object=$<TARGET_FILE:${TARGET}>" >> ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}/binaries.list
            COMMAND ${CMAKE_COMMAND} -E echo "${CMAKE_CURRENT_BINARY_DIR}/default.profraw" >> ${CMAKE_COVERAGE_OUTPUT_DIRECTORY}/profraw.list
        )

        add_dependencies(coverage coverage-${TARGET})
    endfunction()

else()
    message(FATAL_ERROR "Unsupported compiler")
endif()
