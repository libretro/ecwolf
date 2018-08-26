cmake_minimum_required( VERSION 2.8.12 )

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

target_include_directories(touchcontrols PRIVATE ${SIGCPP_INCLUDE_DIR} ${TINYXML_INCLUDE_DIR} ${LIBPNG_INCLUDE_DIR})

target_link_libraries(touchcontrols PRIVATE GLESv1_CM log ${TINYXML_LIBRARY} ${SIGCPP_LIBRARY} ${LIBPNG_LIBRARY})
 
