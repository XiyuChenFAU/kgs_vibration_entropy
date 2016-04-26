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
#include "RRTPlanner.h"

#include <iomanip>
#include <stack>

#include "core/Molecule.h"
#include "core/Chain.h"
#include "Logger.h"
#include "core/Transformation.h"
#include "CTKTimer.h"
#include "metrics/RMSD.h"
#include "metrics/Dihedral.h"


using namespace std;

//const double RMSD_DIFF_THRESHOLD = 0.01;
const double MOV_DIH_THRESHOLD = 0.001;

extern double selectNodeTime;


RRTPlanner::RRTPlanner(Molecule *protein, Move& move, metrics::Metric& metric, Direction& direction):
	SamplingPlanner(move,metric),
	m_protein(protein),
  direction(direction),
	m_max_distance(SamplingOptions::getOptions()->explorationRadius)
{
	m_numDOFs = m_protein->m_spanning_tree->getNumDOFs();//Edges.size();
	Configuration *pSmp = new Configuration(m_protein);
  pSmp->updateMolecule();
  pSmp->computeCycleJacobianAndNullSpace();
	m_protein->m_conf = pSmp;
	m_protein->m_conf_backup = pSmp;
	m_target = nullptr;
	m_samples.push_back(pSmp);
	pSmp->m_vdwEnergy = 99999;
	pSmp->m_id = 0; // root

	m_deform_mag = 0.25;
	m_rand_radius = 2;

	m_numBuckets  = NUM_BINS;
	m_bucket_size = m_max_distance/(m_numBuckets-1); //=0.25-0.50 ? the last bucket holds values bigger than m_max_distance

	//m_bucket_size = 2*options.stepSize;
	//m_numBuckets = min(MAX_BUCKET_NUM, int(m_max_distance /m_bucket_size +1));
	//m_bucket_size = m_max_distance/(m_numBuckets-1);

	distance_buckets[0].push_back(pSmp);
	m_current_max_bucket_id = 0;

	m_top_min_rmsd = 99999;
	m_top_min_rmsd_id = -1;
	m_numSamples = 0;
	m_minMovDihDistance = 99999;
	m_minMovDihDistance_id = -1;
}

RRTPlanner::~RRTPlanner(){
	Configuration *pSmp;
	for (list<Configuration *>::iterator iter = m_samples.begin(); iter != m_samples.end(); iter++)
	{
		pSmp = *iter;
		delete pSmp;
	}

}


