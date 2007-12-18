/*
 *  AnnotatedMapTest.cpp
 *  hog
 *
 *  Created by Daniel Harabor on 3/12/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#include "TestHelper.h"
#include "AnnotatedMapAbstractionTest.h"

#include "mapAbstraction.h"
#include "map.h"
#include "graph.h"
#include "aStar3.h"
#include "AnnotatedAStar.h"

CPPUNIT_TEST_SUITE_REGISTRATION( AnnotatedMapAbstractionTest );

void AnnotatedMapAbstractionTest::setUp()
{
	// need to setup a map
	testmap = new Map(maplocation.c_str());
	ama = new AnnotatedMapAbstraction(testmap);
	g = ama->getAbstractGraph(0);
}

void AnnotatedMapAbstractionTest::tearDown()
{
	delete ama; // also kills testmap
}

/* 
	No test required here; pathable just calls AnnotatedAStar's getPath method; if that passes this must also. */
void AnnotatedMapAbstractionTest::PathableTest()
{	
}

void AnnotatedMapAbstractionTest::ValidateAnnotationsTest() 
{
	stringstream ss;
	for(int x=0; x<testmap->getMapWidth(); x++) 
	{
		for(int y=0; y<testmap->getMapHeight(); y++)
		{ 
			node *n = ama->getNodeFromMap(x,y);
			if(n)
				checkSingleNodeAnnotations(n, x, y);
		}
	}
}

void AnnotatedMapAbstractionTest::checkSingleNodeAnnotations(node* n, int x, int y)
{
			int nid = n->getNum(); //NB: should change to getUniqueId() but then getNode needs to switch to find or graphs use hashtables to store nodes/edges
		
			/* Test 1: check that missing edges have been re-added */
			node *nadj = ama->getNodeFromMap(x-1,y-1);
			if(nadj) 
				CPPUNIT_ASSERT_MESSAGE("expected edge; none found", g->findEdge(nid,nadj->getNum()) != 0);
			
			nadj = ama->getNodeFromMap(x,y-1);
			if(nadj)
				CPPUNIT_ASSERT_MESSAGE("expected edge; none found",g->findEdge(nid,nadj->getNum()) != 0);
				
			nadj = ama->getNodeFromMap(x-1,y);
			if(nadj)
				CPPUNIT_ASSERT_MESSAGE("expected edge; none found",g->findEdge(nid,nadj->getNum()) != 0);
			

			/* Test 2: validate clearance value annotations for each node. */
			int clVals[4];
			node* nadj2, *nadj3;

			nadj = ama->getNodeFromMap(x+1,y);
			nadj2 = ama->getNodeFromMap(x,y+1);
			nadj3 = ama->getNodeFromMap(x+1,y+1);
			
			/* check if node is traversable (anything not in the conjunction @ validterrains[3] is considered nontraversable & has clearance=0 */
			int nterrain = n->getTerrainType();
			if(nterrain&validterrains[3] != nterrain)
				CPPUNIT_ASSERT(clVals[0] == 0);
			else
				for(int i=0;i<3;i++) 
				{ 
					if(nadj && nadj2 && nadj3) // clearance value is dependent on clearance values of 3 neighbours
					{		
							clVals[0] = n->getClearance(validterrains[i]);
							clVals[1] = nadj->getClearance(validterrains[i]);
							clVals[2] = nadj2->getClearance(validterrains[i]);
							clVals[3] = nadj3->getClearance(validterrains[i]);

							// clearace >0 for combinations of terrain that include the basic (single) terrain type of the node
							if((validterrains[i]&n->getTerrainType())==n->getTerrainType()) 
							{
								/* get minimum clearance amongst neighbours */
								int min=MAXINT;
								for(int j=1;j<4;j++)
									if(clVals[j] < min)
										min = clVals[j];
								min++; // clearance = min among neighbours + base clearance (1); so we increment min
									
								CPPUNIT_ASSERT(((clVals[0] > 0) && (clVals[0] < MAXINT)));
								CPPUNIT_ASSERT(clVals[0] == min); 
							}
							else
								CPPUNIT_ASSERT(clVals[0] == 0);

							
							
									
					}
					else // some neighbours don't exist; check for min clearance
						CPPUNIT_ASSERT_EQUAL(/*getMessage(ss << "invalid clearance for tile @ "<<x<<","<<y<<". terrain "<<validterrains[i]),*/ n->getClearance(validterrains[i]), 1);
				}
}

