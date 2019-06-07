#include "radix_sort.h"
#include "extrema.h"
#include "general_reduce.h"
#include "built_in.h"

#include "copy.h"
#include "general_scan.h"
#include "cuda_wrapper.h"

static bool s_bit_mask32(TRTCContext& ctx, const DVVectorLike& vec, const DVVector& dv_min, size_t begin, size_t end, uint32_t& bit_mask)
{
	DVSizeT dvbegin(begin);

	Functor src(ctx, { {"vec", &vec}, {"begin", &dvbegin }, {"v_min", &dv_min } }, { "idx" },
		"        uint32_t cur_v = d_u32(vec[idx + begin]);\n"
		"        uint32_t min_v = d_u32(v_min[0]);\n"
		"        uint32_t diff = cur_v - min_v;\n"
		"        return diff;\n");

	static Functor op(ctx, { }, { "i1", "i2" },
		"        return i1|i2;\n");

	ViewBuf buf;
	if (!general_reduce(ctx, end - begin, "uint32_t", src, op, buf)) return false;
	bit_mask = *(uint32_t*)buf.data();

	return true;
}

static bool s_bit_mask64(TRTCContext& ctx, const DVVectorLike& vec, const DVVector& dv_min, size_t begin, size_t end, uint64_t& bit_mask)
{
	DVSizeT dvbegin(begin);

	Functor src(ctx, { {"vec", &vec}, {"begin", &dvbegin }, {"v_min", &dv_min } }, { "idx" },
		"        uint64_t cur_v = d_u64(vec[idx + begin]);\n"
		"        uint64_t min_v = d_u64(v_min[0]);\n"
		"        uint64_t diff = cur_v - min_v;\n"
		"        return diff;\n");

	static Functor op(ctx, { }, { "i1", "i2" },
		"        return i1|i2;\n");

	ViewBuf buf;
	if (!general_reduce(ctx, end - begin, "uint64_t", src, op, buf)) return false;
	bit_mask = *(uint64_t*)buf.data();

	return true;
}

static bool s_partition_scan_32(TRTCContext& ctx, size_t n, const DVVector& src, const DVVector& dv_min, int bit, DVVector& inds, uint32_t& count)
{
	static Functor plus(ctx, {}, { "x", "y" },
		"        return Pair<uint32_t, uint32_t>({x.first + y.first , x.second + y.second });\n");

	DVInt32 dvbit(bit);
	Functor src_scan(ctx, { {"src", &src}, {"v_min", &dv_min }, {"bit", &dvbit} }, { "idx" },
		"        uint32_t cur_v = d_u32(src[idx]);\n"
		"        uint32_t min_v = d_u32(v_min[0]);\n"
		"        uint32_t diff = cur_v - min_v;\n"
		"        bool pred = (diff & (((uint32_t)1)<<bit)) == 0;\n"
		"        return pred ? Pair<uint32_t, uint32_t>({(uint32_t)1, (uint32_t)0}):Pair<uint32_t, uint32_t>({(uint32_t)0, (uint32_t)1});\n");

	if (!general_scan(ctx, n, src_scan, inds, plus, 0)) return false;

	Pair<uint32_t, uint32_t> sums;
	cuMemcpyDtoH(&sums, (CUdeviceptr)((Pair<uint32_t, uint32_t>*)inds.data() + n - 1), sizeof(Pair<uint32_t, uint32_t>));
	count = sums.first;
	return true;
}

static bool s_partition_scan_reverse_32(TRTCContext& ctx, size_t n, const DVVector& src, const DVVector& dv_min, int bit, DVVector& inds, uint32_t& count)
{
	static Functor plus(ctx, {}, { "x", "y" },
		"        return Pair<uint32_t, uint32_t>({x.first + y.first , x.second + y.second });\n");

	DVInt32 dvbit(bit);
	Functor src_scan(ctx, { {"src", &src}, {"v_min", &dv_min }, {"bit", &dvbit} }, { "idx" },
		"        uint32_t cur_v = d_u32(src[idx]);\n"
		"        uint32_t min_v = d_u32(v_min[0]);\n"
		"        uint32_t diff = cur_v - min_v;\n"
		"        bool pred = (diff & (((uint32_t)1)<<bit)) != 0;\n"
		"        return pred ? Pair<uint32_t, uint32_t>({(uint32_t)1, (uint32_t)0}):Pair<uint32_t, uint32_t>({(uint32_t)0, (uint32_t)1});\n");

	if (!general_scan(ctx, n, src_scan, inds, plus, 0)) return false;

	Pair<uint32_t, uint32_t> sums;
	cuMemcpyDtoH(&sums, (CUdeviceptr)((Pair<uint32_t, uint32_t>*)inds.data() + n - 1), sizeof(Pair<uint32_t, uint32_t>));
	count = sums.first;
	return true;
}

