import os, sys, json, numpy as np
from OpenGL.GL import *
from OpenGL.GLUT import *
import OpenGL.GL.shaders
import cv2
import time
from ctypes import c_void_p
from copy import deepcopy


'''
The design here is very messy, high coupled, and incoherent.
I aim to do the simplest thing, not necessary the best designed.
This, alongside adding more features (like KHR_techniques_webgl) leads to some
spaghetti code.
In some parts, the mixing of responsibility is horrible programming practice ... but it works
'''

def ortho_z_forward(left, right, bottom, top, near, far):
    return np.array((
        (2/(right-left), 0, 0, 0),
        (0, 2/(top-bottom), 0, 0),
        (0, 0, 2/(far-near), 0),
        (-(right+left)/(right-left), -(top+bottom)/(top-bottom), -(far+near)/(far-near), 1)), dtype=np.float32).T
def frustum_z_forward(left, right, bottom, top, near, far):
    return np.array((
        (2*near/(right-left), 0, (right+left)/(right-left), 0),
        (0, 2*near/(top-bottom), (top+bottom)/(top-bottom), 0),
        (0, 0, (far+near)/(far-near), -2*far*near/(far-near)),
        (0,0,1.,0)), dtype=np.float32)
def look_at_z_forward(eye, center, up):
    forward = -center + eye; forward /= np.linalg.norm(forward)
    side = np.cross(forward, up); side /= np.linalg.norm(side)
    up = np.cross(side, forward)
    m,mt = np.eye(4, dtype=np.float32), np.eye(4, dtype=np.float32)
    m[:3,:3] = np.stack((side,up,-forward))
    mt[:3,3] = -eye
    return m @ mt

def readFile(f_, possiblePaths, sz=-1, binary=True):
    for path in possiblePaths:
        f = os.path.join(path, f_)
        if os.path.exists(f):
            if binary:
                with open(f, 'rb') as fp: return fp.read(sz)
            else:
                with open(f, 'r') as fp: return fp.read(sz)

def load_image(j, possiblePaths, bufferViews=None, glbBuffer=None, buffers=None):
    if 'uri' in j:
        for path in possiblePaths:
            f = os.path.join(path, j['uri'])
            if os.path.exists(f):
                return cv2.imread(f)
    else:
        assert(bufferViews is not None)
        assert(glbBuffer is not None)
        assert('bufferView' in j)
        bv = bufferViews[j['bufferView']]
        assert(bv['buffer'] == 0) # the bin chunk.
        assert(bv.get('byteStride',0) == 0) # We could support this, I guess.
        #mime = j.get('mimeType', 'image/jpeg')
        bits = np.frombuffer(glbBuffer[bv['byteOffset']:bv['byteOffset']+bv['byteLength']], dtype=np.uint8)
        # TODO We want color, but not sure if this will work if it is grayscale. Can look into JPG header tho.
        img = cv2.imdecode(bits, 1)
        #cv2.imshow('img',img); cv2.waitKey(0)
        return img

def parse_accessor(j):
    return j
def parse_material(j):
    return j
ALLOW_MIPMAPPING = True
def needs_mipmap(filter_):
    return filter_ in (GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST, \
                      GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR)
def make_texture(j, imgs, samplers):
    tex_id = glGenTextures(1)
    glEnable(GL_TEXTURE_2D)
    glBindTexture(GL_TEXTURE_2D, tex_id)
    img = imgs[j['source']]
    print(' - Making tex ({} x {})'.format(img.shape[1],img.shape[0]))
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.shape[1],img.shape[0], 0, GL_BGR, GL_UNSIGNED_BYTE, img)
    sampler = samplers[j['sampler']]
    magfilter = sampler.get('magFilter', GL_LINEAR)
    minfilter = sampler.get('minFilter', GL_LINEAR)
    wraps = sampler.get('wrapS', GL_REPEAT)
    wrapt = sampler.get('wrapT', GL_REPEAT)
    if not ALLOW_MIPMAPPING:
        if magfilter != GL_NEAREST: magfilter = GL_LINEAR
        if minfilter != GL_NEAREST: minfilter = GL_LINEAR
    if needs_mipmap(magfilter) or needs_mipmap(minfilter):
        print(' - Texture requires mipmaps.')
        glGenerateMipmap(GL_TEXTURE_2D)
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wraps )
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapt )
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter )
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter )

    glBindTexture(GL_TEXTURE_2D, 0)
    glDisable(GL_TEXTURE_2D)
    #return {'tex_id': tex_id, 'sampler': j['sampler']}
    return tex_id

