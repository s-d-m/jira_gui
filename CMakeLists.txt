cmake_minimum_required(VERSION 3.5)

project(jira_gui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets WebEngineWidgets WebEngineCore)
find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets WebEngineCore)


set(CMAKE_EXPORT_COMPILE_COMMANDS "ON" CACHE BOOL "whether to export compile commands or not" )


if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # using Clang
    add_compile_options(
            -Weverything
            -Werror
            --analyze
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-c99-extensions
            -Wno-padded
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC

    set(CXX_WARN_FLAGS_FOR_GCC_7
            -Werror
            -Wdeprecated
            -Wno-pedantic
            -Wpointer-arith
            -Wall
            -Wextra
            -Wformat=2
            -Wnonnull
            -Winit-self
            -Wmissing-include-dirs
            #    -Wswitch-default  # need to see how to avoid it if all values are handled
            -Wswitch-enum
            -Wuninitialized
            -fstrict-aliasing
            -Wstrict-overflow=5
            -Warray-bounds
            -Wfloat-equal
            -Wundef
            -Wshadow
            -Wcast-qual
            -Wcast-align
            -Wwrite-strings
            -Wconversion
            -Wsign-conversion
            -Wmissing-declarations
            -Wpacked
            -Wredundant-decls
            -Winvalid-pch
            #    -Wlong-long # int64_t is fine
            -Wvarargs
            -Wvla
            -funsafe-loop-optimizations
            -Wunsafe-loop-optimizations
            -Wdouble-promotion
            -Wsuggest-attribute=pure
            -Wsuggest-attribute=const
            -Wsuggest-attribute=noreturn
            -Wsuggest-attribute=format
            -Wtrampolines
            -Wzero-as-null-pointer-constant
            -Wuseless-cast
            -Wlogical-op
            -Wvector-operation-performance
            -Wabi=11
            -Wctor-dtor-privacy
            -Wnoexcept
            -Weffc++
            -Wstrict-null-sentinel
            -Wold-style-cast
            -Woverloaded-virtual
            -Wsign-promo
            -Wswitch-bool
            -Wlogical-not-parentheses
            -Wsizeof-array-argument
            -Wbool-compare
            -Wodr
            -Wsuggest-final-types
            -Wsuggest-final-methods
            -Wsuggest-override
            -Wnull-dereference -fdelete-null-pointer-checks
            -Wduplicated-cond
            -Wplacement-new=2
            -Wconditionally-supported
            -fsized-deallocation
            -Wsized-deallocation
            -Wduplicated-branches
            -Wrestrict
            -Wregister
            -Wdangling-else
            -Walloc-size-larger-than=1073741824
            -Walloc-zero
            -Walloca
            -Wformat-overflow=2
            -Wformat-truncation=2
            -Wstringop-overflow=4
            -Waligned-new
    )

    if (NOT ALLOW_WARN)
        set(CXX_WARN_FLAGS_FOR_GCC_7 "${CXX_WARN_FLAGS_FOR_GCC_7}"
                )
    endif()

    set(CXX_WARN_FLAGS_FOR_GCC_8
            ${CXX_WARN_FLAGS_FOR_GCC_7}
            -Wmultistatement-macros
            -Wcast-align=strict
    )

    set(CXX_WARN_FLAGS_FOR_GCC_9
            ${CXX_WARN_FLAGS_FOR_GCC_8}
    )

    set(CXX_WARN_FLAGS_FOR_GCC_10
            ${CXX_WARN_FLAGS_FOR_GCC_9}
            -Wmismatched-tags
            -Wredundant-tags
    )

    set(CXX_WARN_FLAGS_FOR_GCC_11
            ${CXX_WARN_FLAGS_FOR_GCC_10}
            -Wctad-maybe-unsupported
            -Wdeprecated-enum-enum-conversion
            -Wdeprecated-enum-float-conversion
    )

    set(CXX_WARN_FLAGS_FOR_GCC_12
            ${CXX_WARN_FLAGS_FOR_GCC_11}
            -Wbidi-chars
            -Warray-compare
            -Wmissing-requires
            -Winterference-size
    )

    set(CXX_WARN_FLAGS_FOR_GCC_13
            ${CXX_WARN_FLAGS_FOR_GCC_12}
            -Wxor-used-as-pow
            -Wself-move
            -Wdangling-reference
    )

    set(ANALYSER_FLAGS_FOR_GCC_10
            -fanalyzer
            -Wanalyzer-too-complex
            -fanalyzer-checker=taint
            -fanalyzer-transitivity
    )

    set(ANALYSER_FLAGS_FOR_GCC_11
            ${ANALYSER_FLAGS_FOR_GCC_10}
            -Wanalyzer-mismatching-deallocation
    )

    set(ANALYSER_FLAGS_FOR_GCC_12
            ${ANALYSER_FLAGS_FOR_GCC_11}
    )

    set(ANALYSER_FLAGS_FOR_GCC_13
            ${ANALYSER_FLAGS_FOR_GCC_12}
            -Wanalyzer-allocation-size
            -Wanalyzer-deref-before-check
            -Wanalyzer-exposure-through-uninit-copy
            -Wanalyzer-fd-access-mode-mismatch
            -Wanalyzer-fd-double-close
            -Wanalyzer-fd-leak
            -Wanalyzer-fd-phase-mismatch
            -Wanalyzer-fd-type-mismatch
            -Wanalyzer-fd-use-after-close
            -Wanalyzer-fd-use-without-check
            -Wanalyzer-imprecise-fp-arithmetic
            -Wanalyzer-infinite-recursion
            -Wanalyzer-jump-through-null
            -Wanalyzer-out-of-bounds
            -Wanalyzer-putenv-of-auto-var
            -Wanalyzer-tainted-assertion
            -Wanalyzer-va-list-leak
            -Wanalyzer-va-list-use-after-va-end
            -Wanalyzer-va-arg-type-mismatch
            -Wanalyzer-va-list-exhausted
    )

    if (CMAKE_CXX_COMPILER_VERSION MATCHES "^7\.")
        message("\nCmake compiler version ${CMAKE_CXX_COMPILER_VERSION}")
        add_compile_options(${CXX_WARN_FLAGS_FOR_GCC_7})
    elseif(CMAKE_CXX_COMPILER_VERSION MATCHES "^8\.")
        add_compile_options(${CXX_WARN_FLAGS_FOR_GCC_8})
    elseif(CMAKE_CXX_COMPILER_VERSION MATCHES "^9\.")
        add_compile_options(${CXX_WARN_FLAGS_FOR_GCC_9})
    elseif(CMAKE_CXX_COMPILER_VERSION MATCHES "^10\.")
        add_compile_options(${CXX_WARN_FLAGS_FOR_GCC_10})
        #    add_compile_options(${ANALYSER_FLAGS_FOR_GCC_10})
    elseif(CMAKE_CXX_COMPILER_VERSION MATCHES "^11\.")
        add_compile_options(${CXX_WARN_FLAGS_FOR_GCC_11})
        #    add_compile_options(${ANALYSER_FLAGS_FOR_GCC_11})
    elseif(CMAKE_CXX_COMPILER_VERSION MATCHES "^12\.")
        add_compile_options(${CXX_WARN_FLAGS_FOR_GCC_12})
        #    add_compile_options(${ANALYSER_FLAGS_FOR_GCC_12})
    elseif(CMAKE_CXX_COMPILER_VERSION MATCHES "^13\.")
        add_compile_options(${CXX_WARN_FLAGS_FOR_GCC_13})
        #    add_compile_options(${ANALYSER_FLAGS_FOR_GCC_13})
    else()
        add_compile_options(${CXX_WARN_FLAGS_FOR_GCC_13})
        #    add_compile_options(${ANALYSER_FLAGS_FOR_GCC_13})
    endif()

else()
    message("unsupported compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

add_compile_options(
    -fno-common
    -fbounds-check
    -fno-delete-null-pointer-checks
    -fstrict-aliasing
    -fstrict-overflow
    -frecord-gcc-switches
    -fvisibility=hidden
    # -fsanitize=undefined
    # -fsanitize=address
    -fstack-protector-all
    -fno-omit-frame-pointer
    # -static-libasan
    # -static-libubsan
)

# add_link_options(
#     -pie
#     -z noexecstack
#     -z now
#     -z relro
#     -z nodlopen
#     -z nodump
# )

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

add_executable(jira_gui ${PROJECT_SOURCES})

target_link_libraries(jira_gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(jira_gui PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineWidgets)
target_link_libraries(jira_gui PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineCore)