static bool s_partition_scan_64(TRTCContext& ctx, size_t n, const DVVector& src, const DVVector& dv_min, int bit, DVVector& inds, uint32_t& count)
{
	static Functor plus(ctx, {}, { "x", "y" },
		"        return Pair<uint32_t, uint32_t>({x.first + y.first , x.second + y.second });\n");

	DVInt32 dvbit(bit);
	Functor src_scan(ctx, { {"src", &src}, {"v_min", &dv_min }, {"bit", &dvbit} }, { "idx" },
		"        uint64_t cur_v = d_u64(src[idx]);\n"
		"        uint64_t min_v = d_u64(v_min[0]);\n"
		"        uint64_t diff = cur_v - min_v;\n"
		"        bool pred = (diff & (((uint64_t)1)<<bit)) == 0;\n"
		"        return pred ? Pair<uint32_t, uint32_t>({(uint32_t)1, (uint32_t)0}):Pair<uint32_t, uint32_t>({(uint32_t)0, (uint32_t)1});\n");

	if (!general_scan(ctx, n, src_scan, inds, plus, 0)) return false;

	Pair<uint32_t, uint32_t> sums;
	cuMemcpyDtoH(&sums, (CUdeviceptr)((Pair<uint32_t, uint32_t>*)inds.data() + n - 1), sizeof(Pair<uint32_t, uint32_t>));
	count = sums.first;
	return true;
}

static bool s_partition_scan_reverse_64(TRTCContext& ctx, size_t n, const DVVector& src, const DVVector& dv_min, int bit, DVVector& inds, uint32_t& count)
{
	static Functor plus(ctx, {}, { "x", "y" },
		"        return Pair<uint32_t, uint32_t>({x.first + y.first , x.second + y.second });\n");

	DVInt32 dvbit(bit);
	Functor src_scan(ctx, { {"src", &src}, {"v_min", &dv_min }, {"bit", &dvbit} }, { "idx" },
		"        uint64_t cur_v = d_u64(src[idx]);\n"
		"        uint64_t min_v = d_u64(v_min[0]);\n"
		"        uint64_t diff = cur_v - min_v;\n"
		"        bool pred = (diff & (((uint64_t)1)<<bit)) != 0;\n"
		"        return pred ? Pair<uint32_t, uint32_t>({(uint32_t)1, (uint32_t)0}):Pair<uint32_t, uint32_t>({(uint32_t)0, (uint32_t)1});\n");

	if (!general_scan(ctx, n, src_scan, inds, plus, 0)) return false;

	Pair<uint32_t, uint32_t> sums;
	cuMemcpyDtoH(&sums, (CUdeviceptr)((Pair<uint32_t, uint32_t>*)inds.data() + n - 1), sizeof(Pair<uint32_t, uint32_t>));
	count = sums.first;
	return true;
}

static bool s_partition_scatter(TRTCContext& ctx, size_t n, const DVVector& src, const DVVector& inds, DVVectorLike& dst, uint32_t count)
{
	static TRTC_For s_for_scatter({ "src", "inds", "dst", "count" }, "idx",
		"    if ((idx==0 && inds[idx].first>0) || (idx>0 && inds[idx].first>inds[idx-1].first))\n"
		"        dst[inds[idx].first -1] = src[idx];\n"
		"    else\n"
		"        dst[count + inds[idx].second - 1] = src[idx];\n"
	);

	DVUInt32 dvcount(count);
	const DeviceViewable* args[] = { &src, &inds, &dst, &dvcount };
	return s_for_scatter.launch_n(ctx, n, args);
}

