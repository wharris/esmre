/* esm.c - Python extension module for efficient string matching
   Copyright (C) 2007 Tideway Systems Limited.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Python.h>
#include <structmember.h>

#include "aho_corasick.h"

typedef struct {
    PyObject_HEAD
    ac_index*   index;
} esm_IndexObject;

ac_error_code
decref_result_object(void* item, void* data) {
    Py_DECREF((PyObject*) item);
    return AC_SUCCESS;
}

static void
esm_Index_dealloc(esm_IndexObject* self) {
    ac_index_free(self->index, decref_result_object);
    self->ob_type->tp_free((PyObject*) self);
}

static PyObject*
esm_Index_new(PyTypeObject *type, PyObject* args, PyObject* kwds) {
    esm_IndexObject* self;
    
    self = (esm_IndexObject*) type->tp_alloc(type, 0);
    if (self != NULL) {
        if ( ! (self->index = ac_index_new())) {
            Py_DECREF(self);
            return PyErr_NoMemory();
        }
    }
    
    
    return (PyObject*) self;
}

static int
esm_Index_init(esm_IndexObject* self, PyObject* args, PyObject* kwds) {
    return 0;
}

static PyObject*
esm_Index_enter(esm_IndexObject* self, PyObject* args) {
    char*     keyword;
    int       length;
    PyObject* object = NULL;
    
    if (self->index->index_state != AC_INDEX_UNFIXED) {
        PyErr_SetString(PyExc_TypeError, "Can't call enter after fix");
        return NULL;
    }
    
    if ( ! PyArg_ParseTuple(args, "s#|O", &keyword, &length, &object)) {
        return NULL;
    }
    
    if (object == NULL) {
        object = PyTuple_GetItem(args, 0);
    }
        
    if (ac_index_enter(self->index,
                       (ac_symbol*) keyword,
                       (ac_offset) length,
                       (void*) object) != AC_SUCCESS) {
        return PyErr_NoMemory();
    }
    
    Py_INCREF(object);

    Py_RETURN_NONE;
}

static PyObject*
esm_Index_fix(esm_IndexObject* self) {
    if (self->index->index_state != AC_INDEX_UNFIXED) {
        PyErr_SetString(PyExc_TypeError, "Can't call fix repeatedly");
        return NULL;
    }
    
    if (ac_index_fix(self->index) != AC_SUCCESS) {
        return PyErr_NoMemory();
    }

    Py_RETURN_NONE;
}

static PyObject*
esm_Index_query(esm_IndexObject* self, PyObject* args) {
    char*         phrase = NULL;
    int           length = 0;
    ac_list*      results = NULL;
    ac_list_item* result_item = NULL;
    ac_result*    result = NULL;
    PyObject*     result_list = NULL;
    PyObject*     result_tuple = NULL;
    
    if (self->index->index_state != AC_INDEX_FIXED) {
        PyErr_SetString(PyExc_TypeError, "Can't call query before fix");
        return NULL;
    }
    
    if ( ! PyArg_ParseTuple(args, "s#", &phrase, &length)) {
        return NULL;
    }
    
    if ( ! (results = ac_list_new())) {
        return PyErr_NoMemory();
    }
    
    if (ac_index_query(self->index,
                       (ac_symbol*) phrase,
                       (ac_offset) length,
                       results) != AC_SUCCESS) {
        ac_result_list_free(results);
        return PyErr_NoMemory();
    }
    
    if ( ! (result_list = PyList_New(0))) {
        ac_result_list_free(results);
        return PyErr_NoMemory();        
    }
    
    result_item = results->first;
    while (result_item) {
        result = (ac_result*) result_item->item;                                
        result_tuple = Py_BuildValue("((ii)O)", result->start,
                                                result->end,
                                                (PyObject*) result->object);
        
        if (PyList_Append(result_list, result_tuple)) {
            Py_DECREF(result_tuple);
            return PyErr_NoMemory();
        }
        
        Py_DECREF(result_tuple);
        result_item = result_item->next;
    }
    
    ac_result_list_free(results);
    
    return result_list;
}

static PyMethodDef esm_Index_methods[] = {
    {"enter", (PyCFunction) esm_Index_enter, METH_VARARGS,
     "I.enter(string[, object])\nAdd a string to the keyword index and "
     "associate it with the object or with the string itself if no object is "
     "provided."},
    {"fix", (PyCFunction) esm_Index_fix, METH_NOARGS,
     "I.fix()\nMake the index ready for queries."},
    {"query", (PyCFunction) esm_Index_query, METH_VARARGS,
     "I.query(string) -> sequence\nSearch for matches in a string. Returns a "
     "list containing one tuple like ((i,j),o) for each match where "
     "string[i:j] equals the keyword and o is the associated object."},
    {NULL}     /* Sentinel */
};


static PyMemberDef esm_Index_members[] = {
    {NULL}  /* Sentinel */
};


static PyTypeObject esm_IndexType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /*ob_size*/
    "esm.Index",                        /*tp_name*/
    sizeof(esm_IndexObject),            /*tp_basicsize*/
    0,                                  /*tp_itemsize*/
    (destructor) esm_Index_dealloc,     /*tp_dealloc*/
    0,                                  /*tp_print*/
    0,                                  /*tp_getattr*/
    0,                                  /*tp_setattr*/
    0,                                  /*tp_compare*/
    0,                                  /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    0,                                  /*tp_hash */
    0,                                  /*tp_call*/
    0,                                  /*tp_str*/
    0,                                  /*tp_getattro*/
    0,                                  /*tp_setattro*/
    0,                                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Index() -> new efficient string matching index",  /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    esm_Index_methods,             /* tp_methods */
    esm_Index_members,             /* tp_members */
    0,           /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc) esm_Index_init,      /* tp_init */
    0,                         /* tp_alloc */
    esm_Index_new,                 /* tp_new */
};

static PyMethodDef esm_methods[] = {
    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initesm(void) 
{
    PyObject* m;

    if (PyType_Ready(&esm_IndexType) < 0)
        return;

    m = Py_InitModule3("esm", esm_methods,
                       "Support for efficient string matching.");
    
    if (m == NULL) {
        return;
    }

    Py_INCREF(&esm_IndexType);
    PyModule_AddObject(m, "Index", (PyObject *)&esm_IndexType);
}

