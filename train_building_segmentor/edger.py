import os, numpy as np, sys, cv2
import torch, torch.nn as nn, torch.nn.functional as F
import torchvision
import ignite
from ignite.engine import create_supervised_trainer, Events, Engine
from ignite.metrics import Accuracy
from matplotlib.cm import inferno
from ignite.handlers import Checkpoint, DiskSaver
from .dataset import AerialImageLabelingDataset, fix_batch

def color_normalize_img(a):
    #aa = cv2.cvtColor(a, cv2.COLOR_GRAY2RGB)
    r = np.copy(a); r[r>0] = 0
    r = abs(r)
    g = np.copy(a); g[g<0] = 0
    b = np.copy(a); b *= 0
    aa = np.stack( (b,g,r), -1)
    max_ = aa.max()
    return (255*(aa)/(max_)).astype(np.uint8)

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

def get_edges(x):
    if len(x.shape) == 3 and x.shape[-1] == 3:
        x = cv2.cvtColor(x, cv2.COLOR_RGB2GRAY)

    #e = cv2.Laplacian(x, cv2.CV_32F) ** 2

    ex = cv2.Sobel(x, cv2.CV_32F, 1, 0)
    ey = cv2.Sobel(x, cv2.CV_32F, 0, 1)
    e = np.sqrt(ex**2 + ey**2)
    #e = e ** 2
    '''TEMP = .005
    e_max = e.max()*TEMP
    e = np.exp(e*TEMP-e_max) / np.exp(e*TEMP-e_max).sum()'''

    return e

