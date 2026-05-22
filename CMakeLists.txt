cmake_minimum_required(VERSION 3.18)
project(php_glaze VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Fetch Glaze
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG        v7.7.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glaze)

# ---------------------------------------------------------------------------
# Locate PHP build configuration
# ---------------------------------------------------------------------------
find_program(PHP_CONFIG php-config REQUIRED)

execute_process(COMMAND ${PHP_CONFIG} --include-dir
    OUTPUT_VARIABLE PHP_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PHP_CONFIG} --extension-dir
    OUTPUT_VARIABLE PHP_EXTENSION_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PHP_CONFIG} --version
    OUTPUT_VARIABLE PHP_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)

message(STATUS "PHP ${PHP_VERSION} — include: ${PHP_INCLUDE_DIR}, ext: ${PHP_EXTENSION_DIR}")

# ---------------------------------------------------------------------------
# Extension target
# ---------------------------------------------------------------------------
add_library(php_glaze MODULE src/php_glaze.cpp)

set_target_properties(php_glaze PROPERTIES
    OUTPUT_NAME "glaze"
    PREFIX      ""
    SUFFIX      ".so"
    CXX_VISIBILITY_PRESET default
)

target_include_directories(php_glaze PRIVATE
    ${PHP_INCLUDE_DIR}
    ${PHP_INCLUDE_DIR}/main
    ${PHP_INCLUDE_DIR}/Zend
    ${PHP_INCLUDE_DIR}/TSRM
    ${PHP_INCLUDE_DIR}/ext
)

target_link_libraries(php_glaze PRIVATE glaze::glaze)

target_compile_definitions(php_glaze PRIVATE
    COMPILE_DL_GLAZE=1
    ZEND_ENABLE_STATIC_TSRMLS_CACHE=1
)

target_compile_options(php_glaze PRIVATE
    -Wall -Wextra
    $<$<CXX_COMPILER_ID:GNU>:-Wno-literal-suffix>
    -Wno-deprecated-declarations
    -Wno-unused-parameter   # PHP macros introduce unused params
)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS php_glaze LIBRARY DESTINATION ${PHP_EXTENSION_DIR})
