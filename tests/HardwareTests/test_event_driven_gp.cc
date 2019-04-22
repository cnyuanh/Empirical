#define CATCH_CONFIG_MAIN

#include "third-party/Catch/single_include/catch.hpp"

#include "hardware/EventDrivenGP.h"

#include <sstream>
#include <iostream>

TEST_CASE("Test Event Driven GP", "[Hardware]")
{
	// Event lib
	std::function<void(emp::EventDrivenGP&, const emp::EventDrivenGP::Event &)>  HandleEvent_Message = [](emp::EventDrivenGP & hw, const emp::EventDrivenGP::Event & event) { hw.SpawnCore(event.affinity, hw.GetMinBindThresh(), event.msg); };
	emp::EventDrivenGP::Instruction inst(0, 1); // increment id = 0, inc arg 0 = 1
	std::function<void(emp::EventDrivenGP &, const emp::EventDrivenGP::Event &)> func = [inst](emp::EventDrivenGP & hd, const emp::EventDrivenGP::Event & ev){ if(ev.HasProperty("add")){ hd.ProcessInst(inst); } };
	emp::EventLib<emp::EventDrivenGP> event_lib;
	event_lib.AddEvent("Message", HandleEvent_Message, "Event for exchanging messages (agent-agent, world-agent, etc.)");
	event_lib.RegisterDispatchFun(0, func);
    
	// Instruction lib
	std::function<void(emp::EventDrivenGP&, const emp::EventDrivenGP::Instruction &)>  Inst_Inc = [](emp::EventDrivenGP & hw, const emp::EventDrivenGP::Instruction & inst) { emp::EventDrivenGP::State & state = hw.GetCurState(); ++state.AccessLocal(inst.args[0]);};
	std::function<void(emp::EventDrivenGP&, const emp::EventDrivenGP::Instruction &)>  Inst_Dec = [](emp::EventDrivenGP & hw, const emp::EventDrivenGP::Instruction & inst) { emp::EventDrivenGP::State & state = hw.GetCurState(); --state.AccessLocal(inst.args[0]);};
	std::function<void(emp::EventDrivenGP&, const emp::EventDrivenGP::Instruction &)>  Inst_Not = [](emp::EventDrivenGP & hw, const emp::EventDrivenGP::Instruction & inst) { emp::EventDrivenGP::State & state = hw.GetCurState(); state.SetLocal(inst.args[0], state.GetLocal(inst.args[0]) == 0.0);};
	emp::InstLib<emp::EventDrivenGP> inst_lib;
	inst_lib.AddInst("Inc", Inst_Inc, 1, "Increment value in local memory Arg1");
	inst_lib.AddInst("Dec", Inst_Dec, 1, "Decrement value in local memory Arg1");
	inst_lib.AddInst("Not", Inst_Not, 1, "Logically toggle value in local memory Arg1");
	
	// Constructing EventDrivenGP
	emp::EventDrivenGP gp_default;
	REQUIRE( gp_default.GetInstLib() == gp_default.DefaultInstLib() );
	REQUIRE( gp_default.GetEventLib() == gp_default.DefaultEventLib() );
	emp::Random rnd(50);
	emp::Ptr<emp::Random> rndp(&rnd);
	emp::EventDrivenGP gp(inst_lib, event_lib, rndp);
	REQUIRE( (gp.GetRandom().GetSeed() == 50) );
	REQUIRE( gp.GetRandomPtr() == rndp );
	REQUIRE( gp.IsStochasticFunCall() );
	
	// Getters
	std::stringstream ss;
	emp::EventDrivenGP::Program c_prgm = gp.GetConstProgram();
	emp::EventDrivenGP::Program& prgm = gp.GetProgram();
	REQUIRE(c_prgm.GetSize() == 0);
	REQUIRE(prgm.GetSize() == 0);
	REQUIRE(gp.GetNumErrors() == 0);
	REQUIRE(gp.GetCurCoreID() == 0);
	REQUIRE(gp.GetCores().size() == gp.GetMaxCores());
	gp.PrintProgram(ss);
	REQUIRE(ss.str() == "");
	ss.str(std::string());
	
	emp::EventDrivenGP::Function fx;
	fx.PushInst(inst);
	gp.PushFunction(fx);
	REQUIRE(prgm.GetSize() == 1);
	REQUIRE(c_prgm.GetSize() == 0);
	REQUIRE(gp.GetFunction(0) == fx);
	gp.PrintProgramFull(ss);
	std::string savedProgram = ss.str();
	REQUIRE(savedProgram == "Fn-00000000:\n  Inc[00000000](1,0,0)\n\n");
	ss.str(std::string());
	
	emp::EventDrivenGP::Instruction i_dec(1, 1); // decrement id = 1, dec arg 0 = 1
	gp.SetInst(0, 0, i_dec); // do same but just with inst params
	REQUIRE(gp.GetFunction(0)[0] == i_dec);
	gp.PrintInst(i_dec, ss);
	REQUIRE(ss.str() == "Dec 1");
	ss.str(std::string());
	
	emp::EventDrivenGP::Instruction i_not(2, 0); // not id = 2, not arg 0 = 0
	gp.PushInst(i_not); // will be pushed onto the end of the last function
	REQUIRE(gp.GetFunction(0).GetSize() == 2);
	REQUIRE(gp.GetCurState().GetLocal(0) == 0.0);
	gp.CallFunction(0);
	REQUIRE(gp.GetCurState().GetLocal(1) == 0.0);
	gp.SingleProcess();
	REQUIRE(gp.GetCurState().GetLocal(1) == -1.0); // 1st inst is i_dec
	REQUIRE(gp.GetCurState().GetLocal(0) == 0.0);
	gp.SingleProcess();
	REQUIRE(gp.GetCurState().GetLocal(0) == 1.0); // 2nd inst is i_not
	gp.PrintState(ss);
	REQUIRE(ss.str() == "Shared memory: \nTraits: []\nErrors: 0\nEvent queue: \nCore 0(CID=0):\n  Call stack (2):\n    --TOP--\n    Inst ptr: 2 (NONE)\n    Func ptr: 0\n    Input memory: \n    Local memory: {0:1}{1:-1}\n    Output memory: \n    ---\n    Inst ptr: 0 (Dec 1)\n    Func ptr: 0\n    Input memory: \n    Local memory: \n    Output memory: \n    ---\n");
	ss.str(std::string());
	
	REQUIRE(gp.GetCurState().GetLocal(1) == -1.0);
	gp.ProcessInst(inst); // will increment local mem 1
	REQUIRE(gp.GetCurState().GetLocal(1) == 0.0);
	REQUIRE(gp.GetDefaultMemValue() == 0.0);
	gp.SetDefaultMemValue(5.0);
	gp.ProcessInst(inst);
	REQUIRE(gp.GetCurState().GetLocal(1) == 1.0);
	inst.Set(0, 2);
	gp.ProcessInst(inst);
	REQUIRE(gp.GetCurState().GetLocal(2) == 6.0);
	
	gp.NewRandom(150);
	(gp.GetRandom().GetSeed() == 150);
	
	emp::EventDrivenGP::Event ev(0); // event 0 in event lib is message
	ev.properties.insert("add");
	gp.TriggerEvent(ev);
	REQUIRE(gp.GetCurState().GetLocal(1) == 2.0);
	gp.TriggerEvent(0); // doesn't have property add
	REQUIRE(gp.GetCurState().GetLocal(1) == 2.0);
	gp.PrintEvent(ev, ss);
	REQUIRE(ss.str() == "[Message,00000000,(),(Properties: add)]");
	ss.str(std::string());
	
	gp.SetTrait(0, 2.0);
	REQUIRE(gp.GetTrait(0) == 2.0);
	gp.IncTrait(0, 5.0);
	REQUIRE(gp.GetTrait(0) == 7.0);
	gp.DecTrait(0);
	REQUIRE(gp.GetTrait(0) == 6.0);
	gp.PushTrait(3.0);
	REQUIRE(gp.GetTrait(1) == 3.0);
	gp.PrintTraits(ss);
	REQUIRE(ss.str() == "[6, 3]");
	ss.str(std::string());
	
	gp.PrintProgramFull(ss);
	REQUIRE(ss.str() == "Fn-00000000:\n  Dec[00000000](1,0,0)\n  Not[00000000](0,0,0)\n\n");
	ss.str(std::string());
	ss << savedProgram;
	gp.GetProgram().Load(ss);
	ss.str(std::string());
	
	std::stringstream os;
	gp.GetProgram().PrintProgramFull(os);
	REQUIRE(os.str() == savedProgram);
	os.str(std::string());
	REQUIRE(gp.GetProgram().GetSize() == 1);
	
	gp.SetMaxCores(6);
	REQUIRE(gp.GetMaxCores() == 6);
	
	gp.SetStochasticFunCall(false);
	REQUIRE(gp.IsStochasticFunCall() == false);
}