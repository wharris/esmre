#!/usr/bin/env python
# encoding: utf-8

# esm_tests.py - tests for esm extension module
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

import unittest
import esm
import esmre

class IndexTests(unittest.TestCase):
    def testHasConstructor(self):
        self.assertTrue(esm.Index())

    def testIndexHasEnterMethod(self):
        esm.Index().enter("keyword")
        
    def testIndexMustHaveStringForFirstArgument(self):
        self.assertRaises(TypeError, esm.Index().enter, 0)
        self.assertRaises(TypeError, esm.Index().enter, [])
        self.assertRaises(TypeError, esm.Index().enter)
        
    def testIndexFix(self):
        index = esm.Index()
        index.fix()
        
    def testQuery(self):
        index = esm.Index()
        index.enter("he")
        index.enter("she")
        index.enter("his")
        index.enter("hers")
        index.fix()
        
        self.assertEqual([((1, 4), "his"),
                           ((5, 7), "he"),
                           ((13, 16), "his")],
                         index.query("this here is history"))
#                                     0123456789.123456789

    def testEnterObject(self):
        index = esm.Index()
        
        mint_object = dict()
        index.enter("mint", mint_object)
        
        pepper_object = dict()
        index.enter("pepper", pepper_object)
        
        index.fix()
        results = index.query("mint sauce")
        
        self.assertEqual(1, len(results))
        self.assertTrue(isinstance(results[0], tuple))
        
        slice_indices, associated_object = results[0]
        
        self.assertEqual((0,4), slice_indices)
        self.assertTrue(associated_object is mint_object)
        
    def testCantFixIndexWhenAlreadyFixed(self):
        index = esm.Index()
        index.fix()
        
        self.assertRaises(TypeError, index.fix)

    def testCantEnterWhenAlreadyFixed(self):
        index = esm.Index()
        index.fix()
        
        self.assertRaises(TypeError, index.enter, "foo")

    def testQueryUntilFixed(self):
        index = esm.Index()
        self.assertRaises(TypeError, index.query, "foo")
        
    def testObjectsForCommonEndingsAreDecrefedCorrectly(self):
        o = "Owt"
        import sys
        initial_ref_count = sys.getrefcount(o)
        
        index = esm.Index()
        index.enter("food", o)
        index.enter("ood", o)
        index.fix()
        index.query("blah")
        
        index = None
        del index
        
        self.assertEqual(initial_ref_count, sys.getrefcount(o))


if __name__ == '__main__':
    unittest.main()