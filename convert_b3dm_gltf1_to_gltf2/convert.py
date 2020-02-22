import argparse
import json
import sys, os
import subprocess
import numpy as np

# https://github.com/CesiumGS/3d-tiles/tree/master/specification/TileFormats/Batched3DModel
# https://github.com/CesiumGS/gltf-pipeline

'''
NOTE: You must run bootstrap.sh first to install npm package 'gltf-pipeline'

I have many b3dm's which embed gltf 1.0 (with KHR_binary_extension).
There is a converter ('gltf-pipeline'), but it is only aware of glTF and not b3dm.
Luckily it handles that extension and converts it to glTF 2.0 GLB, which is exactly
what I need.

So we need to extract the glTF embedded in the b3dm, transform it, then make a new b3dm.

This script does that.
'''

parser = argparse.ArgumentParser()
parser.add_argument('--input', required=True)
parser.add_argument('--output', required=True)

args = parser.parse_args()

#uint32_t = np.uint32.newbyteorder('L')

with open(args.input, 'rb') as fp:
    inb = fp.read()

    gltf_start = 0

    # Try to parse b3dm header, if that fails
    # just look for the magic 'glTF'.
    if 'b3dm' in args.input:
        assert(inb[0:4] == b'b3dm')
        version = int.from_bytes(inb[4:8], 'little')
        byteLen = int.from_bytes(inb[8:12], 'little')
        featureTableJsonLen = int.from_bytes(inb[12:16], 'little')
        featureTableBinLen = int.from_bytes(inb[16:20], 'little')
        batchTableJsonLen = int.from_bytes(inb[20:24], 'little')
        batchTableBinLen = int.from_bytes(inb[24:28], 'little')
        featureTableLen = featureTableJsonLen + featureTableBinLen
        batchTableLen = batchTableJsonLen + batchTableBinLen
        print(' - Read input file', args.input)
        print('    - magic   :', inb[0:4].decode('ascii'))
        print('    - v       :', version)
        print('    - len     :', byteLen)
        print('    - ft_j_len:', featureTableJsonLen)
        print('    - ft_b_len:', featureTableBinLen)
        print('    - bt_j_len:', batchTableJsonLen)
        print('    - bt_b_len:', batchTableBinLen)
        offset = 28

        # Some b3dm files I have have either a corrupted or deprecated header,
        # where there seems to be no batch table length specified.
        # I try to over come this by inserting empty tables and instead of going exactly
        # to the glTF portion based on offsets, I search for the magic 'glTF'
        if featureTableLen + batchTableLen > len(inb) - 28:
            print(' - Corrupted Feature/Batch Table!')
            print('   Will still search for glTF and output empty tables for both!')
            featureTable = '{"BATCH_LENGTH":0}'.encode('ascii')
            featureTableLen = len(featureTable)
            batchTable = b''
            batchTableLen = 0
            out_b3dm_top = \
                    b'b3dm' + \
                    int.to_bytes(1, 4, 'little') + \
                    int.to_bytes(0, 4, 'little') + \
                    int.to_bytes(featureTableJsonLen, 4, 'little') + \
                    int.to_bytes(featureTableBinLen, 4, 'little') + \
                    int.to_bytes(batchTableJsonLen, 4, 'little') + \
                    int.to_bytes(batchTableBinLen, 4, 'little') + \
                    featureTable + batchTable
            offset = inb.find(b'glTF')
            if offset == -1:
                print(' - Failed to recover from corrupt header. No glTF portion found. Exiting.')
                sys.exit(1)
        else:
            featureTable = inb[offset:offset+featureTableLen]
            offset += featureTableLen
            batchTable = inb[offset:offset+batchTableLen]
            offset += batchTableLen
            out_b3dm_top = inb[0:offset]
    else:
        assert(False and 'only b3dm is supported right now.')

    glb_1_bytes = inb[offset:]
    assert(glb_1_bytes[0:4] == b'glTF')

    # We now have a .glb in glTF 1.0 with the KHR_binary_gltf extension
    # We will use the npm package 'gltf-pipeline' to convert
    # it to glTF 2.0 glb.
    with open('tmp.glb', 'wb') as fp:
        fp.write(glb_1_bytes)

    subprocess.getoutput('npx gltf-pipeline -i {} -o {}' \
            .format('tmp.glb', args.output + '.glb'))


    # Now we have our glb 2.0 bytes to splice back into our
    # initial b3dm
    with open(args.output + '.glb', 'rb') as fp:
        out_glb_bytes = fp.read()

    print(' - Deleting temp file', args.output+'.glb')
    os.unlink(args.output+'.glb')

    out_glb_offset = len(out_b3dm_top)
    out_byte_len = len(out_b3dm_top) + len(out_glb_bytes)
    out_byte_len = int.to_bytes(out_byte_len, 4, 'little')
    out_bytes = out_b3dm_top[0:8] + out_byte_len + out_b3dm_top[12:] + out_glb_bytes
    print(' - Writing final output file:', args.output)
    print('    - b3dm magic:', out_bytes[0:4].decode('ascii'))
    print('    - b3dm v    :', int.from_bytes(out_bytes[4:8],'little'))
    print('    - b3dm len  :', int.from_bytes(out_bytes[8:12],'little'))
    print('    - glb  magic:', out_bytes[out_glb_offset+0:out_glb_offset+4].decode('ascii'))
    print('    - glb  v    :', int.from_bytes(out_bytes[out_glb_offset+4:out_glb_offset+8],'little'))
    print('    - glb  len  :', int.from_bytes(out_bytes[out_glb_offset+8:out_glb_offset+12],'little'))

    with open(args.output, 'wb') as fp:
        fp.write(out_bytes)
    print(' - Done.')