static bool s_partition_scatter_by_keys(TRTCContext& ctx, size_t n, const DVVector& src_keys, const DVVector& src_values, const DVVector& inds, DVVectorLike& dst_keys, DVVectorLike& dst_values, uint32_t count)
{
	static TRTC_For s_for_scatter({ "src_keys", "src_values", "inds", "dst_keys", "dst_values", "count" }, "idx",
		"    if ((idx==0 && inds[idx].first>0) || (idx>0 && inds[idx].first>inds[idx-1].first))\n"
		"    {\n"
		"        dst_keys[inds[idx].first -1] = src_keys[idx];\n"
		"        dst_values[inds[idx].first -1] = src_values[idx];\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"        dst_keys[count + inds[idx].second - 1] = src_keys[idx];\n"
		"        dst_values[count + inds[idx].second - 1] = src_values[idx];\n"
		"    }\n"
	);

	DVUInt32 dvcount(count);
	const DeviceViewable* args[] = { &src_keys, &src_values, &inds, &dst_keys, &dst_values, &dvcount };
	return s_for_scatter.launch_n(ctx, n, args);
}

bool radix_sort_32(TRTCContext& ctx, DVVectorLike& vec, size_t begin, size_t end)
{
	if (end == (size_t)(-1)) end = vec.size();

	size_t id_min;
	if (!TRTC_Min_Element(ctx, vec, id_min, begin, end)) return false;

	DVVector dv_min(ctx, vec.name_elem_cls().c_str(), 1);
	TRTC_Copy(ctx, vec, dv_min, id_min, id_min + 1);

	uint32_t bit_mask;
	if (!s_bit_mask32(ctx, vec, dv_min, begin, end, bit_mask)) return false;
	if (bit_mask == 0) return true; // already sorted

	size_t n = end - begin;
	DVVector tmp1(ctx, vec.name_elem_cls().c_str(), n);
	DVVector tmp2(ctx, vec.name_elem_cls().c_str(), n);
	if (!TRTC_Copy(ctx, vec, tmp1, begin, end)) return false;
	DVVector* p1 = &tmp1;
	DVVector* p2 = &tmp2;
	DVVector inds(ctx, "Pair<uint32_t, uint32_t>", n);
	for (int bit = 0; bit <32; bit++)
	{
		if ( (bit_mask & (((uint32_t)1) << bit)) == 0) continue;
		
		uint32_t count;
		if (!s_partition_scan_32(ctx, n, *p1, dv_min, bit, inds, count)) return false;
		if (!s_partition_scatter(ctx, n, *p1, inds, *p2, count)) return false;

		{
			DVVector* tmp = p1;
			p1 = p2;
			p2 = tmp;
		}
	}
	if (!TRTC_Copy(ctx, *p1, vec, 0, n, begin)) return false;
	return true;
}

bool radix_sort_reverse_32(TRTCContext& ctx, DVVectorLike& vec, size_t begin, size_t end)
{
	if (end == (size_t)(-1)) end = vec.size();

	size_t id_min;
	if (!TRTC_Min_Element(ctx, vec, id_min, begin, end)) return false;

	DVVector dv_min(ctx, vec.name_elem_cls().c_str(), 1);
	TRTC_Copy(ctx, vec, dv_min, id_min, id_min + 1);

	uint32_t bit_mask;
	if (!s_bit_mask32(ctx, vec, dv_min, begin, end, bit_mask)) return false;
	if (bit_mask == 0) return true; // already sorted

	size_t n = end - begin;
	DVVector tmp1(ctx, vec.name_elem_cls().c_str(), n);
	DVVector tmp2(ctx, vec.name_elem_cls().c_str(), n);
	if (!TRTC_Copy(ctx, vec, tmp1, begin, end)) return false;
	DVVector* p1 = &tmp1;
	DVVector* p2 = &tmp2;
	DVVector inds(ctx, "Pair<uint32_t, uint32_t>", n);
	for (int bit = 0; bit < 32; bit++)
	{
		if ((bit_mask & (((uint32_t)1) << bit)) == 0) continue;

		uint32_t count;
		if (!s_partition_scan_reverse_32(ctx, n, *p1, dv_min, bit, inds, count)) return false;
		if (!s_partition_scatter(ctx, n, *p1, inds, *p2, count)) return false;

		{
			DVVector* tmp = p1;
			p1 = p2;
			p2 = tmp;
		}
	}
	if (!TRTC_Copy(ctx, *p1, vec, 0, n, begin)) return false;
	return true;
}