def q2r(r,i,j,k):
    return np.array((
        1-2*(j*j+k*k),   2*(i*j-k*r),   2*(i*k+j*r),
          2*(i*j+k*r), 1-2*(i*i+k*k),   2*(j*k-i*r),
          2*(i*k-j*r),   2*(j*k+i*r), 1-2*(i*i+j*j))).reshape(3,3)

def parse_any_transform(j):
    if 'matrix' in j:
        return np.array(np.matrix(j['matrix'])).reshape(4,4)
    if 'transform' in j: # 3d tile.
        return np.array(np.matrix(j['transform'])).reshape(4,4)
    elif 'scale' in j or 'rotation' in j or 'translation' in j:
        t = np.eye(4)
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
        print('Transform:\n',t)
        return t.astype(np.float32)
    else:
        return None

# https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_techniques_webgl
# We call the constructor with data from Model().
# In Model::upload(), we call uploadReturnShader(). We pass a dict of already created programs,
# if a program already exists with the technique's name, we return None, otherwise a new Shader().
# At render time, we need to manage the default uniform. Do this by having material write to renderState.
def handle_auto_uniforms(src):
    if 'czm_sunDirectionEC' in src:
        end = src.find(';')+1
        src = src[:end] + 'uniform vec3 czm_sunDirectionEC = vec3(1.0,0.0,0.0);' + src[end:]
    return src
class KhrTechniquesWebgl:
    def __init__(self, assetExtensions, assetMaterials, possiblePaths, bufferViews, cpuBuffers):
        assert('KHR_techniques_webgl' in assetExtensions)
        self.techSpec = assetExtensions['KHR_techniques_webgl']
        # Parse Shaders.
        shaderSpecs = self.techSpec['shaders']
        self.cpuShaders = []
        for shaderSpec in shaderSpecs:
            if 'uri' in shaderSpec:
                src = readFile(shaderSpec['uri'], possiblePaths, binary=False)
            else:
                bv = bufferViews[shaderSpec['bufferView']]
                src = cpuBuffers[bv['buffer']][bv['byteOffset']:bv['byteOffset']+bv['byteLength']]
                src = src.decode('ascii')
            if '#version' not in src: src = '#version 120\n' + src # Seems we sometimes need this.
            src = handle_auto_uniforms(src)
            type = shaderSpec['type']
            name = shaderSpec.get('name',None)
            self.cpuShaders.append((name, src, type))
        # Parse Programs.
        self.cpuPrograms = self.techSpec['programs']
        # Parse Techniques.
        self.techniques = self.techSpec['techniques']
        # Parse Materials.
        self.materials = [m for m in assetMaterials if 'extensions' in m and 'KHR_techniques_webgl' in m['extensions']]

        self.shaders = [None]*len(self.cpuShaders)
        self.programs = {}


    def uploadReturnPrograms(self, dictOfPrograms, dictOfMaterials):
        # Only compile shaders if the program doesn't already exist.
        for prog in self.cpuPrograms:
            if 'name' not in prog:
                print("\nWARNING:\n\tPassing 'name' field in KHR_techniques_webgl's program is highly recommended")
                prog['name'] = 'program_'+str(hash(time.time()))[:10]
            if prog['name'] in dictOfPrograms:
                self.programs.append(dictOfPrograms[prog['name']])
                continue
            fs = self.shaders[prog['fragmentShader']]
            if fs is None:
                name, src, type = self.cpuShaders[prog['fragmentShader']]
                #fs = self.shaders[prog['fragmentShader']] = OpenGL.GL.shaders.compileShader(src, type)
                fs = src
            vs = self.shaders[prog['vertexShader']]
            if vs is None:
                name, src, type = self.cpuShaders[prog['vertexShader']]
                #vs = self.shaders[prog['vertexShader']] = OpenGL.GL.shaders.compileShader(src, type)
                vs = src
            #gl_program = OpenGL.GL.shaders.compileProgram(vs, fs)
            self.programs[prog['name']] = Shader(vs,fs)


        return self.programs





