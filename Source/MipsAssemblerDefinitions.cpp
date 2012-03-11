#include "MipsAssemblerDefinitions.h"
#include "MIPSAssembler.h"
#include <stdexcept>
#include <functional>
#include <boost/lexical_cast.hpp>
#include "lexical_cast_ex.h"
#include "placeholder_def.h"

namespace MipsAssemblerDefinitions
{
	//RsRtImm Parser
	//-----------------------------
	struct RsRtImm
	{
		typedef void (CMIPSAssembler::*AssemblerFunctionType) (unsigned int, unsigned int, uint16);

		RsRtImm(const AssemblerFunctionType& assemblerFunction) 
		: m_assemblerFunction(assemblerFunction)
		{
			
		}

		void operator ()(boost::tokenizer<>& Tokens, boost::tokenizer<>::iterator& itToken, CMIPSAssembler* pAssembler)
		{
			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nRS = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nRT = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(itToken == Tokens.end()) throw std::exception();
			uint16 nImm = lexical_cast_hex<std::string>((*(++itToken)).c_str());

			if(nRT == -1) throw std::exception();
			if(nRS == -1) throw std::exception();

			((pAssembler)->*(m_assemblerFunction))(nRS, nRT, nImm);
		}

		AssemblerFunctionType m_assemblerFunction;
	};

	//RtRsImm Parser
	//-----------------------------
	struct RtRsImm
	{
		typedef void (CMIPSAssembler::*AssemblerFunctionType) (unsigned int, unsigned int, uint16);

		RtRsImm(const AssemblerFunctionType& assemblerFunction)
		: m_assemblerFunction(assemblerFunction)
		{
			
		}

		void operator ()(boost::tokenizer<>& Tokens, boost::tokenizer<>::iterator& itToken, CMIPSAssembler* pAssembler)
		{
			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nRT = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nRS = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(itToken == Tokens.end()) throw std::exception();
			uint16 nImm = lexical_cast_hex<std::string>((*(++itToken)).c_str());

			if(nRT == -1) throw std::exception();
			if(nRS == -1) throw std::exception();

			((pAssembler)->*(m_assemblerFunction))(nRT, nRS, nImm);
		}

		AssemblerFunctionType m_assemblerFunction;
	};

	//RdRsRt Parser
	//-----------------------------
	struct RdRsRt
	{
		typedef void (CMIPSAssembler::*AssemblerFunctionType) (unsigned int, unsigned int, unsigned int);

		RdRsRt(const AssemblerFunctionType& assemblerFunction)
		: m_assemblerFunction(assemblerFunction)
		{
			
		}

		void operator ()(boost::tokenizer<>& Tokens, boost::tokenizer<>::iterator& itToken, CMIPSAssembler* pAssembler)
		{
			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nRD = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nRS = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nRT = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(nRT == -1) throw std::exception();
			if(nRS == -1) throw std::exception();
			if(nRD == -1) throw std::exception();

			((pAssembler)->*(m_assemblerFunction))(nRD, nRS, nRT);
		}

		AssemblerFunctionType m_assemblerFunction;
	};

	//RsImm Parser
	//-----------------------------
	struct RsImm
	{
		typedef void (CMIPSAssembler::*AssemblerFunctionType) (unsigned int, uint16);

		RsImm(const AssemblerFunctionType& assemblerFunction)
		: m_assemblerFunction(assemblerFunction)
		{
			
		}

		void operator ()(boost::tokenizer<>& Tokens, boost::tokenizer<>::iterator& itToken, CMIPSAssembler* pAssembler)
		{
			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nRS = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(itToken == Tokens.end()) throw std::exception();
			uint16 nImm = lexical_cast_hex<std::string>((*(++itToken)).c_str());

			if(nRS == -1) throw std::exception();

			((pAssembler)->*(m_assemblerFunction))(nRS, nImm);
		}

		AssemblerFunctionType m_assemblerFunction;
	};

	//RtImm Parser
	//-----------------------------
	struct RtImm
	{
		typedef void (CMIPSAssembler::*AssemblerFunctionType) (unsigned int, uint16);

		RtImm(const AssemblerFunctionType& assemblerFunction)
		: m_assemblerFunction(assemblerFunction)
		{
			
		}

		void operator ()(boost::tokenizer<>& Tokens, boost::tokenizer<>::iterator& itToken, CMIPSAssembler* pAssembler)
		{
			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nRT = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(itToken == Tokens.end()) throw std::exception();
			uint16 nImm = lexical_cast_hex<std::string>((*(++itToken)).c_str());

			if(nRT == -1) throw std::exception();

			((pAssembler)->*(m_assemblerFunction))(nRT, nImm);
		}

		AssemblerFunctionType m_assemblerFunction;
	};

	//RtRsSa Parser
	//-----------------------------
	struct RtRsSa
	{
		typedef void (CMIPSAssembler::*AssemblerFunctionType) (unsigned int, unsigned int, unsigned int);

		RtRsSa(const AssemblerFunctionType& assemblerFunction)
		: m_assemblerFunction(assemblerFunction)
		{
			
		}

		void operator()(boost::tokenizer<>& Tokens, boost::tokenizer<>::iterator& itToken, CMIPSAssembler* pAssembler)
		{
			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nRT = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nRS = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nSA = boost::lexical_cast<unsigned int>((*(++itToken)).c_str());

			if(nRT == -1) throw std::exception();
			if(nRS == -1) throw std::exception();
			
			((pAssembler)->*(m_assemblerFunction))(nRT, nRS, nSA);
		}

