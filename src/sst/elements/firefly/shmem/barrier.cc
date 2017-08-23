// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "shmem/barrier.h"
#include "hadesSHMEM.h"

using namespace SST;
using namespace Firefly;


void ShmemBarrier::start( int PE_start, int logPE_stride, int PE_size, 
        Hermes::Vaddr pSync, Hermes::Shmem::Callback returnCallback )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);

    if ( 1 == PE_size ) {
        m_api.delay(returnCallback,0,0);
        return;
    }

    m_returnCallback = returnCallback;
    m_pSync = pSync;

    if ( PE_size == num_pes() ) {
        printf(":%d:%s():%d full tree\n",my_pe(),__func__,__LINE__);
       /* we're the full tree, use the binomial tree */
        m_parent = full_tree_parent();
        m_num_children = full_tree_num_children();
        m_children = &full_tree_children()[0];    
    } else {
        printf(":%d:%s():%d part full tree\n",my_pe(),__func__,__LINE__);
        m_part_tree_children.resize( tree_radix() );
        m_children = &m_part_tree_children[0];
        m_common.build_kary_tree(tree_radix(), PE_start, 1 << logPE_stride , PE_size, 0, &m_parent, &m_num_children, m_children);
    } 

    if ( m_num_children != 0 ) {
        m_api.quiet( std::bind( &ShmemBarrier::not_leaf_0, this, std::placeholders::_1 ) );
    } else {
        m_api.quiet( std::bind( &ShmemBarrier::leaf_0, this, std::placeholders::_1 ) );
    }
}

void ShmemBarrier::not_leaf_0( int )
{
    printf(":%d:%s():%d num_children=%d\n",my_pe(),__func__,__LINE__,m_num_children);
    m_value.set( (long) m_num_children ); 
    /* wait for num_children callins up the tree */
    if (m_parent == my_pe()) {
        m_api.wait_until( m_pSync, Shmem::EQ, m_value, 
           std::bind( &ShmemBarrier::root_0, this, std::placeholders::_1 ) );
    } else {
        m_api.wait_until( m_pSync, Shmem::EQ, m_value, 
           std::bind( &ShmemBarrier::node_0, this, std::placeholders::_1 ) );
    }

    // SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, num_children);
}

void ShmemBarrier::root_0( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);

    /* Clear pSync */
    m_api.putv( m_pSync, m_zero, my_pe(), 
           std::bind( &ShmemBarrier::root_1, this, std::placeholders::_1 ) );

    //shmem_internal_put_small(pSync, &zero, sizeof(zero), shmem_internal_my_pe); 
} 
void ShmemBarrier::root_1( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);

    m_api.wait_until( m_pSync, Shmem::EQ, m_zero, 
           std::bind( &ShmemBarrier::root_2, this, std::placeholders::_1 ) );
    //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, 0);

    m_iteration = 0;
}

void ShmemBarrier::root_2( int )
{
    printf(":%d:%s():%d child=%d\n",my_pe(),__func__,__LINE__,m_children[m_iteration]);

    Shmem::Callback callback;
    if ( m_iteration < m_num_children - 1 ) {
        callback =  std::bind( &ShmemBarrier::root_2, this, std::placeholders::_1 );
    } else {
        callback =  std::bind( &ShmemBarrier::fini, this, std::placeholders::_1 );
    }

    m_api.fadd( m_retval, m_pSync, m_one, m_children[m_iteration], callback );
    ++m_iteration;
#if 0
    /* Send acks down to children */
    for (i = 0 ; i < num_children ; ++i) {
        shmem_internal_atomic_small(pSync, &one, sizeof(one),
                                            children[i],
                                            SHM_INTERNAL_SUM, SHM_INTERNAL_LONG);
    }
#endif
}


void ShmemBarrier::node_0( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* send ack to parent */
    m_api.fadd( m_retval, m_pSync, m_one, m_parent, 
           std::bind( &ShmemBarrier::node_1, this, std::placeholders::_1 ) );
    //shmem_internal_atomic_small(pSync, &one, sizeof(one),
                // parent, SHM_INTERNAL_SUM, SHM_INTERNAL_LONG);
}

void ShmemBarrier::node_1( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* wait for ack from parent */
    m_value.set( (long) m_num_children + 1 ); 
    m_api.wait_until( m_pSync, Shmem::EQ, m_value, 
           std::bind( &ShmemBarrier::node_2, this, std::placeholders::_1 ) );
    //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, num_children  + 1);
} 

void ShmemBarrier::node_2( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* Clear pSync */
    m_api.putv( m_pSync, m_zero, my_pe(), 
           std::bind( &ShmemBarrier::node_3, this, std::placeholders::_1 ) );
    //shmem_internal_put_small(pSync, &zero, sizeof(zero), shmem_internal_my_pe);
}
    
void ShmemBarrier::node_3( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.wait_until( m_pSync, Shmem::EQ, m_zero, 
           std::bind( &ShmemBarrier::node_4, this, std::placeholders::_1 ) );
    //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, 0);
    m_iteration = 0; 
}

void ShmemBarrier::node_4( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    Shmem::Callback callback;
    if ( m_iteration < m_num_children - 1 ) {
        callback =  std::bind( &ShmemBarrier::node_4, this, std::placeholders::_1 );
    } else {
        callback =  std::bind( &ShmemBarrier::fini, this, std::placeholders::_1 );
    }

    m_api.fadd( m_retval, m_pSync, m_one, m_children[m_iteration], callback ); 
    ++m_iteration;

#if 0
    /* Send acks down to children */
    for (i = 0 ; i < num_children ; ++i) {
        shmem_internal_atomic_small(pSync, &one, sizeof(one), children[i], 
                SHM_INTERNAL_SUM, SHM_INTERNAL_LONG);
    }
#endif
}

void ShmemBarrier::leaf_0( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
   /* send message up psync tree */
    m_api.fadd( m_retval, m_pSync, m_one, m_parent, 
           std::bind( &ShmemBarrier::leaf_1, this, std::placeholders::_1 ) );

#if 0
    shmem_internal_atomic_small(pSync, &one, sizeof(one), parent,
                                    SHM_INTERNAL_SUM, SHM_INTERNAL_LONG);
#endif
}

void ShmemBarrier::leaf_1( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* wait for ack down psync tree */
    m_api.wait_until( m_pSync, Shmem::NE, m_zero, 
           std::bind( &ShmemBarrier::leaf_2, this, std::placeholders::_1 ) );

   //   SHMEM_WAIT(pSync, 0);
}

void ShmemBarrier::leaf_2( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* Clear pSync */
    m_api.putv( m_pSync, m_zero, my_pe(), 
           std::bind( &ShmemBarrier::leaf_3, this, std::placeholders::_1 ) );
#if 0
        shmem_internal_put_small(pSync, &zero, sizeof(zero),
                                 shmem_internal_my_pe);
#endif
}

void ShmemBarrier::leaf_3( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.wait_until( m_pSync, Shmem::EQ, m_zero, 
           std::bind( &ShmemBarrier::fini, this, std::placeholders::_1 ) );
#if 0
    SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, 0);
#endif
}
