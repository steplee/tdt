from .gltf import *

def parse_bounding_volume(j):
    if 'box' in j:
        return np.array(j['box'])

class Tileset:
    def __init__(self, path):
        self.path = path
        self.baseDir = os.path.split(path)[0]
        self.possiblePaths = (self.baseDir, os.getcwd())

        assert(path.endswith('json'))
        with open(path) as fp:
            self.j = json.load(fp)

        self.parse()

    def parse(self):
        self.geometricError = self.j['geometricError']
        self.root = Tile(self, self.j['root'])
        self.root.load()
        self.root.upload()

    def __repr__(self): return str(self)
    def __str__(self):
        s = ''
        s += '\nTileset:'
        s += '\n    - geometricError ({}):'.format(self.geometricError)
        s += '\n    - root:' + str(self.root).replace('\n','\n\t')
        return s

class Tile:
    def __init__(self, tileset, j):
        self.tileset = tileset
        self.j = j
        self.content = None
        self.parse()

    def parse(self):
        self.transform = parse_any_transform(self.j)
        self.boundingVolume = parse_bounding_volume(self.j['boundingVolume'])
        self.geometricError = self.j['geometricError']
        self.refine = self.j['refine']

    def upload(self):
        assert(self.content)
        self.content.upload()

    def load(self):
        if 'url' in self.j['content']:
            url = self.j['content']['url']
            if url.endswith('b3dm'):
                bits = readFile(url, self.tileset.possiblePaths)
                self.content = B3DM(bits)
            else:
                assert(False)
        else:
            assert(False)

    def __repr__(self): return str(self)
    def __str__(self):
        s = ''
        s += '\nTile:'
        s += '\n    - geometricError ({}):'.format(self.geometricError)
        s += '\n    - content:' + str(self.content).replace('\n','\n\t')
        return s


# TODO: Use memoryview's instead of bytes slices, I heard they copy every slice!
# TODO: cleanup the code, its copied from another project.
class B3DM:
    def __init__(self, bits):
        assert(bits[0:4] == b'b3dm')
        version = int.from_bytes(bits[4:8], 'little')
        byteLen = int.from_bytes(bits[8:12], 'little')
        featureTableJsonLen = int.from_bytes(bits[12:16], 'little')
        featureTableBinLen = int.from_bytes(bits[16:20], 'little')
        batchTableJsonLen = int.from_bytes(bits[20:24], 'little')
        batchTableBinLen = int.from_bytes(bits[24:28], 'little')
        featureTableLen = featureTableJsonLen + featureTableBinLen
        batchTableLen = batchTableJsonLen + batchTableBinLen
        offset = 28

        # Some b3dm files I have have either a corrupted or deprecated header,
        # where there seems to be no batch table length specified.
        # I try to over come this by inserting empty tables and instead of going exactly
        # to the glTF portion based on offsets, I search for the magic 'glTF'
        if featureTableLen + batchTableLen > len(bits) - 28:
            print(' - Corrupted Feature/Batch Table!')
            print('   Will still search for glTF and use empty tables for both!')
            featureTable = '{"BATCH_LENGTH":0}'.encode('ascii')
            while len(featureTable) % 4 != 0:
                featureTable += bytes([0x20])
            featureTableJsonLen = len(featureTable)
            featureTableBinLen = 0
            featureTableLen = featureTableJsonLen + featureTableBinLen
            batchTable = b''
            batchTableLen = 0
            batchTableJsonLen = 0
            batchTableBinLen = 0
            batchTableLen = batchTableJsonLen + batchTableBinLen
            offset = bits.find(b'glTF')
            if offset == -1:
                print(' - Failed to recover from corrupt header. No glTF portion found. Exiting.')
                sys.exit(1)
        else:
            featureTable = bits[offset:offset+featureTableLen]
            offset += featureTableLen
            batchTable = bits[offset:offset+batchTableLen]
            offset += batchTableLen

        self.batchTableJsonLen, self.batchTableBinLen = batchTableJsonLen, batchTableBinLen
        self.featureTableJsonLen, self.featureTableBinLen = featureTableJsonLen, featureTableBinLen

        glb_bytes = bits[offset:]
        assert(glb_bytes[0:4] == b'glTF')
        gltf_version = int.from_bytes(glb_bytes[4:8], 'little')
        if gltf_version != 2:
            print(' - ERROR:')
            print('      Detected non glTF version ({}). Only glTF 2.0 is supported.'.format(gltf_version))
            print('      Use the tool "./convert_b3dm_gltf1_to_gltf2/convert.py" to upgrade glTF 1 -> 2')

        self.model = GltfModel(glb=glb_bytes)

    def upload(self):
        self.model.upload()

    def __repr__(self): return str(self)
    def __str__(self):
        s = ''
        s += '\nB3DM Tile:'
        s += '\n    - FeatureTable ({} {}):'.format(self.featureTableJsonLen, self.featureTableBinLen)
        s += '\n    - BatchTable ({} {}):'.format(self.batchTableJsonLen, self.batchTableBinLen)
        s += '\n    - Model:' + str(self.model).replace('\n','\n\t')
        return s



if __name__ == '__main__':
    ts = Tileset('./assets/tileset2.json')
    print(ts)
