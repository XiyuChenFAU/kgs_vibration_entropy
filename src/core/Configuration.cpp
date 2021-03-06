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


#include <string>
#include <fstream>
#include <iomanip>
#include <math/math.h>
#include <assert.h>
#include <set>
#include <math/SVDGSL.h>
#include <math/SVDMKL.h>
#include <math/Eigenvalue.h>
#include <gsl/gsl_matrix_double.h>
#include <gsl/gsl_sort_vector_double.h> //provides vector sorting
#include <math/gsl_helpers.h>
#include <math/NullspaceSVD.h>
#include <math/Eigenvalue.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include "Grid.h"

#include "Configuration.h"
#include "Molecule.h"
#include "CTKTimer.h"
#include "Logger.h"
#include "Bond.h"
#include "math3d/primitives.h"
#include "../math3d/primitives.h"
#include "Selection.h"

using namespace std;

double jacobianAndNullspaceTime = 0;
double rigidityTime = 0;

gsl_matrix* Configuration::CycleJacobian = nullptr;
//gsl_matrix* Configuration::CycleJacobianligand = nullptr;
//gsl_matrix* Configuration::CycleJacobianentropy = nullptr;
gsl_matrix* Configuration::CycleJacobiannocoupling = nullptr;
gsl_matrix* Configuration::HBondJacobian = nullptr;
gsl_matrix* Configuration::HydrophobicBondJacobian = nullptr;
gsl_matrix* Configuration::DBondJacobian = nullptr;
Configuration* Configuration::CycleJacobianOwner = nullptr;
SVD* Configuration::JacobianSVD = nullptr;
SVD* Configuration::JacobianSVDnocoupling = nullptr;
//SVD* Configuration::JacobianSVDligand = nullptr;
//gsl_matrix* Configuration::Hessianmatrix_cartesian = nullptr;
gsl_matrix* Configuration::Massmatrix=nullptr;
gsl_matrix* Configuration::distancematrix=nullptr;
gsl_matrix* Configuration::coefficientmatrix=nullptr;

//gsl_matrix* Configuration::ClashAvoidingJacobian = nullptr;
//Nullspace* Configuration::ClashAvoidingNullSpace = nullptr;

Configuration::Configuration(Molecule * mol):
  m_molecule(mol),
  nullspace(nullptr),
  nullspacenocoupling(nullptr),
  CycleJacobianentropy(nullptr),
  CycleJacobianentropycoupling(nullptr),
  CycleJacobianentropynocoupling(nullptr),
  Entropyeigen(nullptr),
  Hessianmatrix_cartesian(nullptr),
  m_parent(nullptr),
  m_dofs_global(nullptr),
  m_treeDepth(0)
{
  assert(m_molecule!=nullptr);

  m_id 										 = 0;
  m_vdwEnergy 						 = 99999;
  m_deltaH                 = 0;
  m_distanceToTarget       = 99999;
  m_paretoFrontDistance 	 = 99999;
  m_distanceToParent       = 0;
  m_distanceToIni          = 0;
  m_maxIndex               = 0;
  m_maxSize                = 0;
  m_maxConstraintViolation = 99999;
  m_numClusters            = 0;
  m_minCollisionFactor     = 0;
  m_usedClashPrevention    = false;
  m_clashFreeDofs          = m_molecule->m_spanningTree->getNumDOFs();

  // Set up DOF-values and set them to 0
  m_dofs = new double[getNumDOFs()];
  //m_sumProjSteps = new double[getNumDOFs()];
  for(int i=0; i<getNumDOFs(); ++i){
    m_dofs[i] = 0;
    //m_sumProjSteps[i]=0;
  }
}

Configuration::Configuration(Configuration* parent_):
    m_molecule(parent_->m_molecule),
    m_parent(parent_),
    m_dofs_global(nullptr),
    nullspace(nullptr),
    m_treeDepth(parent_->m_treeDepth +1)
{
  assert(m_molecule!=nullptr);
  if(m_molecule==NULL){
    std::cerr<<"Configuration(..) - molecule is NULL"<<std::endl;
  }
  m_id                     = -1;//Setting id to -1 by default is important. Check out PoissonSampler2.cpp for example
  m_vdwEnergy              = 99999;
  m_deltaH                 = 0;
  m_distanceToTarget       = 99999;
  m_paretoFrontDistance    = 99999;
  m_distanceToParent       = 0;
  m_distanceToIni          = 0;
  m_maxIndex               = 0;
  m_maxSize                = 0;
  m_maxConstraintViolation = 99999;
  m_numClusters            = 0;
  m_minCollisionFactor     = 0;
  m_usedClashPrevention    = false;
  m_clashFreeDofs          = m_molecule->m_spanningTree->getNumDOFs();

  parent_->m_children.push_back(this);

  // Set up DOF-values and set them to 0
  m_dofs = new double[getNumDOFs()];
  //m_sumProjSteps = new double[getNumDOFs()];
  for(int i=0; i<getNumDOFs(); ++i){
    m_dofs[i] = 0;
    //m_sumProjSteps[i]=0;
  }
}

Configuration::~Configuration(){
  if (m_molecule->m_conf==this)
    m_molecule->setConfiguration(nullptr);

  // Remove DOF-value arrays
  if (m_dofs != nullptr)
    delete[] m_dofs;
  if(m_dofs_global != nullptr)
    delete[] m_dofs_global;
  //if( m_sumProjSteps != nullptr)
  //  delete[] m_sumProjSteps;

//  //m_biggerRBMap.clear();
//  for(auto const& rbPair: m_biggerRBMap)
//    delete rbPair.second;

  //m_sortedRBs.clear();
  if(nullspace)
    delete nullspace;

  //if(nullspaceligand)
      //delete nullspaceligand;

  if(nullspacenocoupling)
      delete nullspacenocoupling;

  if(CycleJacobianentropy)
      gsl_matrix_free(CycleJacobianentropy);

  if(CycleJacobianentropycoupling)
      gsl_matrix_free(CycleJacobianentropycoupling);

  if(CycleJacobianentropynocoupling)
      gsl_matrix_free(CycleJacobianentropynocoupling);

  if(Hessianmatrix_cartesian)
      gsl_matrix_free(Hessianmatrix_cartesian);

  if(Massmatrix)
      gsl_matrix_free(Massmatrix);

  if(Entropyeigen)
      delete Entropyeigen;

  if(distancematrix)
      gsl_matrix_free(distancematrix);

  if(coefficientmatrix)
      gsl_matrix_free(coefficientmatrix);

  if( m_parent!=nullptr )
    m_parent->m_children.remove(this);

  for( auto &child: m_children )
    child->m_parent = nullptr;
}

void Configuration::computeCycleJacobianAndNullSpace() {
  CTKTimer timer;
  timer.Reset();
  double old_time = timer.LastElapsedTime();

  //Compute the Jacobian matrix
  computeJacobians();

  if (JacobianSVD!=nullptr) {
    nullspace = new NullspaceSVD(JacobianSVD);
    nullspace->updateFromMatrix();
  }

  double new_time = timer.ElapsedTime();
  jacobianAndNullspaceTime += new_time - old_time;

//  if(CycleJacobian!=nullptr) {
//    nullspace->performRigidityAnalysis(HBondJacobian);
////    identifyBiggerRigidBodies();
//  }

  double new_time_2 = timer.ElapsedTime();
  rigidityTime += new_time_2 - new_time;
}

void Configuration::rigidityAnalysis() {

  //Checks if Jacobians need update
  computeJacobians();

  if(nullspace==nullptr && nullspace==nullptr){ //update nullspace if necessary
    computeCycleJacobianAndNullSpace();
  }
  CTKTimer timer;
  timer.Reset();
  double old_time = timer.LastElapsedTime();

  if(CycleJacobian!=nullptr) {///identifies rigid/rotatable hbonds and bonds based on set cut-off
    nullspace->performRigidityAnalysis(HBondJacobian,DBondJacobian,HydrophobicBondJacobian);
  }

  int hIdx=0; //indexing for hBonds
  int hydroIdx=0; //indexing for hydrophobic bonds
  int dIdx=0; //indexing for dBonds

  //First, constrain cycle edge bonds
  for (auto const &edge_nca_pair : m_molecule->m_spanningTree->m_cycleAnchorEdges) {

    KinEdge *edge = edge_nca_pair.first;
    KinVertex *common_ancestor = edge_nca_pair.second;

    Bond *bond = edge->getBond();

    if (bond->isDBond()) { /// Distance bond
      //Get corresponding rigidity information
      if (nullspace->isDBondRigid(dIdx++)) {
        bond->rigidified = true;
      }
    }
    else if (bond->isHydrophobicBond()) { /// Hydrophobic bond
      //Get corresponding rigidity information
      if (nullspace->isHydrophobicBondRigid(hydroIdx++)) {
        bond->rigidified = true;
      }
    }
    else{///Hbond or Default
      if (!(bond->isHBond())) {///covalent cycle bonds, e.g. in ligands
        log("debug")<<"Default bond between "<<bond->m_atom1<<", "<<bond->m_atom2<<endl;
      }
      if (nullspace->isHBondRigid(hIdx++)) {
        bond->rigidified = true;
      }
    }
  }
  //Second, constrain covalent edge bonds
  for (auto const &edge : m_molecule->m_spanningTree->m_edges) {
      int dof_id = edge->getDOF()->getCycleIndex();
      if (dof_id!=-1) { // this edge is a cycle DOF, dof_id is the corresponding column!
        if( nullspace->isCovBondRigid(dof_id) ) {
          edge->getBond()->rigidified = true;
        }
      }
  }
  double new_time = timer.ElapsedTime();
  rigidityTime += new_time - old_time;
}

