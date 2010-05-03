#include "Crc32Test.h"
#include "MultTest.h"
#include "Jitter_CodeGen_x86.h"

int main(int argc, char** argv)
{
	Jitter::CJitter jitter(new Jitter::CCodeGen_x86());

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
