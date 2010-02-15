/*
 *  HPACluster.cpp
 *  hog
 *
 *  Created by dharabor on 11/11/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "HPACluster.h"
#include "HPAClusterAbstraction.h"
#include "ClusterNode.h"
#include "HPAUtil.h"
#include "IEdgeFactory.h"

#include "ClusterAStar.h"
#include "graph.h"

#include <stdexcept>
#include <sstream>
#include <string>

unsigned HPACluster::uniqueClusterIdCnt = 0;

HPACluster::HPACluster(const int x, const int y, const int _width, const int _height, AbstractClusterAStar* _alg) throw(std::invalid_argument)
{
	init(x, y, _width, _height, _alg);
}

HPACluster::HPACluster(const int x, const int y, const int _width, const int _height) throw(std::invalid_argument)
{
	init(x, y, _width, _height, new ClusterAStar());

}

void HPACluster::init(const int x, const int y, const int _width, const int _height, AbstractClusterAStar* _alg) throw(std::invalid_argument)
{
	if(_width <= 0 || _height <= 0)
		throw std::invalid_argument("HPACluster::HPACluster: cluster height and width cannot be <= 0");
	if(x < 0 || y < 0)
		throw std::invalid_argument("HPACluster::HPACluster: cluster (x,y) coordinates must be >= 0");
	if(_alg == NULL)
		throw std::invalid_argument("HPACluster::HPACluster: search algorithm parameter cannot be null");

	this->clusterId = ++uniqueClusterIdCnt;
	startx = x;
	starty = y;
	width = _width;
	height = _height;
	alg = _alg;
}

HPACluster::~HPACluster()
{
	delete alg;
	nodes.erase(nodes.begin(), nodes.end());
	parents.erase(parents.begin(), parents.end());
}


void HPACluster::addNode(node* _mynode) throw(std::invalid_argument)
{
	ClusterNode* mynode = dynamic_cast<ClusterNode*>(_mynode);
	if(mynode == NULL)
	{
		std::ostringstream oss;
		oss << "tried to add null node to cluster at origin: "<<this->getHOrigin()<<", "<<this->getVOrigin()<<" with id: "<<this->getClusterId();
		throw std::invalid_argument(oss.str().c_str());
	}

	if(nodes[mynode->getUniqueID()] != NULL) // already added
		return;

	int nx = mynode->getLabelL(kFirstData);
	int ny = mynode->getLabelL(kFirstData+1);
	
	if(nx < this->getHOrigin() || nx >= this->getHOrigin() + this->getWidth())
	{
		std::ostringstream oss;
		oss << "node @ " << nx<<","<<ny<<" has x coordinate outside cluster boundary. Cluster horigin: "<<this->getHOrigin();
		oss <<" yorigin: "<<this->getVOrigin()<<" width: "<<this->getWidth()<<" height: "<<this->getHeight();
		throw std::invalid_argument(oss.str().c_str());
	}
	
	if(mynode->getParentClusterId() != -1)
	{
		std::ostringstream oss;
		oss << "node @ "<< nx <<","<< ny <<" is already assigned to cluster "<<mynode->getParentClusterId();
		throw std::invalid_argument(oss.str().c_str());
	}
			
	mynode->setParentClusterId(this->getClusterId());
	nodes[mynode->getUniqueID()] = mynode;
}

void HPACluster::addParent(node* _parentnode, HPAClusterAbstraction* hpamap) throw(std::invalid_argument)
{
	if(hpamap == NULL || _parentnode == NULL)
		throw std::invalid_argument("HPACluster::addParent called with NULL arguments");

	ClusterNode* parentnode = dynamic_cast<ClusterNode*>(_parentnode);

	if(parentnode == NULL)
		throw std::invalid_argument("HPACluster::addParent; failed trying to cast node parameter to type ClusterNode");

	if(parents.find(parentnode->getUniqueID()) != parents.end())
		return; // node already in parents collection
	
	if(parentnode->getParentClusterId() != -1 && parentnode->getParentClusterId() != this->getClusterId())
	{
			std::stringstream ss;
			ss << "HPACluster::addParent: Tried to add to cluster "<<this->getClusterId() << " node @ (" <<parentnode->getLabelL(kFirstData)<<
				","<<parentnode->getLabelL(kFirstData+1)<< ") but this node is already assigned to cluster "<<parentnode->getParentClusterId()<<std::endl;
			throw std::invalid_argument(ss.str());
	}
	else
		parentnode->setParentClusterId(this->getClusterId());

	if(hpamap->getAbstractGraph(1)->getNode(parentnode->getNum()) == NULL)
	{
			std::stringstream ss;
			ss << "HPACluster::addParent: Tried to add to cluster "<<this->getClusterId() << " node @ (" <<parentnode->getLabelL(kFirstData)<<
				","<<parentnode->getLabelL(kFirstData+1)<< ") but this node is not in the abstract graph "<<std::endl;
			throw std::invalid_argument(ss.str());
	}
	
	this->connectParent(parentnode,hpamap);	
	parents[parentnode->getUniqueID()] = parentnode;
}

void HPACluster::removeParent(int clusterId)
{
	node* n = nodes[clusterId];
	if(n != 0)
	{
		nodes.erase(clusterId); // only removed from parents hashmap
	}	
}

/* add all traversable nodes in the cluster area to the cluster */
void HPACluster::addNodesToCluster(HPAClusterAbstraction* aMap) throw(std::invalid_argument)
{

	if(aMap == NULL)
		throw std::invalid_argument("tried to add nodes to cluster but cluster abstraction parameter is null");
		
	for(int x=this->getHOrigin(); x<getHOrigin()+getWidth(); x++)
		for(int y=this->getVOrigin(); y<getVOrigin()+getHeight(); y++)
		{	
			node* n = aMap->getNodeFromMap(x,y);
			if(n)
				addNode(aMap->getNodeFromMap(x,y));
		}
}