bool radix_sort_64(TRTCContext& ctx, DVVectorLike& vec, size_t begin, size_t end)
{
	if (end == (size_t)(-1)) end = vec.size();

	size_t id_min;
	if (!TRTC_Min_Element(ctx, vec, id_min, begin, end)) return false;

	DVVector dv_min(ctx, vec.name_elem_cls().c_str(), 1);
	TRTC_Copy(ctx, vec, dv_min, id_min, id_min + 1);

	uint64_t bit_mask;
	if (!s_bit_mask64(ctx, vec, dv_min, begin, end, bit_mask)) return false;
	if (bit_mask == 0) return true; // already sorted

	size_t n = end - begin;
	DVVector tmp1(ctx, vec.name_elem_cls().c_str(), n);
	DVVector tmp2(ctx, vec.name_elem_cls().c_str(), n);
	if (!TRTC_Copy(ctx, vec, tmp1, begin, end)) return false;
	DVVector* p1 = &tmp1;
	DVVector* p2 = &tmp2;
	DVVector inds(ctx, "Pair<uint32_t, uint32_t>", n);
	
	for (int bit = 0; bit < 64; bit++)
	{
		if ((bit_mask & (((uint64_t)1) << bit)) == 0) continue;

		uint32_t count;
		if (!s_partition_scan_64(ctx, n, *p1, dv_min, bit, inds, count)) return false;
		if (!s_partition_scatter(ctx, n, *p1, inds, *p2, count)) return false;

		{
			DVVector* tmp = p1;
			p1 = p2;
			p2 = tmp;
		}
	}
	if (!TRTC_Copy(ctx, *p1, vec, 0, n, begin)) return false;
	return true;
}

bool radix_sort_reverse_64(TRTCContext& ctx, DVVectorLike& vec, size_t begin, size_t end)
{
	if (end == (size_t)(-1)) end = vec.size();

	size_t id_min;
	if (!TRTC_Min_Element(ctx, vec, id_min, begin, end)) return false;

	DVVector dv_min(ctx, vec.name_elem_cls().c_str(), 1);
	TRTC_Copy(ctx, vec, dv_min, id_min, id_min + 1);

	uint64_t bit_mask;
	if (!s_bit_mask64(ctx, vec, dv_min, begin, end, bit_mask)) return false;
	if (bit_mask == 0) return true; // already sorted

	size_t n = end - begin;
	DVVector tmp1(ctx, vec.name_elem_cls().c_str(), n);
	DVVector tmp2(ctx, vec.name_elem_cls().c_str(), n);
	if (!TRTC_Copy(ctx, vec, tmp1, begin, end)) return false;
	DVVector* p1 = &tmp1;
	DVVector* p2 = &tmp2;
	DVVector inds(ctx, "Pair<uint32_t, uint32_t>", n);

	for (int bit = 0; bit < 64; bit++)
	{
		if ((bit_mask & (((uint64_t)1) << bit)) == 0) continue;

		uint32_t count;
		if (!s_partition_scan_reverse_64(ctx, n, *p1, dv_min, bit, inds, count)) return false;
		if (!s_partition_scatter(ctx, n, *p1, inds, *p2, count)) return false;

		{
			DVVector* tmp = p1;
			p1 = p2;
			p2 = tmp;
		}
	}
	if (!TRTC_Copy(ctx, *p1, vec, 0, n, begin)) return false;
	return true;
}

bool radix_sort_by_key_32(TRTCContext& ctx, DVVectorLike& keys, DVVectorLike& values, size_t begin_keys, size_t end_keys, size_t begin_values)
{
	if (end_keys == (size_t)(-1)) end_keys = keys.size();

	size_t id_min;
	if (!TRTC_Min_Element(ctx, keys, id_min, begin_keys, end_keys)) return false;

	DVVector dv_min(ctx, keys.name_elem_cls().c_str(), 1);
	TRTC_Copy(ctx, keys, dv_min, id_min, id_min + 1);

	uint32_t bit_mask;
	if (!s_bit_mask32(ctx, keys, dv_min, begin_keys, end_keys, bit_mask)) return false;
	if (bit_mask == 0) return true; // already sorted

	size_t n = end_keys - begin_keys;
	DVVector tmp_keys1(ctx, keys.name_elem_cls().c_str(), n);
	DVVector tmp_keys2(ctx, keys.name_elem_cls().c_str(), n);
	DVVector tmp_values1(ctx, values.name_elem_cls().c_str(), n);
	DVVector tmp_values2(ctx, values.name_elem_cls().c_str(), n);
	if (!TRTC_Copy(ctx, keys, tmp_keys1, begin_keys, end_keys)) return false;
	if (!TRTC_Copy(ctx, values, tmp_values1, begin_values, begin_values + n)) return false;
	DVVector* p_keys1 = &tmp_keys1;
	DVVector* p_keys2 = &tmp_keys2;
	DVVector* p_values1 = &tmp_values1;
	DVVector* p_values2 = &tmp_values2;
	DVVector inds(ctx, "Pair<uint32_t, uint32_t>", n);

	for (int bit = 0; bit < 32; bit++)
	{
		if ((bit_mask & (((uint32_t)1) << bit)) == 0) continue;

		uint32_t count;
		if (!s_partition_scan_32(ctx, n, *p_keys1, dv_min, bit, inds, count)) return false;
		if (!s_partition_scatter_by_keys(ctx, n, *p_keys1, *p_values1, inds, *p_keys2, *p_values2, count)) return false;

		{
			DVVector* tmp = p_keys1;
			p_keys1 = p_keys2;
			p_keys2 = tmp;

			tmp = p_values1;
			p_values1 = p_values2;
			p_values2 = tmp;
		}
	}
	if (!TRTC_Copy(ctx, *p_keys1, keys, 0, n, begin_keys)) return false;
	if (!TRTC_Copy(ctx, *p_values1, values, 0, n, begin_values)) return false;

	return true;
}

