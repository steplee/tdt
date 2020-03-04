import cv2
import numpy as np
import matplotlib.pyplot as plt
import os
import random

import torch
import torch.utils.data
import torchvision

from PIL import Image

import pickle,json

class AerialImageLabelingDataset(torch.utils.data.Dataset):
    def __init__(self,
            root = '/data/aerialimagelabeling/AerialImageDataset/',
            kind = 'train',
            size=512,
            drop_empty=True,
            augmentChance = .5,
            patchesFromOneLoad = 3, # Reuse same image since disk read/decode is costly
            decimate = 2,
            ):

        print('AerialImageLabelingDataset Constructor.')

        self.train_root = os.path.join(root, kind)

        self.train_examples = [f for f in  os.listdir(os.path.join(self.train_root,'images'))]

        self.patchesFromOneLoad = patchesFromOneLoad
        self.size = size
        self.decimate = decimate

        self.is_test = kind == 'test'
        self.is_train = not self.is_test

        print(' - Dataset with {} examples.'.format(len(self.train_examples)))

    def __getitem__(self, idx):
        ex = self.train_examples[idx%len(self.train_examples)]
        rgb = cv2.cvtColor(cv2.imread(os.path.join(self.train_root, 'images', ex)), cv2.COLOR_BGR2RGB)
        if self.is_train:
            label = cv2.imread(os.path.join(self.train_root, 'gt', ex), 0)
        else:
            label = np.zeros((*rgb.shape[0:2],1), dtype=np.uint8)

        x = np.empty((self.patchesFromOneLoad, self.size,self.size,3), dtype=np.uint8)
        y = np.zeros((self.patchesFromOneLoad, self.size,self.size,1), dtype=np.uint8)

        scale = np.random.sample() * .5 + .7
        rgb = cv2.resize(rgb, (0,0), fx=scale,fy=scale)
        if self.is_train:
            label = cv2.resize(label, (0,0), fx=scale,fy=scale)
            if len(label.shape) == 2: label = label[..., np.newaxis]

        sz = self.size
        for j in range(self.patchesFromOneLoad):
            xx = np.random.randint(0, rgb.shape[1]-sz)
            yy = np.random.randint(0, rgb.shape[0]-sz)

            rr = np.random.sample()
            if rr < .25:
                x[j] = rgb[yy:yy+sz, xx:xx+sz].transpose(1,0, 2)
                y[j] = label[yy:yy+sz, xx:xx+sz].transpose(1,0, 2)
            elif rr < .5:
                x[j] = rgb[yy:yy+sz, xx:xx+sz][::-1]
                y[j] = label[yy:yy+sz, xx:xx+sz][::-1]
            else:
                x[j] = rgb[yy:yy+sz, xx:xx+sz]
                y[j] = label[yy:yy+sz, xx:xx+sz]

            #b = np.random.sample() * .6 + .7
            #x[j] = cv2.multiply(x[j], b)

        y = y[:, ::self.decimate, ::self.decimate]
        y = torch.from_numpy(y.transpose(0,3,1,2))
        x = torch.from_numpy(x.transpose(0,3,1,2))
        return (x,y)

    def __len__(self): return 9999


IMGNET_MU = torch.cuda.FloatTensor([.4,.4,.4]).view(1,3,1,1)
IMGNET_SIG = torch.cuda.FloatTensor([.2,.2,.2]).view(1,3,1,1)
def fix_batch(b):
    x = torch.cat(tuple(b[0]), dim=0).cuda().to(torch.float32)
    y = torch.cat(tuple(b[1]), dim=0).cuda().to(torch.float32)

    x.div_(255.).sub_(.4).div_(.25)
    x.div_(255.).sub_(IMGNET_MU).div_(IMGNET_SIG)

    y.div_(255.)

    return x,y
