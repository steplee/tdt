#version 330

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

uniform mat4 mvp;

out vec2 v_uv;
out vec3 v_normal;

void main() {
  gl_Position = mvp * vec4(in_pos, 1.0);

  v_uv = in_uv;
  v_normal = in_normal;
}

