#!/usr/bin/env python
# encoding: utf-8

# esmre_tests.py - tests for esmre module
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

import sys
import os.path
sys.path.insert(0, os.path.join(os.path.curdir, '../src'))
import unittest
import esmre

class HintExtractionTests(unittest.TestCase):
    def checkHints(self, expected_hints, regex):
        self.assertEqual(expected_hints, esmre.hints(regex))
        
    def testSimpleString(self):
        self.checkHints(["yarr"], "yarr")
    
    def testSkipsOptionalCharacter(self):
        self.checkHints(["dubloon"], "dubloons?")
        
    def testStartsNewStringAfterOptionalCharacter(self):
        self.checkHints(["ship", "shape"], "ship ?shape")
        
    def testSkipsOptionalRepeatedCharacter(self):
        self.checkHints(["bristol", "fasion"], "bristol *fasion")
        
    def testIncludesRepeatedCharacterButStartsNewHint(self):
        self.checkHints(["ava", "st me harties"],
                         "ava+st me harties")
    
    def testSkipsGroups(self):
        self.checkHints(["Hoist the ", ", ye ", "!"],
             "Hoist the (mizzen mast|main brace), "
                       "ye (landlubbers|scurvy dogs)!")
                       
    def testSkipsAny(self):
        self.checkHints(["After 10 paces, ", " marks the spot"],
                         "After 10 paces, . marks the spot")
                       
    def testSkipsOneOrMoreAny(self):
        self.checkHints(["Hard to ", "!"],
                         "Hard to .+!")
                         
    def testSkipsNestedGroups(self):
        self.checkHints(["Squark!"],
                         "Squark!( Pieces of (.+)!)")
                         
    def testSkipsCharacterClass(self):
        self.checkHints(["r"],
                         "[ya]a*r+")

    def testRightBracketDoesNotCloseGroupIfInClass(self):
        self.checkHints([":=", "X"],
                         ":=([)D])X")
                         
    def testSkipsBackslashMetacharacters(self):
        self.checkHints(["Cap'n", " "],
                         r"Cap'n\b ([\S] Beard)")
                         
    def testBackslashBracketDoesNotCloseGroup(self):
        self.checkHints([":=", "X"],
                         r":=(\)|D)X")
    
    def testBackslashSquareBracketDoesNotCloseClass(self):
        self.checkHints([":=", "X"],
                         r":=[)D\]]X")
                         
    def testSkipsMetacharactersAfterGroups(self):
        self.checkHints(["Yo ", " and a bottle of rum"],
                        r"Yo (ho )+ and a bottle of rum")
    
    def testSkipsRepetionBraces(self):
        self.checkHints(["A", ", me harties"],
                        r"Ar{2-10}, me harties")
                        
    def testAlternationCausesEmptyResult(self):
        self.checkHints([], r"rum|grog")
        
    def testSkipMatchBeginning(self):
        self.checkHints(["The black perl"], "^The black perl")
        
    def testSkipMatchEnd(self):
        self.checkHints(["Davey Jones' Locker"], r"Davey Jones' Locker$")
        
    def testOnlyGroupGivesEmptyResult(self):
        self.checkHints([], r"(rum|grog)")
    

class ShortlistTests(unittest.TestCase):
    def checkShortlist(self, expected_shortlist, hints):
        self.assertEqual(expected_shortlist, esmre.shortlist(hints))
        
    def testShortlistIsEmptyForEmptyCandidates(self):
        self.checkShortlist([], [])
        
    def testShortlistIsOnlyCandidate(self):
        self.checkShortlist(["Blue Beard"], ["Blue Beard"])
        
    def testShorlistSelectsLongestCandidate(self):
        self.checkShortlist(["Black Beard"], ["Black Beard", "Blue Beard"])
    
    def testShorlistSelectsLongestCandidateAtEnd(self):
        self.checkShortlist(["Yellow Beard"],
                            ["Black Beard", "Blue Beard", "Yellow Beard"])


class IndexTests(unittest.TestCase):
    def setUp(self):
        self.index = esmre.Index()
        self.index.enter(r"Major-General\W*$", "savoy opera")
        self.index.enter(r"\bway\W+haye?\b", "sea shanty")
        
    def testSingleQuery(self):
        self.assertEqual(["savoy opera"], self.index.query(
            "I am the very model of a modern Major-General."))
            
    def testCannotEnterAfterQuery(self):
        self.index.query("blah")
        self.assertRaises(TypeError, self.index.enter, "foo", "bar")
            
    def testCaseInsensitive(self):
        self.assertEqual(["sea shanty"], self.index.query(
            "Way, hay up she rises,"))
        self.assertEqual(["sea shanty"], self.index.query(
            "To my way haye, blow the man down,"))
            
    def testAlwaysReportsOpjectForHintlessExpressions(self):
        self.index.enter(r"(\d+\s)*(paces|yards)", "distance")
        self.assertTrue("distance" in self.index.query("'til morning"))


class NewIndexTests(unittest.TestCase):
    def setUp(self):
        self.index = esmre.NewIndex()
        self.index.enter(r"foo(?!d)", "metasyntactic")
    
    def testReturnsMatchingObject(self):
        self.assertEqual(["metasyntactic"], self.index.query("foo"))
    
    def testDoesNotReturnNonMatchingObjectWithCorrectHint(self):
        self.assertEqual([], self.index.query("food"))


if __name__ == '__main__':
    unittest.main()