// Connects a new parent node with every other other parent node in the cluster by using A*. 
// Each successful search results in a new edge being added to the abstract graph.
void HPACluster::connectParent(node* absStart, HPAClusterAbstraction* hpamap)
{	
	for(nodeTable::iterator nodeIter = parents.begin(); nodeIter != parents.end(); nodeIter++)
	{
		node* absGoal = (*nodeIter).second;
		graph* absg = hpamap->getAbstractGraph(1);
		
		alg->setCorridorNodes(&nodes);

		// get low-level nodes
		node* from = hpamap->getNodeFromMap(absStart->getLabelL(kFirstData),absStart->getLabelL(kFirstData+1)); 
		node* to = hpamap->getNodeFromMap(absGoal->getLabelL(kFirstData),absGoal->getLabelL(kFirstData+1)); 

		path* solution = alg->getPath(hpamap, from, to);
		if(solution != 0)
		{
			double dist = hpamap->distance(solution);
			edge* e = new edge(absStart->getNum(), absGoal->getNum(), dist);
			absg->addEdge(e);
			hpamap->addPathToCache(e, solution);				
			//std::cout << "\n adding way cool edege for cluster "<<getClusterId();
		}

		/* record some metrics about the operation */
		hpamap->setNodesExpanded(hpamap->getNodesExpanded() + alg->getNodesExpanded());
		hpamap->setNodesTouched(hpamap->getNodesTouched() + alg->getNodesTouched());
		hpamap->setPeakMemory(alg->getPeakMemory()>hpamap->getPeakMemory()?alg->getPeakMemory():hpamap->getPeakMemory());
		hpamap->setSearchTime(hpamap->getSearchTime() + alg->getSearchTime());
	}
}

void HPACluster::buildEntrances(HPAClusterAbstraction* hpamap) throw(std::invalid_argument)
{
	if(hpamap == NULL)
		throw std::invalid_argument("HPACluster::buildEntrances: HPAClusterAbstraction parameter cannot be null");
	
	buildHorizontalEntrances(hpamap);
	buildVerticalEntrances(hpamap);
}

// Each cluster only considers veritcal entrances along the length of its eastern border. 
// A naive method would duplicate the creation of some entrances 
void HPACluster::buildVerticalEntrances(HPAClusterAbstraction* hpamap)
{	
	int mapwidth = hpamap->getMap()->getMapWidth();
	int x = this->getHOrigin()+this->getWidth();
	if(x == mapwidth)
		return; 

	// scan for vertical entrances along the eastern border 
	int y = this->getVOrigin();
	while(y < this->getVOrigin()+this->getHeight())
	{
		int length = findVerticalEntranceLength(x,y, hpamap);
	
		// build transition points; long entrances have 2, short entrances have 1.
		if(length == 0)
			y++;
		else
		{
			if(length >= MAX_SINGLE_TRANSITION_ENTRANCE_SIZE) 
			{
				// place one transition point at each end of the entrance area
				node* endpoint1 = hpamap->getNodeFromMap(x, y); // NB: (x,y) is inside eastern neighbour
				node* endpoint2 = hpamap->getNodeFromMap(x-1, y);
				this->addTransitionPoint(endpoint1, endpoint2, hpamap);

				endpoint1 = hpamap->getNodeFromMap(x, y+length-1); 
				endpoint2 = hpamap->getNodeFromMap(x-1, y+length-1);
				this->addTransitionPoint(endpoint1, endpoint2, hpamap);			
			}
			else
			{
				// place a transition point in the middle of the entrance area 
				int ty = y + (length/2);
				int tx = x;
				node* endpoint1 = hpamap->getNodeFromMap(tx, ty); 
				node* endpoint2 = hpamap->getNodeFromMap(tx-1, ty);
				this->addTransitionPoint(endpoint1, endpoint2, hpamap);
			}
			y += length;
		}
	}
}