void Configuration::deleteNullspace(){
  if(nullspace) {
    delete nullspace;
    nullspace = nullptr;
  }
}

//void Configuration::identifyBiggerRigidBodies(){
//
//  /// Now, all dihedrals and hBonds that are fixed have the set flag constrained = true!
//  m_biggerRBMap.clear();
//  m_sortedRBs.clear();
//
//  readBiggerSet();
//
//  /// Now, we have the map of bigger rigid bodies with atoms
//  /// We will order the atoms by ID for a nicer display
//  m_numClusters = 0;
//  m_maxSize = 0;
//  m_maxIndex=0;
//
////	cout<<"Map of bigger rigid bodies:"<<endl;
//  auto it = m_biggerRBMap.begin();
//  while( it != m_biggerRBMap.end() ){
//    m_sortedRBs.push_back( make_pair( (it->second)->size(), it->first ));
//    m_numClusters++;
//    int num = 0;
//
//    ///Sorting
//    vector<Atom*>::iterator sit = it->second->Atoms.begin();
//    vector<Atom*>::iterator eit = it->second->Atoms.end();
//    sort(sit,eit,Atom::compare);
//
////		cout<<"Bigger rb with ID "<<it->first<<" contains atoms with IDs: "<<endl;
//    auto ait = it->second->Atoms.begin();
//    while( ait != it->second->Atoms.end() ){
//      //cout<<(*ait)->m_id<<endl;
//      ait++;
//      num++;
//    }
//
//    if( num > m_maxSize ){
//      m_maxSize = num;
//      m_maxIndex = it->first;
//    }
//    ++it;
//  }
//
//  vector< pair<int, unsigned int> >::iterator vsit = m_sortedRBs.begin();
//  vector< pair<int, unsigned int> >::iterator veit = m_sortedRBs.end();
//
//  sort(vsit, veit,compareSize);
//
//}

////---------------------------------------------------------
//bool Configuration::compareSize(pair<int, unsigned int> firstEntry, pair<int, unsigned int> secondEntry) {
//  if( firstEntry.first > secondEntry.first )
//    return true;
//  if( firstEntry.first < secondEntry.first )
//    return false;
//
//  return (firstEntry.second < secondEntry.second);
//}

//void Configuration::readBiggerSet(){
//
//  //Create disjoint set
//  DisjointSets ds(m_molecule->getAtoms()[m_molecule->size() - 1]->getId() + 1); //Assumes the last atom has the highest id.
//
//  //For each atom, a1, with exactly one cov neighbor, a2, call Union(a1,a2)
//  for (int i=0;i< m_molecule->size();i++){
//    Atom* atom = m_molecule->getAtoms()[i];
//    if(atom->Cov_neighbor_list.size()==1 && atom->Hbond_neighbor_list.size()==0){
//      ds.Union(atom->getId(), atom->Cov_neighbor_list[0]->getId());
//    }
//  }
//
//
//  //For each fixed or rigidified bond (a1,a2) call Union(a1,a2)
//  for (auto const& bond: m_molecule->getCovBonds()){
//    //First, simply check if bond is rigidified
//    if( bond->rigidified || bond->m_bars == 6){
//      ds.Union(bond->Atom1->getId(), bond->m_atom2->getId());
//    }
//  }
//
//  //Also, do the same thing for the hydrogen bonds insert the h-bonds at the correct place
//  for(auto const& bond: m_molecule->getHBonds()){
//    if( bond->rigidified ){
//      ds.Union(bond->Atom1->getId(), bond->m_atom2->getId());
//    }
//  }
//
//  //All disjoint sets have been united if they are rigidified or bonded fix!
//  //We now just add the covalent neighbors to have the representation with atoms in multiple rbs!
//
//  int c=0;
//  map<int,int> idMap;//Maps atom id's to rigid body id's for use in the DS structure.
//
//  //Map the set-ID's (first map entry) to RB-ID's (second map entry) and add bonded atoms to RBs.
//  for (int i=0;i< m_molecule->size();i++){
//    Atom* atom = m_molecule->getAtoms()[i];
//
//    //Map the set-id to the RB-id
//    int set_id = ds.FindSet(atom->getId());
//    int body_id;
//    if(idMap.find(set_id)!=idMap.end())
//      body_id = idMap.find(set_id)->second;
//    else {
//      body_id = c++;
//      idMap[set_id] = body_id;
//    }
//    //If the set containing a1 is not a rigid body: create one
//    if ( m_biggerRBMap.find(body_id) == m_biggerRBMap.end() ) {
//      Rigidbody* new_rb = new Rigidbody(body_id);
//      m_biggerRBMap[body_id] = new_rb;
//    }
//    Rigidbody* rb = m_biggerRBMap[body_id];
//    if (!rb->containsAtom(atom)){
//      rb->Atoms.push_back(atom);
//      atom->setBiggerRigidbody(rb);
//      //For graphical display, we assign this rb-id to the rbColumn variable of the atom
//      atom->setBFactor( float( rb->id() ) );
//    }
//
//  }
//
//}

//------------------------------------------------------
void Configuration::writeQToBfactor(){
  for (auto const& edge: m_molecule->m_spanningTree->m_edges) {
    if(edge->EndVertex->m_rigidbody == NULL)
      continue; //global dofs
    float value = m_dofs[edge->getDOF()->getIndex() ];
    for (auto const& atom : edge->EndVertex->m_rigidbody->Atoms ){
      atom->setBFactor(value);
    }
  }
}

//------------------------------------------------------
void Configuration::updateGlobalTorsions(){
  if(m_dofs_global != nullptr)
    return;

  updateMolecule();
  m_dofs_global = new double[getNumDOFs()];
  for(int i=0; i<getNumDOFs(); ++i){
    m_dofs_global[i] = m_molecule->m_spanningTree->getDOF(i)->getGlobalValue();
  }
  //for (vector<KinEdge*>::iterator itr= m_molecule->m_spanningTree->m_edges.begin(); itr!= m_molecule->m_spanningTree->m_edges.end(); ++itr) {
  //  KinEdge* pEdge = (*itr);
  //  int dof_id = pEdge->DOF_id;
  //  if (dof_id==-1)
  //    continue;
  //  m_dofs_global[dof_id] = pEdge->getBond()->getTorsion();
  //}
}

double Configuration::getGlobalTorsion( int i ) {
  assert(i>=0 && i<getNumDOFs());
  updateGlobalTorsions();
  return m_dofs_global[i];
}

double* Configuration::getGlobalTorsions() {
  updateGlobalTorsions();
  return m_dofs_global;
}

unsigned int Configuration::getNumDOFs() const {
  return m_molecule->m_spanningTree->getNumDOFs();
}

Configuration* Configuration::clone() const {

  Configuration* ret;

  if (m_parent) {
    ret = new Configuration(m_parent);
  }else{
    ret = new Configuration(m_molecule);
  }

  ret->m_id = m_id;
  ret->m_vdwEnergy = m_vdwEnergy;

  for(int i=0; i <getNumDOFs(); ++i){
    ret->m_dofs[i]        = m_dofs[i];
  }
  if(m_dofs_global!=nullptr){
    ret->m_dofs_global = new double[getNumDOFs()];
    for(int i=0; i<getNumDOFs(); ++i) {
      ret->m_dofs_global[i]  = m_dofs_global[i];
    }
  }else{
    ret->m_dofs_global = nullptr;
  }
  return ret;
}

void Configuration::Print () {
  for (int i=0; i < getNumDOFs(); ++i){
    cout << "Relative: " << m_dofs[i] << " ";
    cout << "Global: " << m_dofs_global[i] << " ";
  }
  cout << endl;
}

bool Configuration::checknocoupling() {
    for (std::pair<KinEdge*,KinVertex*>& edge_vertex_pair: m_molecule->m_spanningTree->m_cycleAnchorEdges) {
        KinEdge *edge_ptr = edge_vertex_pair.first;

        KinVertex *vertex1 = edge_ptr->StartVertex;
        KinVertex *vertex2 = edge_ptr->EndVertex;

        int vertex1ligand = 0;
        int vertex2ligand = 0;
        if (vertex1->isligand()) { vertex1ligand = 1; }
        if (vertex2->isligand()) { vertex2ligand = 1; }


        if (vertex1ligand + vertex2ligand == 1) { return false; }
    }

    return true;
}


