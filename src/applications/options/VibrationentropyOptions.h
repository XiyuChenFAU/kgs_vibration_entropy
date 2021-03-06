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
//
// Created by Dominik Budday on 19.12.16.
//

#ifndef KGS_VIBRATIONENTROPYOPTIONS_H
#define KGS_VIBRATIONENTROPYOPTIONS_H



#include <string>
#include <vector>
#include <stdio.h>
#include "Selection.h"

class Molecule;

class Atom;

class VibrationentropyOptions {
 public:

  /** The working directory */
  std::string workingDirectory;
  /** File-path for the ligand structure. */
  std::string initialStructureFile;
  /** File-path for the euilibrium structure. */
  std::string equilibriumStructureFile;
  /** File-path for the no ligand structure. */
  //std::string noligandStructureFile;
  /** Annotation file-path common for all initial structures. */
  std::string annotationFile;
  /** File containing hydrogen bond definitions. */
  std::string hydrogenbondFile;
  /** Method of identifying hydrogen bonds. */
  std::string hydrogenbondMethod;
  /** Atom-pairs that should be treated as h-bonds. */
  std::vector<std::string> extraCovBonds;
  /** Collision factor indicating the multiplication factor of all atoms van
        der Waals radii. */
  double collisionFactor;
  /** potential distance cut off distance. */
  std::vector<double> entropycutoff;
  /** coefficient for calculation of frequency. */
  double coefficient;
  /** potential energy cut off distance. */
  std::vector<double> vdwenergycutoff;
  /** The random generator seed. */
  int seed;
  /** Save output: Indicate the level of files that shall be saved. */
  int saveData;
  /** List of residues supposed active in transition, TODO: allow moveable subset in rigidity analysis as well*/
  std::string residueNetwork;
  /** Atom selection for gradients, distance computation etc., e.g. heavy, name Ca, backbone, all. Default is heavy*/
  bool preventClashes;
  /** The vector of atom IDs that will be part of root rigid bodies */
  std::vector<int> roots;
  /** Atoms used for collision detection and clash constraints: "all, backbone, heavy" */
  std::string collisionCheck;
  /** Cut-off for svd computation (magnitude of smallest singular value in the nullspace)*/
  double svdCutoff;
  /** Option for collapsing rigid edges. */
  int collapseRigid;
  /** Sink/Source to identify transferred DoF between different areas. */
  std::string sink;
  /** Sink/Source to identify transferred DoF between different areas. */
  std::string source;
  /** set the ligand. */
  std::string setligand;

  /** run no binding between ligand and protein. */
  std::string nocoupling;

  /** only analysis the vibrational entropy change of protein*/
  bool proteinonly;

  /** output the Hessian matrix*/
  bool getHessian;



  void print();

 private:
  VibrationentropyOptions();

  VibrationentropyOptions(int argc, char *argv[]);

  void printUsage(char *pname);

  inline bool fileExists(const std::string &name);

  void initializeVariables();

  //Singleton pattern
  static VibrationentropyOptions *instance;
 public:
  static VibrationentropyOptions *getOptions();

  static VibrationentropyOptions *createOptions(int argc, char *argv[]);

  static VibrationentropyOptions *createOptions();

};
#endif //KGS_RIGIDITYOPTIONS_H