void RRTPlanner::GenerateSamples() {
	string out_path = SamplingOptions::getOptions()->workingDirectory;
	string name = m_protein->getName();
	int nBatch = SamplingOptions::getOptions()->samplesToGenerate;

	int sample_id = 0, max_depth = 0, failed_trials = 0, total_trials = 0;
	Configuration *pTarget = nullptr, *pClosestSmp, *pNewSmp = nullptr;
	gsl_vector *gradient = gsl_vector_alloc(m_protein->totalDofNum());

	//Save initial file (for movie)
	writeNewSample(m_samples.front(), m_samples.front(), sample_id);

	bool createNewTarget = false;

	while (sample_id < nBatch) {
		++total_trials;

		CTKTimer timer;
		double start_time = timer.getTimeNow();

		if (SamplingOptions::getOptions()->sampleRandom || pNewSmp == nullptr || createNewTarget) {
			log("dominik") << "Generating new target, getting new seed" << endl;
			createNewTarget = false;
			pTarget = GenerateRandConf();//used in selection ONLY if no m_target m_protein specified
			pClosestSmp = SelectNodeFromBuckets(pTarget);
			//pClosestSmp = SelectNodeFromBuckets(pTarget,nBatch);
			double end_time = timer.getTimeNow();
			selectNodeTime += end_time - start_time;
		} else {
			log("dominik") << "Using latest sample as seed" << endl;
			pClosestSmp = m_samples.back();
		}

		direction.gradient(pClosestSmp, nullptr, gradient);
		pNewSmp = move.move(pClosestSmp, gradient);

		if (pNewSmp != nullptr) {
			if (!pNewSmp->updatedMolecule()->inCollision()) {
				++sample_id;
				m_numSamples = sample_id;
				pNewSmp->m_distanceToIni = m_metric.distance(pNewSmp, m_samples.front());
				pNewSmp->m_distanceToParent = m_metric.distance(pNewSmp, pClosestSmp);
				pNewSmp->m_id = sample_id;
				m_samples.push_back(pNewSmp);

				// Put pNewSmp in the appropriate bucket
				int bucket_id = int(floor(pNewSmp->m_distanceToIni / m_bucket_size));
				if (bucket_id >= NUM_BINS)
					bucket_id = NUM_BINS - 1;
				//cout<<"Bucket id: "<<bucket_id<<", max bucket id: "<<m_current_max_bucket_id<<", bucket size: "<<m_bucket_size<<endl;

				distance_buckets[bucket_id].push_back(pNewSmp);
				if (bucket_id > m_current_max_bucket_id)
					m_current_max_bucket_id = bucket_id;

				writeNewSample(pNewSmp, m_samples.front(), sample_id);


				if (pNewSmp->m_treeDepth > max_depth)
					max_depth = pNewSmp->m_treeDepth;

				double distToRandGoal = m_metric.distance(pNewSmp, pTarget);

				//log("samplingStatus") << "> New structure: newpdb_"<<sample_id<<".pdb .. RMSD to initial: "<< pNewSmp->m_rmsd_initial<<endl;
				//log("samplingStatus") << "> New structure: "<<name<<"_new_"<<sample_id<<".pdb .. Distance to initial: "<< setprecision(6)<<pNewSmp->m_distanceToIni<<" .. Distance to current target: "<< setprecision(3)<<distToRandGoal<<" .. Null-space dimension: "<<pNewSmp->CycleNullSpace->nullspaceSize<<endl;
				log("samplingStatus") << "> New structure: " << name << "_new_" << sample_id << ".pdb";
				log("samplingStatus") << " .. Distance to initial: " << setprecision(6) << pNewSmp->m_distanceToIni;
				log("samplingStatus") << " .. Distance to current target: " << setprecision(3) << distToRandGoal;
				//TODO: Dont just calculate nullspace for this
				log("samplingStatus") << " .. Null-space dimension: " << pNewSmp->getNullspace()->NullspaceSize();
				log("samplingStatus") << endl;

				if (distToRandGoal <= MOV_DIH_THRESHOLD) {//current target reached
					delete pTarget;
					createNewTarget = true;
				}
				//Check if target has been reached!
				//if(m_target != nullptr && (m_top_min_rmsd < RMSD_DIFF_THRESHOLD )){
				//cout<<"Reached target, not creating more samples!"<<" rmsd: "<<m_top_min_rmsd<<", dih: "<<m_minMovDihDistance<<endl;
				//break;
				//}
			} else {
				++failed_trials;
				//Only for the straight path testing
				//if( !options.sampleRandom )
				//return nullptr;
				delete pTarget;
				createNewTarget = true;
			}
		}
	}
	gsl_vector_free(gradient);
}

Configuration* RRTPlanner::GenerateRandConf() {
	Configuration *pNewSmp = new Configuration(m_protein);

	double num_dofs = m_protein->totalDofNum();
	double length=0.0;
	for (int i=0; i<num_dofs; ++i) {
		pNewSmp->m_dofs[i] = Math3D::dPi * RandomN1P1();
		length+= pNewSmp->m_dofs[i] * pNewSmp->m_dofs[i];
	}
	length = sqrt(length);
	if(SamplingOptions::getOptions()->scaleToRadius){
		double factor = pow( Random01(), 1.0/num_dofs )*SamplingOptions::getOptions()->explorationRadius/length;
		for (int i=0; i<num_dofs; ++i) {
			pNewSmp->m_dofs[i] = factor * pNewSmp->m_dofs[i];
		}
	}

	pNewSmp->m_id = -1;
  pNewSmp->updateMolecule();
	pNewSmp->updateGlobalTorsions();
	return pNewSmp;
}

Configuration *RRTPlanner::SelectNodeFromBuckets (Configuration *pTarget) {
	Configuration *pSmp, *pMinSmp;
	double min_distance = 1000000.0;
	double distance;
	pMinSmp = nullptr;

	int selected_bucket_id;
	while ( pMinSmp==nullptr ) {
		do {
			selected_bucket_id = rand() % (m_numBuckets-1);
		} while ( distance_buckets[selected_bucket_id].empty() );
		log("dominik")<<"Seed from bucket: "<<selected_bucket_id<<endl;
		for (list<Configuration*>::iterator iter=distance_buckets[selected_bucket_id].begin(); iter!=distance_buckets[selected_bucket_id].end(); ++iter) {
			pSmp = *iter;
			if (pSmp->m_distanceToIni>m_max_distance) {
				continue;
			}
			// Using closest to random
			distance = m_metric.distance(pSmp,pTarget);

			if (distance<min_distance) {
				min_distance = distance;
				pMinSmp = pSmp;
			}
		}
	}

	return pMinSmp;
}	