void Configuration::computeJacobiansnocoupling() {
//  log("debug")<<"computeJacobians"<<endl;

    computeJacobians();

    int row_num=CycleJacobian->size1;
    int col_num=CycleJacobian->size2;

    if(CycleJacobiannocoupling==nullptr){
        CycleJacobiannocoupling = gsl_matrix_calloc(row_num,col_num);
        JacobianSVDnocoupling = SVD::createSVD(CycleJacobiannocoupling);//new SVDMKL(CycleJacobiannocoupling);
    }else if(CycleJacobiannocoupling->size1==row_num && CycleJacobiannocoupling->size2==col_num){
        gsl_matrix_set_zero(CycleJacobiannocoupling);
    }else{
        gsl_matrix_free(CycleJacobiannocoupling);
        delete JacobianSVDnocoupling;
        CycleJacobiannocoupling = gsl_matrix_calloc(row_num,col_num);
        JacobianSVDnocoupling = SVD::createSVD(CycleJacobiannocoupling);//new SVDMKL(CycleJacobiannocoupling);
    }

    gsl_matrix_memcpy(CycleJacobiannocoupling, CycleJacobian);

    // for each cycle, fill in the Jacobian entries
    int i=0; // cycleAnchorIndices, all constraints together
    for (std::pair<KinEdge*,KinVertex*>& edge_vertex_pair: m_molecule->m_spanningTree->m_cycleAnchorEdges)
    {
        // get end-effectors
        KinEdge* edge_ptr = edge_vertex_pair.first;
        Bond * bond_ptr = edge_ptr->getBond();

        //End-effectors and their positions, corresponds to a and b
        Atom* atom1 = bond_ptr->m_atom1;
        Atom* atom2 = bond_ptr->m_atom2;

        KinVertex* vertex1 = edge_ptr->StartVertex;
        KinVertex* vertex2 = edge_ptr->EndVertex;
        if(find(vertex1->m_rigidbody->Atoms.begin(),vertex1->m_rigidbody->Atoms.end(),atom1) == vertex1->m_rigidbody->Atoms.end()){
            vertex1=edge_ptr->EndVertex;
            vertex2=edge_ptr->StartVertex;
        }

        int vertex1ligand=0;
        int vertex2ligand=0;
        if(vertex1->isligand()){vertex1ligand=1;}
        if(vertex2->isligand()){vertex2ligand=1;}



        if ( vertex1ligand+vertex2ligand==1 ) {
            gsl_vector *setzero =gsl_vector_calloc(col_num);
            if(bond_ptr->isHydrophobicBond()){
                gsl_matrix_set_row(CycleJacobiannocoupling, i + 0, setzero);
            }
            else {
                /// These three constraints are equal for distance and hydrogen bond
                gsl_matrix_set_row(CycleJacobiannocoupling, i + 0, setzero); //set: Matrix, row, column, what to set
                gsl_matrix_set_row(CycleJacobiannocoupling, i + 1, setzero);
                gsl_matrix_set_row(CycleJacobiannocoupling, i + 2, setzero);

                if (!bond_ptr->isDBond()) {//Dbonds
                    gsl_matrix_set_row(CycleJacobiannocoupling, i + 3, setzero);
                    gsl_matrix_set_row(CycleJacobiannocoupling, i + 4, setzero);
                }
            }
            gsl_vector_free(setzero);
        }

        if(bond_ptr->isDBond()){//3 constraints, 3 rel. DOF
            i+=3;
        }
        else if(bond_ptr->isHydrophobicBond()){//hydrophobic bond, 1 constraint, 5 rel. DOF
            i += 1;
        }
        else{ //Hbonds or default; 5 constraints, 1 rel. DOF
            i+=5;
        }
    }
}


