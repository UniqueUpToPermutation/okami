file(GLOB_RECURSE shader_SRC CONFIGURE_DEPENDS "*.hlsl" "*.psh" "*.gsh" "*.fxh" "*.vsh")

add_custom_target(OkamiDiligentShaders SOURCES ${shader_SRC})

message("Embedded Shaders: ${shader_SRC}")