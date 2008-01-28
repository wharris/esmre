esmre - Efficient String Matching Regular Expressions
=====================================================

esmre is a Python module that can be used to speed up the execution of a large
collection of regular expressions. It works by building a index of compulsory
substrings from a collection of regular expressions, which it uses to quickly
exclude those expressions which trivially do not match each input.

Here is some example code that uses esmre:

>>> import esmre
>>> index = esmre.Index()
>>> index.enter(r"Major-General\W*$", "savoy opera")
>>> index.enter(r"\bway\W+haye?\b", "sea shanty")
>>> index.query("I am the very model of a modern Major-General.")
['savoy opera']
>>> index.query("Way, hay up she rises,")
['sea shanty']
>>> 

The esmre module builds on the simpler string matching facilities of the esm
module, which wraps a C implementation some of the algorithms described in
Aho's and Corasick's paper on efficient string matching [Aho, A.V, and
Corasick, M. J. Efficient String Matching: An Aid to Bibliographic Search.
Comm. ACM 18:6 (June 1975), 333-340]. Some minor modifications have been made
to the algorithms in the paper and one algorithm is missing (for now), but
there is enough to implement a quick string matching index.

Here is some example code that uses esm directly:

>>> import esm
>>> index = esm.Index()
>>> index.enter("he")
>>> index.enter("she")
>>> index.enter("his")
>>> index.enter("hers")
>>> index.fix()
>>> index.query("this here is history")
[((1, 4), 'his'), ((5, 7), 'he'), ((13, 16), 'his')]
>>> index.query("Those are his sheep!")
[((10, 13), 'his'), ((14, 17), 'she'), ((15, 17), 'he')]
>>> 

You can see more usage examples in the tests.
