/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2014, Ben Charrow
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of Willow Garage, Inc. nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

#include "Python.h"

#include "roslz4/lz4s.h"

struct module_state {
  PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

/* Taken from Python's _bz2module.c */
static int
grow_buffer(PyObject **buf)
{
    /* Expand the buffer by an amount proportional to the current size,
       giving us amortized linear-time behavior. Use a less-than-double
       growth factor to avoid excessive allocation. */
    size_t size = PyBytes_GET_SIZE(*buf);
    size_t new_size = size + (size >> 3) + 6;
    if (new_size > size) {
        return _PyBytes_Resize(buf, new_size);
    } else {  /* overflow */
        PyErr_SetString(PyExc_OverflowError,
                        "Unable to allocate buffer - output too large");
        return -1;
    }
}

/*============================== LZ4Compressor ==============================*/

typedef struct {
    PyObject_HEAD
    roslz4_stream stream;
} LZ4Compressor;

static void
LZ4Compressor_dealloc(LZ4Compressor *self)
{
  roslz4_compressEnd(&self->stream);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

static int
LZ4Compressor_init(LZ4Compressor *self, PyObject *args, PyObject *kwds)
{
  (void)kwds;
  if (!PyArg_ParseTuple(args, ":__init__")) {
    return -1;
  }

  int ret = roslz4_compressStart(&self->stream, 6);
  if (ret != ROSLZ4_OK) {
    PyErr_SetString(PyExc_RuntimeError, "error initializing roslz4 stream");
    return -1;
  }
  return 0;
}

static PyObject *
compress_impl(LZ4Compressor *self, Py_buffer *input, PyObject *output)
{
  /* Allocate output string */
  int initial_size = roslz4_blockSizeFromIndex(self->stream.block_size_id) + 64;
  output = PyBytes_FromStringAndSize(NULL, initial_size);
  if (!output) {
    if (input != NULL) { PyBuffer_Release(input); }
    return NULL;
  }

  /* Setup stream */
  int action;
  if (input != NULL) {
    action = ROSLZ4_RUN;
    self->stream.input_next = input->buf;
    self->stream.input_left = input->len;
  } else {
    action = ROSLZ4_FINISH;
    self->stream.input_next = NULL;
    self->stream.input_left = 0;
  }
  self->stream.output_next = PyBytes_AS_STRING(output);
  self->stream.output_left = PyBytes_GET_SIZE(output);

  /* Compress data */
  int status;
  int output_written = 0;
  while ((action == ROSLZ4_FINISH) ||
         (action == ROSLZ4_RUN && self->stream.input_left > 0)) {
    int out_start = self->stream.total_out;
    status = roslz4_compress(&self->stream, action);
    output_written += self->stream.total_out - out_start;
    if (status == ROSLZ4_OK) {
      continue;
    } else if (status == ROSLZ4_STREAM_END) {
      break;
    } else if (status == ROSLZ4_OUTPUT_SMALL) {
      if (grow_buffer(&output) < 0) {
        goto error;
      }
      self->stream.output_next = PyBytes_AS_STRING(output) + output_written;
      self->stream.output_left = PyBytes_GET_SIZE(output) - output_written;
    } else if (status == ROSLZ4_PARAM_ERROR) {
      PyErr_SetString(PyExc_IOError, "bad block size parameter");
      goto error;
    } else if (status == ROSLZ4_ERROR) {
      PyErr_SetString(PyExc_IOError, "error compressing");
      goto error;
    } else {
      PyErr_Format(PyExc_RuntimeError, "unhandled return code %i", status);
      goto error;
    }
  }

  /* Shrink return buffer */
  if (output_written != PyBytes_GET_SIZE(output)) {
    _PyBytes_Resize(&output, output_written);
  }

  if (input != NULL) { PyBuffer_Release(input); }
  return output;

error:
  if (input != NULL) { PyBuffer_Release(input); }
  Py_XDECREF(output);
  return NULL;
}

static PyObject *
LZ4Compressor_compress(LZ4Compressor *self, PyObject *args)
{
  Py_buffer input;
  PyObject *output = NULL;

  /* TODO: Keyword argument */
  if (!PyArg_ParseTuple(args, "s*:compress", &input)) {
    return NULL;
  }

  return compress_impl(self, &input, output);
}

static PyObject *
LZ4Compressor_flush(LZ4Compressor *self, PyObject *args)
{
  PyObject *output = NULL;

  if (!PyArg_ParseTuple(args, ":flush")) {
    return NULL;
  }

  return compress_impl(self, NULL, output);
}


static PyMethodDef LZ4Compressor_methods[] = {
  {"compress", (PyCFunction)LZ4Compressor_compress, METH_VARARGS, "method doc"},
  {"flush", (PyCFunction)LZ4Compressor_flush, METH_VARARGS, "method doc"},
  {NULL} /* Sentinel */
};

static PyTypeObject LZ4Compressor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_roslz4.LZ4Compressor",           /* tp_name */
    sizeof(LZ4Compressor),             /* tp_basicsize */
    0,                                 /* tp_itemsize */
    (destructor)LZ4Compressor_dealloc, /* tp_dealloc */
    0,                                 /* tp_print */
    0,                                 /* tp_getattr */
    0,                                 /* tp_setattr */
    0,                                 /* tp_compare */
    0,                                 /* tp_repr */
    0,                                 /* tp_as_number */
    0,                                 /* tp_as_sequence */
    0,                                 /* tp_as_mapping */
    0,                                 /* tp_hash */
    0,                                 /* tp_call */
    0,                                 /* tp_str */
    0,                                 /* tp_getattro */
    0,                                 /* tp_setattro */
    0,                                 /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                /* tp_flags */
    "LZ4Compressor objects",           /* tp_doc */
    0,                                 /* tp_traverse */
    0,                                 /* tp_clear */
    0,                                 /* tp_richcompare */
    0,                                 /* tp_weaklistoffset */
    0,                                 /* tp_iter */
    0,                                 /* tp_iternext */
    LZ4Compressor_methods,             /* tp_methods */
    0,                                 /* tp_members */
    0,                                 /* tp_getset */
    0,                                 /* tp_base */
    0,                                 /* tp_dict */
    0,                                 /* tp_descr_get */
    0,                                 /* tp_descr_set */
    0,                                 /* tp_dictoffset */
    (initproc)LZ4Compressor_init       /* tp_init */
};

/*============================= LZ4Decompressor =============================*/

typedef struct {
    PyObject_HEAD
    roslz4_stream stream;
} LZ4Decompressor;

static void
LZ4Decompressor_dealloc(LZ4Decompressor *self)
{
  roslz4_decompressEnd(&self->stream);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

static int
LZ4Decompressor_init(LZ4Decompressor *self, PyObject *args, PyObject *kwds)
{
  (void)kwds;
  if (!PyArg_ParseTuple(args, ":__init__")) {
    return -1;
  }

  int ret = roslz4_decompressStart(&self->stream);
  if (ret != ROSLZ4_OK) {
    PyErr_SetString(PyExc_RuntimeError, "error initializing roslz4 stream");
    return -1;
  }
  return 0;
}

static PyObject *
LZ4Decompressor_decompress(LZ4Decompressor *self, PyObject *args)
{
  Py_buffer input;
  PyObject *output = NULL;

  /* TODO: Keyword argument */
  if (!PyArg_ParseTuple(args, "s*:decompress", &input)) {
    return NULL;
  }

  /* Allocate 1 output block. If header not read, use compression block size */
  int block_size;
  if (self->stream.block_size_id == -1 ) {
    block_size = roslz4_blockSizeFromIndex(6);
  } else {
    block_size = roslz4_blockSizeFromIndex(self->stream.block_size_id);
  }

  output = PyBytes_FromStringAndSize(NULL, block_size);
  if (!output) {
    PyBuffer_Release(&input);
    return NULL;
  }

  /* Setup stream */
  self->stream.input_next = input.buf;
  self->stream.input_left = input.len;
  self->stream.output_next = PyBytes_AS_STRING(output);
  self->stream.output_left = PyBytes_GET_SIZE(output);

  int output_written = 0;
  while (self->stream.input_left > 0) {
    int out_start = self->stream.total_out;
    int status = roslz4_decompress(&self->stream);
    output_written += self->stream.total_out - out_start;
    if (status == ROSLZ4_OK) {
      continue;
    } else if (status == ROSLZ4_STREAM_END) {
      break;
    } else if (status == ROSLZ4_OUTPUT_SMALL) {
      if (grow_buffer(&output) < 0) {
        goto error;
      }
      self->stream.output_next = PyBytes_AS_STRING(output) + output_written;
      self->stream.output_left = PyBytes_GET_SIZE(output) - output_written;
    } else if (status == ROSLZ4_ERROR) {
      PyErr_SetString(PyExc_IOError, "error decompressing");
      goto error;
    } else if (status == ROSLZ4_DATA_ERROR) {
      PyErr_SetString(PyExc_IOError, "malformed data to decompress");
      goto error;
    } else {
      PyErr_Format(PyExc_RuntimeError, "unhandled return code %i", status);
      goto error;
    }
  }

  if (output_written != PyBytes_GET_SIZE(output)) {
    _PyBytes_Resize(&output, output_written);
  }

  PyBuffer_Release(&input);
  return output;

error:
  PyBuffer_Release(&input);
  Py_XDECREF(output);
  return NULL;
}

static PyMethodDef LZ4Decompressor_methods[] = {
  {"decompress", (PyCFunction)LZ4Decompressor_decompress, METH_VARARGS, "method doc"},
  {NULL} /* Sentinel */
};

static PyTypeObject LZ4Decompressor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_roslz4.LZ4Decompressor",           /* tp_name */
    sizeof(LZ4Decompressor),             /* tp_basicsize */
    0,                                   /* tp_itemsize */
    (destructor)LZ4Decompressor_dealloc, /* tp_dealloc */
    0,                                   /* tp_print */
    0,                                   /* tp_getattr */
    0,                                   /* tp_setattr */
    0,                                   /* tp_compare */
    0,                                   /* tp_repr */
    0,                                   /* tp_as_number */
    0,                                   /* tp_as_sequence */
    0,                                   /* tp_as_mapping */
    0,                                   /* tp_hash */
    0,                                   /* tp_call */
    0,                                   /* tp_str */
    0,                                   /* tp_getattro */
    0,                                   /* tp_setattro */
    0,                                   /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                  /* tp_flags */
    "LZ4Decompressor objects",           /* tp_doc */
    0,                                   /* tp_traverse */
    0,                                   /* tp_clear */
    0,                                   /* tp_richcompare */
    0,                                   /* tp_weaklistoffset */
    0,                                   /* tp_iter */
    0,                                   /* tp_iternext */
    LZ4Decompressor_methods,             /* tp_methods */
    0,                                   /* tp_members */
    0,                                   /* tp_getset */
    0,                                   /* tp_base */
    0,                                   /* tp_dict */
    0,                                   /* tp_descr_get */
    0,                                   /* tp_descr_set */
    0,                                   /* tp_dictoffset */
    (initproc)LZ4Decompressor_init       /* tp_init */
};


/*========================== Module initialization ==========================*/

#if PY_MAJOR_VERSION >= 3

static int roslz4_traverse(PyObject *m, visitproc visit, void *arg) {
  Py_VISIT(GETSTATE(m)->error);
  return 0;
}

static int roslz4_clear(PyObject *m) {
  Py_CLEAR(GETSTATE(m)->error);
  return 0;
}

static struct PyModuleDef moduledef = {
  PyModuleDef_HEAD_INIT,
  "_roslz4",
  NULL,
  sizeof(struct module_state),
  NULL,
  NULL,
  roslz4_traverse,
  roslz4_clear,
  NULL
};
#define INITERROR return NULL

PyObject *
PyInit__roslz4(void)

#else
#define INITERROR return

void
init_roslz4(void)
#endif
{
  PyObject *m;

  LZ4Compressor_Type.tp_new = PyType_GenericNew;
  if (PyType_Ready(&LZ4Compressor_Type) < 0) {
    INITERROR;
  }

  LZ4Decompressor_Type.tp_new = PyType_GenericNew;
  if (PyType_Ready(&LZ4Decompressor_Type) < 0) {
    INITERROR;
  }

#if PY_MAJOR_VERSION >= 3
  m = PyModule_Create(&moduledef);
#else
  m = Py_InitModule("_roslz4", NULL);
#endif

  if (m == NULL) {
    INITERROR;
  }

  Py_INCREF(&LZ4Compressor_Type);
  PyModule_AddObject(m, "LZ4Compressor", (PyObject *)&LZ4Compressor_Type);
  Py_INCREF(&LZ4Decompressor_Type);
  PyModule_AddObject(m, "LZ4Decompressor", (PyObject *)&LZ4Decompressor_Type);

#if PY_MAJOR_VERSION >= 3
  return m;
#endif
}
