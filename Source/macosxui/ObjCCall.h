#ifndef _OBJCCALL_H_
#define _OBJCCALL_H_

#include <objc/objc-runtime.h>

/*
template <typename R = id>
struct method : std::unary_function<id, R>
{
	SEL M_selector;
	method (char const* str) : M_selector(sel_getUid(str)) { }
	method (SEL selector) : M_selector(selector) { }
	R operator() (id obj) const { return ((R(*)(id, SEL, ...))objc_msgSend)(obj, M_selector); }
};
*/
struct ObjCCall
{
	ObjCCall(id object, const char* selector) :
	m_object(object),
	m_selector(sel_getUid(selector))
	{
		
	}

	void operator()() const
	{
		objc_msgSend(m_object, m_selector);
	}
	
	id m_object;
	SEL m_selector;
};

#endif
