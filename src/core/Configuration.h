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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <list>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <vector>
#include <tuple>

#include "math/Nullspace.h"
#include "math/Eigenvalue.h"
#include "core/graph/KinGraph.h"

class Molecule;
class Selection;


/**
 * A configuration holds all DOF-values necessary to update a molecules atom-positions.
 * DOF-values are relative to whatever is stored in the Atom::reference_positions, so calling
 * Configuration* conf = new Configuration(m_protein);
 * will create a configuration where all DOF-values are 0 and which represents whatever is in
 * the reference positions of m_protein. Modifying for example
 * conf->m_dofs[2] += 0.1
 * will add 0.1 units (often radians) to the third DOF. To see the resulting structure call either
 * m_protein->setConfiguration(conf);
 * or
 * conf->updatedProtein();
 * and then access Atom::position (not Atom::m_referencePosition).
 *
 * As new configurations are generated from older ones, the field m_children and m_parent store the
 * connectivity information.
 */
class Configuration
{
 public:
  double *m_dofs;                    ///< DOF-values (relative to Atom::m_referencePosition)
  //double *m_sumProjSteps;          //TODO: What is this?

  /** Construct a configuration with all DOF-values set to 0 and no m_parent. */
  Configuration(Molecule * mol);

  /** Construct a configuration with all DOF-values set to 0 and the specified m_parent. */
  Configuration(Configuration* parent);

  ~Configuration();

  double getGlobalTorsion( int i ) ; ///< Get a global DOF-value
  double* getGlobalTorsions() ;     ///< Get global DOF-value array

  unsigned int getNumDOFs() const;

  /** Set the specified dof to the global torsion value. Convenient function for
   * calculating difference between reference torsion and val. */
  void setGlobalTorsion(int i, double val);

  Configuration* clone() const;          ///< Copy this configuration

  void Print();                          // TODO: Remove or rename to printDOFs
//  Molecule* collapseRigidBonds();
//  void identifyBiggerRigidBodies();      ///< Identify clusters
//  void readBiggerSet();                  ///< read the set of clusters, related to identifying clusters
  void projectOnCycleNullSpace (gsl_vector *to_project, gsl_vector *after_project);

  void convertAllDofsToCycleDofs( gsl_vector *cycleDofs, gsl_vector *allDofs);
  void convertCycleDofsToAllDofs( gsl_vector *allDofsAfter, gsl_vector *cycleDofs, gsl_vector *allDofsBefore = nullptr);

//  static bool compareSize(std::pair<int, unsigned int> firstEntry, std::pair<int, unsigned int> secondEntry);//TODO: What is this?

  void writeQToBfactor();

  // When the samples are generated as an expanding tree, the m_children and m_parent store the connectivity information of these nodes
  const int m_treeDepth;             ///< Depth in the exploration tree
  int m_id;                         ///< ID of configuration
  double m_vdwEnergy;               ///< van der Waals energy of configuration
  double m_deltaH;                  ///< change in enthalpy due to clash constraints
  double m_distanceToTarget;        ///< Distance to target configuration
  double m_distanceToParent;        ///< Distance to m_parent configuration
  double m_distanceToIni;           ///< Distance to initial configuration
  double m_paretoFrontDistance;
  double m_maxConstraintViolation;  //Maximum detected distance violation of h-bond constraints, Todo: maybe not necessary to keep
  double m_minCollisionFactor;      //minimum necessary clash-factor for configuration to be clash free, Todo: maybe not necessary to keep
  double m_usedClashPrevention;

//  std::map<unsigned int, Rigidbody*> m_biggerRBMap;  // <Cluster-idx, Pointer-to-cluster>
//  std::vector< std::pair<int, unsigned int> > m_sortedRBs; // < cluster-idx, cluster size>

  int m_numClusters;             ///< Number of rigid clusters (super rigid bodies)
  int m_maxIndex;                ///< Index of largest cluster
  int m_maxSize;                 ///< Size of largest cluster
  int m_clashFreeDofs;           ///< Number of clash-free dofs (for post-processing)

  Molecule * updatedMolecule();  ///< Update the atom-positions to reflect this configuration and return the molecule
  Molecule * getMolecule() const;///< Return the associated molecule
  void updateMolecule();         ///< Update the atom-positions to reflect this configuration

