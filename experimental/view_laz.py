import numpy as np
import pylas

with pylas.open('031615541.laz') as fh:
    print('Points from Header:', fh.header.point_count)
    las = fh.read()
    print(las)
    print('Points from data:', len(las.points))
    print(las.x)
    print(las.y)
    print(las.z)
    print(las.points_data.point_format.dimension_names)

    n = 50000
    s = 8
    x,y,z = las.x[0:n:s],las.y[:n:s],las.z[:n:s]
    n = len(x)
    print(type(x))
    pts = np.stack((x,y,z), -1).astype(np.float32)
    pts = pts - pts.mean(0)
    pts = pts / abs(pts.max())
    print(pts.max(0), pts.min(0))

from OpenGL.GL import *
from OpenGL.GLUT import *
from OpenGL.GLU import *
import time

glutInit()
glutInitWindowSize(800,800)
glutCreateWindow('a')

glEnableClientState(GL_VERTEX_ARRAY)
glVertexPointer(3, GL_FLOAT, 0, pts)

glMatrixMode(GL_PROJECTION)
glLoadIdentity()
#glOrtho(-1,1,-1,1,-1000,1000)
glFrustum(-.1,.1,-.1,.1,5,500)

for i in range(100000):
    glClear(GL_COLOR_BUFFER_BIT)

    glMatrixMode(GL_MODELVIEW)
    glLoadIdentity()
    r = 20
    gluLookAt(np.sin(time.time())*r,50,np.cos(time.time())*r, 0,0,0, 0,1,0)

    glDrawArrays(GL_POINTS, 0, n)

    glutSwapBuffers()
    glutMainLoopEvent()
    time.sleep(.01)
