//------------------------------------------------------------------------------
// LAGraph/src/test/test_CheckGraph.c:  test LAGraph_CheckGraph
//------------------------------------------------------------------------------

// LAGraph, (c) 2021 by The LAGraph Contributors, All Rights Reserved.
// SPDX-License-Identifier: BSD-2-Clause
//
// See additional acknowledgments in the LICENSE file,
// or contact permission@sei.cmu.edu for the full terms.

//------------------------------------------------------------------------------

#include "LAGraph_test.h"

//------------------------------------------------------------------------------
// global variables
//------------------------------------------------------------------------------

LAGraph_Graph G = NULL ;
char msg [LAGRAPH_MSG_LEN] ;
GrB_Matrix A = NULL, B_bool = NULL, B_int32 = NULL ;
GrB_Vector d_int64 = NULL, d_bool = NULL ;
GrB_Type atype = NULL ;
#define LEN 512
char filename [LEN+1] ;

//------------------------------------------------------------------------------
// setup: start a test
//------------------------------------------------------------------------------

void setup (void)
{
    OK (LAGraph_Init (msg)) ;
}

//------------------------------------------------------------------------------
// teardown: finalize a test
//------------------------------------------------------------------------------

void teardown (void)
{
    OK (LAGraph_Finalize (msg)) ;
}

//------------------------------------------------------------------------------
// test_CheckGraph:  test LAGraph_CheckGraph
//------------------------------------------------------------------------------

typedef struct
{
    LAGraph_Kind kind ;
    const char *name ;
}
matrix_info ;

const matrix_info files [ ] =
{
    LAGRAPH_ADJACENCY_DIRECTED,   "cover.mtx",
    LAGRAPH_ADJACENCY_DIRECTED,   "ldbc-directed-example.mtx",
    LAGRAPH_ADJACENCY_UNDIRECTED, "ldbc-undirected-example.mtx",
    LAGRAPH_UNKNOWN,              ""
} ;

void test_CheckGraph (void)
{
    setup ( ) ;

    for (int k = 0 ; ; k++)
    {

        // load the adjacency matrix as A
        const char *aname = files [k].name ;
        LAGraph_Kind kind = files [k].kind ;
        if (strlen (aname) == 0) break;
        TEST_CASE (aname) ;
        snprintf (filename, LEN, LG_DATA_DIR "%s", aname) ;
        FILE *f = fopen (filename, "r") ;
        TEST_CHECK (f != NULL) ;
        OK (LAGraph_MMRead (&A, &atype, f, msg)) ;
        OK (fclose (f)) ;
        TEST_MSG ("Loading of adjacency matrix failed") ;

        // create the graph
        OK (LAGraph_New (&G, &A, atype, kind, msg)) ;
        TEST_CHECK (A == NULL) ;    // A has been moved into G->A

        // check the graph
        OK (LAGraph_CheckGraph (G, msg)) ;
        TEST_CHECK (G->kind == kind) ;
        if (kind == LAGRAPH_ADJACENCY_DIRECTED)
        {
            TEST_CHECK (G->A_pattern_is_symmetric == LAGRAPH_UNKNOWN) ;
        }
        else
        {
            TEST_CHECK (G->A_pattern_is_symmetric == LAGRAPH_TRUE) ;
        }

        // create its properties
        OK (LAGraph_Property_AT (G, msg)) ;
        OK (LAGraph_CheckGraph (G, msg)) ;

        OK (LAGraph_Property_RowDegree (G, msg)) ;
        OK (LAGraph_CheckGraph (G, msg)) ;

        OK (LAGraph_Property_ColDegree (G, msg)) ;
        OK (LAGraph_CheckGraph (G, msg)) ;

        // free the graph
        OK (LAGraph_Delete (&G, msg)) ;
        TEST_CHECK (G == NULL) ;

    }

    teardown ( ) ;
}

//------------------------------------------------------------------------------
// test_CheckGraph_failures:  test error handling of LAGraph_CheckGraph
//------------------------------------------------------------------------------

