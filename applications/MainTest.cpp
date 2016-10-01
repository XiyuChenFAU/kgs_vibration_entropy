#include <string>
#include <ctime>
#include <Logger.h>
#include <math/SVDMKL.h>
#include <math/SVDGSL.h>
#include <math/CudaSVD.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_matrix.h>
#include <moves/Move.h>
#include <moves/NullspaceMove.h>
#include <moves/RawMove.h>
#include <directions/Direction.h>
#include <directions/MSDDirection.h>
#include <metrics/RMSDnosuper.h>
#include <directions/BlendedDirection.h>
#include <directions/RandomDirection.h>
#include <sys/time.h>
#include <math/QR.h>
#include <math/QRMKL.h>
#include <math/QRGSL.h>
#include <math/NullspaceSVD.h>
#include <math/NullspaceQR.h>

#include "core/Chain.h"
#include "IO.h"
#include "loopclosure/ExactIK.h"
#include "SamplingOptions.h"
#include "math/gsl_helpers.h"
#include "core/Configuration.h"

using namespace std;

void testGlobalMSD();
void testGlobalGradient();
void testQR();
void testSelection();

int main( int argc, char* argv[] ) {
  enableLogger("default");
//  testGlobalMSD();
//  testGlobalGradient();
  testQR();
//  testSelection();
}


