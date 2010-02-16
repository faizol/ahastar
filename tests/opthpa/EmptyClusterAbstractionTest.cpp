#include "EmptyClusterAbstractionTest.h"

#include "ClusterNodeFactory.h"
#include "EdgeFactory.h"
#include "EmptyCluster.h"
#include "EmptyClusterAbstraction.h"
#include "EmptyClusterFactory.h"
#include "HPAUtil.h"
#include "map.h"
#include "TestConstants.h"

CPPUNIT_TEST_SUITE_REGISTRATION( EmptyClusterAbstractionTest );

void EmptyClusterAbstractionTest::setUp()
{
}

void EmptyClusterAbstractionTest::tearDown()
{
}

void EmptyClusterAbstractionTest::buildClustersDecomposesTheMapIntoEmptyClusters()
{
	EmptyClusterAbstraction ecmap(new Map(hpastartest.c_str()), new EmptyClusterFactory(), 
			new ClusterNodeFactory(), new EdgeFactory());
//	ecmap.setVerbose(true);
	ecmap.buildClusters();

	int numExpectedClusters = 9;

	CPPUNIT_ASSERT_EQUAL_MESSAGE("failed to build correct # of clusters", 
			numExpectedClusters, ecmap.getNumClusters());

	int csizes[42] = {15, 3, 2, 6, 5, 4, 2, 1, 1};

	int cszIndex = 0;
	cluster_iterator it = ecmap.getClusterIter();
	EmptyCluster* cluster = ecmap.clusterIterNext(it);
	while(cluster)
	{
		CPPUNIT_ASSERT_EQUAL_MESSAGE("cluster has wrong # of nodes", csizes[cszIndex],
				cluster->getNumNodes());
		cluster = ecmap.clusterIterNext(it);
		cszIndex++;
		
	}
}

void EmptyClusterAbstractionTest::buildEntrancesConnectsAllClusters()
{
	EmptyClusterAbstraction ecmap(new Map(hpastartest.c_str()), new EmptyClusterFactory(), 
			new ClusterNodeFactory(), new EdgeFactory());
//	ecmap.setVerbose(true);
	ecmap.buildClusters();

	int abstractionLevel = 1;
	graph *g = ecmap.getAbstractGraph(abstractionLevel);
	int expectedNumAbstractNodes = g->getNumNodes(); // before
	int expectedNumAbstractEdges = g->getNumEdges(); // before

	ecmap.buildEntrances();

	expectedNumAbstractEdges += 16; // # of transition points across all entrances

	CPPUNIT_ASSERT_EQUAL_MESSAGE("abstract edge count is wrong", expectedNumAbstractEdges, 
			g->getNumEdges());
}

void EmptyClusterAbstractionTest::insertStartAndGoalNodesIntoAbstractGraphWorksAsAdvertised()
{
	EmptyClusterAbstraction ecmap(new Map(hpastartest.c_str()), new EmptyClusterFactory(), 
			new ClusterNodeFactory(), new EdgeFactory());
//	ecmap.setVerbose(true);
	ecmap.buildClusters();
	ecmap.buildEntrances();

	int abstractionLevel = 1;
	graph *g = ecmap.getAbstractGraph(abstractionLevel);
	int expectedNumAbstractNodes = g->getNumNodes(); // before
	int expectedNumAbstractEdges = g->getNumEdges(); // before

	node* start = ecmap.getNodeFromMap(1, 1);
	node* goal = ecmap.getNodeFromMap(3, 1);
	//ecmap.setVerbose(true);
	ecmap.insertStartAndGoalNodesIntoAbstractGraph(start, goal);

	expectedNumAbstractEdges += 8;
	expectedNumAbstractNodes += 2;

	CPPUNIT_ASSERT_EQUAL_MESSAGE("wrong # of nodes in abstract graph following insertion of s+g",
			expectedNumAbstractNodes, g->getNumNodes());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("wrong # of edges in abstract graph following insertion of s+g",
			expectedNumAbstractEdges, g->getNumEdges());

	node* absStart = g->getNode(start->getLabelL(kParent));
	node* absGoal = g->getNode(goal->getLabelL(kParent));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("start node has wrong abstract parent!", true, 
			start->getLabelL(kParent) == absStart->getNum());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("goal node has wrong abstact parent!", true, 
			goal->getLabelL(kParent) == absGoal->getNum());

	int expectedDegree = 4;
	CPPUNIT_ASSERT_EQUAL_MESSAGE("start node has wrong degree, ", expectedDegree,
			absStart->getNumEdges());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("goal node has wrong degree, ", expectedDegree,
			absGoal->getNumEdges());

	// check lengths of edges; should be equal to set of mincost edges connecting each s+g 
	// to the closest node on each border
	double sEdgeWeights[4] = {1, 1, 1, 3};
	double gEdgeWeights[4] = {1, 1, 3, 1};
	edge_iterator ei = absStart->getEdgeIter();
	edge* e = absStart->edgeIterNext(ei);
	int index=0;
	while(e)
	{
		node* borderNode = g->getNode(e->getTo());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("absStart edge weight != manhattan distance",
				sEdgeWeights[index], e->getWeight());
		index++;
		e = absStart->edgeIterNext(ei);

	}

	ei = absGoal->getEdgeIter();
	e = absGoal->edgeIterNext(ei);
	index=0;
	while(e)
	{
		node* borderNode = g->getNode(e->getTo());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("absGoal edge weight != manhattan distance",
				gEdgeWeights[index], e->getWeight());
		index++;
		e = absGoal->edgeIterNext(ei);
	}

}

void EmptyClusterAbstractionTest::insertStartAndGoalNodesIntoAbstractGraphDoesNotAddAnythingIfStartOrGoalExistInGraphAlready()
{
	EmptyClusterAbstraction ecmap(new Map(hpastartest.c_str()), new EmptyClusterFactory(), 
			new ClusterNodeFactory(), new EdgeFactory());
//	ecmap.setVerbose(true);
	ecmap.buildClusters();
	ecmap.buildEntrances();

	int abstractionLevel = 1;
	graph *g = ecmap.getAbstractGraph(abstractionLevel);
	int expectedNumAbstractNodes = g->getNumNodes(); // before
	int expectedNumAbstractEdges = g->getNumEdges(); // before

	node* start = ecmap.getNodeFromMap(0, 1);
	node* goal = ecmap.getNodeFromMap(4, 1);
	//ecmap.setVerbose(true);
	ecmap.insertStartAndGoalNodesIntoAbstractGraph(start, goal);

	CPPUNIT_ASSERT_EQUAL_MESSAGE("wrong # of nodes in abstract graph following insertion of s+g",
			expectedNumAbstractNodes, g->getNumNodes());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("wrong # of edges in abstract graph following insertion of s+g",
			expectedNumAbstractEdges, g->getNumEdges());
}

void EmptyClusterAbstractionTest::manhattanComputesTileDistanceBetweenTwoNodes()
{
	EmptyClusterAbstraction ecmap(new Map(hpastartest.c_str()), new EmptyClusterFactory(), 
			new ClusterNodeFactory(), new EdgeFactory());

	node* start = ecmap.getNodeFromMap(1, 1);
	node* goal = ecmap.getNodeFromMap(5, 2);

	int expectedCost = 5;
	CPPUNIT_ASSERT_EQUAL_MESSAGE("manhattan computes wrong cost", expectedCost, 
		   ecmap.manhattan(start, goal));	
	CPPUNIT_ASSERT_EQUAL_MESSAGE("manhattan computes wrong cost", expectedCost, 
		   ecmap.manhattan(goal, start));	
}
