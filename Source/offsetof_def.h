#ifndef _OFFSETOF_DEF_H_
#define _OFFSETOF_DEF_H_

#undef offsetof
#define offsetof(a, b) (reinterpret_cast<uint8*>(&reinterpret_cast<a*>(0x10)->b) - reinterpret_cast<uint8*>(0x10))

#endif