		AssemblerFunctionType m_assemblerFunction;
	};

	//RtOfsBase Parser
	//-----------------------------
	struct RtOfsBase
	{
		typedef void (CMIPSAssembler::*AssemblerFunctionType) (unsigned int, uint16, unsigned int);

		RtOfsBase(const AssemblerFunctionType& assemblerFunction)
		: m_assemblerFunction(assemblerFunction)
		{
			
		}

		void operator ()(boost::tokenizer<>& Tokens, boost::tokenizer<>::iterator& itToken, CMIPSAssembler* pAssembler)
		{
			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nRT = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(itToken == Tokens.end()) throw std::exception();
			uint16 nOfs = lexical_cast_hex<std::string>((*(++itToken)).c_str());

			if(itToken == Tokens.end()) throw std::exception();
			unsigned int nBase = CMIPSAssembler::GetRegisterIndex((*(++itToken)).c_str());

			if(nBase == -1) throw std::exception();
			if(nRT == -1) throw std::exception();

			((pAssembler)->*(m_assemblerFunction))(nRT, nOfs, nBase);
		}

		AssemblerFunctionType m_assemblerFunction;
	};

	template <typename Functor> 
	struct SpecInstruction : public Instruction
	{
		SpecInstruction(const char* sMnemonic, const Functor& Parser) :
		Instruction(sMnemonic),
		m_Parser(Parser)
		{
			
		}

		virtual void Invoke(boost::tokenizer<>& Tokens, boost::tokenizer<>::iterator& itToken, CMIPSAssembler* pAssembler)
		{
			m_Parser(Tokens, itToken, pAssembler);
		}

		Functor m_Parser;
	};

	//Instruction definitions
	//-------------------------------

	SpecInstruction<RdRsRt>		Instruction_ADDU	= SpecInstruction<RdRsRt>("ADDU", RdRsRt(&CMIPSAssembler::ADDU));
	SpecInstruction<RtRsImm>	Instruction_ADDIU	= SpecInstruction<RtRsImm>("ADDIU", RtRsImm(&CMIPSAssembler::ADDIU));
	SpecInstruction<RsRtImm>	Instruction_BEQ		= SpecInstruction<RsRtImm>("BEQ", RsRtImm(&CMIPSAssembler::BEQ));
	SpecInstruction<RsImm>		Instruction_BGEZ	= SpecInstruction<RsImm>("BGEZ", RsImm(&CMIPSAssembler::BGEZ));
	SpecInstruction<RsImm>		Instruction_BLEZ	= SpecInstruction<RsImm>("BLEZ", RsImm(&CMIPSAssembler::BLEZ));
	SpecInstruction<RsRtImm>	Instruction_BNE		= SpecInstruction<RsRtImm>("BNE", RsRtImm(&CMIPSAssembler::BNE));
	SpecInstruction<RtRsImm>	Instruction_DADDIU	= SpecInstruction<RtRsImm>("DADDIU", RtRsImm(&CMIPSAssembler::DADDIU));
	SpecInstruction<RtRsSa>		Instruction_DSRA32	= SpecInstruction<RtRsSa>("DSRA32", RtRsSa(&CMIPSAssembler::DSRA32));
	SpecInstruction<RtOfsBase>	Instruction_LHU		= SpecInstruction<RtOfsBase>("LHU", RtOfsBase(&CMIPSAssembler::LHU));
	SpecInstruction<RtImm>		Instruction_LUI		= SpecInstruction<RtImm>("LUI", RtImm(&CMIPSAssembler::LUI));
	SpecInstruction<RtOfsBase>	Instruction_LW		= SpecInstruction<RtOfsBase>("LW", RtOfsBase(&CMIPSAssembler::LW));
	SpecInstruction<RdRsRt>		Instruction_MULT	= SpecInstruction<RdRsRt>("MULT", RdRsRt(&CMIPSAssembler::MULT));
	SpecInstruction<RdRsRt>		Instruction_OR		= SpecInstruction<RdRsRt>("OR", RdRsRt(&CMIPSAssembler::OR));
	SpecInstruction<RtRsSa>		Instruction_SLL		= SpecInstruction<RtRsSa>("SLL", RtRsSa(&CMIPSAssembler::SLL));
	SpecInstruction<RtRsImm>	Instruction_SLTI	= SpecInstruction<RtRsImm>("SLTI", RtRsImm(&CMIPSAssembler::SLTI));
	SpecInstruction<RtRsImm>	Instruction_SLTIU	= SpecInstruction<RtRsImm>("SLTIU", RtRsImm(&CMIPSAssembler::SLTIU));
	SpecInstruction<RdRsRt>		Instruction_SLTU	= SpecInstruction<RdRsRt>("SLTU", RdRsRt(&CMIPSAssembler::SLTU));
	SpecInstruction<RtRsSa>		Instruction_SRA		= SpecInstruction<RtRsSa>("SRA", RtRsSa(&CMIPSAssembler::SRA));

	Instruction* g_Instructions[] = 
	{
		&Instruction_ADDU,
		&Instruction_ADDIU,
		&Instruction_BEQ,
		&Instruction_BGEZ,
		&Instruction_BLEZ,
		&Instruction_BNE,
		&Instruction_DADDIU,
		&Instruction_DSRA32,
		&Instruction_LHU,
		&Instruction_LUI,
		&Instruction_LW,
		&Instruction_MULT,
		&Instruction_OR,
		&Instruction_SLL,
		&Instruction_SLTI,
		&Instruction_SLTIU,
		&Instruction_SLTU,
		&Instruction_SRA,
		NULL,
	};

}

