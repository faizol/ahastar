#include "FlexibleAStar.h"

#include "altheap.h"
#include "DebugUtility.h"
#include "ExpansionPolicy.h"
#include "fpUtil.h"
#include "graph.h"
#include "Heuristic.h"
#include "mapAbstraction.h"
#include "path.h"
#include "ProblemInstance.h"
#include "reservationProvider.h"
#include "timer.h"
#include "unitSimulation.h"

FlexibleAStar::FlexibleAStar(ExpansionPolicy* policy, Heuristic* heuristic)
	: searchAlgorithm()
{
	this->policy = policy;
	this->heuristic = heuristic;
	this->markForVis = true;
}

FlexibleAStar::~FlexibleAStar()
{
	delete policy;
	delete heuristic;
}

const char* 
FlexibleAStar::getName()
{
	return "FlexibleAStar";
}

path* 
FlexibleAStar::getPath(graphAbstraction *aMap, node *start, node *goal,
		reservationProvider *rp)
{
	debug = new DebugUtility(aMap, heuristic);
	policy->setProblemInstance(new ProblemInstance(start, goal, 
				dynamic_cast<mapAbstraction*>(aMap), heuristic));
	path* p = search(start, goal);
	delete debug;
	return p;
}

path* 
FlexibleAStar::search(node* start, node* goal)
{
	nodesExpanded=0;
	nodesTouched=0;
	searchTime =0;
	nodesGenerated = 0;

	if(verbose) 
	{
		std::cout << "getPath() mapLevel: ";
		std::cout <<start->getLabelL(kAbstractionLevel)<<std::endl;
	}

	if(!checkParameters(start, goal))
		return NULL;

	start->setLabelF(kTemporaryLabel, heuristic->h(start, goal));
	start->backpointer = 0;
	
	altheap openList(heuristic, goal, 30);
	std::map<int, node*> closedList;
	
	openList.add(start);
	path *p = NULL;
	
	Timer t;
	t.startTimer();
	while(1) 
	{
		node* current = ((node*)openList.remove()); 

		// check if the current node is the goal (early termination)
		if(current == goal)
		{
			closeNode(current, &closedList);
			p = extractBestPath(current);
			if(verbose)
				debug->printNode(std::string("goal found! "), current);
			break;
		}
		
		// expand current node
		expand(current, goal, &openList, &closedList);
		closeNode(current, &closedList);
				
		// terminate when the open list is empty
		if(openList.empty())
		{
			if(verbose) std::cout << "search failed. ";
			break;
		}
	}
	searchTime = t.endTimer();
	closedList.clear();

	if(verbose)
	{
		std::cout << "\n";
		debug->printPath(p); 
	}

	return p;	
}

void 
FlexibleAStar::closeNode(node* current, std::map<int, node*>* closedList)
{
	if(markForVis)
		current->drawColor = 2; // visualise expanded

	if(verbose)
	{	
		debug->printNode(std::string("closing... "), current);
		std::cout << " f: "<<current->getLabelF(kTemporaryLabel) <<std::endl;
	}
	closedList->insert(std::pair<int, node*>(current->getUniqueID(), current));


}

void 
FlexibleAStar::expand(node* current, node* goal, altheap* openList,
		std::map<int, node*>* closedList)
{
	// expand the current node
	if(verbose) 
	{
		double fVal = current->getLabelF(kTemporaryLabel);
		double gVal = fVal - heuristic->h(current, goal);
		debug->printNode(std::string("expanding... "), current);
		std::cout << " g: "<<gVal<<" f: "<<fVal<<std::endl;
	}

	nodesExpanded++;

	policy->expand(current);
	for(node* neighbour = policy->first(); neighbour != 0; 
			neighbour = policy->next())
	{
		nodesTouched++;			
		if(closedList->find(neighbour->getUniqueID()) == closedList->end()) 
		{
			if(openList->isIn(neighbour)) 
			{	
				double fVal = neighbour->getLabelF(kTemporaryLabel);
				relaxNode(current, neighbour, goal, policy->cost_to_n(), openList); 

				if(verbose) 
				{
					if(neighbour->getLabelF(kTemporaryLabel) < fVal)
					{
						debug->printNode("\trelaxing...", neighbour);
						std::cout << " gOld: "<< 
							(fVal - heuristic->h(neighbour, goal)) <<
							" fOld: "<< fVal;
						double fVal = neighbour->getLabelF(kTemporaryLabel);
						std::cout << " gNew: " <<
							(fVal - heuristic->h(neighbour, goal)) <<
							" fNew: "<<neighbour->getLabelF(kTemporaryLabel);
					}
					else
					{
						debug->printNode("\tcannot relax node...", neighbour);
					}
				}
			}
			else
			{
				if(verbose) 
					debug->printNode("\tgenerating...", neighbour);

				neighbour->setLabelF(kTemporaryLabel, MAXINT); // initial fCost 
				neighbour->setKeyLabel(kTemporaryLabel); // store priority here 
				neighbour->backpointer = 0;  // reset any marked edges 
				openList->add(neighbour);
				relaxNode(current, neighbour, goal, policy->cost_to_n(), openList); 
				nodesGenerated++;

				if(verbose)
				{
					double fVal = neighbour->getLabelF(kTemporaryLabel);
					std::cout << " g: "<<(fVal - heuristic->h(neighbour, goal))
							<< " fNew: "<< fVal;
				}
			}
			if(markForVis)
				neighbour->drawColor = 1; // visualise touched
		}
		else
		{
			if(verbose)
			{
				debug->printNode("\tclosed...", neighbour);
				double fCur = current->getLabelF(kTemporaryLabel);
				double gCur =  fCur - heuristic->h(current, goal);
				double gAlt = gCur + policy->cost_to_n();
				double fAlt = gAlt + heuristic->h(neighbour, goal);
				double fClosed = neighbour->getLabelF(kTemporaryLabel);
				std::cout << " (fClosed: "<<fClosed<<"; fAlt: "<<fAlt<<")";
			}
			debug->debugClosedNode(current, neighbour, policy->cost_to_n(), goal);
		}

		if(verbose)
			std::cout << std::endl;
	}
}

bool 
FlexibleAStar::checkParameters(node* from, node* to)
{
	if(!from || !to)
		return false;

	if(from->getUniqueID() == to->getUniqueID())
		return false;
		
	if(from->getLabelL(kFirstData) == to->getLabelL(kFirstData) 
			&& from->getLabelL(kFirstData+1) == to->getLabelL(kFirstData+1))
		return false;

	if(from->getLabelL(kAbstractionLevel) != to->getLabelL(kAbstractionLevel))
		return false;

	return true;
	
}

void 
FlexibleAStar::relaxNode(node* from, node* to, node* goal, double cost, 
		altheap* openList)
{
	double g_from = from->getLabelF(kTemporaryLabel) - heuristic->h(from, goal);
	double f_to = g_from + cost + heuristic->h(to, goal);
	
	if(fless(f_to, to->getLabelF(kTemporaryLabel)))
	{
		to->setLabelF(kTemporaryLabel, f_to);
		to->backpointer = from;
		openList->decreaseKey(to);
	}
}

path* 
FlexibleAStar::extractBestPath(node* goal)
{
	path* p = 0;
	for(node* n = goal; n != 0; n = n->backpointer)
	{
		p = new path(n, p);
		if(markForVis)
			assert(n->drawColor == 2);
	}

	return p;
}

