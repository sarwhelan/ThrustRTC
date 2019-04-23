#include <stdio.h>
#include "TRTCContext.h"
#include "DVVector.h"
#include "reduce.h"

int main()
{
	TRTCContext::set_ptx_cache("__ptx_cache__");
	TRTCContext ctx;

	int harr[6] = { 1, 0, 2, 2, 1, 3 };
	DVVector darr(ctx, "int32_t", 6, harr);

	ViewBuf ret;
	TRTC_Reduce(ctx, darr, ret);

	printf("%d\n", *(int*)ret.data());

	return 0;
}