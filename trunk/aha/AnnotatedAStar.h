/*
 *  AnnotatedAStar.h
 *  hog
 *
	Search algorithm which supports different agent capabilities:
		- terrain traversal (agent differentiates between different types of terrain)
		- size (agents may occupy several tiles at a time)
		
	Intended for graphs produced by AnnotatedMapAbstraction and AnnotatedClusterAbstraction.
	
	TODO: HOG's AStar hierarchy is a mess. Lots of re-implementations, no shared code etc. Need an abstract parent implementing things like
	edge relaxation, path extraction and drawing paths(maybe).
	
 *  Created by Daniel Harabor on 14/12/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef ANNOTATEDASTAR_H
#define ANNOTATEDASTAR_H

#include <ext/hash_map>
#include "heap.h"
#include "aStar3.h"
//#include "searchAlgorithm.h"
#include "graph.h"
#include "graphAbstraction.h"

namespace AAStarUtil {      
  typedef __gnu_cxx::hash_map<int,node*> NodeMap;
}

class AbstractAnnotatedAStar : public aStarOld
{
	public:
		virtual path* getPath(graphAbstraction*, node*, node*, int, int) = 0;
		virtual const char* getName() { return "AbstractAnnotatedAStar"; }
	
	protected:
		virtual bool evaluate(node* n, node* target, edge* e) = 0; // check if a node is traversable
		
		/* TODO: all these might need to be private at some point. */
		int searchterrain, minclearance; 
		AAStarUtil::NodeMap closedList;		
		heap *openList;
};

class AnnotatedAStar : public AbstractAnnotatedAStar
{
	public:
		virtual path* getPath(graphAbstraction*, node*, node*, int, int);
		virtual const char* getName() { return "AnnotatedAStar"; }
		
	protected:
		virtual bool evaluate(node* n, node* target, edge* e);
	
	private:
		tDirection getDirection(node* current, node* target); // TODO: move this to a common AStar base class
};

#endif