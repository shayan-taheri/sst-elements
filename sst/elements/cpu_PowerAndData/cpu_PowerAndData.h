// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// A cpu component that can report unit power 
// Built for testing the power model.

#ifndef _CPU_POWERANDDATA_H
#define _CPU_POWERANDDATA_H

#include <sst/core/eventFunctor.h>
#include <sst/core/introspectedComponent.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include "../power/power.h"

using namespace SST;

#if DBG_CPU_POWERANDDATA
#define _CPU_POWERANDDATA_DBG( fmt, args...)\
         printf( "%d:Cpu_PowerAndData::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _CPU_POWERANDDATA_DBG( fmt, args...)
#endif

class Cpu_PowerAndData : public IntrospectedComponent {
        typedef enum { WAIT, SEND } state_t;
        typedef enum { WHO_NIC, WHO_MEM } who_t;
    public:
        Cpu_PowerAndData( ComponentId_t id, Params_t& params ) :
            IntrospectedComponent( id ),
            params( params ),
            state(SEND),            
	    area(0.0),
	    who(WHO_MEM),
            frequency( "2.2GHz" )
        {
            _CPU_POWERANDDATA_DBG( "new id=%lu\n", id );

	    registerExit();

            Params_t::iterator it = params.begin(); 
            while( it != params.end() ) { 
                _CPU_POWERANDDATA_DBG("key=%s value=%s\n",
                            it->first.c_str(),it->second.c_str());
                if ( ! it->first.compare("clock") ) {
                    frequency = it->second;
                }    
		else if ( ! it->first.compare("push_introspector") ) {
                    pushIntrospector = it->second;
                }    

                ++it;
            } 
            
            mem = configureLink( "MEM" );
            handler = new EventHandler< Cpu_PowerAndData, bool, Cycle_t >
                                                ( this, &Cpu_PowerAndData::clock );
	    handlerPush = new EventHandler< Cpu_PowerAndData, bool, Cycle_t >
                                                ( this, &Cpu_PowerAndData::pushData );

            TimeConverter* tc = registerClock( frequency, handler );
	    TimeConverter* tcPush = registerClock( frequency, handlerPush );

	    mem->setDefaultTimeBase(tc);
	    printf("CPU_POWERANDDATA period: %ld\n",tc->getFactor());
            _CPU_POWERANDDATA_DBG("Done registering clock\n");

	    counts = 0;
	    num_il1_read = 0;
	    num_branch_read = 0;
	    num_branch_write = 0;
	    num_RAS_read = 0;
	    num_RAS_write = 0;

	    registerMonitorInt("il1_read");
	    registerMonitorInt("core_temperature");
	    registerMonitorInt("branch_read");
	    registerMonitorInt("RAS_read");
	    registerMonitorInt("RAS_write");
	    
	    //registerMonitorDouble("CPUarea");

        }
        int Setup() {
 	    
	    power = new Power(getId());
            power->setTech(getId(), params, CACHE_IL1, McPAT);
	    //power->setTech(getId(), params, CACHE_IL2);
	    /*power->setTech(getId(), params, CACHE_DL1);
	    //power->setTech(getId(), params, CACHE_DL2);
	    power->setTech(getId(), params, CACHE_ITLB);
	    power->setTech(getId(), params, CACHE_DTLB);
	    power->setTech(getId(), params, RF);
	    power->setTech(getId(), params, IB);	    
    	    power->setTech(getId(), params, ISSUE_Q);
	    power->setTech(getId(), params, INST_DECODER);
	    power->setTech(getId(), params, PIPELINE);
	    power->setTech(getId(), params, BYPASS);	    	    	    	    
	    power->setTech(getId(), params, LOGIC);
	    power->setTech(getId(), params, ALU);
	    power->setTech(getId(), params, FPU);
	    power->setTech(getId(), params, EXEU);
	    power->setTech(getId(), params, LSQ);
	    power->setTech(getId(), params, BPRED);
	    power->setTech(getId(), params, RAT);
	    power->setTech(getId(), params, ROB);
	    power->setTech(getId(), params, BTB);
	    power->setTech(getId(), params, CACHE_L2);
	    power->setTech(getId(), params, CLOCK);*/ //clock should be the last in McPAT
	    area = power->estimateAreaMcPAT();    

	    //query the (push)introspector on how often this should push data (power/energy)
	    //IntrospectedComponent *c;
	    //for (std::list<ComponentId_t>::iterator i = MyIntroList.begin();
	    //    i != MyIntroList.end(); ++i) {
	    	  //printf("ID %lu will monitor me\n",*i);
	    //}
	    //c = c_sim->getComponent(pushIntrospector.c_str());
	    //printf("component found its introspector whose period = %lu\n", c->getIntrospectFreq()); 
            //pushFreq = c->getFreq() / this->getFreq();
   
           return 0;
        }
        int Finish() {
            _CPU_POWERANDDATA_DBG("\n");
	    //unregisterExit();
            return 0;
        }
	
	uint64_t getIntData(int dataID, int index=0)
	{ 
	  switch(dataID)
	  {
	    case 0:
	    //core_temperature
		return (mycore_temperature);
		break;
	    case 1:
	    //branch_read
		return (num_branch_read);
		break;
	    case 2: 
	    //branch_write
		return (num_branch_write);
		break;
	    case 3: 
	    //RAS_read 
		return (num_RAS_read);
		break;
	    case 4:
	    //RAS_write
		return (num_RAS_write);
		break;
	    case 5:
	    //il1_read
		return (num_il1_read);
		break;
	    default:
		return (0);
		break;	
	  }
	}
	

    public:
	int counts;
	double area;
	uint64_t num_il1_read;
	uint64_t mycore_temperature;
	uint64_t num_branch_read;
	uint64_t num_branch_write;
	uint64_t num_RAS_read;
	uint64_t num_RAS_write;
	//Simulation *c_sim;

    private:

        Cpu_PowerAndData( const Cpu_PowerAndData& c );
	Cpu_PowerAndData() :  IntrospectedComponent(-1) {} // for serialization only


        bool clock( Cycle_t );
	bool pushData( Cycle_t);
        ClockHandler_t *handler, *handlerPush;
        bool handler1( Time_t time, Event *e );

        Params_t    params;
        Link*       mem;
        state_t     state;
        who_t       who;
	std::string frequency;
	std::string pushIntrospector;
	//SimTime_t pushFreq;

	Pdissipation_t pdata, pstats;
	Power *power;
	std::pair<bool, int*> p_int;
	std::pair<bool, double*> p_double;
	usagecounts_t mycounts;  //over-specified struct that holds usage counts of its sub-components

 friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
        ar & BOOST_SERIALIZATION_NVP(handler);
        ar & BOOST_SERIALIZATION_NVP(params);
        ar & BOOST_SERIALIZATION_NVP(mem);
        ar & BOOST_SERIALIZATION_NVP(state);
        ar & BOOST_SERIALIZATION_NVP(who);
        ar & BOOST_SERIALIZATION_NVP(frequency);
    }
};

#endif
