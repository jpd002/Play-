#pragma once

class CVirtualMachine;

class CScopedVmPauser
{
public:
	           CScopedVmPauser(CVirtualMachine&);
	virtual    ~CScopedVmPauser();

private:
	CVirtualMachine&    m_virtualMachine;
	bool                m_paused = false;
};
