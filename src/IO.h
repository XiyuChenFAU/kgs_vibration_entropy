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



#ifndef IO_H
#define IO_H

#include <string>
#include <vector>

#include "core/Molecule.h"
#include "core/Residue.h"
#include "Util.h"
#include "Selection.h"
#include "ResidueProfiles.h"


class IO {
 public:
  static ResidueProfile readResidueProfile();

  static Molecule *readPdb(
      const std::string &pdb_file,
      const std::vector<std::string> &extraCovBonds = {},
      const std::string &hbondMethod = "",
      const std::string &hbondFile = "",
      const Molecule *reference = nullptr
  );

  static void readHbonds(const std::string &hbondMethod, const std::string &hbondFile, Molecule *mol);

//	static void readDssp (Molecule * protein, std::string dssp_file);
//	static void readRigidbody (Molecule * molecule);
//  static void readRigidbody (Molecule * molecule, Selection& movingResidues);
  static void writePdb(Molecule *molecule, std::string output_file_name);

  static void writePyMolScript(Molecule *rigidified, std::string pdb_file, std::string output_file_name, Molecule* iniMolecule = nullptr);

  static void writePyMolrigidScript(std::string pdb_file, std::string output_file_name, Molecule* iniMolecule = nullptr);

  static void writeBondLengthsAndAngles(Molecule *molecule, std::string output_file_name);

  static void writeCovBonds(Molecule *molecule, std::string output_file_name);

//	static void readCovBonds  (Molecule *molecule, std::string input_file_name);
  static void writeHbondsIn(Molecule *molecule, std::string output_file_name);

  static void writeHbonds(Molecule *molecule, std::string output_file_name);

  static void writeHbondsChange(Configuration *conf, std::string output_file_name);

  static void readHbonds(Molecule *molecule, std::string hbond_file_name);

  static void readAnnotations(Molecule *molecule, std::string annotation_file_name);

  static void writeRBs(Molecule *molecule, std::string output_file_name);

  static void writeStats(Molecule *molecule, std::string output_file_name, Molecule *rigidified = nullptr); ///version when collapsed rbs

  static void writeQ(Molecule *molecule, Configuration *referenceConf, std::string output_file_name);

  static void
  writeTrajectory(Molecule *molecule, std::string output_file, std::string output_mdl, Molecule *target = nullptr, const std::vector<std::tuple<Atom *, Atom *, double> >* deer = nullptr);

  static std::vector<std::tuple<Atom *, Atom *, double> >
  readRelativeDistances(const std::string &fname, Molecule *mol);

  static void
  writeNewSample(Configuration *conf, Configuration *ref, int sample_num, const std::string &workingDir, int saveData);

 private:
//	static void makeCovBond (Residue* res1, Residue* res2, std::string atom_name1, std::string atom_name2);
  static void readHbonds_dssr(Molecule *molecule, std::string dssrFile);

  static void readHbonds_rnaview(Molecule *molecule, std::string file, bool fillAnnotations);

  static void readHbonds_first(Molecule *molecule, std::string file);

  static void readHbonds_kinari(Molecule *molecule, std::string file);

  static void readHbonds_hbPlus(Molecule *molecule, std::string file);

  static void readHbonds_vadar(Molecule *molecule, std::string file);
};

#endif
