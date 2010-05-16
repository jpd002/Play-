#include "Crc32Test.h"
#include "MultTest.h"
#include "Jitter_CodeGen_x86_32.h"
#include "Jitter_CodeGen_x86_64.h"

int main(int argc, char** argv)
{
#ifdef AMD64
	Jitter::CJitter jitter(new Jitter::CCodeGen_x86_64());
#elif defined(WIN32)
	Jitter::CJitter jitter(new Jitter::CCodeGen_x86_32());
#endif
	{
		CCrc32Test test("Hello World!", 0x67FCDACC);
		test.Compile(jitter);
		test.Run();
	}

	{
		CMultTest test(true);
		test.Compile(jitter);
		test.Run();
	}

	{
		CMultTest test(false);
		test.Compile(jitter);
		test.Run();
	}

	return 0;
}