void Configuration::computeJacobians() {
//  log("debug")<<"computeJacobians"<<endl;
  if(CycleJacobianOwner==this) return;
  CycleJacobianOwner = this;

  updateMolecule();

  // No cycles
  if(m_molecule->m_spanningTree->m_cycleAnchorEdges.size() == 0) {
    CycleJacobian = nullptr; //TODO: Memory leak
    return;
  }
  //ToDo: update this for rigidity analysis with D-bonds and hydrophobic bonds
  // ToDo: either separate matrices (DBondJacobian), or new number of rows
  int hConstraint_row_num = 0; ///h-bond motion Jacobian
  int hydroConstraint_row_num = 0; ///hydrophobic motion Jacobian
  int dConstraint_row_num=0; /// d-bond motion Jacobian
  int row_num = 0; /// constraint Jacobian, contains all constraint bonds

  for(auto const& edge: m_molecule->m_spanningTree->m_cycleAnchorEdges){
    if(edge.first->getBond()->isHBond()){
      row_num += 5;
      hConstraint_row_num += 1;
    }
    else if(edge.first->getBond()->isDBond()) {
      row_num += 3;
      dConstraint_row_num += 3;
    }
    else if(edge.first->getBond()->isHydrophobicBond()) {
      row_num += 1;
      hydroConstraint_row_num += 5;
    }
    else {  // default (happens for cycle-edges from covalent bonds, within hBondJacobian
      log("debug")<<"Configuration::computeJacobians: using default 5-valued constraint for edge between"<<endl;
      log("debug") << "> " << edge.first << endl;
      row_num += 5;
      hConstraint_row_num += 1;
    }
  }

  int col_num = m_molecule->m_spanningTree->getNumCycleDOFs(); // number of DOFs in cycles

  if(CycleJacobian==nullptr){
    CycleJacobian = gsl_matrix_calloc(row_num,col_num);
    JacobianSVD = SVD::createSVD(CycleJacobian);//new SVDMKL(CycleJacobian);
  }else if(CycleJacobian->size1==row_num && CycleJacobian->size2==col_num){
    gsl_matrix_set_zero(CycleJacobian);
  }else{
    gsl_matrix_free(CycleJacobian);
    delete JacobianSVD;
    CycleJacobian = gsl_matrix_calloc(row_num,col_num);
    JacobianSVD = SVD::createSVD(CycleJacobian);//new SVDMKL(CycleJacobian);
  }

  ///HBond Jacobian
  if(hConstraint_row_num != 0) {
    if (HBondJacobian == nullptr) {
      HBondJacobian = gsl_matrix_calloc(hConstraint_row_num, col_num);

    } else if (HBondJacobian->size1 == hConstraint_row_num && HBondJacobian->size2 == col_num) {
      gsl_matrix_set_zero(HBondJacobian);

    } else {
      gsl_matrix_free(HBondJacobian);
      HBondJacobian = gsl_matrix_calloc(hConstraint_row_num, col_num);
    }
  }

  ///HydrophobicBond Jacobian
  if(hydroConstraint_row_num != 0) {
    if (HydrophobicBondJacobian == nullptr) {
      HydrophobicBondJacobian = gsl_matrix_calloc(hydroConstraint_row_num, col_num);

    } else if (HydrophobicBondJacobian->size1 == hydroConstraint_row_num && HydrophobicBondJacobian->size2 == col_num) {
      gsl_matrix_set_zero(HydrophobicBondJacobian);

    } else {
      gsl_matrix_free(HydrophobicBondJacobian);
      HydrophobicBondJacobian = gsl_matrix_calloc(hydroConstraint_row_num, col_num);
    }
  }

  ///Dbond Jacobian
  if(dConstraint_row_num != 0) {
    if (DBondJacobian == nullptr) {
      DBondJacobian = gsl_matrix_calloc(dConstraint_row_num, col_num);

    } else if (DBondJacobian->size1 == dConstraint_row_num && DBondJacobian->size2 == col_num) {
      gsl_matrix_set_zero(DBondJacobian);

    } else {
      gsl_matrix_free(DBondJacobian);
      DBondJacobian = gsl_matrix_calloc(dConstraint_row_num, col_num);
    }
  }

  // for each cycle, fill in the Jacobian entries
  int i=0; // cycleAnchorIndices, all constraints together
  int hbidx=0; // hydrogen bond index, used for HBondJacobian (includes default bonds)
  int hydroidx=0; //hydrophobic index, used for HydrophobicJacobian
  int didx=0;// distance bond index, used for DBondJacobian
  for (std::pair<KinEdge*,KinVertex*>& edge_vertex_pair: m_molecule->m_spanningTree->m_cycleAnchorEdges)
  {
    // get end-effectors
    KinEdge* edge_ptr = edge_vertex_pair.first;
    KinVertex* common_ancestor = edge_vertex_pair.second;
    Bond * bond_ptr = edge_ptr->getBond();
//    int bondIndex = bond_ptr

    //End-effectors and their positions, corresponds to a and b
    Atom* atom1 = bond_ptr->m_atom1;
    Atom* atom2 = bond_ptr->m_atom2;
    Coordinate p1 = atom1->m_position; //end-effector, position 1
    Coordinate p2 = atom2->m_position; //end-effector, position 2
    log("debug")<<"Jacobian row "<<i<<", atom 1: "<<atom1->getId()<<", atom 2: "<<atom2->getId()<<endl;

    KinVertex* vertex1 = edge_ptr->StartVertex;
    KinVertex* vertex2 = edge_ptr->EndVertex;
    if(find(vertex1->m_rigidbody->Atoms.begin(),vertex1->m_rigidbody->Atoms.end(),atom1) == vertex1->m_rigidbody->Atoms.end()){
      vertex1=edge_ptr->EndVertex;
      vertex2=edge_ptr->StartVertex;
    }

    //Use the covalently bonded atoms to find A-1 and B-1
    Atom* atom1_prev = atom1->Cov_neighbor_list[0] == atom2 ? atom1->Cov_neighbor_list[1] : atom1->Cov_neighbor_list[0];
    Atom* atom2_prev = atom2->Cov_neighbor_list[0] == atom1 ? atom2->Cov_neighbor_list[1] : atom2->Cov_neighbor_list[0];

     //Make sure a1 is the covalent neighbor of Atom1 with lexicographically smallest name
    for(vector<Atom*>::iterator ait = atom1->Cov_neighbor_list.begin(); ait!=atom1->Cov_neighbor_list.end(); ait++){
        Atom* a = *ait;
        if(a!=atom2 && a->getName()<atom1_prev->getName())
          atom1_prev = a;
    }
    //Make sure a4 is the covalent neighbor of m_atom2 with lexicographically smallest name
    for(vector<Atom*>::iterator ait = atom2->Cov_neighbor_list.begin(); ait!=atom2->Cov_neighbor_list.end(); ait++){
        Atom* a = *ait;
        if(a!=atom1 && a->getName()<atom2_prev->getName())
          atom2_prev = a;
    }

    if(!atom1 || !atom1_prev || !atom2 || !atom2_prev){
      cerr<<"Not all neighboring atoms for non-redundant Jacobian defined, quitting!"<<endl;
      exit(-1);
    }

    Coordinate p1_prev = atom1_prev->m_position;
    Coordinate p2_prev = atom2_prev->m_position;
    //Now we have all atoms we need --> Calculate Jacobian

    //Normal between atoms, used for hydrophobic constraint Tangent to normal
    Math3D::Vector3 constraintNormal = p2-p1;
    constraintNormal.getNormalized(constraintNormal);

      /// Workflow Hydrophobics:
      // 1. compute constraint tangents t1 und t2, orthogonal to constraint normal
      // 2. 2 translational entries mit t1 und t2
      // 3. 3 rotational entries, jacobianEntryRot1, jacobianEntryRot2, hBondEntry
    Math3D::Vector3 t1(constraintNormal.x, constraintNormal.y, -1.0*(constraintNormal.x*constraintNormal.x+constraintNormal.y*constraintNormal.y)/constraintNormal.z);
    Math3D::Vector3 t2 = cross(t1,constraintNormal);
    t1.getNormalized(t1);
    t2.getNormalized(t2);

    // trace back until the common ancestor from vertex1
    while ( vertex1 != common_ancestor ) {
      KinVertex* parent = vertex1->m_parent;
      KinEdge* p_edge = parent->findEdge(vertex1);

      //int dof_id = p_edge->Cycle_DOF_id;
      int dof_id = p_edge->getDOF()->getCycleIndex();
      if (dof_id!=-1) { // this edge is a DOF

          Math3D::Vector3 derivativeP1 = p_edge->getDOF()->getDerivative(p1);
          Math3D::Vector3 derivativeP2 = p_edge->getDOF()->getDerivative(p2);
          Math3D::Vector3 jacobianEntryTrans=0.5*(derivativeP1 + derivativeP2);
          Math3D::Vector3 derivativeP1_prev = p_edge->getDOF()->getDerivative(p1_prev);
          double jacobianEntryRot1 = dot((p2 - p1), (derivativeP1 - derivativeP1_prev));
          double jacobianEntryRot2 = dot((p2 - p2_prev), (derivativeP1 - derivativeP2));
          double hBondEntry = dot((p1_prev - p2_prev), (derivativeP1_prev));

        //separate entries for Hydrophobic bonds; see clash avoiding Jacobian
        if(bond_ptr->isHydrophobicBond()){
          double jacobianEntry1D = dot(constraintNormal, derivativeP1);
          double HydroTranslationEntry1= dot(t1,derivativeP1);
          double HydroTranslationEntry2= dot(t2,derivativeP1);

          gsl_matrix_set(CycleJacobian,i + 0, dof_id, jacobianEntry1D); //set: Matrix, row, column, what to set
          gsl_matrix_set(HydrophobicBondJacobian, hydroidx + 0,dof_id,HydroTranslationEntry1);
          gsl_matrix_set(HydrophobicBondJacobian, hydroidx + 1,dof_id,HydroTranslationEntry2);
          gsl_matrix_set(HydrophobicBondJacobian, hydroidx + 2,dof_id,jacobianEntryRot1);
          gsl_matrix_set(HydrophobicBondJacobian, hydroidx + 3,dof_id,jacobianEntryRot2);
          gsl_matrix_set(HydrophobicBondJacobian, hydroidx + 4,dof_id,hBondEntry);
        }
        else {
            /// These three constraints are equal for distance and hydrogen bond
            gsl_matrix_set(CycleJacobian, i + 0, dof_id, jacobianEntryTrans.x); //set: Matrix, row, column, what to set
            gsl_matrix_set(CycleJacobian, i + 1, dof_id, jacobianEntryTrans.y);
            gsl_matrix_set(CycleJacobian, i + 2, dof_id, jacobianEntryTrans.z);

            if (bond_ptr->isDBond()) {//Dbonds
              gsl_matrix_set(DBondJacobian, didx + 0, dof_id, jacobianEntryRot1);
              gsl_matrix_set(DBondJacobian, didx + 1, dof_id, jacobianEntryRot2);
              gsl_matrix_set(DBondJacobian, didx + 2, dof_id, hBondEntry);
            }
            else{ //HBonds and default
              gsl_matrix_set(CycleJacobian, i + 3, dof_id, jacobianEntryRot1);
              gsl_matrix_set(CycleJacobian, i + 4, dof_id, jacobianEntryRot2);
              ///Matrix to check hBond Rotation
              gsl_matrix_set(HBondJacobian, hbidx, dof_id, hBondEntry);
            }
        }

      }
      vertex1 = parent;
    }
    // trace back until the common ancestor from vertex2
    while ( vertex2 != common_ancestor ) {
      KinVertex *parent = vertex2->m_parent;
      KinEdge *p_edge = parent->findEdge(vertex2);

//			int dof_id = p_edge->Cycle_DOF_id;
      int dof_id = p_edge->getDOF()->getCycleIndex();
      if (dof_id != -1) { // this edge is a DOF

          Math3D::Vector3 derivativeP1 = p_edge->getDOF()->getDerivative(p1);
          Math3D::Vector3 derivativeP2 = p_edge->getDOF()->getDerivative(p2);
          Math3D::Vector3 jacobianEntryTrans = -0.5 * (derivativeP1 + derivativeP2);
          Math3D::Vector3 derivativeP2_prev = p_edge->getDOF()->getDerivative(p2_prev);
          double jacobianEntryRot1 = dot((p1 - p1_prev), (derivativeP2 - derivativeP1));
          double jacobianEntryRot2 = dot((p1 - p2), (derivativeP2 - derivativeP2_prev));
          double hBondEntry = dot((p1_prev - p2_prev), (-derivativeP2_prev));

        //separate entries for Hydrophobic bonds; see clash avoiding Jacobian
        if (bond_ptr->isHydrophobicBond()) {
            double jacobianEntry1D = -dot(constraintNormal, derivativeP2);//opposite sign to other branch
            double HydroTranslationEntry1= -dot(t1,derivativeP2);
            double HydroTranslationEntry2= -dot(t2,derivativeP2);

            gsl_matrix_set(CycleJacobian, i + 0, dof_id, jacobianEntry1D); //set: Matrix, row, column, what to set
            gsl_matrix_set(HydrophobicBondJacobian, hydroidx + 0,dof_id,HydroTranslationEntry1);
            gsl_matrix_set(HydrophobicBondJacobian, hydroidx + 1,dof_id,HydroTranslationEntry2);
            gsl_matrix_set(HydrophobicBondJacobian, hydroidx + 2,dof_id,jacobianEntryRot1);
            gsl_matrix_set(HydrophobicBondJacobian, hydroidx + 3,dof_id,jacobianEntryRot2);
            gsl_matrix_set(HydrophobicBondJacobian, hydroidx + 4,dof_id,hBondEntry);
        }
        else {//DBond or HBond or Default
          /// These three constraints are equal for distance and hydrogen bond
          gsl_matrix_set(CycleJacobian, i + 0, dof_id, jacobianEntryTrans.x); //set: Matrix, row, column, what to set
          gsl_matrix_set(CycleJacobian, i + 1, dof_id, jacobianEntryTrans.y);
          gsl_matrix_set(CycleJacobian, i + 2, dof_id, jacobianEntryTrans.z);

          if (bond_ptr->isDBond()) {//Dbonds
            gsl_matrix_set(DBondJacobian, didx + 0, dof_id, jacobianEntryRot1);
            gsl_matrix_set(DBondJacobian, didx + 1, dof_id, jacobianEntryRot2);
            gsl_matrix_set(DBondJacobian, didx + 2, dof_id, hBondEntry);
          }
          else{ //HBonds and default
            gsl_matrix_set(CycleJacobian, i + 3, dof_id, jacobianEntryRot1);
            gsl_matrix_set(CycleJacobian, i + 4, dof_id, jacobianEntryRot2);
            ///Matrix to check hBond Rotation
            gsl_matrix_set(HBondJacobian, hbidx, dof_id, hBondEntry);
          }
        }
      }
      vertex2 = parent;
    }

    if(bond_ptr->isDBond()){//3 constraints, 3 rel. DOF
      i+=3;
      didx+=3;
    }
    else if(bond_ptr->isHydrophobicBond()){//hydrophobic bond, 1 constraint, 5 rel. DOF
      i += 1;
      hydroidx+=5;
    }
    else{ //Hbonds or default; 5 constraints, 1 rel. DOF
      i+=5;
      hbidx++;
    }
  }
}


