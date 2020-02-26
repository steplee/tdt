
# I had to add the uniform czm_sunDirectionEC, add the '#version 130' specs.
fsrc = '''
#version 130
precision mediump float;
uniform vec4 u_ambient;
uniform vec4 u_diffuse;
uniform vec4 u_emission;
uniform vec4 u_specular;
uniform float u_shininess;
uniform float u_transparency;
uniform vec3 czm_sunDirectionEC;
varying vec3 v_positionEC;
varying vec3 v_normal;
void main(void) {
  vec3 normal = normalize(v_normal);
  vec4 diffuse = u_diffuse;
  vec3 diffuseLight = vec3(0.0, 0.0, 0.0);
  vec3 specular = u_specular.rgb;
  vec3 specularLight = vec3(0.0, 0.0, 0.0);
  vec3 emission = u_emission.rgb;
  vec3 ambient = u_ambient.rgb;
  vec3 viewDir = -normalize(v_positionEC);
  vec3 ambientLight = vec3(0.0, 0.0, 0.0);
  ambientLight += vec3(0.2, 0.2, 0.2);
  vec3 l = normalize(czm_sunDirectionEC);
  diffuseLight += vec3(1.0, 1.0, 1.0) * max(dot(normal,l), 0.);
  vec3 h = normalize(l + viewDir);
  float specularIntensity = max(0., pow(max(dot(normal, h), 0.), u_shininess));
  specularLight += vec3(1.0, 1.0, 1.0) * specularIntensity;
  vec3 color = vec3(0.0, 0.0, 0.0);
  color += diffuse.rgb * diffuseLight;
  color += specular * specularLight;
  color += emission;
  color += ambient * ambientLight;
  gl_FragColor = vec4(color * diffuse.a, diffuse.a * u_transparency);
}
'''
vsrc = '''
#version 130
precision mediump float;
uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat3 u_normalMatrix;
attribute vec3 a_position;
varying vec3 v_positionEC;
attribute vec3 a_normal;
varying vec3 v_normal;
void main(void) {
  vec4 pos = u_modelViewMatrix * vec4(a_position,1.0);
  v_positionEC = pos.xyz;
  gl_Position = u_projectionMatrix * pos;
  v_normal = u_normalMatrix * a_normal;
}
'''

from OpenGL.GL import *
from OpenGL.GLUT import *

glutInit()
glutCreateWindow('test')

import OpenGL.GL.shaders
vs = OpenGL.GL.shaders.compileShader(vsrc, GL_VERTEX_SHADER)
fs = OpenGL.GL.shaders.compileShader(fsrc, GL_FRAGMENT_SHADER)
print('Cesium dragon shader:', OpenGL.GL.shaders.compileProgram(vs, fs))



fsrc = '''
#version 130
precision highp float;varying vec2 v_texcoord0;varying vec3 v_normal;uniform sampler2D u_diffuse;uniform vec4 u_emission;uniform float u_shininess;void main(void){  vec4 color = vec4(0., 0., 0., 0.);  vec4 diffuse = vec4(0., 0., 0., 1.);  vec4 emission;  diffuse = texture2D(u_diffuse, v_texcoord0);  emission = u_emission;  color.xyz += diffuse.xyz;  color.xyz += emission.xyz;  color = vec4(color.rgb * diffuse.a, diffuse.a);  gl_FragColor = color;}
'''

vsrc = '''
#version 130
attribute float a_batchId;
precision highp float;attribute vec3 a_position;attribute vec2 a_texcoord0;attribute vec2 a_normal;uniform mat4 u_modelViewMatrix;uniform mat4 u_projectionMatrix;uniform mat3 u_normalMatrix;varying vec2 v_texcoord0;varying vec3 v_normal;void main(void) {  vec4 pos = u_modelViewMatrix * vec4(a_position, 1.0);  v_texcoord0 = a_texcoord0;  gl_Position = u_projectionMatrix * pos;}
'''
vs = OpenGL.GL.shaders.compileShader(vsrc, GL_VERTEX_SHADER)
fs = OpenGL.GL.shaders.compileShader(fsrc, GL_FRAGMENT_SHADER)
print('Vricon tile shader:', OpenGL.GL.shaders.compileProgram(vs, fs))
