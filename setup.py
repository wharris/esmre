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

from setuptools import setup, Extension
from Cython.Build import cythonize

module1 = Extension(
    "esm", ["src/esm.pyx", "src/esm/aho_corasick.c", "src/esm/ac_list.c"]
)

with open("README.md", "r") as readme:
    long_description = readme.read()

setup(
    name="esmre",
    version="1.0",
    description="Regular expression accelerator",
    long_description=long_description,
    long_description_content_type="text/markdown",
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: "
        "GNU Library or Lesser General Public License (LGPL)",
        "Operating System :: POSIX",
        "Programming Language :: C",
        "Programming Language :: Cython",
        "Programming Language :: Python",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: Text Processing :: Indexing",
    ],
    install_requires=["setuptools"],
    author="Will Harris",
    author_email="esmre@greatlibrary.net",
    url="https://github.com/wharris/esmre",
    license="GNU LGPL",
    platforms=["POSIX"],
    ext_modules=cythonize([module1]),
    package_dir={"": "src"},
    py_modules=["esmre"],
    test_suite="nose.collector",
    tests_require="nose",
)