int Configuration::getNumRigidDihedralsligand() {
    Nullspace* Nulls = getNullspace();
    gsl_vector* rigiddof = Nulls->buildDofRigid();

    for(int i=0;i<rigiddof->size;i++){
        if(gsl_vector_get(rigiddof, i)>0.9){
            if (m_molecule->m_spanningTree->getCycleDOF(i)->isDOFligand()) {
                numligandRigidDihedrals++;
            }
        }
    }

    return numligandRigidDihedrals;

}


//------------------------------------------------------------
//We compute another Jacobian that also considers motion along clash normal directions as a constraint
//--> allowed motions will not move clashing atoms further into each other

/*
void Configuration::ComputeClashAvoidingJacobianAndNullSpace (std::map< std::pair<Atom*,Atom*>,int > allCollisions,bool firstTime, bool projectConstraints) {
//	CTKTimer timer;
//	timer.Reset();
//	double old_time = timer.LastElapsedTime();

  //First, recompute the Jacobian for the current configuration
  //As CycleJacobian is static, the current Jacobian might not correspond to the current configuration
  if(firstTime)//only have to do this in the first clash-prevention trial, after the Jacobian is the same
    computeJacobians();
  //Add the clash constraints
  computeClashAvoidingJacobian(allCollisions,projectConstraints);

  if (JacobianSVD != nullptr) {
    // CycleNullSpace needs to be deleted to free memory.
    if( ClashAvoidingNullSpace!=nullptr ) {
      delete ClashAvoidingNullSpace;
    }
    ClashAvoidingNullSpace = new Nullspace(JacobianSVD);
    ClashAvoidingNullSpace->updateFromMatrix();
  }

//	double new_time = timer.ElapsedTime();
//	clashJacobianTime += new_time - old_time;
}

//---------------------------------------------------------
void Configuration::computeClashAvoidingJacobian (std::map< std::pair<Atom*,Atom*>,int > allCollisions, bool projectConstraints) {

  updateMolecule();

  //The clash Jacobian is the regular Jacobian's constraints, plus one constraint per pair of clashing atoms
  int numCollisions = allCollisions.size();
  int rowNum, colNum;

  //Clashes can occur also for previously free dihedrals!
  //Therefore, we use the full set of dihedrals to determine this matrix!

  if( CycleJacobian != nullptr){
    rowNum = CycleJacobian->size1 + numCollisions;
    colNum = m_molecule->m_spanningTree->getNumDOFs();
  }
  else{
    rowNum = numCollisions;
    colNum = m_molecule->m_spanningTree->getNumDOFs();
  }

  if(ClashAvoidingJacobian==nullptr){
    ClashAvoidingJacobian = gsl_matrix_calloc(rowNum,colNum);
    JacobianSVD = SVD::createSVD(ClashAvoidingJacobian);//new SVDMKL(ClashAvoidingJacobian);
  }else if(ClashAvoidingJacobian->size1==rowNum && ClashAvoidingJacobian->size2==colNum){
    gsl_matrix_set_zero(ClashAvoidingJacobian);
  }else{
    gsl_matrix_free(ClashAvoidingJacobian);
    ClashAvoidingJacobian = gsl_matrix_calloc(rowNum,colNum);
    delete JacobianSVD;
    JacobianSVD = SVD::createSVD(ClashAvoidingJacobian);//new SVDMKL(ClashAvoidingJacobian);
  }

  //Convert the cycle Jacobian to a full Jacobian
  //Columns correspond to cycle_dof_ids
  if(projectConstraints){
//		for (vector<KinEdge*>::iterator eit=m_molecule->m_spanningTree->m_edges.begin(); eit!=m_molecule->m_spanningTree->m_edges.end(); ++eit) {
    for (auto const& edge: m_molecule->m_spanningTree->m_edges){
      int dof_id = edge->getDOF()->getIndex();
      int cycle_dof_id = edge->getDOF()->getCycleIndex();
      if ( cycle_dof_id!=-1 ) {
        for( int i=0; i!=CycleJacobian->size1; i++){
          gsl_matrix_set(ClashAvoidingJacobian, i, dof_id, gsl_matrix_get(CycleJacobian,i,cycle_dof_id));
        }
      }
    }
  } // else: we maintain a zero-valued Jacobian to not consider the constraints (only for testing of constraint limitations)

  int i=CycleJacobian->size1;//start entries after all cycles

  //Quick trick for plotting the clash-cycle network
//	int consCounter = 1;
//	vector<Atom*>::iterator ait;
//	for(ait=protein->Atom_list.begin(); ait != protein->Atom_list.end(); ait++){
//		(*ait)->m_assignedBiggerRB_id = consCounter;
//	}

  for (std::map< std::pair<Atom*,Atom*>,int >::const_iterator mit=allCollisions.begin(); mit!=allCollisions.end(); ++mit) {
//		consCounter++;
    Atom* atom1 = mit->first.first;
    Atom* atom2 = mit->first.second;
    log("planner") << "Using clash constraint for atoms: "<<atom1->getId() << " " << atom2->getId() << endl;

    Coordinate p1 = atom1->m_position; //end-effector, position 1
    Coordinate p2 = atom2->m_position; //end-effector, position 2

    Math3D::Vector3 clashNormal = p2-p1;
    clashNormal.getNormalized(clashNormal);

    //Vertices
    KinVertex* vertex1 = atom1->getRigidbody()->getVertex();
    KinVertex* vertex2 = atom2->getRigidbody()->getVertex();
    KinVertex* common_ancestor = m_molecule->m_spanningTree->findCommonAncestor(vertex1, vertex2);

    // trace back until the common ancestor from vertex1
    while ( vertex1 != common_ancestor ) {
      KinVertex* parent = vertex1->m_parent;
      KinEdge* p_edge = parent->findEdge(vertex1);

      int dof_id = p_edge->getDOF()->getIndex();
      if (dof_id!=-1) { // this edge is a DOF

//				Math3D::Vector3 derivativeP1 = ComputeJacobianEntry(p_edge->getBond()->Atom1->m_position,p_edge->getBond()->m_atom2->m_position,p1);
        Math3D::Vector3 derivativeP1 = p_edge->getDOF()->getDerivative(p1);
        double jacobianEntryClash = dot(clashNormal, derivativeP1);

        gsl_matrix_set(ClashAvoidingJacobian,i,dof_id,jacobianEntryClash); //set: Matrix, row, column, what to set
//				log("planner")<<"Setting clash entry on left branch at "<<dof_id<<endl;
      }

      //Quick trick for plotting the clash cycles (don't use in real sampling)
//			for (ait=vertex1->m_rigidbody->Atoms.begin(); ait !=vertex1->m_rigidbody->Atoms.end(); ait++){
//				Atom* atom = (*ait);
//				atom->m_assignedBiggerRB_id = consCounter;
//			}
      vertex1 = parent;
    }

    // trace back until the common ancestor from vertex2
    while ( vertex2 != common_ancestor ) {
      KinVertex* parent = vertex2->m_parent;
      KinEdge* p_edge = parent->findEdge(vertex2);

      int dof_id = p_edge->getDOF()->getCycleIndex();
      if (dof_id!=-1) { // this edge is a DOF

//				Math3D::Vector3 derivativeP2 = ComputeJacobianEntry(
//            p_edge->getBond()->Atom1->m_position,
//            p_edge->getBond()->m_atom2->m_position,
//            p2); //b
        Math3D::Vector3 derivativeP2 = p_edge->getDOF()->getDerivative(p2);
        double jacobianEntryClash = - dot(clashNormal, derivativeP2);

        gsl_matrix_set(ClashAvoidingJacobian,i,dof_id,jacobianEntryClash); //set: Matrix, row, column, what to set
//				log("planner")<<"Setting clash entry on right branch at "<<dof_id<<endl;
      }

//			//Quick trick for plotting the clash cycles (don't use in real sampling)
//			for (ait=vertex2->m_rigidbody->Atoms.begin(); ait !=vertex2->m_rigidbody->Atoms.end(); ait++){
//				Atom* atom = (*ait);
//				atom->m_assignedBiggerRB_id = consCounter;
//			}
      vertex2 = parent;
    }
    ++i;
  }
}

 */

/// This function converts a vector with all dof entries to a vector with cycle dofs only, by extracting correct entries
void Configuration::convertAllDofsToCycleDofs( gsl_vector *cycleDofs, gsl_vector *allDofs){

  Molecule* M = getMolecule();

  for (auto const& edge: M->m_spanningTree->m_edges){
    int dof_id = edge->getDOF()->getIndex();
    int cycle_dof_id = edge->getDOF()->getCycleIndex();
    if ( cycle_dof_id!=-1 ) {
      gsl_vector_set(cycleDofs,cycle_dof_id,gsl_vector_get(allDofs,dof_id));
    }
  }

}

