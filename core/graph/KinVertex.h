#ifndef KGS_KINVERTEX_H
#define KGS_KINVERTEX_H


#include "core/graph/KinEdge.h"

class KinEdge;

class KinVertex {
public:
  const int id;
  Rigidbody * const m_rigidbody;
  std::vector<KinEdge*> m_edges;             ///< Child-m_edges after spanning tree has been created
  KinVertex *m_parent;                       ///< m_parent-vertex after spanning tree has been created
  bool Visited;   ///< When finding common ancestor, vertices are marked as visited up to the root
  Math3D::RigidTransform m_transformation;   ///< The transformation to apply to atoms in the rigid body

  bool isRibose;

  KinVertex();
  KinVertex(int id, Rigidbody* rb);
  virtual ~KinVertex();

  void addEdge(unsigned int neighbor_vertex_id, KinEdge *edge);
  KinEdge* findEdge(KinVertex* v) const;
  virtual void setParent(KinVertex* v);
  void print() const;

  void forwardPropagate();
private:
  void TransformAtomPosition(Math3D::RigidTransform *trsfm);
  void transformAtoms();
};


#endif //KGS_KINVERTEX_H
