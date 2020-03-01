import os, sys, json, numpy as np
from OpenGL.GL import *
import cv2

def readFile(f, sz=-1):
    with open(f, 'rb') as fp: return fp.read(sz)

def load_image(j):
    if 'uri' in j:
        for path in possiblePaths(j['uri']):
            return cv2.imread(j['uri'])
    else:
        assert(False)

def parse_accessor(j):
    return j
def parse_material(j):
    return j
def make_texture(j, imgs):
    tex_id = glGenTextures(1)
    glBindTexture(GL_TEXTURE_2D, tex_id)
    img = imgs[j['source']]
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.shape[1],img.shape[0], 0, GL_BGR, GL_UNSIGNED_BYTE, img)
    glBindTexture(GL_TEXTURE_2D, 0)
    return {'tex_id': tex_id, 'sampler': j['sampler']}

def q2r(r,i,j,k):
    return np.array((
        1-2*(j*j+k*k),   2*(i*j-k*r),   2*(i*k+j*r),
          2*(i*j+k*r), 1-2*(i*i+k*k),   2*(j*k-i*r),
          2*(i*k-j*r),   2*(j*k+i*r), 1-2*(i*i+j*j))).reshape(3,3)

def parse_any_transform(j):
    if 'matrix' in j:
        return np.array(np.matrix(j['matrix'])).reshape(4,4)
    t = np.eye(4)
    # TODO: I think this is wrong order.
    if 'scale' in j:
        scale = np.array(np.matrix(j['scale'])).reshape(3)
        t = np.diag((*scale,1)) @ t
    if 'rotation' in j:
        r = np.array(np.matrix(j['rotation'])).reshape(4)
        t = q2r(*r) @ t
    if 'translation' in j:
        tt = np.eye(4)
        tt[:3,3] = np.array(np.matrix(j['translation'])).reshape(3)
        t = tt @ t
    return t


class RenderState:
    def __init__(self, view=None, proj=None, overrideShader=None):
        if view is None: view = np.eye(4)
        if proj is None: proj = np.eye(4)
        self.view,self.proj = view,proj
        self.overrideShader = None

    def compose(self, model_xform):
        return RenderState(self.view@model_xform, self.proj, overrideShader=self.overrideShader)

def convert_type(a):
    if a == 'VEC2': return 2
    if a == 'VEC3': return 3
    if a == 'VEC4': return 4
    # TODO many many more.

class Mesh:
    def __init__(self, j, accessors, bufferViews):
        self.name = j.get('name', '')
        self.primitives = []
        #self.primitives = j.get('primitives', [])
        for prim in j.get('primitives', []):
            attrs = []
            for attr,idx in prim['attributes']:
                acc = accessors[idx]
                bv = bufferViews[acc['bufferView']]
                vbo = vb['buffer']
                offset = vb['byteOffset'] + acc['byteOffset']
                stride = vb['byteStride']
                type = convert_type(acc['type'])
                compType = convert_type(acc['compType'])
                attrs.append( dict(attr=attr, vbo=vbo,offset=offset,stride=stride,type=type,compType=compType) )

            self.primitives.append(dict(
                attrs=attrs,
                indices=indices, material=material, mode=mode))





    def render(self, shader):
        for prim in self.primitives:
            for attrSpec in prim['attributes']:
                a_id = glGetAttribLocation(shader.prog_id, attr['attr'])
                glEnableVertexAttribArray(a_id)
                glVertexAttribPointer(a_id, attr['type'], attr['compType'], GL_FALSE, attr['stride'], attr['offset'])
                pass
            # Bind indices.
            # Bind material (actually... do it in Node).
            # Draw.

class Node:
    def __init__(self, j):
        self.name = j.get('name','')
        self.mesh = j.get('mesh', -1)
        self.transform = parse_any_transform(j)
        self.children = j.get('children', [])

    def render(self, rs):
        pass

class Model:
    def __init__(self,path):
        self.j = json.load(path)

    def parse_and_load(self):
        # Buffers (GL).
        buffers = self.j['buffers']
        self.vbos = []
        for buffer in buffers:
            if 'uri' in buffer:
                data = readFile(buffer['uri'])
                vbo = glGenBuffers(1)
                glBindBuffer(GL_ARRAY_BUFFER, vbo)
                glBufferData(GL_ARRAY_BUFFER, len(data), data, GL_STATIC_DRAW)
                self.vbos.append(vbo)
        glBindBuffer(GL_ARRAY_BUFFER, 0)
        # Views.
        self.bufferViews = self.j['bufferViews']
        # Accessors.
        self.accessors = [parse_accessor(jj) for jj in self.j['accessors']]
        # Images.
        images = load_image(img_ for img_ in self.j['images'])
        # Textures (GL).
        self.textures = [make_texture(t, images) for t in self.j['textures']]
        # Samplers.
        self.samplers = self.j['samplers']
        # Materials.
        self.accessors = [parse_material(m) for m in self.j['materials']]
        # Meshes.
        self.meshes = self.j['meshes']
        # Nodes.
        self.nodes = [Node(n) for n in self.j['nodes']]


    def renderNode(self, idx):
        rs = RenderState()
        self.nodes[idx].render()
