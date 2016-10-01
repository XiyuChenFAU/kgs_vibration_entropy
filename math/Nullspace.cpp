
#include <gsl/gsl_matrix_double.h>
#include <gsl/gsl_blas.h>

#include "Nullspace.h"
#include "Logger.h"
#include "gsl_helpers.h"
#include "QRGSL.h"
#include "QRMKL.h"
#include "NullspaceSVD.h"

using namespace std;

Nullspace::Nullspace(gsl_matrix* M) :
    m_matrix(M),
    m(M->size1),
    n(M->size2),
    numCoordinatedDihedrals( 0 ),
    numRigidDihedrals( 0 ),
    numRigidHBonds( 0 ),
    m_nullspaceBasis(nullptr),
    rigidAngles(gsl_vector_alloc(n)),
    rigidHBonds(gsl_vector_alloc(n))
{
}

Nullspace::~Nullspace ()
{
  if(m_nullspaceBasis)
    gsl_matrix_free(m_nullspaceBasis);

  gsl_vector_free(rigidAngles);
  gsl_vector_free(rigidHBonds);
}


/** Analyzes which dihedrals and hydrogen bonds are rigidified by constraints */
void Nullspace::performRigidityAnalysis(gsl_matrix *HBondJacobian)
{
  // First, check the dihedral angles for rigidity

  // Nullspace dimension
  gsl_vector* currentRow = gsl_vector_alloc(n);

  gsl_vector_set_zero(rigidAngles);
  gsl_vector_set_zero(rigidHBonds);

  numCoordinatedDihedrals = 0;
  numRigidDihedrals = 0;
  gsl_matrix* N = getBasis();

  for(int i=0; i<n; i++){
    bool moving=false;

    for(int j=n-m_nullspaceSize; j<n; j++){
      double val = fabs( gsl_matrix_get(N,i,j) );
      if( val > RIGID_TOL ) {
        moving = true;
        break;
      }
    }

    // at least one entry was greater than threshold --> dihedral is coordinated
    if( moving ) {
      numCoordinatedDihedrals++;
    } else {
      gsl_vector_set(rigidAngles,i,1); /// binary list of rigid dihedrals, complement to coordinated version
      numRigidDihedrals++;
    }
  }

  log("constraints")<<"There are "<<numRigidDihedrals << " rigidified and " << numCoordinatedDihedrals << " coordinated dihedrals" << endl;

  // Now, check the hydrogen Bonds for rigidity
  int numHBonds = HBondJacobian->size1;
  gsl_matrix* hBondNullspace;// = gsl_matrix_alloc(numHBonds,getNullspaceSize);
  gsl_vector* currentHBondRow;// = gsl_vector_alloc(getNullspaceSize);
  if(m_nullspaceSize==0){
    hBondNullspace = gsl_matrix_calloc(numHBonds,1);
    currentHBondRow = gsl_vector_calloc(1);
  }else{
    hBondNullspace = gsl_matrix_alloc(numHBonds,m_nullspaceSize);
    currentHBondRow = gsl_vector_alloc(m_nullspaceSize);
  }

  ///Calculate the "In-Nullspace" Rotation of the hBonds
  //gsl_matrix_view m_nullspaceBasis = gsl_matrix_submatrix(svd->V,0,svd->V->size2-m_nullspaceSize,n,m_nullspaceSize); //Matrix, top-left element, num rows, num cols
  //gsl_blas_dgemm (CblasNoTrans, CblasNoTrans, 1.0, HBondJacobian, &m_nullspaceBasis.matrix, 0.0, hBondNullspace);
  gsl_blas_dgemm (CblasNoTrans, CblasNoTrans, 1.0, HBondJacobian, m_nullspaceBasis, 0.0, hBondNullspace);

  //Only for test analysis: write hBondNullspace=J_h*N to file
//	string outNullH="KINARI_Comparison/output/hBondnullSpace.txt";
//	gsl_matrix_outtofile(hBondNullspace, outNullH);

  double maxVal;
  double minVal;
  numRigidHBonds = 0;

  for(int i=0; i<numHBonds; i++){
    gsl_matrix_get_row(currentHBondRow,hBondNullspace,i);
    gsl_vector_minmax(currentHBondRow, &minVal, &maxVal);

    if (minVal > -RIGID_TOL && maxVal < RIGID_TOL){
      gsl_vector_set(rigidHBonds,i,1);
      numRigidHBonds++;
    }
  }

//	log("constraints")<<"There are "<<m_numRigidHBonds<<" rigid out of "<<numHBonds<<" hydrogen bonds!"<<endl;
	log("constraints")<<"There are "<<numRigidHBonds<<" rigid out of "<<numHBonds<<" hydrogen bonds!"<<endl;

  gsl_vector_free(currentHBondRow);
  gsl_matrix_free(hBondNullspace);
}

void Nullspace::projectOnNullSpace(gsl_vector *to_project, gsl_vector *after_project) const {

  if(m_nullspaceSize==0){
    gsl_vector_set_zero(after_project);
    return;
  }

  //The projection equation is:   after_project = N * N^T * to_project
  //Dimensions are: [n x 1] = [n x (n-r) ] [(n-r) x n] [n x 1]
  //Computations: N^T * to_project:	(n-r) * n  --> firstResult [(n-r) x 1]
  //Computations: N * firstResult:	(n) * (n-r)  --> finalResult [n x 1]
  //Computations overall: 2*n*(n-r)

  gsl_vector* firstResult = gsl_vector_alloc(m_nullspaceSize);
  gsl_blas_dgemv (CblasTrans, 1.0, m_nullspaceBasis, to_project, 0.0, firstResult);
  gsl_blas_dgemv (CblasNoTrans, 1.0, m_nullspaceBasis, firstResult, 0.0, after_project);
  gsl_vector_free(firstResult);


  //Old version
//	gsl_blas_dgemv (CblasNoTrans, 1.0, nullspace->P, to_project, 0.0, after_project);
}


gsl_matrix *Nullspace::getBasis() const {
  return m_nullspaceBasis;
}

