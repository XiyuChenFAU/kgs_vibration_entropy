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

#ifndef POISSONPLANNER_H_
#define POISSONPLANNER_H_

#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include "metrics/Metric.h"
#include "SamplingOptions.h"
#include "core/Configuration.h"
#include "planners/SamplingPlanner.h"

class Molecule;
class Configuration;


/**
 * A sampling planner based on Poisson-disc sampling as described in Robert Bridsons 2007 SIGGRAPH paper.
 * Each new sample is generated by randomly picking an existing 'open' sample, make a predefined number of
 * attempts to find a valid perturbations and if succesful the perturbed sample will be added to the open
 * set. A valid perturbation gives a non-clashing sample that is not too close to existing samples. If
 * generating a valid perturbation fails the existing sample is moved to a set of 'closed' samples.
 *
 * This is repeated until stop_after samples have been generated, or until there are no more 'open' samples.
 */
class PoissonPlanner : public SamplingPlanner{
 public:
  PoissonPlanner(Molecule *, Move&, metrics::Metric&);
  ~PoissonPlanner();

  void GenerateSamples();

	ConfigurationList& Samples(){ return all_samples; }

  bool m_checkAll = false;
 private:
  /// Even if open_samples is non-empty sampling will stop after this many new samples have been generated
  const int stop_after;
  const int max_rejects_before_close; ///< Number perturbations that are tried before a sample is 'closed'
  const double m_bigRad;              ///< Largest allowed step-size
  const double m_lilRad;              ///< Smallest allowed distance between any two samples

  ConfigurationList open_samples;     ///< Non-closed samples
  ConfigurationList closed_samples;   ///< Samples that have tested more than max_rejects_before_close perturbations
  ConfigurationList all_samples;      ///< For convenience and return

	Molecule * protein;

  Configuration* m_root;

  /** The largest distance from a sample to any of its descendants */
  std::map<Configuration*,double> m_maxDist;

  /** Memoized distances between configuration pointers */
  std::map<std::pair<Configuration*,Configuration*>,double> m_distances;

  /** Compute distance between c1 and c2 using the memoized map if possible */
  double memo_distance(Configuration* c1, Configuration* c2);

  /** Collect all open or closed configurations within dist of conf and add them to ret. */
  void collectPossibleChildCollisions(Configuration* conf,
                                      std::vector<Configuration*>& ret);
  void collectPossibleChildCollisions(Configuration* conf,
                                      std::vector<Configuration*>& ret,
                                      Configuration* v);

  void updateMaxDists(Configuration* newConf);
  void updateMaxDists(Configuration* v, Configuration* newConf);
};

#endif /* POISSONPLANNER_H_ */