  /** Return the cycle jacobian. Calls computeJacobians if CycleJacobian is not up to date */
  gsl_matrix* getCycleJacobian();
  Nullspace* getNullspace();    ///< Compute the nullspace (if it wasn't already) and return it
  //Nullspace* getNullspaceligand();  ///< Compute the nullspace for ligand (if it wasn't already) and return it
  Nullspace* getNullspacenocoupling();
  void Hessianmatrixentropy(double cutoff=20.0, double coefficientvalue=1.0, double vdwenergyvalue=10000.0, Nullspace* Nu=nullptr, Molecule* mol=nullptr, bool proteinonly=false, std::string nocoupling="true");  ///< Compute the nullspace for vibrational entropy (if it wasn't already) and return it
  Eigenvalue* geteigenvalue();
  gsl_matrix* getHydrophobicJacobian();
  gsl_matrix* getHydrogenJacobian();
  gsl_matrix* getDistanceJacobian();

  void rigidityAnalysis();
  void deleteNullspace(); ///if not needed anymore, save memory

  Configuration* getParent();   ///< Access configuration that spawned this one
  std::list<Configuration*>& getChildren(); ///< Access child configurations

  double siteDOFTransfer(Selection& source, Selection& sink,gsl_matrix* baseMatrix); //
  void sortFreeEnergyModes(gsl_matrix* baseMatrix, gsl_vector* singVals, gsl_vector* returnIDs); //return index list of modes sorted by free energy
  int getNumRigidDihedralsligand();

  bool checknocoupling();
  static Nullspace* ClashAvoidingNullSpace; //TODO: Make private (or even better put in ClashAvoidingMove).
  void setrigiddofid();
    
 protected:

    int numligandRigidDihedrals=0; ///< Rigid dihedrals in ligand
  void updateGlobalTorsions();           ///< Update the global DOF-values (m_dofs_global field)
  double *m_dofs_global;                 ///< DOF-values in a global system (not relative to Atom::reference_position)
  Molecule * const m_molecule;           ///< The molecule related to the configuration
  Configuration * m_parent;              ///< The parent-configuration this configuration was generated from
  std::list<Configuration*> m_children;  ///< List of child-configurations

  void computeCycleJacobianAndNullSpace();
  void computeCycleJacobianentropyforall();
  void computeCycleJacobianentropy(Nullspace* Nu,std::string nocoupling);
  void computeMassmatrix();
  void computedistancematrix(Molecule* mol);
  void computeHessiancartesian(double cutoff, double coefficientvalue, double vdwenergyvalue,Molecule* mol);
  void computeJacobians();               ///< Compute non-redundant cycle jacobian and hbond-jacobian // and also HydrophobicBond-jacobian
  // Jacobian matrix of all the cycles of rigid bodies
  void computeJacobiansnocoupling();
  static gsl_matrix* CycleJacobian; // column dimension is the number of DOFs; row dimension is 5 times the number of cycles because 2 atoms on each cycle-closing edge
  //static gsl_matrix* CycleJacobianligand;// column dimension is the number of DOFS; row dimension is the number of cycles\//
  //static gsl_matrix* ClashAvoidingJacobian;
  //Nullspace* nullspaceligand;
  //static SVD* JacobianSVDligand;
  static gsl_matrix* CycleJacobiannocoupling;
  gsl_matrix* CycleJacobianentropy;// column dimension is the number of DOFS; row dimension is the number of cycles\//
  gsl_matrix* CycleJacobianentropycoupling;
  gsl_matrix* CycleJacobianentropynocoupling;
  gsl_matrix* Hessianmatrix_cartesian;
  static gsl_matrix* Massmatrix;
  static gsl_matrix* distancematrix;
  static gsl_matrix* coefficientmatrix;
  Eigenvalue* Entropyeigen;
  static gsl_matrix* HBondJacobian; // column dimension is the number of DOFS; row dimension is the number of cycles\//
  static gsl_matrix* HydrophobicBondJacobian; //column dimension is the number of DOFs; row dimension is the 5 times the number of Hydrophobic bond
  static gsl_matrix* DBondJacobian; //column dimension is the number of DOFs; row dimension is the 5 times the number of Hydrophobic bond
  static Configuration* CycleJacobianOwner;

  static SVD* JacobianSVD;
  static SVD* JacobianSVDnocoupling;
  Nullspace* nullspace;                  ///< Nullspace of hbond of this configuration
  Nullspace* nullspacenocoupling;
  Nullspace* nullspaceHydro;
  
};




#endif

