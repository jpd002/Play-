#pragma once

#include "Iop_Module.h"
#include "Iop_Stdio.h"

namespace Iop
{
	class CSysclib : public CModule
	{
	public:
						CSysclib(uint8*, CStdio&);
		virtual			~CSysclib() = default;

		std::string		GetId() const override;
		std::string		GetFunctionName(unsigned int) const override;
		void			Invoke(CMIPS&, unsigned int) override;

	private:
		uint8*			GetPtr(uint32) const;

		uint32			__look_ctype_table(uint32);
		uint32			__memcmp(const void*, const void*, uint32);
		void			__memcpy(void*, const void*, unsigned int);
		void			__memmove(void*, const void*, uint32);
		void			__memset(void*, int, unsigned int);
		uint32			__sprintf(CMIPS& context);
		uint32			__strcat(uint32, uint32);
		uint32			__strlen(const char*);
		uint32			__strcmp(const char*, const char*);
		void			__strcpy(char*, const char*);
		uint32			__strncmp(const char*, const char*, uint32);
		void			__strncpy(char*, const char*, unsigned int);
		uint32			__strchr(uint32, uint32);
		uint32			__strrchr(uint32, uint32);
		uint32			__strstr(uint32, uint32);
		uint32			__strcspn(uint32, uint32);
		uint32			__strtol(uint32, uint32, uint32);
		uint32			__wmemcopy(uint32, uint32, uint32);
		uint8*			m_ram;
		CStdio&			m_stdio;
	};
}