class Shader:
    def __init__(self, vsrc=None, fsrc=None):
        self.vs = OpenGL.GL.shaders.compileShader(vsrc, GL_VERTEX_SHADER)
        self.fs = OpenGL.GL.shaders.compileShader(fsrc, GL_FRAGMENT_SHADER)
        self.prog = OpenGL.GL.shaders.compileProgram(self.vs, self.fs)
        assert(self.prog > 0)

        # Find attributes and uniforms.
        glUseProgram(self.prog)
        nUniforms = glGetProgramiv(self.prog, GL_ACTIVE_UNIFORMS)
        nAttributes = glGetProgramiv(self.prog, GL_ACTIVE_ATTRIBUTES)
        allAttributes, allUniforms = {}, {}
        for i in range(nUniforms):
            name, size, type = glGetActiveUniform(self.prog, i)
            name = name.decode('ascii')
            allUniforms[name] = i
        for i in range(nAttributes):
            l,s,t,n = '',bytes([0]*32),bytes([0]*32),bytes([0]*32)
            r = glGetActiveAttrib(self.prog, i, 32,l,s,t,n)
            n = n.decode('ascii').strip('\t\r\n\0')
            print(i,s,t,n)
            loc = glGetAttribLocation(self.prog, n)
            allAttributes[n] = loc
        self.allAttribs, self.allUnis = allAttributes, allUniforms
        print(' - Program', self.prog)
        print(' - Program Attribs :', (self.allAttribs))
        print(' - Program Uniforms:', (self.allUnis))
        glUseProgram(0)

    def use(self, model, attribs, uniforms):
        glUseProgram(self.prog)
        # Set attribs.
        #for attrib, spec in attribs.items():
        for spec in attribs:
            attrib = spec.name
            a_id = self.allAttribs.get(attrib, -1)
            if a_id >= 0:
                glBindBuffer(GL_ARRAY_BUFFER, model.bos[spec.buffer])
                glEnableVertexAttribArray(a_id)
                glVertexAttribPointer(a_id, spec.type, spec.compType, GL_FALSE, spec.stride, spec.offset)
            else:
                print(' - failed to use attribute', attrib)

        # Set uniforms.
        for uni, val in uniforms.items():
            u_id = self.allUnis.get(uni, None)
            if u_id is None:
                print(' - failed to use uniform  ',uni)
                continue
            if type(val) == int or val.dtype == np.uint32 or (val.shape==(1,) and val.dtype==int):
                glUniform1i(u_id, val)
            elif val.shape == (4,4):
                glUniformMatrix4fv(u_id, 1,False, val.T) # note: transpose!
            elif val.shape == (3,3):
                glUniformMatrix3fv(u_id, 1,False, val.T) # note: transpose!
            elif val.shape == (3,):
                glUniform3f(u_id, *val)
            elif type(val) == float:
                glUniform1f(u_id, val)
            elif val.shape == (4,):
                glUniform4f(u_id, *val)
            elif val.shape == (1,) or val.shape == ():
                glUniform1f(u_id, val)
            else:
                print(uni,val, type(val), val.dtype, val.shape)
                assert(False)

    def unuse(self):
        for ai in self.allAttribs.values(): glDisableVertexAttribArray(ai)
        glUseProgram(0)

    def __del__(self):
        if self.prog:
            glDeleteShader(self.fs)
            glDeleteShader(self.vs)
            #glDeleteProgram(self.prog) # This doesn't seem to have a binding.
            self.prog = 0


class RenderState:
    def __init__(self, view=None, proj=None, shaders={}, overrideShader=None):
        if view is None: view = np.eye(4)
        if proj is None: proj = np.eye(4)
        self.view,self.proj = view,proj
        self.overrideShader = overrideShader
        self.shaders = shaders
        self.mvp_ = proj @ view

    def compose(self, model_xform):
        return RenderState(self.view@model_xform, self.proj,
                overrideShader=self.overrideShader, shaders=self.shaders)

    #def mvp(self): return self.proj @ self.view
    def mvp(self): return self.mvp_
    def getShader(self, s): return self.overrideShader if self.overrideShader else \
            self.shaders[s]

def convert_type(a):
    if isinstance(a, int): return a
    elif a == 'SCALAR': return 1
    elif a == 'VEC2': return 2
    elif a == 'VEC3': return 3
    elif a == 'VEC4': return 4
    elif a == 'MAT2': return 32+2
    elif a == 'MAT3': return 32+3
    elif a == 'MAT4': return 32+4
    print("Couldn't convert:",a)
    sys.exit(1)


