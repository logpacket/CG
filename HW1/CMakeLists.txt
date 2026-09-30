cmake_minimum_required(VERSION 3.16)
project(HW1 C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(THIRD_PARTY ${CMAKE_SOURCE_DIR}/third_party)

# 모든 빌드 구성(Debug/Release)에서 exe를 build 폴더에 바로 만든다.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
foreach(config ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${config} config)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} ${CMAKE_BINARY_DIR})
endforeach()

# glad: OpenGL 3.3 core 함수 로더 (생성된 소스)
add_library(glad STATIC ${THIRD_PARTY}/glad/src/glad.c)
target_include_directories(glad PUBLIC ${THIRD_PARTY}/glad/include)
target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})

add_executable(HW1 src/main.cpp)
target_include_directories(HW1 PRIVATE src ${THIRD_PARTY}/glm)
target_link_libraries(HW1 PRIVATE glad)

if(WIN32)
  # GLFW Windows용 빌드된 바이너리 (https://www.glfw.org/download)
  set(GLFW_WIN ${THIRD_PARTY}/glfw-3.4.bin.WIN64)
  if(NOT EXISTS ${GLFW_WIN}/include/GLFW/glfw3.h)
    message(FATAL_ERROR "GLFW Windows binaries not found: ${GLFW_WIN}")
  endif()
  target_include_directories(HW1 PRIVATE ${GLFW_WIN}/include)
  if(MINGW)
    target_link_libraries(HW1 PRIVATE ${GLFW_WIN}/lib-mingw-w64/libglfw3.a opengl32 gdi32)
    # 런타임 라이브러리를 정적으로 링크한다. 그러면 DLL 없이 exe 하나로 실행된다.
    target_link_options(HW1 PRIVATE -static)
  elseif(MSVC)
    target_link_libraries(HW1 PRIVATE ${GLFW_WIN}/lib-vc2022/glfw3.lib opengl32)
    target_compile_definitions(HW1 PRIVATE _CRT_SECURE_NO_WARNINGS)
    set_target_properties(HW1 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  endif()
else()
  find_package(glfw3 3.3 REQUIRED)
  find_package(OpenGL REQUIRED)
  target_link_libraries(HW1 PRIVATE glfw OpenGL::GL)
endif()

# InitShader()는 현재 작업 디렉터리에서 셰이더를 읽는다. 셰이더를 exe 옆으로 복사한다.
# 셰이더 파일이 바뀌면 exe를 다시 빌드하지 않아도 복사한다.
set(SHADER_OUTPUTS)
foreach(shader vshader.glsl fshader.glsl)
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${shader}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/shaders/${shader} ${CMAKE_BINARY_DIR}/${shader}
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/${shader})
  list(APPEND SHADER_OUTPUTS ${CMAKE_BINARY_DIR}/${shader})
endforeach()
add_custom_target(shaders DEPENDS ${SHADER_OUTPUTS})
add_dependencies(HW1 shaders)
