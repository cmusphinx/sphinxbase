/* -*- tab-width: 8; c-file-style: "linux" -*- */
/* Copyright (c) 2008 Carnegie Mellon University. All rights *
 * reserved.
 *
 * You may copy, modify, and distribute this code under the same terms
 * as PocketSphinx or Python, at your convenience, as long as this
 * notice is not removed.
 *
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "Python.h"
#include <ngram_model.h>
#include <logmath.h>
#include <ckd_alloc.h>

/**
 * Generic not-type-safe pointer object for all our various types.
 */
typedef struct sb_ptr_s {
	PyObject_HEAD
	void *ptr;
} sb_ptr_t;

static PyTypeObject sb_ptrType = {
	PyObject_HEAD_INIT(NULL)
	0,                         /* ob_size */
	"_sphinxbase.ptr",         /* tp_name */
	sizeof(sb_ptr_t),          /* tp_basicsize */
	0,                         /* tp_itemsize */
	0,                         /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_compare */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"Generic pointer object for SphinxBase" /* tp_doc */	
};

static PyObject *
sb_ptr_new(void *ptr)
{
	sb_ptr_t *sbp;

	sbp = (sb_ptr_t *)sb_ptrType.tp_alloc(&sb_ptrType, 0);
	sbp->ptr = ptr;
	return (PyObject *)sbp;
}

static PyObject *
sb_logmath_init(PyObject *self, PyObject *args)
{
	logmath_t *lmath;
	double base = 1.0001;
	int shift = 0, use_table = 0;

	if (!PyArg_ParseTuple(args, "|dii", &base, &shift, &use_table))
		return NULL;

	if ((lmath = logmath_init(base, shift, use_table)) == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to create logmath_t");
		return NULL;
	}
	return sb_ptr_new(lmath);
}

static PyObject *
sb_logmath_free(PyObject *self, PyObject *args)
{
	sb_ptr_t *lmath;

	if (!PyArg_ParseTuple(args, "O", &lmath))
		return NULL;

	logmath_free(lmath->ptr);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sb_logmath_log_to_ln(PyObject *self, PyObject *args)
{
	sb_ptr_t *lmath;
	int logval;

	if (!PyArg_ParseTuple(args, "Oi", &lmath, &logval))
		return NULL;

	return Py_BuildValue("d", logmath_log_to_ln(lmath->ptr, logval));
}

static PyObject *
sb_ngram_model_read(PyObject *self, PyObject *args)
{
	sb_ptr_t *lmath;
	ngram_model_t *lm;
	char const *file;

	if (!PyArg_ParseTuple(args, "sO", &file, &lmath))
		return NULL;

	if ((lm = ngram_model_read(NULL, file, NGRAM_AUTO, lmath->ptr)) == NULL) {
		PyErr_SetString(PyExc_IOError, "N-Gram model could not be read");
		return NULL;
	}
	return sb_ptr_new(lm);
}

static PyObject *
sb_ngram_model_free(PyObject *self, PyObject *args)
{
	sb_ptr_t *lm;

	if (!PyArg_ParseTuple(args, "O", &lm))
		return NULL;

	ngram_model_free(lm->ptr);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sb_ngram_model_apply_weights(PyObject *self, PyObject *args)
{
	sb_ptr_t *lm;
	float lw, wip, uw;

	if (!PyArg_ParseTuple(args, "Offf", &lm, &lw, &wip, &uw))
		return NULL;

	ngram_model_apply_weights(lm->ptr, lw, wip, uw);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sb_ngram_score_helper(PyObject *self, PyObject *args,
		      int32 (*scorer)(ngram_model_t *, int32, int32 *, int32, int32 *))
{
	sb_ptr_t *lm;
	PyObject *obj;
	char const *str;
	int32 *history;
	int32 wid, n_hist, n_used, i, score;

	if (!PyTuple_Check(args)) {
		PyErr_SetString(PyExc_TypeError, "args is not a tuple");
		return NULL;
	}
	if ((n_hist = PyTuple_Size(args)) < 2) {
		PyErr_SetString(PyExc_ValueError, "usage: ngram_score(lm, word, ...)");
		return NULL;
	}
	lm = (sb_ptr_t *)PyTuple_GET_ITEM(args, 0);
	obj = PyTuple_GET_ITEM(args, 1);
	if ((str = PyString_AsString(obj)) == NULL)
		return NULL;
	wid = ngram_wid(lm->ptr, str);
	n_hist -= 2;
	history = ckd_calloc(n_hist, sizeof(*history));
	for (i = 0; i < n_hist; ++i) {
		obj = PyTuple_GET_ITEM(args, 2 + i);
		if ((str = PyString_AsString(obj)) == NULL) {
			ckd_free(history);
			return NULL;
		}
		history[i] = ngram_wid(lm->ptr, str);
	}
	score = (*scorer)(lm->ptr, wid, history, n_hist, &n_used);
	ckd_free(history);

	return Py_BuildValue("(ii)", score, n_used);
}

static PyObject *
sb_ngram_score(PyObject *self, PyObject *args)
{
	return sb_ngram_score_helper(self, args, &ngram_ng_score);
}

static PyObject *
sb_ngram_prob(PyObject *self, PyObject *args)
{
	return sb_ngram_score_helper(self, args, &ngram_ng_prob);
}

static PyObject *
sb_ngram_zero(PyObject *self, PyObject *args)
{
	sb_ptr_t *lm;

	if (!PyArg_ParseTuple(args, "O", &lm))
		return NULL;

	return Py_BuildValue("i", ngram_zero(lm->ptr));
}

static PyMethodDef sphinxbase_methods[] = {
	{ "logmath_init", sb_logmath_init, METH_VARARGS,
	  "Initialize a log-math computation object.\n" },
	{ "logmath_free", sb_logmath_free, METH_VARARGS,
	  "Finalize a log-math computation object.\n" },
	{ "logmath_log_to_ln", sb_logmath_log_to_ln, METH_VARARGS,
	  "Get the natural logarithm for a number in logmath space.\n" },
	{ "ngram_model_read", sb_ngram_model_read, METH_VARARGS,
	  "Read an N-Gram model file from disk.\n" },
	{ "ngram_model_free", sb_ngram_model_free, METH_VARARGS,
	  "Finalize an N-Gram model.\n" },
	{ "ngram_model_apply_weights", sb_ngram_model_apply_weights, METH_VARARGS,
	  "Set weighting parameters for an N-Gram model.\n" },
	{ "ngram_score", sb_ngram_score, METH_VARARGS,
	  "Calculate an N-Gram score.\n" },	
	{ "ngram_prob", sb_ngram_prob, METH_VARARGS,
	  "Calculate an N-Gram probability.\n" },
	{ "ngram_zero", sb_ngram_zero, METH_VARARGS,
	  "Get the zero probability for an N-Gram model.\n" },
	{ NULL, NULL, 0, NULL }
};


PyMODINIT_FUNC
init_sphinxbase(void)
{
	PyObject *m;

	sb_ptrType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&sb_ptrType) < 0)
		return;
	m = Py_InitModule("_sphinxbase", sphinxbase_methods);
	Py_INCREF(&sb_ptrType);
	PyModule_AddObject(m, "ptr", (PyObject *)&sb_ptrType);
}