class AttributeSpec:
    # Buffer is the index of the glTF buffer, which should be the index into model.bos to check, which has the actualy GLuint id.
    def __init__(self, name, buffer, count, offset, stride, type, compType):
        self.name, self.buffer, self.count, self.offset, self.stride, self.type, self.compType \
            = name, buffer, count, c_void_p(offset), stride, type, compType

DEFAULT_SEMANTIC_TO_ATTRIBUTE = {
    'POSITION': 'a_position',
    'NORMAL': 'a_normal',
    'TEXCOORD_0': 'a_uv',
    'TANGENT': 'a_tangent',
}

# this is a MESS
class Material:
    PBR = 0 # Not supported yet.
    BASIC = 1
    BASIC_TEXTURED = 2
    TECHNIQUE_WEBGL = 3
    def __init__(self, spec, techniqueContainer=None):
        # Ideally we would have just two kinds of materials: PBR and WebglTechnique.
        # But I have not written the shader for PBR yet.
        self.type = Material.BASIC
        self.spec = spec
        self.semantic2attribute = DEFAULT_SEMANTIC_TO_ATTRIBUTE
        if 'pbrMetallicRoughness' in spec and 'baseColorTexture' in spec:
            self.type = Material.BASIC_TEXTURED
            self.tex_in_model = spec['pbrMetallicRoughness']['baseColorTexture']['index']
        elif 'extensions' in spec and 'KHR_techniques_webgl' in spec['extensions']:
            assert(techniqueContainer)
            self.type = Material.TECHNIQUE_WEBGL
            instance_tech = spec['extensions']['KHR_techniques_webgl']
            ts = instance_tech['technique']
            self.technique = techniqueContainer.techSpec['techniques'][ts]
            self.programName = techniqueContainer.cpuPrograms[self.technique['program']]['name']

            self.semantic2attribute = dict(DEFAULT_SEMANTIC_TO_ATTRIBUTE)
            for k,v in self.technique['attributes'].items():
                self.semantic2attribute[v['semantic']] = k
            print('webgl semantics', self.semantic2attribute)

            self.uniform_defaults = {}
            self.uniform_samplers = {}
            self.uniforms = (self.technique['uniforms'])
            for k,v in instance_tech['values'].items():
                if k in self.uniforms:
                    if self.technique['uniforms'][k]['type'] == 35678:
                        self.uniform_samplers[k] = (v['index'], v.get('texCoord',0))
                    else:
                        self.uniform_defaults[k] = np.array(v)
                else:
                    print('BAD KHR_TECHNIQUE: got value for uniform ({}), but it was not in the main technique spec!!!'.format(k))


    # Returns
    def use(self, rs, attrs, uniforms, model):
        if self.type == Material.BASIC:
            shader = rs.getShader('basic')
            shader.use(model, attrs, uniforms)
        elif self.type == Material.BASIC_TEXTURED:
            glEnable(GL_TEXTURE_2D)
            shader = rs.getShader('basicTextured')
            tex_id = model.gl_textures[self.tex_in_model]
            glActiveTexture(GL_TEXTURE0)
            glBindTexture(GL_TEXTURE_2D, tex_id)
            uniforms['diffuseSampler'] = 0
            shader.use(model, attrs, uniforms)
            print('using tex', tex_id)
        elif self.type == Material.TECHNIQUE_WEBGL:
            glEnable(GL_TEXTURE_2D)
            shader = rs.getShader(self.programName)

            for name,(cpu_tex_id,texCoord) in self.uniform_samplers.items():
                    tex_unit = 0 # It appears extension only supports one texture unit?
                    #tex_unit = texCoord # I don't think this is correct.
                    glActiveTexture(GL_TEXTURE0+tex_unit)
                    tex_id = model.gl_textures[cpu_tex_id]
                    glBindTexture(GL_TEXTURE_2D, tex_id)
                    glActiveTexture(GL_TEXTURE0)
                    uniforms[name] = 0

            for name,val in self.uniform_defaults.items():
                if name not in uniforms: uniforms[name] = val

            #glBindTexture(GL_TEXTURE_2D, tex_id)
            shader.use(model, attrs, uniforms)

        self.curShader = shader

    def unuse(self):
        self.curShader.unuse()
        self.curShader = None


