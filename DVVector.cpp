#include "cuda_wrapper.h"
#include "DVVector.h"
#include "built_in.h"

DVVectorLike::DVVectorLike(TRTCContext& ctx, const char* elem_cls, const char* ref_type, size_t size)
{
	m_elem_cls = elem_cls;
	m_ref_type = ref_type;
	m_elem_size = ctx.size_of(elem_cls);
	m_size = size;
}

DVVector::DVVector(TRTCContext& ctx, const char* elem_cls, size_t size, void* hdata)
	: DVVectorLike(ctx, elem_cls, (std::string(elem_cls)+"&").c_str(), size)
{
	CUdeviceptr dptr;
	cuMemAlloc(&dptr, m_elem_size*m_size);
	m_data = (void*)dptr;
	if (hdata)
		cuMemcpyHtoD(dptr, hdata, m_elem_size*m_size);
	else
		cuMemsetD8(dptr, 0, m_elem_size*m_size);
}

DVVector::~DVVector()
{
	cuMemFree((CUdeviceptr)m_data);
}

void DVVector::to_host(void* hdata, size_t begin, size_t end)
{
	if (end == (size_t)(-1) || end > m_size) end = m_size;
	size_t n = end - begin;
	cuMemcpyDtoH(hdata, (CUdeviceptr)((char*)m_data + begin* m_elem_size), m_elem_size*n);
}

std::string DVVector::name_view_cls() const
{
	return std::string("VectorView<") + m_elem_cls + ">";
}

ViewBuf DVVector::view() const
{
	ViewBuf buf(sizeof(VectorView<char>));
	VectorView<char> *pview = (VectorView<char>*)buf.data();
	pview->_data = (char*)m_data;
	pview->_size = m_size;
	return buf;
}

DVVectorAdaptor::DVVectorAdaptor(TRTCContext& ctx, const char* elem_cls, size_t size, void* ddata)
	: DVVectorLike(ctx, elem_cls, (std::string(elem_cls) + "&").c_str(), size), m_data(ddata){}


std::string DVVectorAdaptor::name_view_cls() const
{
	return std::string("VectorView<") + m_elem_cls + ">";
}

ViewBuf DVVectorAdaptor::view() const
{
	ViewBuf buf(sizeof(VectorView<char>));
	VectorView<char> *pview = (VectorView<char>*)buf.data();
	pview->_data = (char*)m_data;
	pview->_size = m_size;
	return buf;
}
