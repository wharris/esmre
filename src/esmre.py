#!/usr/bin/env python
# encoding: utf-8

# esmre.py - clue-indexed regular expressions module
# Copyright (C) 2007-2008 Tideway Systems Limited.
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

import esm
import threading

class InBackslashState(object):
    def __init__(self, parent_state):
        self.parent_state = parent_state
    
    def process_byte(self, ch):
        return self.parent_state


class InClassState(object):
    def __init__(self, parent_state):
        self.parent_state = parent_state
    
    def process_byte(self, ch):
        if ch == "]":
            return self.parent_state
            
        elif ch == "\\":
            return InBackslashState(self)
        
        else:
            return self


class InBracesState(object):
    def __init__(self, parent_state):
        self.parent_state = parent_state
    
    def process_byte(self, ch):
        if ch == "}":
            return self.parent_state
        
        else:
            return self


class CollectingState(object):
    def __init__(self):
        self.hints = [""]

    def process_byte(self, ch):
        self.update_hints(ch)
        return self.next_state(ch)
    
    def bank_current_hint_with_last_byte(self):
        self.hints.append("")
    
    def bank_current_hint_and_forget_last_byte(self):
        if isinstance(self.hints[-1], list):
            del self.hints[-1]
        else:
            self.hints[-1] = self.hints[-1][:-1]
        
        self.hints.append("")
    
    def forget_all_hints(self):
        self.hints = [""]
    
    def append_to_current_hint(self, ch):
        self.hints[-1] += ch
    
    def update_hints(self, ch):
        if ch in "?*{":
            self.bank_current_hint_and_forget_last_byte()
        
        elif ch in "+.^$([\\":
            self.bank_current_hint_with_last_byte()
        
        elif ch == "|":
            self.forget_all_hints()
            
        else:
            self.append_to_current_hint(ch)
    
    def next_state(self, ch):
        if ch == "(":
            return StartOfGroupState(self)
        
        elif ch == "[":
            return InClassState(self)
        
        elif ch == "{":
            return InBracesState(self)
            
        elif ch == "\\":
            return InBackslashState(self)
            
        elif ch == "|":
            return self.alternation_state()
            
        else:
            return self
    
    def alternation_state(self):
        raise NotImplementedError


class RootState(CollectingState):
    def alternation_state(self):
        raise StopIteration


class StartOfGroupState(object):
    def __init__(self, parent_state):
        self.parent_state = parent_state
    
    def process_byte(self, ch):
        if ch == "?":
            return StartOfExtensionGroupState(self.parent_state)
        else:
            return InGroupState(self.parent_state).process_byte(ch)


class InGroupState(CollectingState):
    def __init__(self, parent_state):
        CollectingState.__init__(self)
        self.parent_state = parent_state
        self.had_alternation = False
    
    def update_hints(self, ch):
        if ch == ")":
            if not self.had_alternation:
                self.parent_state.hints.append(self.hints)
        else:
            CollectingState.update_hints(self, ch)
    
    def next_state(self, ch):
        if ch == ")":
            return self.close_group_state()
        else:
            return CollectingState.next_state(self, ch)
    
    def close_group_state(self):
        return self.parent_state
    
    def alternation_state(self):
        self.had_alternation = True
        return self


class StartOfExtensionGroupState(object):
    def __init__(self, parent_state):
        self.parent_state = parent_state
    
    def process_byte(self, ch):
        if ch == "P":
            return MaybeStartOfNamedGroupState(self.parent_state)
        else:
            return IgnoredGroupState(self.parent_state).process_byte(ch)


class MaybeStartOfNamedGroupState(object):
    def __init__(self, parent_state):
        self.parent_state = parent_state
    
    def process_byte(self, ch):
        if ch == "<":
            return InNamedGroupNameState(self.parent_state)
        else:
            return IgnoredGroupState(self.parent_state)


class InNamedGroupNameState(object):
    def __init__(self, parent_state):
        self.parent_state = parent_state
    
    def process_byte(self, ch):
        if ch == ">":
            return InGroupState(self.parent_state)
        else:
            return self


class IgnoredGroupState(InGroupState):
    def update_hints(self, ch):
        pass


def hints(regex):
    state = RootState()
    
    try:
        for ch in regex:
            state = state.process_byte(ch)
        
    except StopIteration:
        pass
            
    def flattened(l):
        for item in l:
            if isinstance(item, list):
                for i in flattened(item):
                    yield i
            else:
                yield item
    
    return [hint for hint in flattened(state.hints) if hint]


def shortlist(hints):
    if not hints:
        return []
    
    best = ""
    
    for hint in hints:
        if len(hint) > len(best):
            best = hint
            
    return [best]


class Index(object):
    def __init__(self):
        self.esm = esm.Index()
        self.hintless_objects = list()
        self.fixed = False
        self.lock = threading.Lock()
        
        
    def enter(self, regex, obj):
        self.lock.acquire()
        try:
            
            if self.fixed:
                raise TypeError, "enter() cannot be called after query()"
            
            keywords = shortlist(hints(regex))
            
            if not keywords:
                self.hintless_objects.append(obj)
            
            for hint in shortlist(hints(regex)):
                self.esm.enter(hint.lower(), obj)
        
        finally:
            self.lock.release()
            
            
    def query(self, string):
        self.lock.acquire()
        try:
            
            if not self.fixed:
                self.esm.fix()
                self.fixed = True
            
        finally:
            self.lock.release()
        
        return self.hintless_objects + \
            [obj for (_, obj) in self.esm.query(string.lower())]
