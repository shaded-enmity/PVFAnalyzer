/**********************************************************************
          DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

 Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. You just DO WHAT THE FUCK YOU WANT TO.

**********************************************************************/
#pragma once

#include "instruction.h"

#include <udis86.h>

/*
 * ud_t contains ud_operand members that need to have dedicated
 * serialization routine as well.
 */
struct udop : public ud_operand_t
{
	udop(ud_operand_t& op)
	{
		*this = op;
	}

	template <class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & type;
		ar & size;
		ar & lval.uqword;
		ar & base;
		ar & index;
		ar & offset;
		ar & scale;
	}
};


struct udis86_t : public ud_t
{
	udis86_t()
		: ud_t()
	{
		ud_init(udptr());
		ud_set_mode(udptr(), 32);
		ud_set_syntax(udptr(), UD_SYN_INTEL);
	}

	ud_t* udptr() { return reinterpret_cast<ud_t*>(this); }
	ud_t const *udp() const { return reinterpret_cast<ud_t const *>(this); }

	/**
	 * @brief Serialize those parts of ud_t that are relevant for us.
	 *
	 * Note: We assume that we serialize/deserialize single disassembled
	 *       instructions here. This means that the instruction has already
	 *       been successfully disassembled using ud_disassemble(). Hence we
	 *       can ignore the corresponding input buffer.
	 *
	 * @param a serialization archive
	 * @param version version
	 * @return void
	 **/
	template <class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		/* Collecting some info about ud_t components here.
		 *
		 * -> inp* are only used during disassembly -> we don't store them
		 *    as we assume an already disassembled instruction
		 *
		 */
		ar & insn_offset;
		ar & insn_hexcode;
		ar & insn_buffer;
		ar & insn_fill;
		ar & dis_mode;
		ar & pc;
		ar & vendor;

		ar & mnemonic;
		ar & static_cast<udop&> ( operand[0] );
		ar & static_cast<udop&> ( operand[1] );
		ar & static_cast<udop&> ( operand[2] );

		ar & error;
		ar & pfx_rex;
		ar & pfx_seg;
		ar & pfx_opr;
		ar & pfx_adr;
		ar & pfx_lock;
		ar & pfx_rep;
		ar & pfx_repe;
		ar & pfx_repne;
		ar & pfx_insn;
		ar & default64;
		ar & opr_mode;
		ar & adr_mode;
		ar & br_far;
		ar & br_near;
		ar & implicit_addr;
		ar & c1;
		ar & c2;
		ar & c3;
		ar & have_modrm;
		ar & modrm;
	}
};

class Udis86Instruction : public Instruction
{
public:
	Udis86Instruction()
		: Instruction(), _ud_obj()
	{
		ud_set_input_file(_ud_obj.udptr(), stdin);

		ip(_ip);
		membase(_base);
	}
	
	virtual ~Udis86Instruction()
	{ }
	
	virtual unsigned length()
	{
		return ud_insn_len(_ud_obj.udptr());
	}
	
	virtual void ip(Address a)
	{
		Instruction::ip(a);
		ud_set_pc(_ud_obj.udptr(), a);
	}

	virtual void membase(Address a)
	{
		Instruction::membase(a);
		/* We hardcode the input buffer size to 32 here.
		 * As an Instruction always represents exactly one
		 * instruction, this should suffice.
		 */
		ud_set_input_buffer(_ud_obj.udptr(), reinterpret_cast<uint8_t*>(a), 32);
	}

	virtual void print()
	{
		std::cout << "\033[33m";
		std::cout << std::hex << "0x" << std::setfill('0') << std::setw(8) << Instruction::ip();
		std::cout << "    " << "\033[0m";
		std::cout << c_str();
	}


	virtual char const *c_str() const
	{
		return ud_insn_asm(const_cast<ud*>(_ud_obj.udp()));
	}


	/**
	 * @brief Serialize/Deserialize UDis86 instruction
	 **/
	template <class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		boost::serialization::base_object<Instruction>(*this);
		ar & _ud_obj;
	}

	ud* ud_obj() { return _ud_obj.udptr(); }

protected:
	udis86_t _ud_obj;

private:
	Udis86Instruction(const Udis86Instruction&) : _ud_obj() { }
	Udis86Instruction& operator=(Udis86Instruction&) { return *this; }
};

BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");
