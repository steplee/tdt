#version 330
precision mediump float;


void main() {
  gl_FragColor = vec4(gl_FragCoord.xyx, 1.0);
}
