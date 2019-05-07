import ThrustRTC as trtc

trtc.set_ptx_cache('__ptx_cache__')
ctx = trtc.Context()

darr1 = trtc.device_vector_from_list(ctx, [ 1.0, 2.0, 5.0], 'float')
darr2 = trtc.device_vector_from_list(ctx, [ 4.0, 1.0, 5.0], 'float')
print (trtc.Inner_Product(ctx, darr1, darr2, trtc.DVFloat(0.0)))

plus = trtc.Functor( ctx, {}, ['x', 'y'], 
'''
         return x + y;
''')

multiplies = trtc.Functor( ctx, {}, ['x', 'y'],
'''
         return x * y;
''')

print (trtc.Inner_Product(ctx, darr1, darr2, trtc.DVFloat(0.0), plus, multiplies))
