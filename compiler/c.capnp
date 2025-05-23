# Copyright (c) 2016 NetDEF, Inc. and contributors
# Licensed under the MIT License:
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

@0xc0183dd65ffef0f3;

annotation nameinfix @0x85a8d86d736ba637 (file): Text;
# add an infix (middle insert) for output file names
#
# "make" generally has implicit rules for compiling "foo.c" => "foo".  This
# is very annoying with capnp since the rule will be "foo" => "foo.c", leading
# to a loop.  $nameinfix (recommended parameter: "-gen") inserts its parameter
# before the ".c", so the filename becomes "foo-gen.c"
#
# Alternatively, add this Makefile rule to disable compiling "foo.capnp.c" -> "foo.capnp":
#   %.capnp: ;
#
#
# ("foo" is really "foo.capnp", so it's foo.capnp-gen.c)

annotation fieldgetset @0xf72bc690355d66de (file): Void;
# generate getter & setter functions for accessing fields
#
# allows grabbing/putting values without de-/encoding the entire struct.

annotation donotinclude @0x8c99797357b357e9 (file): UInt64;
# do not generate an include directive for an import statement for the file with
# the given ID

annotation typedefto @0xcefaf27713042144 (struct, enum): Text;
# generate a typedef for the annotated struct or enum declaration

annotation namespace @0xf2c035025fec7c2b (file): Text;
# prefix structs with a name space string

annotation extraheader @0xbadb496d09cf4612 (file): Text;
# add extra preprocessor directives to the header

annotation extendedattribute @0xd187bca5c6844c24 (file): Text;
# add an extended attribute to each generated function

annotation codecgen @0xcccaac86283e2609 (file): Void;
# generate codec(encode/decode) to each type

annotation mapname @0xb9edf6fc2d8972b8 (*): Text;
# the mapped type name which will be encoded

annotation maplistcount @0xb6ea49eb8a9b0f9e (field): Text;
# the mapped list count field which will be encoded

annotation mapuniontag @0xdce06d41858f91ac (union, struct): Text;
# the mapped tag (which) of union
