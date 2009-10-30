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

libesm.ac_index_free.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

class ac_list_item(ctypes.Structure):
    pass
    
ac_list_item._fields_ = [('item', ctypes.c_void_p),
                         ('next', ctypes.POINTER(ac_list_item))]

class ac_list(ctypes.Structure):
    _fields_ = [('first', ctypes.POINTER(ac_list_item)),
                ('last', ctypes.POINTER(ac_list_item))]
    

libesm.ac_index_query.argtypes = [ctypes.c_void_p,
                                  ctypes.c_char_p,
                                  ctypes.c_uint,
                                  ctypes.POINTER(ac_list)]

libesm.ac_list_new.restype = ctypes.POINTER(ac_list)

libesm.ac_result_list_free.arg_types = [ctypes.POINTER(ac_list)]

class ac_result(ctypes.Structure):
    _fields_ = [('start', ctypes.c_int),
                ('end', ctypes.c_int),
                ('object', ctypes.c_void_p)]

class Index(object):
    def __init__(self):
        self.index = libesm.ac_index_new()
        self.retains = []
        self.fixed = False
    
    def __del__(self):
        self.retains = None
        libesm.ac_index_free(self.index, libesm.ac_list_free_keep_item)
    
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
        self.retains.append(obj)
    
    def fix(self):
        if self.fixed:
            raise TypeError
        libesm.ac_index_fix(self.index)
        self.fixed = True
    
    def query(self, query):
        if not self.fixed:
            raise TypeError
        
        results = []
        ac_results = libesm.ac_list_new()
        try:
            libesm.ac_index_query(self.index, query, len(query), ac_results)
            i = ac_results.contents.first
            while i:
                result = ctypes.cast(i.contents.item,
                                     ctypes.POINTER(ac_result))
                
                start = result.contents.start
                end = result.contents.end
                obj = ctypes.cast(result.contents.object, ctypes.py_object).value
                results.append(((start,end), obj))
                
                i = i.contents.next
            
            return results
        
        finally:
            libesm.ac_result_list_free(ac_results)
