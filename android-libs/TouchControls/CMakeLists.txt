cmake_minimum_required( VERSION 3.5.0 )

add_library(touchcontrols SHARED
    Button.cpp
    ControlSuper.cpp
    GLLines.cpp
    GLRect.cpp
    JNITouchControlsUtils.cpp
    Mouse.cpp
    MultitouchMouse.cpp
    OpenGLUtils.cpp
    RectF.cpp
    TouchControls.cpp
    TouchControlsContainer.cpp
    TouchJoy.cpp
    WheelSelect.cpp
)

# Add -DUSE_GLES2 for OpenGLES 2
target_compile_definitions(touchcontrols PRIVATE -DANDROID_NDK)

target_include_directories(touchcontrols PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${LIBPNG_INCLUDE_DIR}")
message("->${CMAKE_CURRENT_SOURCE_DIR} ->${LIBPNG_INCLUDE_DIR}")

target_link_libraries(touchcontrols PUBLIC GLESv1_CM log sigc++ TinyXML ${LIBPNG_LIBRARY})
 
