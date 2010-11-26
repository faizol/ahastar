#include "IncidentEdgesExpansionPolicy.h"

#include "graph.h"
#include "graphAbstraction.h"
#include <cassert>

IncidentEdgesExpansionPolicy::IncidentEdgesExpansionPolicy(
		graphAbstraction* map) : SelectiveExpansionPolicy()
{
	this->map = map;
}

IncidentEdgesExpansionPolicy::~IncidentEdgesExpansionPolicy()
{
}

void IncidentEdgesExpansionPolicy::expand(node* n)
{
	g = map->getAbstractGraph(n->getLabelL(kAbstractionLevel));
	ExpansionPolicy::expand(n);
}

node* IncidentEdgesExpansionPolicy::first_impl()
{
	which = 0;
	return n();
}

node* IncidentEdgesExpansionPolicy::n_impl()
{
	node* neighbour = 0;
	if(g && target && which < target->getNumEdges())
	{
		edge* e = target->getEdge(which);
		assert(e);

		int neighbourid = e->getFrom()==target->getNum()?e->getTo():e->getFrom();
		neighbour = g->getNode(neighbourid);
	}
	return neighbour;
}

node* IncidentEdgesExpansionPolicy::next_impl()
{
	node* retVal = 0;
	if(hasNext())
	{
		which++;
		retVal = n();
	}

	return retVal;
}

bool IncidentEdgesExpansionPolicy::hasNext()
{
	if(target && (which+1) < target->getNumEdges())
		return true;
	return false;
}

double IncidentEdgesExpansionPolicy::cost_to_n()
{
	edge* e = target->getEdge(which);
	return e->getWeight();
}