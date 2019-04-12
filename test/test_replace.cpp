#include <stdio.h>
#include "TRTCContext.h"
#include "DVVector.h"
#include "replace.h"

int main()
{
	TRTCContext::set_ptx_cache("__ptx_cache__");
	TRTCContext ctx;
	
	// replace
	int hvec[5] = { 1,2,3,1,2 };
	DVVector vec(ctx, "int32_t", 5, hvec);
	TRTC_Replace(ctx, vec, DVInt32(1), DVInt32(99));
	vec.to_host(hvec);
	printf("%d %d %d %d %d\n", hvec[0], hvec[1], hvec[2], hvec[3], hvec[4]);

	// replace_if
	int hvec2[5] = { 1, -2, 3, -4, 5 };
	DVVector vec2(ctx, "int32_t", 5, hvec2);
	TRTC_Replace_If(ctx, vec2, { {}, { "x" }, "ret",
		"        ret = x<0;\n" }, DVInt32(0));
	vec2.to_host(hvec2);
	printf("%d %d %d %d %d\n", hvec2[0], hvec2[1], hvec2[2], hvec2[3], hvec2[4]);

	// replace_copy
	int hvec3[5] = { 1, 2, 3, 1, 2 };
	DVVector vec3_in(ctx, "int32_t", 5, hvec3);
	DVVector vec3_out(ctx, "int32_t", 5);
	TRTC_Replace_Copy(ctx, vec3_in, vec3_out, DVInt32(1), DVInt32(99));
	vec3_out.to_host(hvec3);
	printf("%d %d %d %d %d\n", hvec3[0], hvec3[1], hvec3[2], hvec3[3], hvec3[4]);

	// replace_copy_if
	int hvec4[5] = { 1, -2, 3, -4, 5 };
	DVVector vec4_in(ctx, "int32_t", 5, hvec4);
	DVVector vec4_out(ctx, "int32_t", 5);
	TRTC_Replace_Copy_If(ctx, vec4_in, vec4_out, { {}, { "x" }, "ret",
		"        ret = x<0;\n" }, DVInt32(0));
	vec4_out.to_host(hvec4);
	printf("%d %d %d %d %d\n", hvec4[0], hvec4[1], hvec4[2], hvec4[3], hvec4[4]);

	return 0;
}