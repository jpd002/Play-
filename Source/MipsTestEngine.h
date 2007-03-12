#ifndef _MIPSTESTENGINE_H_
#define _MIPSTESTENGINE_H_

#include <boost/ptr_container/ptr_list.hpp>
#include "xml/Node.h"
#include "MIPS.h"

class CMipsTestEngine
{
public:
    class CValue
    {
    public:
        virtual             ~CValue();
        virtual void        AssembleLoad() = 0;
        virtual void        Verify() = 0;
    };

    class CRegisterValue : public CValue
    {
    public:
                            CRegisterValue(Framework::Xml::CNode*);
        virtual             ~CRegisterValue();
        virtual void        AssembleLoad();
        virtual void        Verify();

    private:
        unsigned int        m_nRegister;
        unsigned int        m_nValue0;
        unsigned int        m_nValue1;
    };

    class CValueSet
    {
    public:
                            CValueSet(Framework::Xml::CNode*);
        virtual             ~CValueSet();
        unsigned int        GetInputId() const;
        unsigned int        GetInstanceId() const;

    private:
        typedef boost::ptr_list<CValue> ValueListType;

        unsigned int        m_nInputId;
        unsigned int        m_nInstanceId;
        ValueListType       m_Values;

    };

    class CInstance
    {
    public:
                            CInstance(Framework::Xml::CNode*);
        virtual             ~CInstance();
        
    private:
        unsigned int        m_nId;
        std::string         m_sSource;
    };

    typedef boost::ptr_list<CValueSet> InputsType;
    typedef boost::ptr_list<CValueSet> OutputsType;
    typedef boost::ptr_list<CInstance> InstancesType;

                            CMipsTestEngine(const char*);
    virtual                 ~CMipsTestEngine();

    OutputsType::iterator   GetOutputsBegin();
    OutputsType::iterator   GetOutputsEnd();

    CValueSet*              GetInput(unsigned int);
    CInstance*              GetInstance(unsigned int);

private:
    typedef std::map<unsigned int, CValueSet*> InputsByIdMapType;

    void                    LoadInputs(Framework::Xml::CNode*);
    void                    LoadOutputs(Framework::Xml::CNode*);
    void                    LoadInstances(Framework::Xml::CNode*);

    InputsType              m_Inputs;
    OutputsType             m_Outputs;
    InstancesType           m_Instances;
    InputsByIdMapType       m_InputsById;
};

#endif
