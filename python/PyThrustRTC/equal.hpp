#include <Python.h>
#include "TRTCContext.h"
#include "equal.h"
#include "functor.hpp"

static PyObject* n_equal(PyObject* self, PyObject* args)
{
	TRTCContext* ctx = (TRTCContext*)PyLong_AsVoidPtr(PyTuple_GetItem(args, 0));
	DVVectorLike* vec1 = (DVVectorLike*)PyLong_AsVoidPtr(PyTuple_GetItem(args, 1));
	DVVectorLike* vec2 = (DVVectorLike*)PyLong_AsVoidPtr(PyTuple_GetItem(args, 2));
	PyObject* py_binary_pred = PyTuple_GetItem(args, 3);
	bool have_pred = false;
	Functor binary_pred;
	if (py_binary_pred != Py_None)
	{
		binary_pred = PyFunctor_AsFunctor(py_binary_pred);
		have_pred = true;
	}
	size_t begin1 = (size_t)PyLong_AsLong(PyTuple_GetItem(args, 4));
	size_t end1 = (size_t)PyLong_AsLong(PyTuple_GetItem(args, 5));
	size_t begin2 = (size_t)PyLong_AsLong(PyTuple_GetItem(args, 6));
	bool res;
	if (!have_pred)
		if (TRTC_Equal(*ctx, *vec1, *vec2, res, begin1, end1, begin2))
			return PyBool_FromLong(res ? 1 : 0);
		else
			Py_RETURN_NONE;
	else
		if (TRTC_Equal(*ctx, *vec1, *vec2, binary_pred, res, begin1, end1, begin2))
			return PyBool_FromLong(res ? 1 : 0);
		else
			Py_RETURN_NONE;
}