bool radix_sort_by_key_reverse_32(TRTCContext& ctx, DVVectorLike& keys, DVVectorLike& values, size_t begin_keys, size_t end_keys, size_t begin_values)
{
	if (end_keys == (size_t)(-1)) end_keys = keys.size();

	size_t id_min;
	if (!TRTC_Min_Element(ctx, keys, id_min, begin_keys, end_keys)) return false;

	DVVector dv_min(ctx, keys.name_elem_cls().c_str(), 1);
	TRTC_Copy(ctx, keys, dv_min, id_min, id_min + 1);

	uint32_t bit_mask;
	if (!s_bit_mask32(ctx, keys, dv_min, begin_keys, end_keys, bit_mask)) return false;
	if (bit_mask == 0) return true; // already sorted

	size_t n = end_keys - begin_keys;
	DVVector tmp_keys1(ctx, keys.name_elem_cls().c_str(), n);
	DVVector tmp_keys2(ctx, keys.name_elem_cls().c_str(), n);
	DVVector tmp_values1(ctx, values.name_elem_cls().c_str(), n);
	DVVector tmp_values2(ctx, values.name_elem_cls().c_str(), n);
	if (!TRTC_Copy(ctx, keys, tmp_keys1, begin_keys, end_keys)) return false;
	if (!TRTC_Copy(ctx, values, tmp_values1, begin_values, begin_values + n)) return false;
	DVVector* p_keys1 = &tmp_keys1;
	DVVector* p_keys2 = &tmp_keys2;
	DVVector* p_values1 = &tmp_values1;
	DVVector* p_values2 = &tmp_values2;
	DVVector inds(ctx, "Pair<uint32_t, uint32_t>", n);

	for (int bit = 0; bit < 32; bit++)
	{
		if ((bit_mask & (((uint32_t)1) << bit)) == 0) continue;

		uint32_t count;
		if (!s_partition_scan_reverse_32(ctx, n, *p_keys1, dv_min, bit, inds, count)) return false;
		if (!s_partition_scatter_by_keys(ctx, n, *p_keys1, *p_values1, inds, *p_keys2, *p_values2, count)) return false;

		{
			DVVector* tmp = p_keys1;
			p_keys1 = p_keys2;
			p_keys2 = tmp;

			tmp = p_values1;
			p_values1 = p_values2;
			p_values2 = tmp;
		}
	}
	if (!TRTC_Copy(ctx, *p_keys1, keys, 0, n, begin_keys)) return false;
	if (!TRTC_Copy(ctx, *p_values1, values, 0, n, begin_values)) return false;

	return true;
}

