#ifndef _REGISTERSTATEFILE_H_
#define _REGISTERSTATEFILE_H_

#include "zip/ZipFile.h"
#include "uint128.h"
#include <map>

class CRegisterStateFile : public Framework::CZipFile
{
public:
						CRegisterStateFile(const char*);
						CRegisterStateFile(Framework::CStream&);
	virtual				~CRegisterStateFile();

	void				SetRegister32(const char*, uint32);
	void				SetRegister64(const char*, uint64);
	void				SetRegister128(const char*, uint128);

	uint32				GetRegister32(const char*) const;
	uint64				GetRegister64(const char*) const;
	uint128				GetRegister128(const char*) const;

	void				Read(Framework::CStream&);
	virtual void		Write(Framework::CStream&);

private:
	typedef std::pair<uint8, uint128> Register;
	typedef std::map<std::string, Register> RegisterList;

	RegisterList		m_registers;
};

#endif
