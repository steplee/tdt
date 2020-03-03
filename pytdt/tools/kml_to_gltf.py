import os, sys, numpy as np
import re

def get_kml_polygons(kml):
    kml = kml.replace('\n','').encode('utf8')
    import lxml.etree as etree
    import lxml.objectify as objectify
    root = etree.fromstring(kml)
    for elem in root.getiterator(): elem.tag = etree.QName(elem).localname

    all_shapes = []
    placemarks = root.findall('.//Placemark')
    for placemark in placemarks:
        coords = placemark.find('.//coordinates').text
        pts = []
        for c in coords.split(' '):
            c = c.strip()
            if c:
                pts.append([float(cc) for cc in c.split(',')])
        pts = np.array(pts)

        all_shapes.append(dict(kind='poly', pts=pts))

    return all_shapes

polys = get_kml_polygons(open('/home/slee/Downloads/Untitled map.kml').read())
print('polys',polys)

class GltfWriter:
    def __init__(self, outPath):
        self.path = os.split(outPath)[0]
        self.filename = outPath

        self.images = self.nodes = self.meshes = []
        self.buffers = self.bufferViews = []
        self.mainBufferData = bytearray()
        self.mainBufferIdx = 0

        if outPath.endswith('glb'):
            self.binary = True
        elif outPath.endswith('gltf'):
            self.binary = False
        else: assert(False)

    # Note: In the general case primitives can share vertex data thorugh same accessors.
    # But to simplify this code, we only allow 1-1 mappings.
    # Note: Returns accessor id. Will create bufferView + accessor (and push data to main buffer)
    def putVertexData(self, data, semantic):
        pass

    def putMesh(self, mesh):
        assert(False and 'TODO: put all vertex data passed in this mesh')

    def putNode(self, nodeSpec):
        assert('mesh' in nodeSpec) # Not really ... we can have empty nodes.
        assert(nodeSpec['mesh'] <= len(self.meshes)) # Put nodes only after meshes
        self.nodes.append(nodeSpec)
        return len(self.nodes)-1

    def putImage(self, imageData):
        assert(imageData.shape[-1] == 3)
        if self.binary:
            img_bytes = cv2.imencode('jpg', imageData)
            self.appendMainBuffer(img_bytes)
            spec = dict(bufferView=bv, mimeType='image/jpeg')
            self.images.append(spec)

        else:
            # TODO Allow base64 encoding.
            imageName = 'img_{}.jpg'.format(len(self.images))
            imagePath = os.path.join(self.path, imageName)
            cv2.imwrite(imagePath, imageData)
            spec = dict(uri=imagePath)
            self.images.append(spec)

        return len(self.images)-1

    def putTextureFromImage(self, imageData, sampler=0):
        source = self.putImage(imageData)
        self.textures.append(dict(source=source,sampler=sampler))
        return len(self.textures)-1

    def appendMainBufferAndGetBufferView(self, data, target=34962):
        self.bufferViews.append({
                'buffer': self.mainBufferIdx,
                'byteOffset': len(self.mainBufferData),
                'byteLength': len(data),
                'target': target })
        self.mainBufferData.append(data)
        return len(self.bufferViews)-1

    def write(self):
        if self.binary:
            #assert(len(self.buffers) == 1)

            data0 = jobj.encode('ascii')
            data1 = self.mainBufferData

            chunk0 = bytearray()
            chunk0.extend((len(data0)).to_bytes(4,'little'))
            chunk0.extend(b'JSON')
            chunk0.extend(data0)
            while len(chunk0) % 4 != 0: chunk0.append(b'\x20')

            chunk1 = bytearray()
            chunk1.extend((len(data1)).to_bytes(4,'little'))
            chunk1.extend(b'BIN ')
            chunk1.extend(data1)
            while len(chunk1) % 4 != 0: chunk1.append(b'\x20')

            header = bytearray()
            header.extend(b'glTF')
            header.extend((2).to_bytes(4, 'little'))
            length = 12 + len(chunk0) + len(chunk1)
            header.extend((length).to_bytes(4, 'little'))

            bits = header + chunk0 + chunk1

            with open(self.filename, 'wb') as fp:
                fp.write(bits)
            print('\n - Done writing', self.filename,'\n')

        else:
            assert(False and 'todo')
