/*
    KGSX: Biomolecular Kino-geometric Sampling and Fitting of Experimental Data
    Yao et al, Proteins. 2012 Jan;80(1):25-43
    e-mail: latombe@cs.stanford.edu, vdbedem@slac.stanford.edu, julie.bernauer@inria.fr

        Copyright (C) 2011-2013 Stanford University

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
#include <math/GSLSVD.h>
#include <math/MKLSVD.h>
#include <gsl/gsl_matrix_double.h>
#include <math/gsl_helpers.h>

#include "Configuration.h"
#include "Molecule.h"
#include "CTKTimer.h"
#include "core/Bond.h"
#include "ProteinHBond.h"
#include "ResidueProfiles.h"
#include "DisjointSets.h"
#include "Logger.h"

double jacobianTime = 0;
double rigidityTime = 0;

gsl_matrix* Configuration::CycleJacobian = NULL;
gsl_matrix* Configuration::HBondJacobian = NULL;
SVD* Configuration::JacobianSVD = NULL;

gsl_matrix* Configuration::ClashAvoidingJacobian = NULL;
Nullspace* Configuration::ClashAvoidingNullSpace = NULL;

Configuration::Configuration(Molecule * protein_):
	m_protein(protein_),
  nullspace(NULL),
  m_parent(NULL),
  m_treeDepth(0)
{
	m_id 										 = 0;
	m_vdwEnergy 						 = 0;
	m_distanceToTarget       = 99999;
	m_paretoFrontDistance 	 = 99999;
	m_distanceToParent       = 0;
	m_distanceToIni          = 0;
	m_maxIndex               = 0;
	m_maxSize                = 0;
	m_maxConstraintViolation = 99999;
	m_numClusters            = 0;
	m_minCollisionFactor     = 0;
	m_usedClashPrevention		 = false;
	m_clashFreeDofs					 = 0;

	// Set up DOF-values and set them to 0
	m_numDOFs = m_protein->m_spanning_tree->m_numDOFs;
	m_dofs = new double[m_numDOFs];
	m_dofs_global = NULL;
	m_sumProjSteps = new double[m_numDOFs];
	for(int i=0; i< m_numDOFs; ++i){
		m_dofs[i] = 0;
		m_sumProjSteps[i]=0;
	}
}

Configuration::Configuration(Configuration* parent_):
    m_protein(parent_->m_protein),
    m_parent(parent_),
    nullspace(NULL),
    m_treeDepth(parent_->m_treeDepth +1)
{
  m_id 										 = 0;
  m_vdwEnergy 						 = 0;
  m_distanceToTarget       = 99999;
	m_paretoFrontDistance 	 = 99999;
  m_distanceToParent       = 0;
  m_distanceToIni          = 0;
  m_maxIndex               = 0;
  m_maxSize                = 0;
  m_maxConstraintViolation = 99999;
  m_numClusters            = 0;
	m_minCollisionFactor     = 0;
	m_usedClashPrevention		 = false;
	m_clashFreeDofs					 = 0;

  parent_->m_children.push_back(this);

  // Set up DOF-values and set them to 0
  m_numDOFs = m_protein->m_spanning_tree->m_numDOFs;
  m_dofs = new double[m_numDOFs];
  m_dofs_global = NULL;
  m_sumProjSteps = new double[m_numDOFs];
  for(int i=0; i< m_numDOFs; ++i){
    m_dofs[i] = 0;
    m_sumProjSteps[i]=0;
  }
}

Configuration::~Configuration(){
  // Remove DOF-value arrays
	if (m_dofs != NULL)
		delete[] m_dofs;
	if(m_dofs_global != NULL)
		delete[] m_dofs_global;
	if( m_sumProjSteps != NULL)
		delete[] m_sumProjSteps;

	//m_biggerRBMap.clear();
  for(auto const& rbPair: m_biggerRBMap)
    delete rbPair.second;

	//m_sortedRBs.clear();
  if(nullspace)
    delete nullspace;

  if( m_parent !=NULL )
    m_parent->m_children.remove(this);
}

void Configuration::computeCycleJacobianAndNullSpace() {
	CTKTimer timer;
	timer.Reset();
	double old_time = timer.LastElapsedTime();
	
	//Compute the Jacobian matrix
  computeJacobians();

	if (JacobianSVD!=NULL) {
    nullspace = new Nullspace(JacobianSVD);
    nullspace->UpdateFromMatrix();
	}

	double new_time = timer.ElapsedTime();
	jacobianTime += new_time - old_time;

  if(CycleJacobian!=NULL) {
    nullspace->RigidityAnalysis(HBondJacobian);
    identifyBiggerRigidBodies();
  }

	double new_time_2 = timer.ElapsedTime();
	rigidityTime += new_time_2 - new_time;
}

void Configuration::identifyBiggerRigidBodies(){

	///We identify the set of bigger rigid bodies, formed by locked bonds that
	///rigidly connect smaller rigid bodies!
  //gsl_vector *rigidDihedrals = CycleNullSpace->m_rigidAngles;
  //gsl_vector *rigidHBonds = CycleNullSpace->m_rigidHBonds;
  gsl_vector *rigidDihedrals = getNullspace()->rigidAngles;
  gsl_vector *rigidHBonds = getNullspace()->rigidHBonds;

	//HBonds aka cycleAnchorEdges
	int i=0; //indexing for hBonds
	int j=0; //numbering for fixed dihedrals

	//First, we set the constrained flag for the corresponding bonds
	for (vector< pair<KinEdge*,KinVertex*> >::iterator it= m_protein->m_spanning_tree->CycleAnchorEdges.begin(); it!=
																																																											 m_protein->m_spanning_tree->CycleAnchorEdges.end(); ++it) {

		KinEdge* edge_ptr = it->first;
		KinVertex* common_ancestor = it->second;

		//Get corresponding rigidity information
		//The m_protein hBonds are ordered in the same way as the rigid HBonds
		double val=gsl_vector_get(rigidHBonds,i);

		if( val == 1 ){
			edge_ptr->getBond()->constrained=true;
		}

		i++;

		//Now, the dihedral angles
		KinVertex* vertex1 = edge_ptr->StartVertex;
		KinVertex* vertex2 = edge_ptr->EndVertex;

		//Trace back along dof m_edges for vertex 1
		while ( vertex1 != common_ancestor ) {

			KinVertex* parent = vertex1->m_parent;
      KinEdge* p_edge = parent->findEdge(vertex1);

			if(parent->isRibose) {//RFonseca
				SugarVertex* v = reinterpret_cast<SugarVertex*>(parent);
				int dof_id = v->Cycle_DOF_id;

				if(dof_id!=-1){

					double valDih=gsl_vector_get(rigidDihedrals,dof_id);

					if( valDih == 1 ) { // && p_edge->getBond()->constrained == false ){
						p_edge->getBond()->constrained=true;
						j++;
					}
				}

			}
			int dof_id = p_edge->Cycle_DOF_id;
			if (dof_id!=-1) { // this edge is a cycle DOF, dof_id is the corresponding column!

				double valDih=gsl_vector_get(rigidDihedrals,dof_id);

				if( valDih == 1 ) { //&& p_edge->getBond()->constrained == false){ //This cycle dof is constrained
					p_edge->getBond()->constrained=true;
					j++;
				}
			}

			vertex1 = parent;
		}

		//Trace back along dof m_edges for vertex 2
		while ( vertex2 != common_ancestor ) {

			KinVertex* parent = vertex2->m_parent;
      KinEdge* p_edge = parent->findEdge(vertex2);

			if(parent->isRibose) {//RFonseca
				SugarVertex* v = reinterpret_cast<SugarVertex*>(parent);
				int dof_id = v->Cycle_DOF_id;

				if(dof_id!=-1){

					double valDih = gsl_vector_get(rigidDihedrals,dof_id);

					if( valDih == 1 ) {//&& p_edge->getBond()->constrained == false){
						p_edge->getBond()->constrained=true;
						j++;
					}
				}
			}

			int dof_id = p_edge->Cycle_DOF_id;
			if (dof_id!=-1) { // this edge is a cycle DOF, dof_id is the corresponding column!

				double valDih = gsl_vector_get(rigidDihedrals,dof_id);

				if( valDih == 1 ) {// && p_edge->getBond()->constrained == false){
					p_edge->getBond()->constrained=true;
					j++;
				}
			}
			vertex2 = parent;
		}
	}


	/// Now, all dihedrals and hBonds that are fixed have the set flag constrained = true!

	m_biggerRBMap.clear();
	m_sortedRBs.clear();

	readBiggerSet();

	/// Now, we have the map of bigger rigid bodies with atoms
	/// We will order the atoms by ID for a nicer display
	m_numClusters = 0;
	m_maxSize = 0;
	m_maxIndex=0;

//	cout<<"Map of bigger rigid bodies:"<<endl;
	map<unsigned int, Rigidbody*>::iterator it=m_biggerRBMap.begin();
	while( it != m_biggerRBMap.end() ){
		m_sortedRBs.push_back( make_pair( (it->second)->size(), it->first ));
		m_numClusters++;
		int num = 0;

		///Sorting
#ifndef WIN32 // Liangjun Zhang's tmp code
	vector<Atom*>::iterator sit = it->second->Atoms.begin();
	vector<Atom*>::iterator eit = it->second->Atoms.end();
	sort(sit,eit,Atom::compare);
#endif

//		cout<<"Bigger rb with ID "<<it->first<<" contains atoms with IDs: "<<endl;
		vector<Atom*>::iterator ait = it->second->Atoms.begin();
		while( ait != it->second->Atoms.end() ){
			//cout<<(*ait)->Id<<endl;
			ait++;
			num++;
		}

		if( num > m_maxSize ){
			m_maxSize = num;
			m_maxIndex = it->first;
		}
		++it;
	}

	vector< pair<int, unsigned int> >::iterator vsit = m_sortedRBs.begin();
	vector< pair<int, unsigned int> >::iterator veit = m_sortedRBs.end();

	sort(vsit, veit,compareSize);

	/// Now, we are done!
	/// m_biggerRBMap contains the set of all bigger rigid bodies!
	/// m_sortedRBs contains size and id of the bigger rbs, sorted by size!

	///More stats!
//	cout<<"Number of rigid clusters: "<<m_numClusters<<endl;
//	cout<<"Biggest rigid body: index "<<m_maxIndex<<" with "<<m_maxSize<<" atoms!"<<endl<<endl;

	//For graphical display, it might be helpful to assign a single bigger rb to each atom

	//vector<Atom*>::iterator atomIt;
	//for( auto const& atom: m_protein->atoms){
  //  atom->m_assignedBiggerRB_id = atom->getBiggerRigidbody()->id();
	//}

}

//---------------------------------------------------------
bool Configuration::compareSize(pair<int, unsigned int> firstEntry, pair<int, unsigned int> secondEntry) {
	if( firstEntry.first > secondEntry.first )
		return true;
	if( firstEntry.first < secondEntry.first )
		return false;

	return (firstEntry.second < secondEntry.second);
}

void Configuration::readBiggerSet(){

	//Create disjoint set
	DisjointSets ds(m_protein->atoms[m_protein->size() - 1]->getId() + 1); //Assumes the last atom has the highest id.

	//For each atom, a1, with exactly one cov neighbor, a2, call Union(a1,a2)
	for (int i=0;i< m_protein->size();i++){
		Atom* atom = m_protein->atoms[i];
		if(atom->Cov_neighbor_list.size()==1 && atom->Hbond_neighbor_list.size()==0){
			ds.Union(atom->getId(), atom->Cov_neighbor_list[0]->getId());
		}
	}


	//For each fixed or constrained bond (a1,a2) call Union(a1,a2)
	for (list<Bond *>::iterator it= m_protein->Cov_bonds.begin(); it != m_protein->Cov_bonds.end(); ++it){
		Bond * bond = *it;

		///First, simply check if bond is constrained
		if( bond->constrained || bond->Bars == 6){
			ds.Union(bond->Atom1->getId(), bond->Atom2->getId());
		}
	}

	///Also, do the same thing for the hydrogen bonds
	///insert the h-bonds at the correct place
	for (list<Hbond *>::iterator bit= m_protein->H_bonds.begin(); bit != m_protein->H_bonds.end(); ++bit){
		Hbond * bond = *bit;
		if( bond->constrained ){
			ds.Union(bond->Atom1->getId(), bond->Atom2->getId());
		}
	}

	///All disjoint sets have been united if they are constrained or bonded fix!
	///We now just add the covalent neighbors to have the representation with atoms in multiple rbs!

	int c=0;
	map<int,int> idMap;//Maps atom id's to rigid body id's for use in the DS structure.

	//Map the set-ID's (first map entry) to RB-ID's (second map entry) and add bonded atoms to RBs.
	for (int i=0;i< m_protein->size();i++){
		Atom* atom = m_protein->atoms[i];

		//Map the set-id to the RB-id
		int set_id = ds.FindSet(atom->getId());
		int body_id;
		if(idMap.find(set_id)!=idMap.end()) body_id = idMap.find(set_id)->second;
		else {
			body_id = c++;
			idMap.insert( make_pair(set_id, body_id) );
		}
		//If the set containing a1 is not a rigid body: create one
		if ( m_biggerRBMap.find(body_id) == m_biggerRBMap.end() ) {
			Rigidbody* new_rb = new Rigidbody(body_id);
			m_biggerRBMap.insert( make_pair(body_id,new_rb) );
		}
		Rigidbody* rb = m_biggerRBMap[body_id];
		if (!rb->containsAtom(atom)){
			rb->Atoms.push_back(atom);
			atom->setBiggerRigidbody(rb);
		}

//		// Add the covalent neighbors of this atom into its bigger rigid body, if it's not there already
//		for ( vector<Atom*>::iterator it=atom->Cov_neighbor_list.begin(); it!=atom->Cov_neighbor_list.end(); ++it) {
//			if ( !rb->containsAtom(*it) ) {
//				rb->Atoms.push_back(*it);
//				(*it)->addBiggerRigidbody(rb);
//			}
//		}
//
//		//Add the Hbond neighbors of this atom into this bigger rigid body, if it's not there
//		for ( vector<Atom*>::iterator it=atom->Hbond_neighbor_list.begin(); it!=atom->Hbond_neighbor_list.end(); ++it) {
//			if ( !rb->containsAtom(*it) ) {
//				rb->Atoms.push_back(*it);
//				(*it)->addBiggerRigidbody(rb);
//			}
//		}
	}

}


//------------------------------------------------------
void Configuration::updateGlobalTorsions(){
  if(m_dofs_global == NULL){
    m_dofs_global = new double[m_numDOFs];
  }
  for(int i=0; i < m_numDOFs; ++i){
    m_dofs_global[i] = 0;
  }
  for (vector<KinEdge*>::iterator itr= m_protein->m_spanning_tree->Edges.begin(); itr!= m_protein->m_spanning_tree->Edges.end(); ++itr) {
    KinEdge* pEdge = (*itr);
    int dof_id = pEdge->DOF_id;
    if (dof_id==-1)
      continue;
    m_dofs_global[dof_id] = pEdge->getBond()->getTorsion();
  }
}
//------------------------------------------------------
double Configuration::getGlobalTorsions(int i) const{
	return m_dofs_global[i];
}

double* Configuration::getGlobalTorsions() const{
	return m_dofs_global;
}


Configuration* Configuration::clone() const {

	Configuration* ret;

	if (m_parent) {
		ret = new Configuration(m_parent);
	}
	else{
		ret = new Configuration(m_protein);

	}

	ret->m_id = m_id;
  ret->m_vdwEnergy = m_vdwEnergy;
  ret->m_numDOFs = m_numDOFs;

	for(int i=0; i < m_numDOFs; ++i){
		ret->m_dofs[i]        = m_dofs[i];
	}
	if(getGlobalTorsions()!=NULL){
    ret->m_dofs_global = new double[m_numDOFs];
    for(int i=0; i < m_numDOFs; ++i) {
      ret->m_dofs_global[i]    = m_dofs_global[i];
      ret->m_sumProjSteps[i] = m_sumProjSteps[i];
    }
	}else{
		ret->m_dofs_global = NULL;
	}
  return ret;
}

//---------------------------------------------------------
void Configuration::Normalize(){
	double s = 0;

	for(int i=0; i < m_numDOFs; ++i)
		s += m_dofs[i] * m_dofs[i];

	s = sqrt(s);
	for(int i=0; i < m_numDOFs; ++i)
		m_dofs[i] /= s;
}
//---------------------------------------------------------
double Configuration::Length(){
	double s = 0;

	for(int i=0; i < m_numDOFs; ++i)
		s += m_dofs[i] * m_dofs[i];

	return sqrt(s);
}
//---------------------------------------------------------
void Configuration::SetAll(double v){
	for(int i=0; i < m_numDOFs; ++i)
		m_dofs[i] = v;
}

void Configuration::Print () {
	for (int i=0; i < m_numDOFs; ++i){
    cout << "Relative: " << m_dofs[i] << " ";
    cout << "Global: " << m_dofs_global[i] << " ";
	}
	cout << endl;
}


void Configuration::computeJacobians() {

  updateProtein();

	// No cycles
	if(m_protein->m_spanning_tree->CycleAnchorEdges.size() == 0) {
		CycleJacobian = NULL; //TODO: Memory leak
		return;
	}

	int hBond_row_num = (m_protein->m_spanning_tree->CycleAnchorEdges).size();
	int row_num = hBond_row_num*5; // 5 times the number of cycles, non-redundant description
	int col_num = m_protein->m_spanning_tree->m_numCycleDOFs; // number of DOFs in cycles

	if(CycleJacobian==NULL){
		CycleJacobian = gsl_matrix_calloc(row_num,col_num);
    JacobianSVD = new MKLSVD(CycleJacobian);
	}else if(CycleJacobian->size1==row_num && CycleJacobian->size2==col_num){
		gsl_matrix_set_zero(CycleJacobian);
	}else{
		gsl_matrix_free(CycleJacobian);
		CycleJacobian = gsl_matrix_calloc(row_num,col_num);
    JacobianSVD = new MKLSVD(CycleJacobian);
	}

	///HBond Jacobian
	if(HBondJacobian==NULL){
			HBondJacobian = gsl_matrix_calloc(hBond_row_num,col_num);

	}else if(HBondJacobian->size1==hBond_row_num && HBondJacobian->size2==col_num){
			gsl_matrix_set_zero(HBondJacobian);

	}	else{
			gsl_matrix_free(HBondJacobian);
			HBondJacobian = gsl_matrix_calloc(hBond_row_num,col_num);
	}

	// for each cycle, fill in the Jacobian entries
	int i=0;
	for (vector< pair<KinEdge*,KinVertex*> >::iterator it= m_protein->m_spanning_tree->CycleAnchorEdges.begin();
       it!= m_protein->m_spanning_tree->CycleAnchorEdges.end();
       ++it)
  {
		// get end-effectors
		KinEdge* edge_ptr = it->first;
		KinVertex* common_ancestor = it->second;
		Bond * bond_ptr = edge_ptr->getBond();

		//End-effectors and their positions, corresponds to a and b
		Atom* atom1 = bond_ptr->Atom1;
		Atom* atom2 = bond_ptr->Atom2;
		Coordinate p1 = atom1->m_Position; //end-effector, position 1
		Coordinate p2 = atom2->m_Position; //end-effector, position 2

		KinVertex* vertex1 = edge_ptr->StartVertex;
		KinVertex* vertex2 = edge_ptr->EndVertex;

		//Use the covalently bonded atoms to find A-1 and B-1
		Atom* atom1_prev = atom1->Cov_neighbor_list[0] == atom2 ? atom1->Cov_neighbor_list[1] : atom1->Cov_neighbor_list[0];
		Atom* atom2_prev = atom2->Cov_neighbor_list[0] == atom1 ? atom2->Cov_neighbor_list[1] : atom2->Cov_neighbor_list[0];

		 //Make sure a1 is the covalent neighbor of Atom1 with lexicographically smallest name
		for(vector<Atom*>::iterator ait = atom1->Cov_neighbor_list.begin(); ait!=atom1->Cov_neighbor_list.end(); ait++){
		    Atom* a = *ait;
		    if(a!=atom2 && a->getName()<atom1_prev->getName())
		    	atom1_prev = a;
		}
		//Make sure a4 is the covalent neighbor of Atom2 with lexicographically smallest name
		for(vector<Atom*>::iterator ait = atom2->Cov_neighbor_list.begin(); ait!=atom2->Cov_neighbor_list.end(); ait++){
		    Atom* a = *ait;
		    if(a!=atom1 && a->getName()<atom2_prev->getName())
		    	atom2_prev = a;
		}

		if(!atom1 || !atom1_prev || !atom2 || !atom2_prev){
			cerr<<"Not all neighboring atoms for non-redundant Jacobian defined, quitting!"<<endl;
			exit(-1);
		}

		Coordinate p1_prev = atom1_prev->m_Position;
		Coordinate p2_prev = atom2_prev->m_Position;
		//Now we have all atoms we need --> Calculate Jacobian


		// trace back until the common ancestor from vertex1
		while ( vertex1 != common_ancestor ) {
			KinVertex* parent = vertex1->m_parent;
      KinEdge* p_edge = parent->findEdge(vertex1);

			if(parent->isRibose) {//RFonseca
				SugarVertex* v = reinterpret_cast<SugarVertex*>(parent);
				int dof_id = v->Cycle_DOF_id;
				//cout<<"Ribose "<<v->DOF_id<<" has Cycle_DOF_id "<<v->Cycle_DOF_id<<endl;
				if(dof_id!=-1){

					Vector3 derivativeP1 = v->computeJacobianEntry(p_edge, m_dofs, p1);
					Vector3 derivativeP2 = v->computeJacobianEntry(p_edge, m_dofs, p2);
					Vector3 derivativeP1_prev = v->computeJacobianEntry(p_edge, m_dofs, p1_prev);
					// Vector3 derivativeP2_prev = v->computeJacobianEntry(p_edge, m_f, p2_prev);

					Vector3 jacobianEntryTrans=derivativeP1 + derivativeP2;
					double jacobianEntryRot1 = dot((p2 - p1),(derivativeP1-derivativeP1_prev));
					double jacobianEntryRot2 = dot((p2 - p2_prev), (derivativeP1 - derivativeP2));

					gsl_matrix_set(CycleJacobian,i*5+0,dof_id,jacobianEntryTrans.x); //set: Matrix, row, column, what to set
					gsl_matrix_set(CycleJacobian,i*5+1,dof_id,jacobianEntryTrans.y);
					gsl_matrix_set(CycleJacobian,i*5+2,dof_id,jacobianEntryTrans.z);
					gsl_matrix_set(CycleJacobian,i*5+3,dof_id,jacobianEntryRot1);
					gsl_matrix_set(CycleJacobian,i*5+4,dof_id,jacobianEntryRot2);

					///Matrix to check hBond Rotation
					double hBondEntry = dot((p1_prev - p2_prev), (derivativeP1_prev));
					gsl_matrix_set(HBondJacobian,i,dof_id,hBondEntry);
				}
			}

			int dof_id = p_edge->Cycle_DOF_id;
			if (dof_id!=-1) { // this edge is a DOF
				//if(dof_id==20)
				//	cout<<"KinEdge "<<p_edge<<endl;

				Atom* ea1 = p_edge->getBond()->Atom1;
				Atom* ea2 = p_edge->getBond()->Atom2;

				// Jacobian_entry is the derivative of the vertices of the hydrogen bond
				// Now, we also have to calculate the derivatives of the other neighboring atoms!
				Vector3 derivativeP1 = ComputeJacobianEntry(ea1->m_Position,ea2->m_Position,p1); //a
				Vector3 derivativeP2 = ComputeJacobianEntry(ea1->m_Position,ea2->m_Position,p2); //b
				Vector3 derivativeP1_prev = ComputeJacobianEntry(ea1->m_Position,ea2->m_Position,p1_prev);
				//Vector3 derivativeP2_prev = ComputeJacobianEntry(ea1->m_Position,ea2->m_Position,p2_prev);

				Vector3 jacobianEntryTrans=derivativeP1 + derivativeP2;
				double jacobianEntryRot1 = dot((p2 - p1),(derivativeP1-derivativeP1_prev));
				double jacobianEntryRot2 = dot((p2 - p2_prev), (derivativeP1 - derivativeP2));
//				cout<<"Jac at i="<<dof_id<<", Trans: "<<jacobianEntryTrans<<", Rot1: "<<jacobianEntryRot1<<", Rot2: "<<jacobianEntryRot2<<endl;
				gsl_matrix_set(CycleJacobian,i*5+0,dof_id,jacobianEntryTrans.x); //set: Matrix, row, column, what to set
				gsl_matrix_set(CycleJacobian,i*5+1,dof_id,jacobianEntryTrans.y);
				gsl_matrix_set(CycleJacobian,i*5+2,dof_id,jacobianEntryTrans.z);
				gsl_matrix_set(CycleJacobian,i*5+3,dof_id,jacobianEntryRot1);
				gsl_matrix_set(CycleJacobian,i*5+4,dof_id,jacobianEntryRot2);

				///Matrix to check hBond Rotation
				double hBondEntry = dot((p1_prev - p2_prev), (derivativeP1_prev));
				gsl_matrix_set(HBondJacobian,i,dof_id,hBondEntry);
			}
			vertex1 = parent;
		}
		// trace back until the common ancestor from vertex2
		while ( vertex2 != common_ancestor ) {
			KinVertex* parent = vertex2->m_parent;
      KinEdge* p_edge = parent->findEdge(vertex2);

			if(parent->isRibose) {//RFonseca
				SugarVertex* v = reinterpret_cast<SugarVertex*>(parent);
				int dof_id = v->Cycle_DOF_id;
				//cout<<"Ribose "<<v->DOF_id<<" has Cycle_DOF_id "<<v->Cycle_DOF_id<<endl;
				if(dof_id!=-1){

					Vector3 derivativeP1 = v->computeJacobianEntry(p_edge, m_dofs, p1);
					Vector3 derivativeP2 = v->computeJacobianEntry(p_edge, m_dofs, p2);
					// Vector3 derivativeP1_prev = v->computeJacobianEntry(p_edge, m_f, p1_prev);
					Vector3 derivativeP2_prev = v->computeJacobianEntry(p_edge, m_dofs, p2_prev);

					Vector3 jacobianEntryTrans= - ( derivativeP1 + derivativeP2);
					double jacobianEntryRot1 = dot((p1 - p1_prev),(derivativeP2-derivativeP1));
					double jacobianEntryRot2 = dot((p1 - p2), (derivativeP2 - derivativeP2_prev));

					gsl_matrix_set(CycleJacobian,i*5+0,dof_id,gsl_matrix_get(CycleJacobian,i*5+0,dof_id) + jacobianEntryTrans.x); //set: Matrix, row, column, what to set
					gsl_matrix_set(CycleJacobian,i*5+1,dof_id,gsl_matrix_get(CycleJacobian,i*5+1,dof_id) + jacobianEntryTrans.y);
					gsl_matrix_set(CycleJacobian,i*5+2,dof_id,gsl_matrix_get(CycleJacobian,i*5+2,dof_id) + jacobianEntryTrans.z);
					gsl_matrix_set(CycleJacobian,i*5+3,dof_id,gsl_matrix_get(CycleJacobian,i*5+3,dof_id) + jacobianEntryRot1);
					gsl_matrix_set(CycleJacobian,i*5+4,dof_id,gsl_matrix_get(CycleJacobian,i*5+4,dof_id) + jacobianEntryRot2);

					///Matrix to check hBond Rotation
					double hBondEntry = dot((p1_prev - p2_prev), (- derivativeP2_prev));
					gsl_matrix_set(HBondJacobian,i,dof_id,hBondEntry);
				}
			}

			int dof_id = p_edge->Cycle_DOF_id;
			if (dof_id!=-1) { // this edge is a DOF
				//if(dof_id==20)
				//	cout<<"KinEdge "<<p_edge<<endl;
				Atom* ea1 = p_edge->getBond()->Atom1;
				Atom* ea2 = p_edge->getBond()->Atom2;

				// Jacobian_entry is the derivative of the vertices of the hydrogen bond
				// Now, we also have to calculate the derivatives of the other neighboring atoms!
				Vector3 derivativeP1 = ComputeJacobianEntry(ea1->m_Position,ea2->m_Position,p1); //a
				Vector3 derivativeP2 = ComputeJacobianEntry(ea1->m_Position,ea2->m_Position,p2); //b
				//Vector3 derivativeP1_prev = ComputeJacobianEntry(ea1->m_Position,ea2->m_Position,p1_prev);
				Vector3 derivativeP2_prev = ComputeJacobianEntry(ea1->m_Position,ea2->m_Position,p2_prev);

				Vector3 jacobianEntryTrans= - (derivativeP1 + derivativeP2);
				double jacobianEntryRot1 = dot((p1 - p1_prev),(derivativeP2-derivativeP1));
				double jacobianEntryRot2 = dot((p1 - p2), (derivativeP2 - derivativeP2_prev));
//				cout<<"Jac at i="<<dof_id<<", Trans: "<<jacobianEntryTrans<<", Rot1: "<<jacobianEntryRot1<<", Rot2: "<<jacobianEntryRot2<<endl;
				gsl_matrix_set(CycleJacobian,i*5+0,dof_id,jacobianEntryTrans.x); //set: Matrix, row, column, what to set
				gsl_matrix_set(CycleJacobian,i*5+1,dof_id,jacobianEntryTrans.y);
				gsl_matrix_set(CycleJacobian,i*5+2,dof_id,jacobianEntryTrans.z);
				gsl_matrix_set(CycleJacobian,i*5+3,dof_id,jacobianEntryRot1);
				gsl_matrix_set(CycleJacobian,i*5+4,dof_id,jacobianEntryRot2);

				///Matrix to check hBond Rotation
				double hBondEntry = dot((p1_prev - p2_prev), (- derivativeP2_prev));
				gsl_matrix_set(HBondJacobian,i,dof_id,hBondEntry);
			}
			vertex2 = parent;
		}
		++i;
	}

}

//------------------------------------------------------------
//We compute another Jacobian that also considers motion along clash normal directions as a constraint
//--> allowed motions will not move clashing atoms further into each other

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

	if (JacobianSVD != NULL) {
		// CycleNullSpace needs to be deleted to free memory.
		if( ClashAvoidingNullSpace!=NULL ) {
			delete ClashAvoidingNullSpace;
		}
		ClashAvoidingNullSpace = new Nullspace(JacobianSVD);
    ClashAvoidingNullSpace->UpdateFromMatrix();
	}

//	double new_time = timer.ElapsedTime();
//	clashJacobianTime += new_time - old_time;
}

//---------------------------------------------------------
void Configuration::computeClashAvoidingJacobian (std::map< std::pair<Atom*,Atom*>,int > allCollisions, bool projectConstraints) {

	//The clash Jacobian is the regular Jacobian's constraints, plus one constraint per pair of clashing atoms
	int numCollisions = allCollisions.size();
	int rowNum, colNum;

	//Clashes can occur also for previously free dihedrals!
	//Therefore, we use the full set of dihedrals to determine this matrix!

	if( CycleJacobian != NULL){
		rowNum = CycleJacobian->size1 + numCollisions;
		colNum = m_protein->m_spanning_tree->m_numDOFs;
	}
	else{
		rowNum = numCollisions;
		colNum = m_protein->m_spanning_tree->m_numDOFs;
	}

	if(ClashAvoidingJacobian==NULL){
    ClashAvoidingJacobian = gsl_matrix_calloc(rowNum,colNum);
    JacobianSVD = new MKLSVD(ClashAvoidingJacobian);
	}else if(ClashAvoidingJacobian->size1==rowNum && ClashAvoidingJacobian->size2==colNum){
		gsl_matrix_set_zero(ClashAvoidingJacobian);
	}else{
		gsl_matrix_free(ClashAvoidingJacobian);
    ClashAvoidingJacobian = gsl_matrix_calloc(rowNum,colNum);
    delete JacobianSVD;
    JacobianSVD = new MKLSVD(ClashAvoidingJacobian);
	}

	//Convert the cycle Jacobian to a full Jacobian
	//Columns correspond to cycle_dof_ids
	if(projectConstraints){
		for (auto vit=m_protein->m_spanning_tree->Vertex_map.begin(); vit!=m_protein->m_spanning_tree->Vertex_map.end(); vit++){
			if( (*vit).second->isRibose ){
				SugarVertex* v = reinterpret_cast<SugarVertex*>((*vit).second);
				int dof_id = v->DOF_id;
				int cycle_dof_id = v->Cycle_DOF_id;
				if ( cycle_dof_id!=-1 ) {
					for( int i=0; i!=CycleJacobian->size1; i++){
						gsl_matrix_set(ClashAvoidingJacobian, i, dof_id, gsl_matrix_get(CycleJacobian,i,cycle_dof_id));
					}
				}
			}
		}
		for (vector<KinEdge*>::iterator eit=m_protein->m_spanning_tree->Edges.begin(); eit!=m_protein->m_spanning_tree->Edges.end(); ++eit) {
			int dof_id = (*eit)->DOF_id;
			int cycle_dof_id = (*eit)->Cycle_DOF_id;
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
		log("dominik") << "Using clash constraint for atoms: "<<atom1->getId() << " " << atom2->getId() << endl;

		Coordinate p1 = atom1->m_Position; //end-effector, position 1
		Coordinate p2 = atom2->m_Position; //end-effector, position 2

		Vector3 clashNormal = p2-p1;
		clashNormal.getNormalized(clashNormal);

		//Vertices
		KinVertex* vertex1 = atom1->getRigidbody()->getVertex();
		KinVertex* vertex2 = atom2->getRigidbody()->getVertex();
		KinVertex* common_ancestor = m_protein->m_spanning_tree->findCommonAncestor(vertex1, vertex2);

		// trace back until the common ancestor from vertex1
		while ( vertex1 != common_ancestor ) {
			KinVertex* parent = vertex1->m_parent;
			KinEdge* p_edge = parent->findEdge(vertex1);

      if(parent->isRibose) {//RFonseca
				SugarVertex* v = reinterpret_cast<SugarVertex*>(parent);
				int dof_id = v->DOF_id;
				if(dof_id!=-1){

					Vector3 derivativeP1 = v->computeJacobianEntry(p_edge, m_dofs, p1);
					double jacobianEntryClash = dot(clashNormal, derivativeP1);

					gsl_matrix_set(ClashAvoidingJacobian,i,dof_id,jacobianEntryClash); //set: Matrix, row, column, what to set
//					log("dominik")<<"Setting sugar clash on left branch entry at "<<dof_id<<endl;
				}
			}

			int dof_id = p_edge->DOF_id;
			if (dof_id!=-1) { // this edge is a DOF

				Vector3 derivativeP1 = ComputeJacobianEntry(p_edge->getBond()->Atom1->m_Position,p_edge->getBond()->Atom2->m_Position,p1);
				double jacobianEntryClash = dot(clashNormal, derivativeP1);

				gsl_matrix_set(ClashAvoidingJacobian,i,dof_id,jacobianEntryClash); //set: Matrix, row, column, what to set
//				log("dominik")<<"Setting clash entry on left branch at "<<dof_id<<endl;
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

			if(parent->isRibose) {//RFonseca
				SugarVertex* v = reinterpret_cast<SugarVertex*>(parent);
//				int dof_id = v->Cycle_DOF_id;
				int dof_id = v->DOF_id;
				if(dof_id!=-1){

					Vector3 derivativeP2 = v->computeJacobianEntry(p_edge, m_dofs, p2);
					double  jacobianEntryClash = - dot(clashNormal, derivativeP2);

					gsl_matrix_set(ClashAvoidingJacobian,i,dof_id,gsl_matrix_get(ClashAvoidingJacobian,i,dof_id) + jacobianEntryClash); //set: Matrix, row, column, what to set
//					log("dominik")<<"Setting sugar clash entry on right branch at "<<dof_id<<endl;
				}
			}

//			int dof_id = p_edge->Cycle_DOF_id;
			int dof_id = p_edge->DOF_id;
			if (dof_id!=-1) { // this edge is a DOF

				Vector3 derivativeP2 = ComputeJacobianEntry(
            p_edge->getBond()->Atom1->m_Position,
            p_edge->getBond()->Atom2->m_Position,
            p2); //b
				double jacobianEntryClash = - dot(clashNormal, derivativeP2);

				gsl_matrix_set(ClashAvoidingJacobian,i,dof_id,jacobianEntryClash); //set: Matrix, row, column, what to set
//				log("dominik")<<"Setting clash entry on right branch at "<<dof_id<<endl;
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


Molecule * Configuration::getProtein() const
{
	return m_protein;
}

Molecule * Configuration::updatedProtein()
{
	m_protein->SetConfiguration(this);
	return m_protein;
}

void Configuration::updateProtein()
{
	m_protein->SetConfiguration(this);
}

gsl_matrix* Configuration::getCycleJacobian() const
{
  return CycleJacobian;
}

Nullspace* Configuration::getNullspace()
{
  if(nullspace==NULL){
    computeCycleJacobianAndNullSpace();
  }

  return nullspace;
}

Configuration* Configuration::getParent()
{
  return m_parent;
}

ConfigurationList& Configuration::getChildren()
{
  return m_children;
}