void Configuration::convertCycleDofsToAllDofs( gsl_vector *allDofsAfter, gsl_vector *cycleDofs, gsl_vector *allDofsBefore){

  Molecule* M = getMolecule();

  // Convert back to full length DOFs vector
  for( auto const& edge: M->m_spanningTree->m_edges){
    int dof_id = edge->getDOF()->getIndex();
    int cycle_dof_id = edge->getDOF()->getCycleIndex();
    if ( cycle_dof_id!=-1 ) {
      gsl_vector_set(allDofsAfter,dof_id,gsl_vector_get(cycleDofs,cycle_dof_id));
    }
    else if ( dof_id!=-1 ) {
      if (allDofsBefore == nullptr)
        gsl_vector_set(allDofsAfter,dof_id,0);
      else
        gsl_vector_set(allDofsAfter,dof_id,gsl_vector_get(allDofsBefore,dof_id));
    }
  }
}

void Configuration::projectOnCycleNullSpace (gsl_vector *to_project, gsl_vector *after_project) {
  Nullspace* N = getNullspace();
  //Since we're only using this for converting Cycle-DOF ids the mol doesnt have to be updated
  Molecule* M = getMolecule();

  if(N==nullptr){
    gsl_vector_memcpy(after_project, to_project);
    return;
  }

  if( to_project->size > N->getNumDOFs() ) {
    // The input vectors contain all DOFs, however, the null space only contains DOFs in cycles.
    // Convert the DOFs in the input vectors to DOFs in cycles.

    gsl_vector *to_proj_short = gsl_vector_calloc(N->getNumDOFs());
    convertAllDofsToCycleDofs(to_proj_short, to_project);

    // Project onto the null space
    double normBefore = gsl_vector_length(to_proj_short);
    gsl_vector *after_proj_short = gsl_vector_calloc(N->getNumDOFs());
    N->projectOnNullSpace(to_proj_short, after_proj_short);
    double normAfter = gsl_vector_length(after_proj_short);

    //Scale projected gradient to same norm as unprojected
    if(normAfter>0.0000001)
      gsl_vector_scale(after_proj_short, normBefore/normAfter);

    // Convert back to full length DOFs vector
    convertCycleDofsToAllDofs(after_project,after_proj_short,to_project);

    gsl_vector_free(to_proj_short);
    gsl_vector_free(after_proj_short);
  }
  else {
    double normBefore = gsl_vector_length(to_project);
    N->projectOnNullSpace(to_project, after_project);
    double normAfter = gsl_vector_length(after_project);
    gsl_vector_scale(after_project, normBefore/normAfter);
  }
}

double Configuration::siteDOFTransfer(Selection& source,Selection& sink,gsl_matrix* baseMatrix){
  ///Identify "allostery" through degrees of freedom shared/linked between two sites
  double ret=0.0;
  std::vector<Residue*> sinkResis = sink.getSelectedResidues(m_molecule);
  std::vector<Residue*> sourceResis = source.getSelectedResidues(m_molecule);

  std::vector<int> sinkDOFIds;
  std::vector<int> sourceDOFIds;

  string output_file_name = "dofTreeAnalysis.txt";
  ofstream output(output_file_name.c_str());

  /// Identify the correct column indices for source and sink DOF, write out tree information
  int countSource=0, countSink=0, countBoth=0;
  //Second, constrain covalent edge bonds
  for (auto const &edge : m_molecule->m_spanningTree->m_edges) {
    int dof_id = edge->getDOF()->getCycleIndex();
    if (dof_id!=-1) { // this edge is a cycle DOF, dof_id is the corresponding column!
      if(edge->getBond() != nullptr){//not a global DOF
        ///write out log for tree-based post-processing
        output<<dof_id<<" "<<edge->getBond()->m_atom1->getResidue()->getId()<<" "<<edge->getBond()->m_atom1->getId()<<" "<<edge->getBond()->m_atom2->getId()<<" '"<<edge->getBond()->m_atom1->getResidue()->getChain()->getName()<<"'"<<endl;

        if(source.inSelection( edge->getBond()) ){//This dof is in the source selection
          sourceDOFIds.push_back(dof_id);
        }
        if(sink.inSelection( edge->getBond() ) ) {//This dof is in the sink selection
          sinkDOFIds.push_back(dof_id);
        }
      }
    }
  }
  output.close();

  if(!sourceDOFIds.empty() & !sinkDOFIds.empty() ){
    output_file_name = "mutualInformation.txt";
    ofstream outputMI(output_file_name.c_str());

    outputMI << "Selection source has " << sourceDOFIds.size() << " dofs, sink has " << sinkDOFIds.size() << " dofs."
         << endl;

    gsl_vector *currentNCol = gsl_vector_alloc(baseMatrix->size1);
/// Geometry and information theory based computation
    double numSource = 0;
    double numSink = 0;
    double numUnion = 0;
    double baseEntropy = 0; //full vector in N
    gsl_vector *sourceVals =  gsl_vector_calloc(sourceDOFIds.size());
    gsl_vector *sinkVals =  gsl_vector_calloc(sinkDOFIds.size());

    ///Implementation with multiplication and sum at the end
//    for(int colID=baseMatrix->size2-1; colID>=0; colID--){
//      gsl_matrix_get_col(currentNCol,baseMatrix, colID);
//      double baseVal = shannonEntropyUnnormalizedInBits(currentNCol);
//      baseEntropy += baseVal;
//      int sourceCounter=0;
//      for (auto sourceID : sourceDOFIds) {
//        gsl_vector_set(sourceVals,sourceCounter,gsl_vector_get(currentNCol, sourceID));
//        sourceCounter++;
//      }
//      double sourceVal = shannonEntropyUnnormalizedInBits(sourceVals);
//      numSource += sourceVal;
//
//      int sinkCounter = 0;
//      for(auto sinkID : sinkDOFIds) {
//        gsl_vector_set(sinkVals,sinkCounter,gsl_vector_get(currentNCol, sinkID));
//        sinkCounter++;
//      }
//      double sinkVal = shannonEntropyUnnormalizedInBits(sinkVals);
//      numSink += sinkVal;
//
//      double unionVal = 0.0;
////      if (sourceVal != 0 && sinkVal != 0)
//      unionVal = sourceVal * sinkVal; // /(sourceVal+sinkVal);
//      numUnion += unionVal;
//
//      log("mi")<<baseVal<<" "<<sourceVal<<" "<<sinkVal<<" "<<unionVal<<endl;
//    }
//
//    gsl_vector_free(sourceVals);
//    gsl_vector_free(sinkVals);
//
//    ret = numUnion/(numSource+numSink);

    /// Implementation with normalized vectors in source, sink, and joint (Henry's final email)
    gsl_vector *jointVals =  gsl_vector_calloc(sourceDOFIds.size() + sinkDOFIds.size());
    for(int colID=baseMatrix->size2-1; colID>=0; colID--){
      gsl_matrix_get_col(currentNCol,baseMatrix, colID);
      double baseVal = shannonEntropyInBits(currentNCol);
      baseEntropy += baseVal;
      int sourceCounter=0;
      int jointCounter=0;
      for (auto sourceID : sourceDOFIds) {
        gsl_vector_set(sourceVals,sourceCounter,gsl_vector_get(currentNCol, sourceID));
        gsl_vector_set(jointVals,jointCounter,gsl_vector_get(currentNCol, sourceID));
        sourceCounter++;
        jointCounter++;
      }
      double sourceVal = shannonEntropyInBits(sourceVals);
      numSource += sourceVal;

      int sinkCounter = 0;
      for(auto sinkID : sinkDOFIds) {
        gsl_vector_set(sinkVals,sinkCounter,gsl_vector_get(currentNCol, sinkID));
        gsl_vector_set(jointVals,jointCounter,gsl_vector_get(currentNCol, sinkID));
        sinkCounter++;
        jointCounter++;
      }
      double sinkVal = shannonEntropyInBits(sinkVals);
      numSink += sinkVal;

      double unionVal = shannonEntropyInBits(jointVals);
      numUnion += unionVal;

      outputMI<<baseVal<<" "<<sourceVal<<" "<<sinkVal<<" "<<unionVal<<endl;
    }

    gsl_vector_free(sourceVals);
    gsl_vector_free(sinkVals);
    gsl_vector_free(jointVals);

    ret = numSource + numSink - numUnion;
    /// Implementation DONE

    outputMI << "Allosteric communication between source and sink:"<< endl;
    outputMI <<"Base "<<baseEntropy<<" source "<<numSource<<" sink "<<numSink<<" union "<<numUnion<<" MutualInformationProxy "<<ret<<endl;
    outputMI.close();
  }
  else{
    log("rigidity")<<"No coordinated motion in source or sink, skipping site DoF transfer analysis."<<endl;
    log("rigidity")<<"Source 0 sink 0 union 0"<<endl;
  }
  return ret;
}

void Configuration::sortFreeEnergyModes(gsl_matrix* baseMatrix,gsl_vector* singVals, gsl_vector* returnIDs)
{
  double maxSingVal = gsl_vector_get(singVals, 0);
  int numCols = baseMatrix->size1;
  int numSingVals = singVals->size;
  double cT = 1.0;
  gsl_vector* freeEnergies = gsl_vector_calloc(numCols);
  gsl_vector *currentCol = gsl_vector_alloc(numCols);

  int idxCount = 0;
  for (int i = numCols; i > 0; --i) {//start from the nullspace, so IDs are sorted from that end
      double enthalpy = 0.0;
      if (i < numSingVals){
          enthalpy = gsl_vector_get(singVals, i - 1) / maxSingVal;
      }
      gsl_matrix_get_col(currentCol,baseMatrix, i-1);
      double entropy = fractionOfSignificantContributors(currentCol);
      gsl_vector_set(freeEnergies,idxCount,enthalpy - cT*entropy);
      gsl_vector_set(returnIDs,idxCount,idxCount);
      idxCount++;
  }
  gsl_sort_vector2(freeEnergies,returnIDs);
  gsl_vector_free(freeEnergies);
}

