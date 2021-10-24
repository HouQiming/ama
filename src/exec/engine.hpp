#ifndef _ENGINE_JCH_HPP
#define _ENGINE_JCH_HPP
#include <string>
#include <vector>
#include "../util/jc_array.h"
#include <unordered_map>
/*#pragma add("jc_files", "./engine.jc");*/
#include "../util/mempool.hpp"
#include "../ast/node.hpp"
namespace ama {
	struct ama::ExecNode;
};
/*
In general, we need to tolerate failures: we don't fully understand the code.
So this engine will be best-effort by design.
We should report our confidence, though.
*/
namespace ama {
	//the immutable pattern doesn't work here: we actually expand stuff
	//ExecNode and ExecEffect must be zero-initializable - they are pooled
	//there is one and only one next for ENODE_NORMAL and ENODE_AFTER
	static const uint8_t ENODE_NORMAL = 0;
	//after a scope or a label
	static const uint8_t ENODE_AFTER = 1;
	//models parallel / non-determinstic execution, all next-s will get executed
	static const uint8_t ENODE_FORK = 2;
	static const uint8_t ENODE_JOIN = 3;
	//only one next gets executed for ENODE_BRANCH, branches always converge into ENODE_PHI
	static const uint8_t ENODE_BRANCH = 4;
	static const uint8_t ENODE_PHI = 5;
	static const uint8_t ENODE_JUMP = 6;
	static const uint8_t ENODE_LOOP = 7;
	static const uint8_t ENODE_DECL_ENTRY = 8;
	////////////
	static const uint16_t ENODEF_FOLDED_CFG = 1;
	static const uint16_t ENODEF_DUMP_ACTIVE = 128;
	//contexts are implicitly defined by nodes: what each node changes in each context
	struct ExecNodeExtraLink {
		ExecNode* target{};
		//we don't want to confuse this with ExecNode::next
		ExecNodeExtraLink* x{};
	};
	struct ExecNodeLinks {
		//ExecNode*+[2]! fast
		ExecNodeExtraLink* more{};
	};
	struct ExecNode {
		//break / continue / return could create a good deal of prevs
		//a linked list is more easily managed in a pool
		ExecNodeLinks prev{};
		ExecNodeLinks next{};
		uint16_t flags{};
		uint8_t kind{};
		ama::Node* nd{};
	};
	///the effect any particular node could have is also lexical, and monotonically expanding as interests were added
	///the effects of a node should exclude its contained branches:
	///    a basic node could contain a branch inside: a=(foo&&bar?baz():0)
	///we don't want effects to model "value"s directly, we want minimal information to construct values in any form required
	///thus we should avoid interpreting the node in any manner: not even read / write, or whether we got the interest right
	///just provide exact location for the alleged use
	struct ExecEffect {
		//again, we don't want the `next` name
		ExecEffect* x{};
		//effects are only interesting when they affect an interest
		//effects are queried from the activating node so we don't need to store it
		intptr_t interest_id{};
		ama::Node* nd_alleged_ref{};
	};
	////////////////////
	struct ExecInterest {
		uint8_t kind{};
		ama::Node* nd_interest{};
		ama::Node* nd_owner{};
		intptr_t same_name_next{};
	};
	struct ExecRange {
		ExecNode* entry{};
		ExecNode* exit{};
	};
	struct ExecEffectList {
		ExecEffect* first{};
		intptr_t last_interest_checked{};
	};
	struct ExecNodeCanonicals {
		//we are lazily filling effects so we need a mutable state for the laziness
		ExecEffectList effects{};
		ExecEffectList effects_after{};
		ExecRange rg_canon{};
		ExecNode* ed_after{};
		uint8_t in_session{};
	};
	//const uint32_t! UNCERTAIN_JUMP_TARGET = 1;
	//const uint32_t! UNCERTAIN_FIELD_RESOLUTION = 2;
	//const uint32_t! UNCERTAIN_FUNCTION_RESOLUTION = 4;
	class ExecSession;
	//passed(*)(session, ed, effect)
	typedef int(*FValueFilter)(ama::ExecSession*, ama::ExecNode*, ama::ExecEffect*);
	static const int32_t REACH_FORWARD = 1;
	static const int32_t REACH_BACKWARD = 2;
	struct ExecSession {
		ama::TMemoryPool pool{};
		std::vector<ExecNode*> entries{};
		//uint32_t! uncertainty;
		//interests are lexical: we don't care which context they are in, we only care which code text they represent 
		std::vector<ExecInterest> interests{};
		std::unordered_map<JC::unique_string, intptr_t> name_to_interest{};
		//COULDDO: put a generic void* into nodes? or cheap mempool-offset-based SxS storage
		std::unordered_map<ama::Node const*, ExecNodeCanonicals> canonicals{};
		std::vector<ama::Node*> owners{};
		ExecSession() {
			memset(&this->pool, 0, sizeof(this->pool));
		}
		ExecSession(ExecSession&& theirs): entries(std::move(theirs.entries)), interests(std::move(theirs.interests)), name_to_interest(std::move(theirs.name_to_interest)), canonicals(std::move(theirs.canonicals)), owners(std::move(theirs.owners)) {
			/*#pragma construct(entries(std::move(theirs.entries)));*/
			/*#pragma construct(interests(std::move(theirs.interests)));*/
			/*#pragma construct(name_to_interest(std::move(theirs.name_to_interest)));*/
			/*#pragma construct(canonicals(std::move(theirs.canonicals)));*/
			/*#pragma construct(owners(std::move(theirs.owners)));*/
			memcpy(&this->pool, &theirs.pool, sizeof(theirs.pool));
			memset(&theirs.pool, 0, sizeof(theirs.pool));
		}
		~ExecSession() {
			ama::poolRelease(&this->pool);
		}
		static std::vector<ama::ExecNode*> ComputeReachableSet(std::span<ama::ExecNode*> entries, int32_t dir);
		intptr_t AddInterest(ama::Node* nd_interest);
		ama::ExecNode* CreateExecNode(uint8_t kind, ama::Node* nd_exec);
		void AddLinkTo(ama::ExecNodeLinks* plinks, ama::ExecNode* ed_target);
		void AddEdge(ama::ExecNode* ed0, ama::ExecNode* ed1);
		static void ReplaceNode(ama::ExecNode* ed_old, ama::ExecRange rg_new);
		int CheckInterest(ama::Node* ndi, ama::Node* nd_entry);
		ama::ExecRange CreateNodeGraph(ama::Node* nd_entry);
		ama::ExecRange LocateNode(ama::Node* nd);
		std::string DumpGraphviz(ama::ExecNode* extra_graph);
		//ama::ExecNode*+[+]! ComputeStatementRangeBackward(ama::ExecNode*+! ed_exit);
		int MatchInterest(ama::Node* ndi, intptr_t i)const;
		ama::ExecEffect* QueryEffects(ama::ExecNode* ed);
		ama::ExecNode* CloneExecNode(ama::ExecNode* edi);
		ama::ExecNode* CreateValueGraphBackward(
		ama::ExecNode* ed_exit, intptr_t interest_id,
		FValueFilter filter);
	};
	ama::ExecSession CreateSession(ama::Node* nd_entry, std::span<ama::Node*> interests);
};

#endif
