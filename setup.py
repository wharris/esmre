# setup.py - distutils configuration for esm and esmre modules
# Copyright (C) 2007 Tideway Systems Limited.
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

from distutils.core import setup, Extension

module1 = Extension("esm",
                    #define_macros=[("HEAP_CHECK", 1)],
                    sources = ['src/esm.c',
                               'src/aho_corasick.c',
                               'src/ac_heap.c',
                               'src/ac_list.c'])
                    
setup (name = "esmre",
       version = '0.1',
       description = 'Regular expression accelerator',
       long_description = "\n".join("""
        Modules used to accelerate execution of a large collection of regular
        expressions using the Aho-Corasick algorithms.
       """.strip().split()),
       author = 'Will Harris',
       author_email = 'w.harris@tideway.com',
       ext_modules = [module1],
       package_dir = {'': 'src'},
       py_modules = ["esmre"])
