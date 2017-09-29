/*

Excited States software: KGS
Contributors: See CONTRIBUTORS.txt
Contact: kgs-contact@simtk.org

Copyright (C) 2009-2017 Stanford University

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

This entire text, including the above copyright notice and this permission notice
shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

*/

#ifndef CTK_TRANSFORMATION_H
#define CTK_TRANSFORMATION_H

#include <core/graph/KinTree.h>
#include "math3d/primitives.h"

class Configuration;

void Confvec2MatrixGlobal(KinTree *pTree, Configuration *q, Math3D::RigidTransform *ms);//, bool usePosition2=false);
void Confvec2MatrixLocal (KinVertex *root, Configuration *q, Math3D::RigidTransform *ms, std::vector<KinVertex*> subVerts);
void Confvec2MatrixIndividual(Configuration *q, KinVertex *node, double* globalRef, Math3D::RigidTransform *ms);

#endif
