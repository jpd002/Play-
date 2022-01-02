#pragma once

#include "Types.h"

namespace Ee
{
	class CIdleEvaluator
	{
	public:
		void Reset();
		bool IsIdle() const;

		enum EVENT
		{
			EVENT_INTERRUPT,
			EVENT_WAITSEMA,
			EVENT_SIGNALSEMA,
			EVENT_ROTATETHREADREADYQUEUE,
			EVENT_CHANGETHREAD,
		};

		void NotifyEvent(EVENT, uint32, uint32 = 0);

	private:
		//For Shadow of the Colossus
		struct STRATEGY_SEMACHECKER
		{
			void Reset();
			void NotifyEvent(EVENT, uint32, uint32);

			int32 m_lastWaitSema = -1;
			uint32 m_lastSemaWaitCounter = 0;
			bool m_isIdle = false;
		} m_semaChecker;

		//For Atelier Elie & Marie: Activated when RotateThreadReadyQueue didn't have any effect
		struct STRATEGY_SELFTHREADROTATE
		{
			void Reset();
			void NotifyEvent(EVENT, uint32, uint32);

			uint32 m_threadId = -1;
			uint32 m_selfRotateThreadCount = 0;
			bool m_isIdle = false;
		} m_selfThreadRotate;

		//For Ys: Ark of Napishtim
		struct STRATEGY_THREADROTATEBOUNCE
		{
			void Reset();
			void NotifyEvent(EVENT, uint32, uint32);

			uint32 m_threadPairFirst = -1;
			uint32 m_threadPairSecond = -1;
			uint32 m_bounceCount = 0;
			bool m_isIdle = false;
		} m_threadRotateBounce;
	};
}