void test_CheckGraph_failures (void)
{
    setup ( ) ;

    printf ("\nTesting LAGraph_CheckGraph error handling:\n") ;

    // construct an invalid graph with a rectangular adjacency matrix
    TEST_CASE ("lp_afiro") ;
    FILE *f = fopen (LG_DATA_DIR "lp_afiro.mtx", "r") ;
    TEST_CHECK (f != NULL) ;
    OK (LAGraph_MMRead (&A, &atype, f, msg)) ;
    OK (fclose (f)) ;
    TEST_MSG ("Loading of lp_afiro.mtx failed") ;

    // create an invalid graph
    OK (LAGraph_New (&G, &A, atype, LAGRAPH_ADJACENCY_DIRECTED, msg)) ;
    TEST_CHECK (A == NULL) ;    // A has been moved into G->A

    // adjacency matrix invalid
    TEST_CHECK (LAGraph_CheckGraph (G, msg) == -1) ;
    printf ("msg: %s\n", msg) ;

    // free the graph
    OK (LAGraph_Delete (&G, msg)) ;
    TEST_CHECK (G == NULL) ;

    // load a valid adjacency matrix
    TEST_CASE ("cover") ;
    f = fopen (LG_DATA_DIR "cover.mtx", "r") ;
    TEST_CHECK (f != NULL) ;
    OK (LAGraph_MMRead (&A, &atype, f, msg)) ;
    OK (fclose (f)) ;
    TEST_MSG ("Loading of cover.mtx failed") ;

    // create an valid graph
    OK (LAGraph_New (&G, &A, atype, LAGRAPH_ADJACENCY_DIRECTED, msg)) ;
    TEST_CHECK (A == NULL) ;    // A has been moved into G->A
    OK (LAGraph_CheckGraph (G, msg)) ;

    OK (GrB_Vector_new (&d_bool,  GrB_BOOL, 7)) ;
    OK (GrB_Vector_new (&d_int64, GrB_INT64, 1000)) ;
    OK (GrB_Matrix_new (&B_bool,  GrB_INT64, 7, 7)) ;
    OK (GrB_Matrix_new (&B_int32, GrB_INT32, 3, 4)) ;

    // G->AT has the right type, but wrong size
    G->AT = B_int32 ;
    TEST_CHECK (LAGraph_CheckGraph (G, msg) == -3) ;
    printf ("msg: %s\n", msg) ;

    // G->AT has the right size, but wrong type
    G->AT = B_bool ;
    TEST_CHECK (LAGraph_CheckGraph (G, msg) == -5) ;
    printf ("msg: %s\n", msg) ;

    #if defined ( GxB_SUITESPARSE_GRAPHBLAS )
    // G->AT must be by-row
    OK (GxB_set (G->AT, GxB_FORMAT, GxB_BY_COL)) ;
    TEST_CHECK (LAGraph_CheckGraph (G, msg) == -4) ;
    printf ("msg: %s\n", msg) ;
    #endif

    G->AT = NULL ;

    // G->rowdegree has the right type, but wrong size
    G->rowdegree = d_int64 ;
    TEST_CHECK (LAGraph_CheckGraph (G, msg) == -6) ;
    printf ("msg: %s\n", msg) ;

    // G->rowdegree has the right size, but wrong type
    G->rowdegree = d_bool ;
    TEST_CHECK (LAGraph_CheckGraph (G, msg) == -7) ;
    printf ("msg: %s\n", msg) ;

    G->rowdegree = NULL ;

    // G->coldegree has the right type, but wrong size
    G->coldegree = d_int64 ;
    TEST_CHECK (LAGraph_CheckGraph (G, msg) == -8) ;
    printf ("msg: %s\n", msg) ;

    // G->coldegree has the right size, but wrong type
    G->coldegree = d_bool ;
    TEST_CHECK (LAGraph_CheckGraph (G, msg) == -9) ;
    printf ("msg: %s\n", msg) ;

    G->coldegree = NULL ;

    #if defined ( GxB_SUITESPARSE_GRAPHBLAS )
    // G->A must be by-row
    OK (GxB_set (G->A, GxB_FORMAT, GxB_BY_COL)) ;
    TEST_CHECK (LAGraph_CheckGraph (G, msg) == -2) ;
    printf ("msg: %s\n", msg) ;
    #endif

    GrB_free (&d_bool) ;
    GrB_free (&d_int64) ;
    GrB_free (&B_bool) ;
    GrB_free (&B_int32) ;

    // mangle G->kind
    G->kind = LAGRAPH_UNKNOWN ;
    TEST_CHECK (LAGraph_CheckGraph (G, msg) == -3) ;
    printf ("msg: %s\n", msg) ;
    G->kind = LAGRAPH_ADJACENCY_DIRECTED ;

    // free the adjacency matrix
    GrB_free (&(G->A)) ;
    TEST_CHECK (G->A == NULL) ;

    TEST_CHECK (LAGraph_CheckGraph (G, msg) == -2) ;
    printf ("msg: %s\n", msg) ;

    // free the graph
    OK (LAGraph_Delete (&G, msg)) ;
    TEST_CHECK (G == NULL) ;

    TEST_CHECK (LAGraph_CheckGraph (NULL, msg) == -1) ;
    printf ("msg: %s\n", msg) ;

    teardown ( ) ;
}

//-----------------------------------------------------------------------------
// TEST_LIST: the list of tasks for this entire test
//-----------------------------------------------------------------------------

TEST_LIST =
{
    { "CheckGraph", test_CheckGraph },
    { "CheckGraph_failures", test_CheckGraph_failures },
    { NULL, NULL }
} ;

