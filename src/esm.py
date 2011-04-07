#!/usr/bin/env python
# encoding: utf-8

# esm.py - efficient string matching
# Copyright (C) 2009 Will Harris
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
# USA
import ctypes
import ctypes.util

libesm = ctypes.cdll.LoadLibrary(ctypes.util.find_library('esm'))

libesm.ac_index_new.restype = ctypes.c_void_p

libesm.ac_index_enter.argtypes = [ctypes.c_void_p,
                                  ctypes.c_char_p,
                                  ctypes.c_uint,
                                  ctypes.c_void_p]

libesm.ac_index_fix.argtypes = [ctypes.c_void_p]

libesm.ac_free_function = ctypes.CFUNCTYPE(ctypes.c_int,
                                           ctypes.c_void_p,
                                           ctypes.c_void_p)
libesm.ac_list_free_keep_item = ctypes.cast(libesm.ac_list_free_keep_item,
                                            libesm.ac_free_function)
libesm.ac_index_free.argtypes = [ctypes.c_void_p, libesm.ac_free_function]

class ac_result(ctypes.Structure):
    _fields_ = [('start', ctypes.c_int),
                ('end', ctypes.c_int),
                ('object', ctypes.c_void_p)]

libesm.ac_result_callback = ctypes.CFUNCTYPE(ctypes.c_int,
                                             ctypes.c_void_p,
                                             ctypes.POINTER(ac_result))

libesm.ac_index_query_cb.argtypes = [ctypes.c_void_p,
                                     ctypes.c_char_p,
                                     ctypes.c_uint,
                                     libesm.ac_result_callback,
                                     ctypes.c_void_p]

def append_result_py(lst_p, res_p):
    lst = ctypes.cast(lst_p, ctypes.py_object).value
    start = res_p.contents.start
    end = res_p.contents.end
    obj = ctypes.cast(res_p.contents.object, ctypes.py_object).value
    lst.append(((start, end), obj))
    return 0

append_result_c = libesm.ac_result_callback(append_result_py)

class ListReferenceCounter(object):
    def __init__(self):
        self.retains = []

    def incref(self, obj):
        self.retains.append(obj)

    def free_index(self, index):
        libesm.ac_index_free(index.index, libesm.ac_list_free_keep_item)
        self.retains = None

try:
    import _ctypes

    def free_result_py(item_p, context_p):
        item = ctypes.cast(item_p, ctypes.py_object).value
        _ctypes.Py_DECREF(item)
        return 0

    free_result_c = libesm.ac_free_function(free_result_py)

    class ReferenceCounter(object):
        def incref(self, obj):
            _ctypes.Py_INCREF(obj)

        def free_index(self, index):
            libesm.ac_index_free(index.index, free_result_c)
            
except ImportError:
    ReferenceCounter = ListReferenceCounter


class Index(object):
    def __init__(self):
        self.index = libesm.ac_index_new()
        self.reference_counter = ReferenceCounter()
        self.fixed = False
    
    def __del__(self):
        self.reference_counter.free_index(self)
    
    def enter(self, keyword, obj=None):
        if self.fixed:
            raise TypeError
        
        if not isinstance(keyword, str):
            raise TypeError
        
        if obj is None:
            obj = keyword
        
        libesm.ac_index_enter(self.index,
                              keyword,
                              len(keyword),
                              ctypes.c_void_p(id(obj)))
        self.reference_counter.incref(obj)
    
    def fix(self):
        if self.fixed:
            raise TypeError
        libesm.ac_index_fix(self.index)
        self.fixed = True
    
    def query(self, query):
        if not self.fixed:
            raise TypeError
        
        results = []
        libesm.ac_index_query_cb(self.index,
                                 query,
                                 len(query),
                                 append_result_c,
                                 ctypes.c_void_p(id(results)))
        return results