Molecule * Configuration::getMolecule() const
{
  return m_molecule;
}

Molecule * Configuration::updatedMolecule()
{
  m_molecule->setConfiguration(this);
  return m_molecule;
}

void Configuration::updateMolecule()
{
  m_molecule->setConfiguration(this);
}

gsl_matrix* Configuration::getCycleJacobian()
{
  computeJacobians();
  return CycleJacobian;
}

Nullspace* Configuration::getNullspace()
{
  if(nullspace==nullptr){
      computeCycleJacobianAndNullSpace();
  }

  return nullspace;
}

Nullspace* Configuration::getNullspacenocoupling()
{
    if(nullspacenocoupling==nullptr){
        computeJacobiansnocoupling();


        if (JacobianSVDnocoupling!=nullptr) {
            nullspacenocoupling = new NullspaceSVD(JacobianSVDnocoupling);
            nullspacenocoupling->updateFromMatrix();
        }
    }

    return nullspacenocoupling;
}

/*Nullspace* Configuration::getNullspaceligand(){
    if(nullspaceligand==nullptr){
        computeCycleJacobianAndNullSpace();
        int row_num = CycleJacobian->size1;
        int col_num = CycleJacobian->size2-m_molecule->m_spanningTree->ligand_cycledof_id.size(); // number of DOFs in cycles
        if(CycleJacobianligand==nullptr){
            CycleJacobianligand = gsl_matrix_calloc(row_num,col_num);
            JacobianSVDligand = SVD::createSVD(CycleJacobianligand);//new SVDMKL(CycleJacobianligand);
        }else if(CycleJacobianligand->size1==row_num && CycleJacobianligand->size2==col_num){
            gsl_matrix_set_zero(CycleJacobianligand);
        }else{
            gsl_matrix_free(CycleJacobianligand);
            delete JacobianSVDligand;
            CycleJacobianligand = gsl_matrix_calloc(row_num,col_num);
            JacobianSVDligand = SVD::createSVD(CycleJacobianligand);//new SVDMKL(CycleJacobian);
        }
        int q=0;
        for(int i=0; i<CycleJacobian->size2; i++){
            if (std::find(m_molecule->m_spanningTree->ligand_cycledof_id.begin(), m_molecule->m_spanningTree->ligand_cycledof_id.end(), i) == m_molecule->m_spanningTree->ligand_cycledof_id.end()){
                gsl_vector* v;
                v=gsl_vector_alloc(row_num);
                gsl_matrix_get_col(v, CycleJacobian, i);
                gsl_matrix_set_col(CycleJacobianligand, i-q, v);
                gsl_vector_free(v);
            }
            else{
                q++;
            }
        }
    }



    if (JacobianSVDligand!=nullptr) {
        nullspaceligand = new NullspaceSVD(JacobianSVDligand);
        nullspaceligand->updateFromMatrix();
    }


    return nullspaceligand;
}
*/

void Configuration::computeMassmatrix(){

    int row_num = m_molecule->getAtoms().size()*3;
    Massmatrix = gsl_matrix_calloc(row_num, row_num);
    gsl_matrix_set_zero(Massmatrix);

    int t = 0;
    for (vector<Atom *>::const_iterator itr = m_molecule->getAtoms().begin();
         itr != m_molecule->getAtoms().end(); ++itr) {
        gsl_matrix_set(Massmatrix, 3 * t, 3 * t, (*itr)->getMass());
        gsl_matrix_set(Massmatrix, 3 * t + 1, 3 * t + 1, (*itr)->getMass());
        gsl_matrix_set(Massmatrix, 3 * t + 2, 3 * t + 2, (*itr)->getMass());
        t++;
    }
}

void Configuration::computedistancematrix(Molecule* mol){
    int row_num = mol->getAtoms().size();
    distancematrix = gsl_matrix_calloc(row_num, row_num);
    gsl_matrix_set_zero(distancematrix);
    coefficientmatrix = gsl_matrix_calloc(row_num, row_num);
    gsl_matrix_set_zero(coefficientmatrix);

    int t = 0;
    for (vector<Atom *>::const_iterator itr = mol->getAtoms().begin();
         itr != mol->getAtoms().end(); ++itr) {
        int p=0;
        for (vector<Atom *>::const_iterator itnew = mol->getAtoms().begin();
             itnew != mol->getAtoms().end(); ++itnew) {
            if (p>t){
                double distance = (*itr)->m_position.distanceTo((*itnew)->m_position);
                double coefficient = pow(distance, (-exp(1)));
                double sqr = pow(distance, 2) ;
                gsl_matrix_set(distancematrix, t, p, sqr);
                gsl_matrix_set(distancematrix, p, t, sqr);
                gsl_matrix_set(coefficientmatrix, t, p, coefficient);
                gsl_matrix_set(coefficientmatrix, p, t, coefficient);
            }
            p++;
        }
        t++;
    }
}

void Configuration::computeHessiancartesian(double cutoff, double coefficientvalue, double vdwenergyvalue,Molecule* mol){
    if(distancematrix==nullptr || coefficientmatrix==nullptr){
        computedistancematrix(mol);
    }

    int row_num = m_molecule->getAtoms().size()*3;
    if(Hessianmatrix_cartesian==nullptr){
        Hessianmatrix_cartesian = gsl_matrix_calloc(row_num,row_num);
    }else if( Hessianmatrix_cartesian->size1==row_num &&  Hessianmatrix_cartesian->size2==row_num){
        gsl_matrix_set_zero(Hessianmatrix_cartesian);
    }else{
        gsl_matrix_free(Hessianmatrix_cartesian);
        Hessianmatrix_cartesian = gsl_matrix_calloc(row_num,row_num);
    }

    int t = 0;
    for (vector<Atom *>::const_iterator itr = m_molecule->getAtoms().begin();
         itr != m_molecule->getAtoms().end(); ++itr) {
        //if (!(*itr)->getligand()) {
        Atom* atom1 = *itr;
        vector<Atom*> neighbors = m_molecule->getGrid()->getNeighboringAtomsVDW(atom1,true,true,true,true,cutoff);
        for (vector<Atom *>::const_iterator itnew = neighbors.begin(); itnew != neighbors.end(); ++itnew) {
            //if (!(*itnew)->getligand()) {
            Atom* atom2 = *itnew;
            auto findindex = find(m_molecule->getAtoms().begin(),m_molecule->getAtoms().end(), atom2);
            if (findindex != m_molecule->getAtoms().end()) {
                int p = distance(m_molecule->getAtoms().begin(),findindex);
                double atomContribution, allcutoff;
                double distance = (*itr)->m_position.distanceTo((*itnew)->m_position);
                if(vdwenergyvalue<9999.0){
                    double ratio, vdw_r1, vdw_d12, epsilon1, epsilon_12;
                    vdw_r1 = atom1->getRadius();
                    epsilon1 = atom1->getEpsilon();
                    vdw_d12 = (vdw_r1 + atom2->getRadius())/2.0; // from CHARMM: arithmetic mean
                    ratio = vdw_d12/distance;
                    epsilon_12 = sqrt(epsilon1 * (atom2->getEpsilon()));
                    atomContribution = 4 * epsilon_12 * (pow(ratio,12) - pow(ratio,6));
                    allcutoff = vdwenergyvalue;
                }
                else{
                    atomContribution=distance;
                    allcutoff = cutoff;
                }
                /*if (atomContribution < allcutoff && p > t) {
                    double coefficientxx = pow(distance, (-exp(1))) * coefficientvalue;
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * t, 3 * p, coefficient);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * t + 1, 3 * p + 1, coefficient);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * t + 2, 3 * p + 2, coefficient);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * p, 3 * t, coefficient);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * p + 1, 3 * t + 1, coefficient);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * p + 2, 3 * t + 2, coefficient);
                }*/
                if (atomContribution < allcutoff && p > t) {
                    double distancenow = (*itr)->m_position.distanceTo((*itnew)->m_position);
                    double distanceorigin = gsl_matrix_get(distancematrix, p,t);
                    double coeforigin = gsl_matrix_get(coefficientmatrix,p,t);
                    double coefficientxx = coeforigin * coefficientvalue * (-8 * ((*itr)->m_position.x - (*itnew)->m_position.x) * ((*itr)->m_position.x - (*itnew)->m_position.x) - 4 * (pow(distancenow, 2)- distanceorigin));
                    double coefficientyy = coeforigin * coefficientvalue * (-8 * ((*itr)->m_position.y - (*itnew)->m_position.y) * ((*itr)->m_position.x - (*itnew)->m_position.x) - 4 * (pow(distancenow, 2)- distanceorigin));
                    double coefficientzz = coeforigin * coefficientvalue * (-8 * ((*itr)->m_position.z - (*itnew)->m_position.z) * ((*itr)->m_position.x - (*itnew)->m_position.x) - 4 * (pow(distancenow, 2)- distanceorigin));
                    double coefficientxy = coeforigin * coefficientvalue * (-8 * ((*itr)->m_position.x - (*itnew)->m_position.x) * ((*itr)->m_position.y - (*itnew)->m_position.y));
                    double coefficientxz = coeforigin * coefficientvalue * (-8 * ((*itr)->m_position.x - (*itnew)->m_position.x) * ((*itr)->m_position.z - (*itnew)->m_position.z));
                    double coefficientyz = coeforigin * coefficientvalue * (-8 * ((*itr)->m_position.y - (*itnew)->m_position.y) * ((*itr)->m_position.z - (*itnew)->m_position.z));
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * t, 3 * p, coefficientxx);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * p, 3 * t, coefficientxx);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * t + 1, 3 * p + 1, coefficientyy);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * p + 1, 3 * t + 1, coefficientyy);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * t + 2, 3 * p + 2, coefficientzz);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * p + 2, 3 * t + 2, coefficientzz);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * t, 3 * p + 1, coefficientxy);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * t + 1, 3 * p, coefficientxy);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * p, 3 * t + 1, coefficientxy);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * p + 1, 3 * t, coefficientxy);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * t, 3 * p + 2, coefficientxz);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * t + 2, 3 * p, coefficientxz);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * p, 3 * t + 2, coefficientxz);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * p + 2, 3 * t, coefficientxz);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * t + 1, 3 * p + 2, coefficientyz);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * t + 2, 3 * p + 1, coefficientyz);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * p + 1, 3 * t + 2, coefficientyz);
                    gsl_matrix_set(Hessianmatrix_cartesian, 3 * p + 2, 3 * t + 1, coefficientyz);
                }
            }
        }
        t++;
    }
}

