# CMake entry point
cmake_minimum_required(VERSION 2.6)
project(Tutorials)

find_package(OpenGL REQUIRED)


if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Please select another Build Directory ! (and give it a clever name, like bin_Visual2012_64bits/)")
endif ()
if (CMAKE_SOURCE_DIR MATCHES " ")
    message("Your Source Directory contains spaces. If you experience problems when compiling, this can be the cause.")
endif ()
if (CMAKE_BINARY_DIR MATCHES " ")
    message("Your Build Directory contains spaces. If you experience problems when compiling, this can be the cause.")
endif ()


# Compile external dependencies
add_subdirectory(external)

# On Visual 2005 and above, this module can set the debug working directory
cmake_policy(SET CMP0026 OLD)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/rpavlik-cmake-modules-fe2273")
include(CreateLaunchers)
include(MSVCMultipleProcessCompile) # /MP

if (INCLUDE_DISTRIB)
    add_subdirectory(distrib)
endif (INCLUDE_DISTRIB)


include_directories(
        external/AntTweakBar-1.16/include/
        external/glfw-3.1.2/include/GLFW/
        external/glm-0.9.7.1/
        external/glew-1.13.0/include/
        external/assimp-3.0.1270/include/
        external/bullet-2.81-rev2613/src/
        .
)

set(ALL_LIBS
        ${OPENGL_LIBRARY}
        glfw
        GLEW_1130
        )

add_definitions(
        -DTW_STATIC
        -DTW_NO_LIB_PRAGMA
        -DTW_NO_DIRECT3D
        -DGLEW_STATIC
        -D_CRT_SECURE_NO_WARNINGS
)

# Tutorial 1
add_executable(homework0_test
        homework0_test/homework0.cpp
        )
target_link_libraries(homework0_test
        ${ALL_LIBS}
        )
# Xcode and Visual working directories
set_target_properties(homework0_test PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/homework0_test/")
create_target_launcher(homework0_test WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/homework0_test/")

# Tutorial 2
add_executable(homework1_two_triangles
        homework1_two_triangles/homework1.cpp
        common/shader.cpp
        common/shader.hpp

        homework1_two_triangles/OuterFragmentShader.fragmentshader
        homework1_two_triangles/InnerFragmentShader.fragmentshader
        homework1_two_triangles/SimpleVertexShader.vertexshader
        )
target_link_libraries(homework1_two_triangles
        ${ALL_LIBS}
        )
# Xcode and Visual working directories
set_target_properties(homework1_two_triangles PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/homework1_two_triangles/")
create_target_launcher(homework1_two_triangles WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/homework1_two_triangles/")
create_default_target_launcher(homework1_two_triangles WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/homework1_two_triangles/") # tut 1 is not the default or people would complain that tut 2 doesn't work

# Tutorial 3
add_executable(homework2_camera
        homework2_camera/homework2.cpp
        common/shader.cpp
        common/shader.hpp

        homework2_camera/OuterFragmentShader.fragmentshader
        homework2_camera/InnerFragmentShader.fragmentshader
        )
#set_target_properties(homework2_camera PROPERTIES RUNTIME_OUTPUT_DIRECTORY /test1)
target_link_libraries(homework2_camera
        ${ALL_LIBS}
        )
# Xcode and Visual working directories
set_target_properties(homework2_camera PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/homework2_camera/")
create_target_launcher(homework2_camera WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/homework2_camera/") # Visual

# Tutorial 4
add_executable(homework3_tetrahedron
        homework3_tetrahedron/homework3.cpp
        common/shader.cpp
        common/shader.hpp

        homework3_tetrahedron/TransformVertexShader.vertexshader
        homework3_tetrahedron/ColorFragmentShader.fragmentshader
        )
target_link_libraries(homework3_tetrahedron
        ${ALL_LIBS}
        )
# Xcode and Visual working directories
set_target_properties(homework3_tetrahedron PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/homework3_tetrahedron/")
create_target_launcher(homework3_tetrahedron WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/homework3_tetrahedron/")


SOURCE_GROUP(common REGULAR_EXPRESSION ".*/common/.*")
SOURCE_GROUP(shaders REGULAR_EXPRESSION ".*/.*shader$")


if (NOT ${CMAKE_GENERATOR} MATCHES "Xcode")
    add_custom_command(
            TARGET homework1_two_triangles POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/homework1_two_triangles${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_SOURCE_DIR}/homework1_two_triangles/"
    )
elseif (${CMAKE_GENERATOR} MATCHES "Xcode")

endif (NOT ${CMAKE_GENERATOR} MATCHES "Xcode")

