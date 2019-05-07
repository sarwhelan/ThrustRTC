#include "replace.h"

bool TRTC_Replace(TRTCContext& ctx, DVVectorLike& vec, const DeviceViewable& old_value, const DeviceViewable& new_value, size_t begin, size_t end)
{
	static TRTC_For s_for( {"view_vec", "old_value", "new_value" }, "idx",
		"    if (view_vec[idx]==(decltype(view_vec)::value_t)old_value) view_vec[idx] = (decltype(view_vec)::value_t)new_value;\n"
	);

	if (end == (size_t)(-1)) end = vec.size();
	const DeviceViewable* args[] = { &vec, &old_value, &new_value };
	return s_for.launch(ctx, begin, end, args);
}

bool TRTC_Replace_If(TRTCContext& ctx, DVVectorLike& vec, const Functor& pred, const DeviceViewable& new_value, size_t begin, size_t end)
{
	static TRTC_For s_for({ "view_vec", "pred", "new_value"}, "idx",
		"    if (pred(view_vec[idx])) view_vec[idx] = (decltype(view_vec)::value_t)new_value;\n"
	);

	if (end == (size_t)(-1)) end = vec.size();
	const DeviceViewable* args[] = { &vec, &pred, &new_value };
	return s_for.launch(ctx, begin, end, args);
}

bool TRTC_Replace_Copy(TRTCContext& ctx, const DVVectorLike& vec_in, DVVectorLike& vec_out, const DeviceViewable& old_value, const DeviceViewable& new_value, size_t begin_in, size_t end_in, size_t begin_out)
{
	static TRTC_For s_for(
	{ "view_vec_in", "view_vec_out" , "old_value", "new_value", "delta" }, "idx",
	"    auto value = view_vec_in[idx];\n"
	"    view_vec_out[idx+delta] = value == (decltype(view_vec_in)::value_t)old_value ?  (decltype(view_vec_out)::value_t)new_value :  (decltype(view_vec_out)::value_t)value;\n"
	);

	if (end_in == (size_t)(-1)) end_in = vec_in.size();
	DVInt32 dvdelta((int)begin_out - (int)begin_in);
	const DeviceViewable* args[] = { &vec_in, &vec_out, &old_value, &new_value, &dvdelta };
	return s_for.launch(ctx, begin_in, end_in, args);
}

bool TRTC_Replace_Copy_If(TRTCContext& ctx, const DVVectorLike& vec_in, DVVectorLike& vec_out, const Functor& pred, const DeviceViewable& new_value, size_t begin_in, size_t end_in, size_t begin_out)
{
	static TRTC_For s_for(
		{ "view_vec_in", "view_vec_out" , "pred", "new_value", "delta" }, "idx",
		"    auto value = view_vec_in[idx];\n"
		"    view_vec_out[idx+delta] = pred(value) ?  (decltype(view_vec_out)::value_t)new_value :  (decltype(view_vec_out)::value_t)value;\n"
	);
	
	if (end_in == (size_t)(-1)) end_in = vec_in.size();
	DVInt32 dvdelta((int)begin_out - (int)begin_in);
	const DeviceViewable* args[] = { &vec_in, &vec_out, &pred, &new_value, &dvdelta };
	return s_for.launch(ctx, begin_in, end_in, args);
}