void Configuration::computeCycleJacobianentropyforall(){
    int row_num = m_molecule->getAtoms().size()*3;
    //int row_num = m_molecule->getAtoms().size()*3-m_molecule->getligands().size()*3;
    int col_num = m_molecule->m_spanningTree->getNumDOFs(); // number of DOFs in cycles
    CycleJacobianentropy = gsl_matrix_calloc(row_num, col_num);

    int t = 0;
    for (vector<Atom *>::const_iterator itr = m_molecule->getAtoms().begin();
         itr != m_molecule->getAtoms().end(); ++itr) {
        //if (!(*itr)->getligand()) {
        KinVertex *vertex1 = (*itr)->getRigidbody()->getVertex();
        Coordinate p1 = (*itr)->m_position;
        while (vertex1->m_parent->m_rigidbody != nullptr && vertex1->m_parent != nullptr) {
            KinVertex *parent = vertex1->m_parent;
            KinEdge *p_edge = parent->findEdge(vertex1);
            int dof_id = p_edge->getDOF()->getIndex();
            int cycle_dof_id = p_edge->getDOF()->getCycleIndex();
            if (dof_id != -1) { // this edge is a DOF  && !p_edge->getDOF()->isDOFligand()
                Math3D::Vector3 derivativeP1 = p_edge->getDOF()->getDerivative(p1);
                gsl_matrix_set(CycleJacobianentropy, 3 * t, dof_id, derivativeP1.x);
                gsl_matrix_set(CycleJacobianentropy, 3 * t + 1, dof_id, derivativeP1.y);
                gsl_matrix_set(CycleJacobianentropy, 3 * t + 2, dof_id, derivativeP1.z);
            }
            vertex1 = parent;
        }
        t++;
        //}
    }
}

void Configuration::computeCycleJacobianentropy(Nullspace* Nu,std::string nocoupling){
    if (CycleJacobianentropy == nullptr) {
        computeCycleJacobianentropyforall();
    }

    int row_num = m_molecule->getAtoms().size()*3;
    int col_num = m_molecule->m_spanningTree->getNumDOFs();

    gsl_vector* rigiddof = Nu->buildDofRigid();
    gsl_matrix* CycleJacobianentropy1 = gsl_matrix_calloc(row_num, col_num);
    gsl_matrix_memcpy(CycleJacobianentropy1, CycleJacobianentropy);

    gsl_vector* setzero =gsl_vector_calloc(row_num);
    gsl_vector_set_zero(setzero);
    for(int i=0;i<rigiddof->size;i++){
        if(gsl_vector_get(rigiddof, i)>0.9){
            int rigid_dof_id = m_molecule->m_spanningTree->getCycleDOF(i)->getIndex();
            gsl_matrix_set_col(CycleJacobianentropy1, rigid_dof_id, setzero);
        }
    }
    gsl_vector_free(setzero);
    gsl_vector_free(rigiddof);
    if(nocoupling=="true"){
        CycleJacobianentropynocoupling = gsl_matrix_calloc(row_num, col_num);
        gsl_matrix_memcpy(CycleJacobianentropynocoupling,CycleJacobianentropy1);
    }

    if(nocoupling=="false"){
        CycleJacobianentropycoupling = gsl_matrix_calloc(row_num, col_num);
        gsl_matrix_memcpy(CycleJacobianentropycoupling,CycleJacobianentropy1);
    }
    gsl_matrix_free(CycleJacobianentropy1);
}

void Configuration::Hessianmatrixentropy(double cutoff, double coefficientvalue,double vdwenergyvalue, Nullspace* Nu, Molecule* mol, bool proteinonly, std::string nocoupling){
    if (Massmatrix == nullptr) {
        computeMassmatrix();
    }

    computeHessiancartesian(cutoff,coefficientvalue,vdwenergyvalue, mol);

    if (CycleJacobianentropy == nullptr) {
        computeCycleJacobianentropyforall();
    }

    int row_num = m_molecule->getAtoms().size()*3;
    int col_num = m_molecule->m_spanningTree->getNumDOFs();
    gsl_matrix* CycleJacobianentropyinput = gsl_matrix_calloc(row_num, col_num);
    if(nocoupling=="true" && CycleJacobianentropynocoupling==nullptr){
        computeCycleJacobianentropy(Nu,nocoupling);
    }

    if(nocoupling=="false" && CycleJacobianentropycoupling==nullptr){
        computeCycleJacobianentropy(Nu,nocoupling);
    }

    if(nocoupling=="true"){gsl_matrix_memcpy(CycleJacobianentropyinput, CycleJacobianentropynocoupling);}
    else{gsl_matrix_memcpy(CycleJacobianentropyinput, CycleJacobianentropycoupling);}

    if(proteinonly){
        gsl_vector* setzero =gsl_vector_calloc(row_num);
        gsl_vector* setzerorow =gsl_vector_calloc(col_num);
        gsl_vector_set_zero(setzero);
        gsl_vector_set_zero(setzerorow);
        for(int i=0;i<m_molecule->m_spanningTree->ligand_dof_id.size();i++){
            int rigid_dof_id = m_molecule->m_spanningTree->ligand_dof_id[i];
            gsl_matrix_set_col(CycleJacobianentropyinput, rigid_dof_id, setzero);
        }

        int t=0;
        for (vector<Atom *>::const_iterator itr = m_molecule->getAtoms().begin();
             itr != m_molecule->getAtoms().end(); ++itr) {
            if((*itr)->getligand()){
                gsl_matrix_set_row(CycleJacobianentropyinput, 3 * t, setzerorow);
                gsl_matrix_set_row(CycleJacobianentropyinput, 3 * t + 1, setzerorow);
                gsl_matrix_set_row(CycleJacobianentropyinput, 3 * t + 2, setzerorow);
            }
            t++;
        }
        gsl_vector_free(setzero);
        gsl_vector_free(setzerorow);
    }
    if(CycleJacobianentropyinput!=nullptr && Hessianmatrix_cartesian!=nullptr && Massmatrix!=nullptr){
        if(Entropyeigen){
            delete Entropyeigen;
            Entropyeigen = new Eigenvalue(CycleJacobianentropyinput,Hessianmatrix_cartesian,Massmatrix);
        }
        else{
            Entropyeigen = new Eigenvalue(CycleJacobianentropyinput,Hessianmatrix_cartesian,Massmatrix);
        }

    }
    gsl_matrix_free(CycleJacobianentropyinput);
}

void Configuration::setrigiddofid(){
    Nullspace* Nu = getNullspace();
    gsl_vector* rigiddof = Nu->buildDofRigid();
    for(int i=0;i<rigiddof->size;i++){
        if(gsl_vector_get(rigiddof, i)>0.9){
            int rigid_dof_id = m_molecule->m_spanningTree->getCycleDOF(i)->getIndex();
            m_molecule->m_spanningTree->all_dof_id[rigid_dof_id]=2;
        }
    }
}


Eigenvalue* Configuration::geteigenvalue(){
    return Entropyeigen;
}

gsl_matrix* Configuration::getHydrophobicJacobian()
{
  computeJacobians();
  return HydrophobicBondJacobian;
}

gsl_matrix* Configuration::getHydrogenJacobian()
{
  computeJacobians();
  return HBondJacobian;
}

gsl_matrix* Configuration::getDistanceJacobian()
{
  computeJacobians();
  return DBondJacobian;
}

Configuration* Configuration::getParent()
{
  return m_parent;
}

std::list<Configuration*>& Configuration::getChildren()
{
  return m_children;
}

