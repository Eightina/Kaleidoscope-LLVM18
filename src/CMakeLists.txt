add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ast)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lexer)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/parser)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/utils)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ast)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lexer)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/parser)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utils)

add_executable(aot_compiler ./utils/utils.cpp ./lexer/lexer.cpp main_aot.cpp)
add_executable(jit_compiler ./utils/utils.cpp ./lexer/lexer.cpp main_jit.cpp)
target_include_directories(aot_compiler PRIVATE ${LLVM_INCLUDE_DIRS})
target_include_directories(jit_compiler PRIVATE ${LLVM_INCLUDE_DIRS})

# message("LLVM_LIBRARIES @ ${LLVM_LIBRARIES}")
# target_link_libraries(parser_test PRIVATE 
#     # ${LLVM_LIBRARIES}    
#     LLVMCore
#     # LLVMSupport
# )
# run llvm-config for extra compiling and linking flags
execute_process(
    COMMAND llvm-config-18 --cxxflags --ldflags --system-libs --libs core
    OUTPUT_VARIABLE LLVM_CONFIG_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
# split compiling and linking flags
set(CXX_FLAGS "")
set(LINK_FLAGS "")
# # add llvm-config outputs
# message(LLVM_CONFIG_FLAGS: ${LLVM_CONFIG_FLAGS} )
separate_arguments(LLVM_CONFIG_FLAGS UNIX_COMMAND "${LLVM_CONFIG_FLAGS}")
foreach(flag IN LISTS LLVM_CONFIG_FLAGS)
    if(flag MATCHES "^-I" OR flag MATCHES "^-D")
        list(APPEND CXX_FLAGS "${flag}")
    elseif(flag MATCHES "^-std=")
        # C++ standard set before, so ignore here
    elseif(flag MATCHES "^-fno-exceptions")
        list(APPEND CXX_FLAGS "${flag}")
    elseif(flag MATCHES "^-L" OR flag MATCHES "^-l")
        string(STRIP "${flag}" stripped_flag) # remove spaces
        list(APPEND LINK_FLAGS "${stripped_flag}")
    endif()
endforeach()

set(LINK_FLAGS "${LINK_FLAGS} -rdynamic -Xlinker --export-dynamic")

# apply compiling and linking options
message(CXX_FLAGS: ${CXX_FLAGS} )
message(LINK_FLAGS: ${LINK_FLAGS} )

target_compile_options(aot_compiler PRIVATE ${CXX_FLAGS})
target_link_libraries(aot_compiler PRIVATE ${LINK_FLAGS})

target_compile_options(jit_compiler PRIVATE ${CXX_FLAGS})
target_link_libraries(jit_compiler PRIVATE ${LINK_FLAGS})
# set(LLVM_TARGETS_TO_BUILD "X86" CACHE STRING "List of targets to build for LLVM")
# include_directories ("${PROJECT_SOURCE_DIR}/include")

# add_executable(lexer_test lexer.cpp)
# target_include_directories(lexer_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
# include_directories ("${PROJECT_SOURCE_DIR}/include")
# include_directories(${LLVM_INCLUDE_DIRS})