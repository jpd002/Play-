#ifndef _MIPSFUNCTIONPATTERNDB_H_
#define _MIPSFUNCTIONPATTERNDB_H_

#include <vector>
#include <unordered_map>
#include "Types.h"
#include "xml/Node.h"

class CMipsFunctionPatternDb
{
public:
	struct PATTERNITEM
	{
		uint32 value;
		uint32 mask;
	};

	class Pattern
	{
	public:
		typedef std::vector<PATTERNITEM> ItemArray;

		bool Matches(uint32*, uint32) const;

		std::string name;
		ItemArray items;
	};

	typedef std::vector<Pattern> PatternArray;

	CMipsFunctionPatternDb(Framework::Xml::CNode*);
	virtual ~CMipsFunctionPatternDb();

	const PatternArray& GetPatterns() const;

private:
	void Read(Framework::Xml::CNode*);
	Pattern ParsePattern(const char*);
	bool ParsePatternItem(const char*, PATTERNITEM&);

	PatternArray m_patterns;
};

#endif
