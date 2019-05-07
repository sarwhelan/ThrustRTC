#include "tabulate.h"

bool TRTC_tabulate(TRTCContext& ctx, DVVectorLike& vec, const Functor& op, size_t begin, size_t end)
{
	static TRTC_For s_for({ "view_vec", "op" }, "idx",
		"    view_vec[idx] = op(view_vec[idx]);\n"
	);

	if (end == (size_t)(-1)) end = vec.size();
	const DeviceViewable* args[] = { &vec, &op };
	return s_for.launch(ctx, begin, end, args);
}
