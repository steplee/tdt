import os, numpy as np, sys, cv2
import torch, torch.nn as nn, torch.nn.functional as F
import torchvision
import ignite
from ignite.engine import create_supervised_trainer, Events, Engine
from ignite.metrics import Accuracy
from matplotlib.cm import inferno

from .dataset import AerialImageLabelingDataset, fix_batch

decimate = 2

# NOTE: due to a questionable design in torch.utils.data, we MUST set num_workers>1
# because they hard set random seeds, which causes the data to loop over a small subset
# over and over if it is done in the main process, when we switch train->eval->train
train_dset = AerialImageLabelingDataset(decimate=decimate, kind='train')
train_loader = torch.utils.data.DataLoader(train_dset, num_workers=1)
#viz_dset = AerialImageLabelingDataset(decimate=decimate, kind='train')
viz_dset = AerialImageLabelingDataset(decimate=decimate, kind='test')
viz_loader = torch.utils.data.DataLoader(viz_dset, num_workers=1, shuffle=True)

bn = torchvision.ops.misc.FrozenBatchNorm2d
'''
model = torchvision.models.segmentation.fcn_resnet101(True, )
model.classifier[4] = nn.Conv2d(512,1, 1) # We only have one output class.
'''
dilate = None
dilate = [2,2,2]
model = torchvision.models.resnet50(True, norm_layer=bn, replace_stride_with_dilation=dilate)
model = list(model.children())[0:7]
model = nn.Sequential(*model, nn.UpsamplingBilinear2d(scale_factor=2), nn.Conv2d(1024, 1, 1))
model = model.cuda().train()

lr = 1e-4
if False:
    opt = torch.optim.Adam(model.parameters(), lr)
else:
    mods = list(model.children())
    mods = [mods[-3][-1], mods[-1]]
    ps = []
    for m in mods:
        print('optimizing layer', m)
        ps.extend(m.parameters())
    opt = torch.optim.Adam(ps, lr)
#criterion = nn.NLLLoss()
criterion = nn.BCEWithLogitsLoss()

viz_every = 20
viz_batches = 4
checkpoint_every = 500

#evaluator = create_supervised_evaluator(model, metrics={'acc':Accuracy()})
trainer = create_supervised_trainer(model, opt, criterion)

viz_cnt = 0
def viz(engine, batch):
    global model, viz_cnt
    with torch.no_grad():
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

        #cv2.imshow('dimg',dimg); cv2.waitKey(0); cv2.destroyAllWindows()
        cv2.imwrite('out/pred_{:06d}_{:02d}.jpg'.format(viz_cnt,engine.state.iteration), cv2.cvtColor(dimg, cv2.COLOR_RGB2BGR))

        model.train()
vizer = Engine(viz)

@trainer.on(Events.ITERATION_STARTED)
def fix_batch_(e):
    e.state.batch = fix_batch(e.state.batch)

    # Verify inputs look good.
    '''x = e.state.batch[0]
    x = (255*((x-x.min())/(x.max()-x.min()))).permute(0,2,3,1).to(torch.uint8).detach().cpu().numpy()
    x = np.vstack(x)
    cv2.imwrite('out/input_{:06d}.jpg'.format(e.state.iteration), cv2.cvtColor(x, cv2.COLOR_RGB2BGR))'''
@trainer.on(Events.ITERATION_COMPLETED(every=5))
def print_stuff(e):
    print(e.state.epoch, e.state.iteration, e.state.output)

@trainer.on(Events.ITERATION_COMPLETED(every=viz_every))
def run_viz(trainer):
    del trainer.state.batch, trainer.state.output
    global viz_cnt
    vizer.run(viz_loader, max_epochs=1, epoch_length=viz_batches)
    viz_cnt += 1

trainer.run(train_loader)
