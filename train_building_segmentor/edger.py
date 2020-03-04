import os, numpy as np, sys, cv2
import torch, torch.nn as nn, torch.nn.functional as F
import torchvision
import ignite
from ignite.engine import create_supervised_trainer, Events, Engine
from ignite.metrics import Accuracy
from matplotlib.cm import inferno
from ignite.handlers import Checkpoint, DiskSaver
from .dataset import AerialImageLabelingDataset, fix_batch

bn = torchvision.ops.misc.FrozenBatchNorm2d
dilate = None
dilate = [2,2,2]
model = torchvision.models.resnet50(True, norm_layer=bn, replace_stride_with_dilation=dilate)
model = list(model.children())[0:7]
model = nn.Sequential(*model, nn.UpsamplingBilinear2d(scale_factor=2), nn.Conv2d(1024, 1, 1))
model = model.cuda().eval()

ckpt = torch.load('saves/checkpoint_1500.pth')
model.load_state_dict(ckpt['model'])

decimate = 2

def run_on(img):
    with torch.no_grad():
        y = np.zeros((1,img.shape[0]//decimate,img.shape[1]//decimate,1), dtype=np.uint8)
        img = img[np.newaxis]
        y = torch.from_numpy(y.transpose(0,3,1,2))
        x = torch.from_numpy(img.transpose(0,3,1,2))
        print(x.shape)
        print(img.shape)

        batch = (x,y)
        batch = fix_batch(batch)
        model.eval()
        pred = torch.sigmoid(model(batch[0]))
        y = batch[1]
        x = F.interpolate(batch[0], (pred.size(2),pred.size(3)))

        x = (255*((x-x.min())/(x.max()-x.min()))).permute(0,2,3,1).to(torch.uint8).cpu().numpy()
        y = y.permute(0,2,3,1).cpu().numpy()
        pred = pred.permute(0,2,3,1).cpu().numpy()

        y,x,pred = np.vstack(y).squeeze(), np.vstack(x), np.vstack(pred).squeeze()
        pred,y = inferno(pred)[..., 0:3], np.stack((y,)*3,-1)*(0,90,255)
        y,pred = (y).astype(np.uint8), (pred*255).astype(np.uint8)
        y_pred = np.hstack((y,pred))

        dimg = np.hstack((x,x))
        dimg = cv2.addWeighted(dimg, .9, y_pred, .9, 0)
        cv2.imwrite('out/pred.jpg', cv2.cvtColor(dimg, cv2.COLOR_RGB2BGR))

#img = np.eye(512,dtype=np.uint8)*200
#img = np.stack((img,)*3,-1)
img = cv2.imread('KHOP_FRAME.png')
img = cv2.resize(img, (512*2,2*512))

run_on(img)