class Mesh:
    def __init__(self, j, accessors, bufferViews, materials):
        self.name = j.get('name', '')
        self.primitives = []
        #self.primitives = j.get('primitives', [])
        for prim in j.get('primitives', []):
            #attrs = {}
            attrs = []
            material = prim.get('material', None)
            for attr,idx in prim['attributes'].items():
                if material is None:
                    attr = DEFAULT_SEMANTIC_TO_ATTRIBUTE[attr]
                else:
                    attr = materials[material].semantic2attribute[attr]
                acc = accessors[idx]
                bv = bufferViews[acc['bufferView']]
                buffer = bv['buffer']
                offset = bv['byteOffset'] + acc['byteOffset']
                stride = bv.get('byteStride', 0)
                type = convert_type(acc['type'])
                compType = (acc['componentType'])
                count = acc['count']
                #attrs[attr] = dict(buffer=buffer,count=count,offset=offset,stride=stride,type=type,compType=compType)
                attrs.append(AttributeSpec(attr, buffer,count,offset,stride,type,compType))
                print('ATTRIBUTE',attr,buffer,count,offset,stride)

            index_accessor = prim.get('indices', None)
            mode = prim.get('mode', 0)

            self.primitives.append(dict(
                attrs=attrs,
                index_accessor=index_accessor, material=material, mode=mode))


    def render(self, model, rs):
        uniforms = {'mvp': rs.mvp()}
        uniforms['u_modelViewMatrix'] = rs.view
        uniforms['u_normalMatrix'] = np.eye(3,dtype=np.float32)
        uniforms['u_projectionMatrix'] = rs.proj
        uniforms['czm_sunDirectionEC'] = np.array((np.sin(time.time()),np.cos(time.time()),np.cos(time.time())+np.sin(time.time())))
        print(' - rendering {} primitives.'.format(len(self.primitives)))
        for prim in self.primitives:
            # Use Shader + Material.
            material = prim.get('material', None)
            material = model.materials[material]

            material.use(rs, prim['attrs'], uniforms, model)

            # Draw.
            idx_acc = prim.get('index_accessor', None)
            if idx_acc is not None:
                idx_acc = model.accessors[idx_acc]
                bv = model.bufferViews[idx_acc['bufferView']]
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.bos[model.bufferViews[idx_acc['bufferView']]['buffer']])
                offset = (bv['byteOffset'] + idx_acc['byteOffset'])
                print(' - Drawing {} elements (offset {}).'.format(idx_acc['count'], offset))
                glDrawElements(prim['mode'], idx_acc['count'], idx_acc['componentType'], c_void_p(offset))
            else:
                count = prim['attrs'][0]['count']
                print(' - Drawing {} arrays.'.format(count))
                glDrawArrays(prim['mode'], count, 0)

            glDisable(GL_TEXTURE_2D)
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)
            glBindBuffer(GL_ARRAY_BUFFER, 0)
            if material: material.unuse()

class Node:
    def __init__(self, j):
        self.name = j.get('name','')
        self.mesh = j.get('mesh', -1)
        self.transform = parse_any_transform(j)
        self.children = j.get('children', [])

    def render(self, model, rs):
        rs = rs.compose(self.transform) if self.transform is not None else rs
        if self.mesh >= 0:
            model.meshes[self.mesh].render(model, rs)

