import numpy as np, cv2

src = np.zeros((128,128), dtype=np.float32)
h = 40
src[40:128-40,h:128-h] = 1
src = cv2.warpAffine(src, cv2.getRotationMatrix2D((64,64),22,1.), (128,128))


cv2.imshow('src', src)
#cv2.waitKey(0); cv2.destroyAllWindows()

#src_decimated = src[::4,::4]
src_decimated = src[::1,::1]
src_decimated[src_decimated<.0001] = 0
inds = np.argwhere(src_decimated)
inds[:,[0,1]] = inds[:,[1,0]]
inds_mu = inds.mean(0)
print('inds mu',inds_mu)
inds = inds-inds_mu

# When data is square (w=h), we have some sort of degenerate
# case when we cannot determine rotation. It must have to do with
# eigenvalues being equal and so not being able to 'pick' evectors.
if True:
    cov = inds.T@inds
    print('cov',cov)
    l,v = np.linalg.eig(cov)
    condition = l[0]/l[1]
    print('ortho', v[0]@v[1])
    print('ortho', v[:,0]@v[:,1])
    T = np.eye(3)
    print('condition',condition)
    d = np.diag(1./np.sqrt(l))
    dd = np.diag(np.sqrt(l))
    T[:2,:2] = v.T # ~PCA, but don't change scale
    #T[:2,:2] = dd @ v.T @ d
    #T[:2,:2] = dd @ v @ d @ v.T # ~ZCA (not wanted)
    angle = np.arccos(v.T[0,0])
    print('angle',np.rad2deg(angle))
else:
    u,s,vt = np.linalg.svd(inds, full_matrices=True)
    T = np.eye(3)
    T[:2,:2] = s**1 * vt / len(inds)
    print(T)



center = np.eye(3)
center[:2,2] = -64
uncenter = np.eye(3)
uncenter[:2,2] = 64
T = (uncenter @ T @ center)[:2]
dst = cv2.warpAffine(src, T, (128,128))

cv2.imshow('fixed', dst)
cv2.waitKey(0); cv2.destroyAllWindows()