def get_polys(edges_pred, edges_x, pred, x):
    '''
    if len(edges.shape) == 3 and edges.shape[-1] == 3:
        edges = cv2.cvtColor(edges, cv2.COLOR_RGB2GRAY)
    _, edges = cv2.threshold(edges, 40, 255, 0)
    tupl = cv2.findContours(edges, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    contours = tupl[-2]
    print(contours, tupl[1])
    dimg = np.copy(x)//2
    cv2.drawContours(dimg, contours, -1, (0,255,0), 1)
    cv2.imshow('contours',dimg)
    cv2.waitKey(0); cv2.destroyAllWindows()
    return contours
    '''

    # A simple hill-climbing algorithm I came up with.
    H,W = pred.shape[0:2]
    pred2 = pred
    pred2 = cv2.GaussianBlur(pred, (7,7), 3.0)
    if False:
        gradx = (cv2.Sobel(pred2, cv2.CV_32F, 1,0)*10).clip(-1,1)
        grady = (cv2.Sobel(pred2, cv2.CV_32F, 0,1)*10).clip(-1,1)
        grads = np.stack((grady,gradx), -1)
        #grads = np.stack((gradx,grady), -1)
        #grads = cv2.GaussianBlur(grads, (7,7), 2.0)
        grads = (grads * 1).astype(np.int16).clip(-1,1)
    else:
        grads = np.zeros((H,W,2), dtype=np.int16)
        for yy in range(0,H-1):
            for xx in range(0,W-1):
                v = pred2[yy,xx]
                for dy in range(-1,2):
                    for dx in range(-1,2):
                        if pred2[yy+dy,xx+dx] > v+1e-6:
                            grads[yy,xx] = (dy,dx)
                            v = pred2[yy+dy,xx+dx]
        gradx = grads[...,1].astype(np.float32)
        grady = grads[...,0].astype(np.float32)

    print(grads)
    POS_THRESH0 = .7
    POS_THRESH = .5
    NEG_THRESH = .01
    WALL_THRESH = 250.4
    WALL, NEGATIVE, POSITIVE = -5,-4, -1
    negative, positive = set([]), set([])
    print(pred, pred.min(), pred.max(), pred.mean(), pred.std())
    # What's better: iterating over each seed and never combining (but having dense matrix), or
    #                having a grid of sets?
    #seeds = np.stack(np.meshgrid((np.arange(0,H),np.arange(0,W))), 0).reshape(2,-1).T
    grid = [[0 for xx in range(W)] for yy in range(W)]
    for yy in range(H):
        for xx in range(W):
            if pred[yy,xx] > POS_THRESH0:   positive.add((yy,xx)); grid[yy][xx] = POSITIVE
            elif pred[yy,xx] < NEG_THRESH: negative.add((yy,xx)); grid[yy][xx] = NEGATIVE
            elif edges_x[yy,xx] > WALL_THRESH: grid[yy][xx] = WALL # A wall can be positive, but that already handled
            else: grid[yy][xx] = set([(yy,xx)])

    print('grad', grads.shape, grads.dtype)

    result = np.zeros((H,W,3), dtype=np.uint8)

    for iter in range(32):
        print('befor iter {}: ({} pos) ({} neg)'.format(iter,len(positive)/(H*W), len(negative)/(H*W)))
        for yy in range(H):
            for xx in range(W):
                if type(grid[yy][xx]) == set and len(grid[yy][xx]):
                    grad = grads[yy,xx]
                    #print(grad)
                    # We tapered out without hitting a positive tile, so we, and anything that lands here, are bad.
                    if abs(grad).sum() == 0 or yy+grad[0] >= H or xx+grad[1] >= W:
                        negative.update(grid[yy][xx])
                        grid[yy][xx] = NEGATIVE
                        continue
                    # Otherwise, move to next tile (or move to pos/neg set if hitting a terminal)
                    nxt = grid[yy+grad[0]][xx+grad[1]]
                    if type(nxt) == set:
                        nxt.update(grid[yy][xx])
                        grid[yy][xx] = nxt
                        result[yy,xx] = (iter*8+5,0,0)
                    elif nxt == POSITIVE:
                        positive.update(grid[yy][xx])
                        grid[yy][xx] = POSITIVE
                    elif nxt <= NEGATIVE:
                        if nxt == WALL: print('hit wall at', yy,xx)
                        negative.update(grid[yy][xx])
                        grid[yy][xx] = NEGATIVE
                    else: assert(False)


    for (yyxx) in list(negative):
        try:
            yy,xx = yyxx
            result[yyxx] = (0,0,100)
        except:
            print(yyxx)
    for (yyxx) in list(positive):
        yy,xx = yyxx
        result[yyxx] = (0,205,0)
    dimg = np.hstack((result, color_normalize_img(gradx), color_normalize_img(grady)))
    cv2.imshow('result', dimg)
    cv2.waitKey(0); cv2.destroyAllWindows()



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
        x = F.interpolate(batch[0], (pred.size(2),pred.size(3)))

        x = (255*((x-x.min())/(x.max()-x.min()))).permute(0,2,3,1).to(torch.uint8).cpu().numpy()
        pred = pred.permute(0,2,3,1).cpu().numpy()

        x,raw_pred = np.vstack(x), np.vstack(pred).squeeze()
        pred = inferno(raw_pred)[..., 0:3]
        pred =  (pred*255).astype(np.uint8)
        x_pred = cv2.addWeighted(x, .9, pred, .9, 0)

        #edges = cv2.Laplacian(cv2.cvtColor(x, cv2.COLOR_RGB2GRAY), cv2.CV_32F)
        #edges_pred = cv2.Laplacian(cv2.cvtColor(pred, cv2.COLOR_RGB2GRAY), cv2.CV_32F)
        edges_x,edges_pred = get_edges(x), get_edges(pred)
        edges_x_c = color_normalize_img((edges_x))
        edges_pred_c = color_normalize_img((edges_pred))

        get_polys(edges_pred,edges_x, raw_pred,x)

        print(edges_x.shape, edges_pred.shape, x_pred.shape)

        dimg = np.hstack((x_pred, edges_x_c, edges_pred_c))
        cv2.imwrite('out/pred.jpg', cv2.cvtColor(dimg, cv2.COLOR_RGB2BGR))

#img = np.eye(512,dtype=np.uint8)*200
#img = np.stack((img,)*3,-1)
#img = cv2.imread('KHOP_FRAME.png')
if True:
    img = cv2.imread('/home/slee/Downloads/kedw_print_50.png')[200:,800:]
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = cv2.pyrDown(img)
    img = cv2.resize(img, (512*2,2*512))
else:
    img = cv2.imread('/home/slee/Pictures/above3.png')[:700,:700]
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = cv2.resize(img, (512*1,1*512))

run_on(img)