int HPACluster::findVerticalEntranceLength(int x, int y, HPAClusterAbstraction* hpamap)
{
	int length = 0;
	while(y < this->getVOrigin()+this->getHeight())
	{
		if(hpamap->getNodeFromMap(x, y) == NULL || hpamap->getNodeFromMap(x-1, y) == NULL)
			break;
		y++;
		length++;
	}
	
	return length;
}

void HPACluster::buildHorizontalEntrances(HPAClusterAbstraction* hpamap)
{
	int mapheight = hpamap->getMap()->getMapHeight();
	int y = this->getVOrigin()+this->getHeight();
	if(y == mapheight)
		return; 

	// scan for vertical entrances along the eastern border 
	int x = this->getHOrigin();
	while(x < this->getHOrigin()+this->getWidth())
	{
		int length = findHorizontalEntranceLength(x,y, hpamap);
	
		// build transition points; long entrances have 2, short entrances have 1.
		if(length == 0)
			x++;
		else
		{
			if(length >= MAX_SINGLE_TRANSITION_ENTRANCE_SIZE) 
			{
				// place one transition point at each end of the entrance area
				node* endpoint1 = hpamap->getNodeFromMap(x, y); // NB: (x,y) is inside eastern neighbour
				node* endpoint2 = hpamap->getNodeFromMap(x, y-1);
				this->addTransitionPoint(endpoint1, endpoint2, hpamap);

				endpoint1 = hpamap->getNodeFromMap(x+length-1, y); 
				endpoint2 = hpamap->getNodeFromMap(x+length-1, y-1);
				this->addTransitionPoint(endpoint1, endpoint2, hpamap);			
			}
			else
			{
				// place a transition point in the middle of the entrance area 
				int tx = x + (length/2);
				int ty = y;
				node* endpoint1 = hpamap->getNodeFromMap(tx, ty); 
				node* endpoint2 = hpamap->getNodeFromMap(tx, ty-1);
				this->addTransitionPoint(endpoint1, endpoint2, hpamap);
			}
			x += length;
		}
	}
}

int HPACluster::findHorizontalEntranceLength(int x, int y, HPAClusterAbstraction* hpamap)
{
	int length = 0;
	while(x < this->getHOrigin()+this->getWidth())
	{
		if(hpamap->getNodeFromMap(x, y) == NULL || hpamap->getNodeFromMap(x, y-1) == NULL)
			break;
		x++;
		length++;
	}
	
	return length;
}

// A transition point is composed of two adjacent nodes in separate clusters.
// Creating a new transition point requires the insertion of two nodes from the original map into the abstract graph.
// These nodes are connected to each other via an inter-edge and to every other node within their respective clusters (if reachable) by intra-edges.
void HPACluster::addTransitionPoint(node* from, node* to, HPAClusterAbstraction* hpamap)
{
	// add internodes; first try to reuse existing nodes from the abstract graph, else create new ones.
	int abstractionLevel = 1;
	graph *g = hpamap->getAbstractGraph(abstractionLevel);
	ClusterNode* absfrom = dynamic_cast<ClusterNode*>(g->getNode(from->getLabelL(kParent)));
	ClusterNode* absto = dynamic_cast<ClusterNode*>(g->getNode(to->getLabelL(kParent)));
	
	if(absfrom == NULL)
	{
		absfrom = dynamic_cast<ClusterNode*>(from->clone());
		absfrom->setLabelL(kAbstractionLevel, abstractionLevel);
		g->addNode(absfrom);
		
		HPACluster* parentCluster = hpamap->getCluster(absfrom->getParentClusterId());
		parentCluster->addParent(absfrom, hpamap); // also creates intra-edges
		from->setLabelL(kParent, absfrom->getNum());
	}
	if(absto == NULL)
	{	
		absto = dynamic_cast<ClusterNode*>(to->clone());
		absto->setLabelL(kAbstractionLevel, abstractionLevel);
		g->addNode(absto);
		
		HPACluster* parentCluster = hpamap->getCluster(absto->getParentClusterId());
		parentCluster->addParent(absto, hpamap);
		to->setLabelL(kParent, absto->getNum());
	}
	
	// add inter-edge
	edge* e = hpamap->getEdgeFactory()->newEdge(absfrom->getNum(), absto->getNum(), 1.0);
	g->addEdge(e);
}

// debug method
void HPACluster::printParents()
{
	nodeTable::iterator it;
	it = parents.begin();
	while(it != parents.end())
	{
		node* n = (*it).second;
		std::cout << "parent node: " <<"addr: "<<&(*n)<<" num: "<<n->getUniqueID() <<" ("<<n->getLabelL(kFirstData)<<","<<n->getLabelL(kFirstData+1)<<")"<<std::endl;
		it++;
	}
}