class GltfModel:
    def __init__(self,path=None, glb=None):
        self.glbBuffer = None
        if path:
            self.path = path
            self.baseDir = os.path.split(path)[0]
            self.possiblePaths = (self.baseDir, os.getcwd())

            if path.endswith('glb'):
                glb = readFile(path, possiblePaths=['.'])
                self.parse_glb_and_load(glb)
            else:
                with open(path) as fp:
                    self.j = json.load(fp)
                self.parse_json_and_load()
        else:
            self.possiblePaths = '.'
            assert(glb is not None)
            self.parse_glb_and_load(glb)


    def parse_glb_and_load(self, glb):
        assert(glb[0:4] == b'glTF')
        version = int.from_bytes(glb[4:8], 'little')
        length = int.from_bytes(glb[8:12], 'little')
        offset = 12
        print('GLB:', version,length)

        # Chunk 0 (the json)
        chunk_length = int.from_bytes(glb[offset:offset+4], 'little')
        chunk_type = glb[offset+4:offset+8]
        print('GLB Chunk 0:', chunk_length, chunk_type)
        offset += 8
        assert(chunk_type == b'JSON')

        self.j = json.loads(glb[offset:offset+chunk_length].decode('ascii')) # Is ascii always the encoding?
        offset += chunk_length

        # Chunk 1 (the data)
        chunk_length = int.from_bytes(glb[offset:offset+4], 'little')
        chunk_type = glb[offset+4:offset+8]
        offset += 8
        assert(chunk_type.find(b'BIN') != -1)
        self.glbBuffer = glb[offset:offset+chunk_length]

        # And finish.
        self.parse_json_and_load()

    # Parse and load all data onto CPU (do not touch gpu)
    def parse_json_and_load(self):
        # Buffers.
        bufferSpecs = self.j['buffers']
        self.bos = []
        self.cpuBuffers = []
        if self.glbBuffer is not None:
            assert(len(bufferSpecs) == 1)
            self.cpuBuffers = [self.glbBuffer]
        else:
            for buffer in bufferSpecs:
                if 'uri' in buffer:
                    data = readFile(buffer['uri'], possiblePaths=self.possiblePaths)
                    self.cpuBuffers.append(data)
                else:
                    assert(False and 'todo: parse base64')
        # Views.
        self.bufferViews = self.j['bufferViews']
        # Accessors.
        self.accessors = [parse_accessor(jj) for jj in self.j['accessors']]
        # Images.
        if 'images' in self.j:
            self.images = [load_image(img_, possiblePaths=self.possiblePaths, \
                                 bufferViews=self.bufferViews,glbBuffer=self.glbBuffer) for img_ in self.j.get('images',[])]
        # Samplers.
        self.samplers = self.j.get('samplers', [])
        # Textures (GL).
        self.texture_specs = self.j.get('textures',[])
        self.gl_textures = []
        # Nodes.
        self.nodes = [Node(n) for n in self.j['nodes']]

        # WebGL Techniques
        self.techniqueContainer = None
        if 'KHR_techniques_webgl' in self.j.get('extensionsUsed', []):
            self.techniqueContainer = KhrTechniquesWebgl(self.j['extensions'],self.j['materials'],self.possiblePaths,
                    self.bufferViews, self.cpuBuffers)

        self.materials = [Material(m, self.techniqueContainer) for m in self.j['materials']]

        # Meshes.
        self.meshes = [Mesh(m, self.accessors, self.bufferViews, self.materials) for m in self.j['meshes']]

        self.glbBuffer = None


    # Upload to gpu, delete some of the larger cpu arrays (cpuBuffers, images)
    def upload(self):
        # Upload buffer objects.
        self.bos = np.array([glGenBuffers(len(self.cpuBuffers))]).ravel()
        for i, bo in enumerate(self.bos):
            # It *appears* that for glBufferData we can bind to GL_ELEMENT_ARRAY_BUFFER or
            # GL_ARRAY_BUFFER, and it makes no difference. That is great news because for
            # the GLB format there is only one buffer!
            # If this were not the case, we'd need a VBO/IBO per bufferView (this is actually what
            # the tinygltf example does, so I'm hoping the good behaviour is not just on my machine).
            glBindBuffer(GL_ARRAY_BUFFER, bo)
            glBufferData(GL_ARRAY_BUFFER, len(self.cpuBuffers[i]), self.cpuBuffers[i], GL_STATIC_DRAW)
            glBindBuffer(GL_ARRAY_BUFFER, 0)
        self.cpuBuffers = []
        # Upload textures.
        self.gl_textures = [make_texture(t, self.images, self.samplers) for t in self.texture_specs]
        self.images = []
        # Create vertex arrays (todo)


    def compileShaders(self, dictOfPrograms):
        if self.techniqueContainer:
            return self.techniqueContainer.uploadReturnPrograms(dictOfPrograms,{})
        return []


    def renderNode(self, idx, rs):
        self.nodes[idx].render(self, rs)

    def __repr__(self): return str(self)
    def __str__(self):
        s = ''
        name = self.path if hasattr(self, 'path') else 'glb'
        s += '\n GltfModel ({}):'.format(name)
        s += '\n    - Buffers ({}cpu {}uploaded):'.format(len(self.cpuBuffers), len(self.bos))
        s += '\n    - BufferViews ({}):'.format(len(self.bufferViews))
        s += '\n    - Accessors ({}):'.format(len(self.accessors))
        s += '\n    - Textures ({}cpu {}uploaded):'.format(len(self.texture_specs), len(self.gl_textures))
        s += '\n    - Meshes ({}):'.format(len(self.meshes))
        s += '\n    - Nodes ({}):'.format(len(self.nodes))
        return s

    def __del__(self):
        glDeleteBuffers(len(self.bos),self.bos)
        glDeleteTextures(len(self.gl_textures),self.gl_textures)
        self.gl_textures, self.bos = [], []


