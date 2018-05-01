#pragma once

#include "Iop_Module.h"
#include "Iop_Stdio.h"

namespace Iop
{
	class CSysclib : public CModule
	{
	public:
		CSysclib(uint8*, uint8*, CStdio&);
		virtual ~CSysclib() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

	private:
		struct JMP_BUF
		{
			uint32 ra;
			uint32 sp;
			uint32 fp;
			uint32 s0;
			uint32 s1;
			uint32 s2;
			uint32 s3;
			uint32 s4;
			uint32 s5;
			uint32 s6;
			uint32 s7;
			uint32 gp;
		};
		static_assert(sizeof(JMP_BUF) == 48, "Size of JMP_BUF must be 48.");

		uint8* GetPtr(uint32, uint32) const;

		int32 __setjmp(CMIPS&);
		void __longjmp(CMIPS&);
		uint32 __look_ctype_table(uint32);
		uint32 __memcmp(const void*, const void*, uint32);
		void __memcpy(void*, const void*, unsigned int);
		void __memmove(void*, const void*, uint32);
		uint32 __memset(uint32, uint32, uint32);
		uint32 __sprintf(CMIPS& context);
		uint32 __strcat(uint32, uint32);
		uint32 __strlen(const char*);
		uint32 __strcmp(const char*, const char*);
		void __strcpy(char*, const char*);
		uint32 __strncmp(const char*, const char*, uint32);
		void __strncpy(char*, const char*, unsigned int);
		uint32 __strchr(uint32, uint32);
		uint32 __strrchr(uint32, uint32);
		uint32 __strstr(uint32, uint32);
		uint32 __strtok(uint32, uint32);
		uint32 __strcspn(uint32, uint32);
		uint32 __index(uint32, uint32);
		uint32 __strtol(uint32, uint32, uint32);
		uint32 __wmemcopy(uint32, uint32, uint32);
		uint32 __vsprintf(CMIPS&, uint32, uint32, uint32);

		uint8* m_ram = nullptr;
		uint8* m_spr = nullptr;
		uint32 m_strtok_prevSPtr = 0;
		CStdio& m_stdio;
	};
}
