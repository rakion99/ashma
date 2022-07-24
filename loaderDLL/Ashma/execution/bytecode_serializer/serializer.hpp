#pragma once
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <TlHelp32.h>

extern "C" {
#include "../utils/lua/lobject.h"
#include "../utils/lua/lstate.h"
#include "../utils/lua/lauxlib.h"
}

#include "../trans_instr/transpiler.hpp"

template<typename int_t = uint64_t>
static size_t encodeVarint(int_t value, uint8_t* output) {
	size_t outputSize = 0;
	//While more than 7 bits of data are left, occupy the last output byte
	// and set the next byte flag
	while (value > 127) {
		//|128: Set the next byte flag
		output[outputSize] = ((uint8_t)(value & 127)) | 128;
		//Remove the seven bits we just wrote
		value >>= 7;
		outputSize++;
	}
	output[outputSize++] = ((uint8_t)value) & 127;

	return outputSize;
}

class VectorStream
{
public:
	std::ostringstream ss;
	std::string str()
	{
		return ss.str();
	}
	void writeByte(unsigned char value)
	{
		ss.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	void writeInt(int value)
	{
		ss.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	void writeDouble(double value)
	{
		ss.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	void writeVarInt(unsigned int value)
	{
		do
		{
			writeByte((value & 127) | ((value > 127) << 7));
			value >>= 7;
		} while (value);
	}

	void writeString(std::string value)
	{
		writeVarInt(value.length());
		for (int i = 0; i < value.length(); i++) {
			writeVarInt(value[i]);
		}
	}
};

static void writeProto(VectorStream& vs, std::unordered_map<TString*, int>& stringTable, std::unordered_map<Proto*, int>& protoTable, Proto* p);

class StringTable
{
	VectorStream& vs;

	size_t stringCount = 1;
	std::unordered_map<TString*, int> stringMap;
	std::vector<TString*> strings;

	void getProtoStrings(Proto* p)
	{
		for (int i = 0; i < p->sizek; i++)
		{
			if (p->k[i].tt == LUA_TSTRING)
				insert(&p->k[i].value.gc->ts);
		}

		for (int i = 0; i < p->sizep; i++)
			getProtoStrings(p->p[i]);
	}

public:
	void insert(TString* ts)
	{
		if (stringMap.find(ts) != stringMap.end())
			return;
		stringMap[ts] = stringCount++;
		strings.push_back(ts);
	}

	std::unordered_map<TString*, int> write(Proto* p)
	{
		getProtoStrings(p);

		vs.writeVarInt(strings.size());
		for (TString* ts : strings)
			vs.writeString(std::string(getstr(ts), ts->tsv.len));

		return stringMap;
	}

	StringTable(VectorStream& vs) : vs(vs) {}
};

class ProtoTable
{
	VectorStream& vs;

	size_t protoCount = 0;
	std::unordered_map<Proto*, int> protoMap;
	std::vector<Proto*> protos;

	void addProto(Proto* p)
	{
		for (int i = 0; i < p->sizep; i++)
			addProto(p->p[i]);

		protoMap[p] = protoCount++;
		protos.push_back(p);
	}

public:
	std::unordered_map<Proto*, int> write(Proto* p, std::unordered_map<TString*, int>& stringTable)
	{
		addProto(p);

		vs.writeVarInt(protos.size());
		for (Proto* np : protos)
			writeProto(vs, stringTable, protoMap, np);

		return protoMap;
	}

	ProtoTable(VectorStream& vs) : vs(vs) {}
};

enum ConstantType
{
	ConstantNil,
	ConstantBoolean,
	ConstantNumber,
	ConstantString
};

static LClosure* compile(lua_State* L, const std::string& src) {
	auto const err = luaL_loadstring(L, src.c_str());
	if (err)
	{
		std::printf("Error: %s\n", lua_tostring(L, -1));
	}
	else
	{
		std::cout << "Execution [Vanilla]: \n" << err << " " << std::endl;
		lua_pcall(L, 1, 0, 0);
	}

	auto _err = luaL_loadbuffer(L, src.c_str(), src.size(), "=Script");
	LClosure* cl = (LClosure*)(L->top - 1)->value.gc;
	return cl;
}

static void writeProto(VectorStream& vs, std::unordered_map<TString*, int>& stringTable, std::unordered_map<Proto*, int>& protoTable, Proto* p)
{
	vs.writeByte(p->maxstacksize);
	vs.writeByte(p->numparams);
	vs.writeByte(p->nups);
	vs.writeByte(p->is_vararg);

	std::vector<int> lineinfo;
	auto instructions = push_instrs(p);

	/* Instructions */
	vs.writeVarInt(instructions.size());
	for (size_t i = 0; i < instructions.size(); i++)
		vs.writeInt(instructions[i].Value);

	/* Constants */
	vs.writeVarInt(p->sizek);
	for (int i = 0; i < p->sizek; i++)
	{
		TValue* k = &p->k[i];
		switch (k->tt)
		{
		case LUA_TNIL:
		{
			vs.writeByte(0);
			break;
		}
		case LUA_TBOOLEAN:
		{
			vs.writeByte(ConstantBoolean);
			vs.writeVarInt(k->value.b);
			break;
		}
		case LUA_TNUMBER:
		{
			vs.writeByte(ConstantNumber);
			vs.writeDouble(k->value.n);
			break;
		}
		case LUA_TSTRING:
		{
			vs.writeByte(ConstantString);
			vs.writeVarInt(stringTable[rawtsvalue(k)]);
			break;
		}
		}
	}

	vs.writeVarInt(p->sizep);
	for (int i = 0; i < p->sizep; i++)
		vs.writeVarInt(protoTable[p->p[i]]);

	vs.writeByte(0);
	vs.writeByte(0);
	vs.writeByte(0);
	vs.writeByte(0);
	std::printf("passed\n");
}

static std::string serialize(lua_State* L, const std::string& source)
{
	VectorStream vs;

	const LClosure* cl = compile(L, source);
	if (cl == nullptr)
	{
		vs.writeVarInt(0);
		vs.writeString(lua_tostring(L, -1));
		return vs.str();
	}


	vs.writeByte(2);

	/* Write string table */
	StringTable st(vs);
	std::unordered_map<TString*, int> stringTable = st.write(cl->p);

	/* Write proto table */
	ProtoTable pt(vs);
	std::unordered_map<Proto*, int> protoTable = pt.write(cl->p, stringTable);

	/* Entry point in proto table */
	vs.writeVarInt(protoTable[cl->p]);

	return vs.str();
}