class ProgramContainer(dict):
    def __init__(self, d):
        self.d = d
        self.cnts = {k:0 for k in d}
    def markModelUses(self, name):
        self.cnts[name] += 1
    def decrementModelUses(self, name):
        self.cnts[name] -= 1
        if self.cnts[name] == 0:
            print(' - Freeing program',name)
            del self.d[name]
            del self.cnts[name]


def make_shaders():
    basic = Shader('''
                #version 400
                in layout(location = 0) vec3 a_position;
                uniform mat4 mvp;
                out vec4 v_color;
                void main() {
                    gl_Position = mvp * vec4(a_position, 1.0);
                    //v_color = vec4(1.0);
                    v_color = vec4(abs(sin(a_position.xyz*2.0)),1.0);
                }''', '''
                #version 400
                in vec4 v_color;
                void main() {
                    gl_FragColor = v_color;
                }''')
    basicTextured = Shader('''
                #version 400
                in layout(location = 0) vec3 a_position;
                in layout(location = 1) vec2 a_uv;
                uniform mat4 mvp;
                out vec2 v_uv;
                void main() {
                    gl_Position = mvp * vec4(a_position, 1.0);
                    v_uv = a_uv;
                }''', '''
                #version 400
                in vec2 v_uv;
                uniform sampler2D diffuseSampler;
                void main() {
                    gl_FragColor = texture2D(diffuseSampler, v_uv);
                }''')
    return dict(locals())

if __name__ == '__main__':
    glutInit()
    glutInitWindowSize(900,900)
    glutCreateWindow('glTF')
    glEnable(GL_DEPTH_TEST)
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    #glDisable(GL_CULL_FACE)

    shaders = make_shaders()

    #model = GltfModel(path='/home/slee/stuff/tdt/3rdparty/tinygltf/models/Cube/Cube.gltf')
    model = GltfModel(path='/home/slee/stuff/tdt/assets/d2.glb')
    #model = GltfModel(path='/home/slee/stuff/tdt/assets/v.glb')
    model.upload()
    shaders.update(model.compileShaders(shaders))
    print('SHADERS:', list(shaders.keys()))

    glClearColor(0,0,0,1)
    glColor4f(1,1,1,1)

    fov = 80
    z_near = 5
    u,v = np.tan(np.deg2rad(fov/2)), np.tan(np.deg2rad(fov/2))
    proj = frustum_z_forward(-u,u,-v,v,z_near,300)

    last_time = time.time()
    avg_dt = 0
    fpss = []
    for i in range(1000):
        glutMainLoopEvent()
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

        view = look_at_z_forward(
                np.array((np.sin(time.time()*2)*40,0,np.cos(time.time()*2)*40),dtype=np.float32),
                #np.array((np.sin(time.time()*.4)*520,20000,np.cos(time.time()*.4)*520),dtype=np.float32),
                np.zeros(3,dtype=np.float32),
                np.array((0,1.,0),dtype=np.float32))
        rs = RenderState(view,proj,shaders=shaders)

        model.renderNode(0, rs)

        glFlush()
        glutSwapBuffers()
        now_time = time.time()
        if i < 10: avg_dt = now_time-last_time
        else: avg_dt = (now_time-last_time)*.1 + avg_dt*.9
        print('dt', now_time-last_time)
        print('fps', 1./(avg_dt))
        last_time = now_time
        fpss.append(1./avg_dt)


    import matplotlib.pyplot as plt
    plt.title('fps'); plt.plot(fpss); plt.show()

    print(model)
    del shaders
    del model
    model = 0
    import gc
    gc.collect()
