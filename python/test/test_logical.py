import ThrustRTC as trtc

trtc.set_ptx_cache('__ptx_cache__')
ctx = trtc.Context()

identity = trtc.Functor( ctx, {}, ['x'], 
'''
         return x;
''')

darr = trtc.device_vector_from_list(ctx, [True, True, False], 'bool')

print(trtc.All_Of(ctx, darr, identity, 0, 2))
print(trtc.All_Of(ctx, darr, identity, 0, 3))
print(trtc.All_Of(ctx, darr, identity, 0, 0))

print(trtc.Any_Of(ctx, darr, identity, 0, 2))
print(trtc.Any_Of(ctx, darr, identity, 0, 3))
print(trtc.Any_Of(ctx, darr, identity, 2, 3))
print(trtc.Any_Of(ctx, darr, identity, 0, 0))

print(trtc.None_Of(ctx, darr, identity, 0, 2))
print(trtc.None_Of(ctx, darr, identity, 0, 3))
print(trtc.None_Of(ctx, darr, identity, 2, 3))
print(trtc.None_Of(ctx, darr, identity, 0, 0))
