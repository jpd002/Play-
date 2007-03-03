#ifndef _MIPSTESTENGINE_H_
#define _MIPSTESTENGINE_H_

#include <boost/ptr_container/ptr_list.hpp>
#include "xml/Node.h"
#include "MIPS.h"

class CMipsTestEngine
{
public:
                        CMipsTestEngine(const char*);
    virtual             ~CMipsTestEngine();

private:
    class CValue
    {
    public:
        virtual         ~CValue();
        virtual void    AssembleLoad() = 0;
        virtual void    Verify() = 0;
    };

    class CRegisterValue : public CValue
    {
    public:
                        CRegisterValue(Framework::Xml::CNode*);
        virtual         ~CRegisterValue();
        virtual void    AssembleLoad();
        virtual void    Verify();

    private:
        unsigned int    m_nRegister;
        unsigned int    m_nValue0;
        unsigned int    m_nValue1;
    };

    class CValueSet
    {
    public:
                        CValueSet(Framework::Xml::CNode*);
        virtual         ~CValueSet();

    private:
        typedef boost::ptr_list<CValue> ValueListType;

        ValueListType   m_Values;

    };

    typedef boost::ptr_list<CValueSet> InputsType;
    typedef boost::ptr_list<CValueSet> OutputsType;

    void                LoadInputs(Framework::Xml::CNode*);
    void                LoadOutputs(Framework::Xml::CNode*);

    InputsType          m_Inputs;
    OutputsType         m_Outputs;
};

#endif
