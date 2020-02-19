#version 330
precision mediump float;

in vec2 v_uv;
in vec3 v_normal;

uniform sampler2D tex;

void main() {
  //gl_FragColor = vec4(gl_FragCoord.x, v_uv.y, v_normal.z, 1.0);
  gl_FragColor = texture(tex, v_uv);
}