bool radix_sort_by_key_64(TRTCContext& ctx, DVVectorLike& keys, DVVectorLike& values, size_t begin_keys, size_t end_keys, size_t begin_values)
{
	if (end_keys == (size_t)(-1)) end_keys = keys.size();

	size_t id_min;
	if (!TRTC_Min_Element(ctx, keys, id_min, begin_keys, end_keys)) return false;

	DVVector dv_min(ctx, keys.name_elem_cls().c_str(), 1);
	TRTC_Copy(ctx, keys, dv_min, id_min, id_min + 1);

	uint64_t bit_mask;
	if (!s_bit_mask64(ctx, keys, dv_min, begin_keys, end_keys, bit_mask)) return false;
	if (bit_mask == 0) return true; // already sorted

	size_t n = end_keys - begin_keys;
	DVVector tmp_keys1(ctx, keys.name_elem_cls().c_str(), n);
	DVVector tmp_keys2(ctx, keys.name_elem_cls().c_str(), n);
	DVVector tmp_values1(ctx, values.name_elem_cls().c_str(), n);
	DVVector tmp_values2(ctx, values.name_elem_cls().c_str(), n);
	if (!TRTC_Copy(ctx, keys, tmp_keys1, begin_keys, end_keys)) return false;
	if (!TRTC_Copy(ctx, values, tmp_values1, begin_values, begin_values + n)) return false;
	DVVector* p_keys1 = &tmp_keys1;
	DVVector* p_keys2 = &tmp_keys2;
	DVVector* p_values1 = &tmp_values1;
	DVVector* p_values2 = &tmp_values2;
	DVVector inds(ctx, "Pair<uint32_t, uint32_t>", n);

	for (int bit = 0; bit < 64; bit++)
	{
		if ((bit_mask & (((uint64_t)1) << bit)) == 0) continue;
	
		uint32_t count;
		if (!s_partition_scan_64(ctx, n, *p_keys1, dv_min, bit, inds, count)) return false;
		if (!s_partition_scatter_by_keys(ctx, n, *p_keys1, *p_values1, inds, *p_keys2, *p_values2, count)) return false;

		{
			DVVector* tmp = p_keys1;
			p_keys1 = p_keys2;
			p_keys2 = tmp;

			tmp = p_values1;
			p_values1 = p_values2;
			p_values2 = tmp;
		}
	}
	if (!TRTC_Copy(ctx, *p_keys1, keys, 0, n, begin_keys)) return false;
	if (!TRTC_Copy(ctx, *p_values1, values, 0, n, begin_values)) return false;

	return true;
}

bool radix_sort_by_key_reverse_64(TRTCContext& ctx, DVVectorLike& keys, DVVectorLike& values, size_t begin_keys, size_t end_keys, size_t begin_values)
{
	if (end_keys == (size_t)(-1)) end_keys = keys.size();

	size_t id_min;
	if (!TRTC_Min_Element(ctx, keys, id_min, begin_keys, end_keys)) return false;

	DVVector dv_min(ctx, keys.name_elem_cls().c_str(), 1);
	TRTC_Copy(ctx, keys, dv_min, id_min, id_min + 1);

	uint64_t bit_mask;
	if (!s_bit_mask64(ctx, keys, dv_min, begin_keys, end_keys, bit_mask)) return false;
	if (bit_mask == 0) return true; // already sorted

	size_t n = end_keys - begin_keys;
	DVVector tmp_keys1(ctx, keys.name_elem_cls().c_str(), n);
	DVVector tmp_keys2(ctx, keys.name_elem_cls().c_str(), n);
	DVVector tmp_values1(ctx, values.name_elem_cls().c_str(), n);
	DVVector tmp_values2(ctx, values.name_elem_cls().c_str(), n);
	if (!TRTC_Copy(ctx, keys, tmp_keys1, begin_keys, end_keys)) return false;
	if (!TRTC_Copy(ctx, values, tmp_values1, begin_values, begin_values + n)) return false;
	DVVector* p_keys1 = &tmp_keys1;
	DVVector* p_keys2 = &tmp_keys2;
	DVVector* p_values1 = &tmp_values1;
	DVVector* p_values2 = &tmp_values2;
	DVVector inds(ctx, "Pair<uint32_t, uint32_t>", n);

	for (int bit = 0; bit < 64; bit++)
	{
		if ((bit_mask & (((uint64_t)1) << bit)) == 0) continue;

		uint32_t count;
		if (!s_partition_scan_reverse_64(ctx, n, *p_keys1, dv_min, bit, inds, count)) return false;
		if (!s_partition_scatter_by_keys(ctx, n, *p_keys1, *p_values1, inds, *p_keys2, *p_values2, count)) return false;

		{
			DVVector* tmp = p_keys1;
			p_keys1 = p_keys2;
			p_keys2 = tmp;

			tmp = p_values1;
			p_values1 = p_values2;
			p_values2 = tmp;
		}
	}
	if (!TRTC_Copy(ctx, *p_keys1, keys, 0, n, begin_keys)) return false;
	if (!TRTC_Copy(ctx, *p_values1, values, 0, n, begin_values)) return false;

	return true;
}