//From http://stackoverflow.com/questions/17432502/how-can-i-measure-cpu-time-and-wall-clock-time-on-both-linux-windows
double get_wall_time(){
  struct timeval time;
  if (gettimeofday(&time,NULL)){
    //  Handle error
    return 0;
  }
  return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

using namespace metrics;

void testSelection(){
  try {
    enableLogger("debug");
    Molecule* mol = new Molecule();
    std::vector<std::string> extraCovBonds;
    IO::readPdb(mol, "/Users/rfonseca/2kmj_1.pdb", extraCovBonds);
    Selection sel("resi 17 and id 4+7");
    for(auto a: sel.getSelectedAtoms(mol)){
      std::cout<<a<<endl;
    }

  } catch (const std::string& ex) {
    cerr<<ex<<endl;
  }
}
void testGlobalGradient(){
  try {
    enableLogger("debug");
    Molecule* mol = new Molecule();
    std::vector<std::string> extraCovBonds;
    IO::readPdb(mol, "/Users/rfonseca/Y.pdb", extraCovBonds);
    Selection sel("all");
    IO::readRigidbody( mol, sel );
    mol->buildSpanningTree();

    Configuration* conf = new Configuration(mol);
    cout<<"DOFS: "<<conf->getNumDOFs()<<endl;

    int dof = 8;
    Coordinate pos = conf->updatedMolecule()->getAtom("A", 29, "OH")->m_position;
    cout<<"Old OH pos: "<<pos<<endl;
    Math3D::Vector3 der = mol->m_spanning_tree->Edges.at(dof)->getDOF()->getDerivative(pos);
    der = der*0.01;
    cout<<der<<endl;
    Configuration* conf2 = new Configuration(conf);
    conf2->m_dofs[dof]+=0.01;
    Coordinate pos2 = conf2->updatedMolecule()->getAtom("A", 29, "OH")->m_position;
    cout<<"New OH pos: "<<pos2<<endl;
    cout<<"Expected:   "<<(pos+der)<<endl;
    if(mol->m_spanning_tree->Edges.at(dof)->getBond()==nullptr) {
      cout << "Global DOF" << endl;
    }else {
      cout << "DOF bond:   " << (*mol->m_spanning_tree->Edges.at(dof)->getBond()) << endl;
    }

  } catch (const std::string& ex) {
    cerr<<ex<<endl;
  }
}

void testGlobalMSD(){
  try {
    enableLogger("debug");
    Molecule* mol = new Molecule();
    std::vector<std::string> extraCovBonds;
    IO::readPdb(mol, "/Users/rfonseca/Y.pdb", extraCovBonds);
    Selection sel("all");
    IO::readRigidbody( mol, sel );
    mol->buildSpanningTree();

    Molecule* tar = new Molecule();
    IO::readPdb(tar, "/Users/rfonseca/Y_target.pdb", extraCovBonds);
    IO::readRigidbody( tar, sel );
    tar->buildSpanningTree();

    Selection dirSelection("heavy");
//    Selection dirSelection("elem O + elem N");
    BlendedDirection* dir = new BlendedDirection();
    dir->addDirection(new MSDDirection(dirSelection), 1.0);
//    dir->addDirection(new RandomDirection(dirSelection, 3.14), 0.3);
    Move* move = new RawMove();
    move->setStepSize(0.01);
    Metric* dist = new RMSDnosuper(dirSelection);

    Configuration* conf = new Configuration(mol);
    Configuration* tarConf = new Configuration(tar);
    gsl_vector* gradient = gsl_vector_calloc(conf->getNumDOFs());

    cout<<"Initial dist: "<<dist->distance(conf, tarConf)<<endl;

    for(size_t i=0;i<400;i++){
      dir->gradient(conf, tarConf, gradient);
      conf = move->move(conf, gradient);
      cout<<"Dist after iteration "<<i<<": "<<dist->distance(conf, tarConf)<<endl;
      IO::writePdb(conf->updatedMolecule(), "/Users/rfonseca/Y_"+to_string(i)+".pdb");
    }

  } catch (const std::string& ex) {
    cerr<<ex<<endl;
  }


}

void testGetTorsion(){
  try {
    Molecule mol;
    std::vector<std::string> extraCovBonds;
    IO::readPdb(&mol, "/Users/rfonseca/1crn.pdb", extraCovBonds);
    IO::readRigidbody( &mol );
    mol.buildSpanningTree();
    Selection sel("backbone");
    for (auto const &b: sel.getSelectedBonds(&mol)) {
      cout << b << " " << b->getTorsion() <<endl;
    }
  } catch (const std::string& ex) {
    cerr<<ex<<endl;
  }
}

void testCuda(){
  srand(101);
  int m = 1000;
  int n = 1000;
  gsl_matrix* A  = gsl_matrix_calloc(m,n);
  rand();rand();rand();
  for(int i=0;i<m;i++)
    for(int j=0;j<n;j++){
      if(rand()%100<10)
        gsl_matrix_set(A, i, j, (rand()*4.0/RAND_MAX));
    }

//  gsl_matrix_cout(A);

  double start = get_wall_time();
  SVD* mklsvd = new SVDMKL(A);
  mklsvd->UpdateFromMatrix();
//  mklsvd->print();
  double duration = ( get_wall_time() - start );
  cout<<"MKL took  "<<duration<<"secs"<<endl;


//  gsl_matrix* S = gsl_matrix_calloc(A->size1, A->size2);
//  for(int i=0;i<std::min(A->size1,A->size2);i++){
//    gsl_matrix_set(S, i,i, gsl_vector_get(mklsvd->S, i));
//  }
//  cout<<"Product: "<<endl;
//  gsl_matrix_cout( gsl_matrix_mul(mklsvd->U, gsl_matrix_mul(S, gsl_matrix_trans(mklsvd->V))) );

//  cout<<" ------------------------------------- "<<endl;

  start = get_wall_time();
  SVD* cudasvd = new CudaSVD(A);
  cudasvd->UpdateFromMatrix();
//  cudasvd->print();
  duration = ( get_wall_time() - start );
  cout<<"CUDA took "<<duration<<"secs"<<endl;

  start = get_wall_time();
  SVD* gslsvd = new SVDGSL(A);
  gslsvd->UpdateFromMatrix();
  duration = ( get_wall_time() - start );
  cout<<"GSL took  "<<duration<<"secs"<<endl;

//  S = gsl_matrix_calloc(A->size1, A->size2);
//  for(int i=0;i<std::min(A->size1,A->size2);i++){
//    gsl_matrix_set(S, i,i, gsl_vector_get(cudasvd->S, i));
//  }
//
//  cout<<"Product: "<<endl;
//  gsl_matrix_cout( gsl_matrix_mul(cudasvd->U, gsl_matrix_mul(S, gsl_matrix_trans(cudasvd->V))) );

}

void testQR(){
//  srand(101);
//  int m = 4;
//  int n = 3;
//  gsl_matrix* A  = gsl_matrix_calloc(m,n);
//  rand();rand();rand();
//  for(int i=0;i<m;i++) {
//    for (int j = 0; j < n; j++) {
//      gsl_matrix_set(A, i, j, rand() % 4);
//    }
//  }
  srand(14);
  rand();rand();rand();
//  int m = 7;
//  int n = 10;
  int m = 5000;
  int n = 1000;
  cout<<m<<" x "<<n<<endl;
  gsl_matrix* A  = gsl_matrix_calloc(m,n);
  for(int i=0;i<m;i++) {
    for (int j = 0; j < n; j++) {
      gsl_matrix_set(A, i, j, (rand()%2==0)?(rand()*1.0/RAND_MAX):(0.0));
//      gsl_matrix_set(A, i, j, rand()%3);
    }
  }

  for(int j=0;j<n;j++){
    gsl_matrix_set(A,3,j, gsl_matrix_get(A,2,j));
  }

//  std::cout<<"A:"<<endl;
//  for(int i=0;i<m;i++){ for(int j=0;j<n;j++) printf(" %5.2f",gsl_matrix_get(A,i,j)); std::cout<<std::endl;}


  double start, duration;

  start = get_wall_time();
  SVD* svd = new SVDMKL(A);//SVD::createSVD(A);
  Nullspace* ns2 = new NullspaceSVD(svd);
  ns2->updateFromMatrix();
  duration = ( get_wall_time() - start );
  cout<<"SVD took "<<duration<<"secs"<<endl;
//  std::cout<<"N_SVD:"<<endl;
//  gsl_matrix_cout(ns2.getBasis());

  start = get_wall_time();
  TransposeQR* qr = new TransposeQR(A);
  Nullspace* ns = new NullspaceQR(qr);
  ns->updateFromMatrix();
  duration = ( get_wall_time() - start );
  cout<<"QR took "<<duration<<"secs"<<endl;
//  std::cout<<"N:"<<endl;
//  gsl_matrix_cout(ns.getBasis());

//  std::cout<<"A·N:"<<endl;
//  gsl_matrix_cout(gsl_matrix_mul(A, ns.getBasis()));
//  std::cout<<"A·N_SVD:"<<endl;
//  gsl_matrix_cout(gsl_matrix_mul(A, ns2.getBasis()));


//  gsl_matrix_cout(A);
//
//  double start, duration;

//  start = get_wall_time();
//  QR* mklqr = new QRMKL(A);
//  mklqr->updateFromMatrix();
//  mklqr->print();
//  duration = ( get_wall_time() - start );
//  cout<<"MKL took  "<<duration<<"secs"<<endl;


//  gsl_matrix* S = gsl_matrix_calloc(A->size1, A->size2);
//  for(int i=0;i<std::min(A->size1,A->size2);i++){
//    gsl_matrix_set(S, i,i, gsl_vector_get(mklqr->S, i));
//  }
//  cout<<"Product: "<<endl;
//  gsl_matrix_cout( gsl_matrix_mul(mklqr->U, gsl_matrix_mul(S, gsl_matrix_trans(mklqr->V))) );

//  cout<<" ------------------------------------- "<<endl;

//  start = get_wall_time();
//  QR* gslqr = new QRGSL(A);
//  gslqr->updateFromMatrix();
//  gslqr->print();
//  duration = ( get_wall_time() - start );
//  cout<<"GSL took  "<<duration<<"secs"<<endl;

//  S = gsl_matrix_calloc(A->size1, A->size2);
//  for(int i=0;i<std::min(A->size1,A->size2);i++){
//    gsl_matrix_set(S, i,i, gsl_vector_get(cudasvd->S, i));
//  }
//
//  cout<<"Product: "<<endl;
//  gsl_matrix_cout( gsl_matrix_mul(cudasvd->U, gsl_matrix_mul(S, gsl_matrix_trans(cudasvd->V))) );

}
