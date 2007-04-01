#ifndef _MIPSTESTENGINE_H_
#define _MIPSTESTENGINE_H_

#include <boost/ptr_container/ptr_list.hpp>
#include "xml/Node.h"
#include "MIPS.h"
#include "MIPSAssembler.h"

class CMipsTestEngine
{
public:
    class CValue
    {
    public:
        virtual                 ~CValue();
        virtual void            AssembleLoad(CMIPSAssembler&) = 0;
        virtual bool            Verify(CMIPS&) = 0;
        virtual std::string     GetString() const = 0;
    };

    class CRegisterValue : public CValue
    {
    public:
                                CRegisterValue(Framework::Xml::CNode*);
        virtual                 ~CRegisterValue();
        virtual void            AssembleLoad(CMIPSAssembler&);
        virtual bool            Verify(CMIPS&);
        virtual std::string     GetString() const;

    private:
        unsigned int            m_nRegister;
        uint32                  m_nValue0;
        uint32                  m_nValue1;
    };

    class CSpecialRegisterValue : public CValue
    {
    public:
                                CSpecialRegisterValue(Framework::Xml::CNode*);
        virtual                 ~CSpecialRegisterValue();

        virtual void            AssembleLoad(CMIPSAssembler&);
        virtual bool            Verify(CMIPS&);
        virtual std::string     GetString() const;

    private:
        enum REGISTER
        {
            LO,
            HI,
            LO1,
            HI1,
            PC,
        };

        REGISTER                m_nRegister;
        uint32                  m_nValue0;
        uint32                  m_nValue1;
    };

    class CMemoryValue : public CValue
    {
    public:
                                CMemoryValue(Framework::Xml::CNode*);
        virtual                 ~CMemoryValue();
        virtual void            AssembleLoad(CMIPSAssembler&);
        virtual bool            Verify(CMIPS&);
        virtual std::string     GetString() const;

    private:
        uint32                  m_nAddress;
        uint32                  m_nValue;
    };

    class CValueSet
    {
    private:
        typedef boost::ptr_list<CValue> ValueListType;

    public:
        typedef ValueListType::const_iterator ValueIterator;

                            CValueSet(Framework::Xml::CNode*);
        virtual             ~CValueSet();

        unsigned int        GetInputId() const;
        unsigned int        GetInstanceId() const;

        ValueIterator       GetValuesBegin() const;
        ValueIterator       GetValuesEnd() const;

        void                AssembleLoad(CMIPSAssembler&);
        bool                Verify(CMIPS&);

    private:

        unsigned int        m_nInputId;
        unsigned int        m_nInstanceId;
        ValueListType       m_Values;

    };

    class CInstance
    {
    public:
                            CInstance(Framework::Xml::CNode*);
        virtual             ~CInstance();
        
        unsigned int        GetId();
        const char*         GetSource();

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
    CValueSet*              GetOutput(unsigned int, unsigned int);
    CInstance*              GetInstance(unsigned int);

private:
    typedef std::map<unsigned int, CValueSet*> InputsByIdMapType;
    typedef std::map<unsigned int, CInstance*> InstancesByIdMapType;

    void                    LoadInputs(Framework::Xml::CNode*);
    void                    LoadOutputs(Framework::Xml::CNode*);
    void                    LoadInstances(Framework::Xml::CNode*);

    InputsType              m_Inputs;
    OutputsType             m_Outputs;
    InstancesType           m_Instances;
    InputsByIdMapType       m_InputsById;
    InstancesByIdMapType    m_InstancesById;
};

#endif
