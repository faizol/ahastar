#ifndef FLEXIBLEASTAR_H
#define FLEXIBLEASTAR_H

// FlexibleAStar.h
//
// A standard implementation of the A* search algorithm which can be
// readily modified with alternative node expansion algorithms and different 
// heuristics.
//
// @author: dharabor
// @created: 26/09/2010


#include "searchAlgorithm.h"
#include <map>
#include <string>

class DebugUtility;
class ExpansionPolicy;
class heap;
class Heuristic;
class FlexibleAStar : public searchAlgorithm
{
	public:
		FlexibleAStar(ExpansionPolicy*, Heuristic*);
		virtual ~FlexibleAStar();

		virtual const char *getName();
		virtual path *getPath(graphAbstraction *aMap, node *from, node *goal,
				reservationProvider *rp = 0);

		bool markForVis;	

	protected:
		ExpansionPolicy* policy;
		Heuristic* heuristic;
		graphAbstraction* map;

		path* search(graph* g, node* from, node* goal);
		void relaxNode(node* from, node* to, node* goal, double cost, heap* openList);
		void expand(node* current, node* goal, heap* openList,
				std::map<int, node*>* closedList);
		path* extractBestPath(node* goal);

	private:
		void closeNode(node* current, std::map<int, node*>* closedList);
		bool checkParameters(graph* g, node* from, node* to);
		DebugUtility* debug;
};

#endif
